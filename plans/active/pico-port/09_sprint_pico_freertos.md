# Sprint Pico-9: FreeRTOS Migration

**Depends on:** Sprint Pico-8 (Architecture Decomposition)
**Blocks:** Sprint Pico-10 (DSP Optimizations)
**Approach:** Add FreeRTOS to Core 0 only. Core 1 stays bare-metal. Convert busy-loop to tasks.
**Estimated effort:** 3-4 days

> **Why this sprint exists:** The current main loop busy-polls serial input in a tight
> `while(true)` loop, consuming 100% of Core 0. This makes it impossible to add USB MIDI,
> WiFi, display, or ADC without priority inversions. FreeRTOS (a lightweight real-time
> operating system that provides priority-based preemptive scheduling) is required for
> CYW43 WiFi/BLE and TinyUSB driver integration.

---

## Goal

Replace the Core 0 busy-loop with FreeRTOS tasks. After this sprint, serial command
handling runs as a FreeRTOS task, and the architecture supports adding MIDI, WiFi, display,
and HID tasks in future sprints.

**Critical constraint:** Core 1 stays bare-metal. FreeRTOS runs on Core 0 only with
`configNUMBER_OF_CORES=1`. See `07_pico_dsp_architecture.md` for the full rationale
(RP2350 SMP spinlock hardfault bugs).

---

## Task 9.1: Add FreeRTOS-Kernel Submodule

### Problem

FreeRTOS is not yet in the project.

### Fix

Add the **Raspberry Pi fork** of FreeRTOS-Kernel (not mainline — mainline crashes on RP2350):

```bash
git submodule add https://github.com/raspberrypi/FreeRTOS-Kernel.git external/FreeRTOS-Kernel
```

**Why the Raspberry Pi fork?** The mainline `ARM_CM33_NTZ/non_secure` port builds but
crashes in `crt0.S` on RP2350. The RPi fork includes a `RP2350_ARM_NTZ` portable layer
that handles RP2350's secure-mode-only execution correctly.

### Validation

- Submodule clones successfully
- `external/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2350_ARM_NTZ/` exists

---

## Task 9.2: Create FreeRTOSConfig.h

### Problem

FreeRTOS requires a project-specific configuration header.

### Fix

Create `src/platform/pico/FreeRTOSConfig.h`:

```c
#define configNUMBER_OF_CORES               1      // Core 0 only — avoids SMP bugs
#define configTICK_CORE                     0
#define configTICK_RATE_HZ                  1000
#define configMAX_PRIORITIES                6
#define configMINIMAL_STACK_SIZE            256    // WARNING: in words, not bytes (= 1024 bytes)
#define configTOTAL_HEAP_SIZE               (48 * 1024)
// 48KB heap budget breakdown:
//   Serial task stack:     4 KB (1024 words)
//   USB MIDI task (future):4 KB
//   WiFi task (future):    8 KB
//   Queue/semaphore alloc: 2 KB
//   Headroom:             30 KB
// Total SRAM: 520KB — this 48KB is <10% of available
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1      // Allow same-priority tasks to share CPU

// RP2350-specific — all required
#define configENABLE_FPU                    1      // Save/restore FPU on context switch
#define configENABLE_MPU                    0      // Not supported on this port
#define configENABLE_TRUSTZONE              0      // Not supported on this port
#define configRUN_FREERTOS_SECURE_ONLY      1      // RP2350 runs in secure mode
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16    // Only tested value on RP2350

// Pico SDK interop — both required
#define configSUPPORT_PICO_SYNC_INTEROP     1     // SDK mutex/semaphore interop
#define configSUPPORT_PICO_TIME_INTEROP     1     // SDK sleep_ms() interop

// Features
#define configUSE_IDLE_HOOK                 1      // LED heartbeat
#define configUSE_TICK_HOOK                 0
#define configUSE_MUTEXES                   1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_TASK_NOTIFICATIONS        1

// Debug (enable during development, disable for release)
#define configCHECK_FOR_STACK_OVERFLOW      2      // Full pattern check
#define configUSE_TIMERS                    1      // Software timers for periodic status
```

See `07_pico_dsp_architecture.md` for full config with rationale for each value.

### Validation

- `just pico-build` succeeds with FreeRTOS linked
- Binary size increase is reasonable (~15-20KB for kernel)

---

## Task 9.3: Update CMakeLists.txt

### Problem

CMake doesn't know about FreeRTOS-Kernel yet.

### Fix

```cmake
# Add FreeRTOS kernel
set(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../external/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2350_ARM_NTZ/FreeRTOS_Kernel_import.cmake)

target_link_libraries(polysynth_pico PRIVATE
    pico_stdlib
    pico_multicore                       # KEEP: Core 1 bare-metal
    FreeRTOS-Kernel-Heap4                # heap_4 — best fit with coalescing
    hardware_pio hardware_dma hardware_clocks
    SEA_DSP SEA_Util
)
```

Note: `pico_cyw43_arch_lwip_sys_freertos` is NOT added yet — that's Sprint 14 (WiFi).
We keep `pico_cyw43_arch_none` for LED access and add only the FreeRTOS kernel now.
The architecture doc's CMake shows the final target state including WiFi libraries.

### Validation

- `just pico-build` succeeds
- FreeRTOS symbols present in .elf

---

## Task 9.4: Convert Main Loop to Serial Task

### Problem

The current `while(true)` loop in main.cpp polls serial, runs demo updates, and
prints diagnostics in a tight busy-loop.

### Fix

Convert to a FreeRTOS task:

```cpp
static void serial_task(void* params) {
    auto* app = static_cast<PicoSynthApp*>(params);
    PicoDemo demo;

    while (true) {
        // Block until character available (yields CPU to lower-priority tasks)
        int ch = getchar_timeout_us(1000);  // 1ms timeout — responsive serial
        if (ch != PICO_ERROR_TIMEOUT) {
            // Feed character to line buffer, dispatch on newline
        }

        // Demo update (if running)
        demo.Update(to_ms_since_boot(get_absolute_time()), *app);
    }
}
```

Created at priority 4 with 1024-word (4KB) stack.

### Validation

- Serial commands still work with same responsiveness
- Demo sequence still works
- Core 0 CPU usage drops from 100% to near 0% when idle

---

## Task 9.5: Add Idle Hook (LED Heartbeat)

### Problem

The current code doesn't provide visual feedback that the system is running normally.

### Fix

```cpp
extern "C" void vApplicationIdleHook() {
    static uint32_t lastToggle = 0;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - lastToggle > 1000) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !led_state);
        led_state = !led_state;
        lastToggle = now;
    }
}
```

1Hz LED blink = "system alive". No blink = system hung.

**Dependency:** The Pico 2 W's onboard LED is connected via the CYW43 WiFi chip, so
`cyw43_arch_init()` must be called before `vTaskStartScheduler()`. This is already
done in the current `main()` — just verify it stays in the correct order after
FreeRTOS migration.

**Guard:** If WiFi init is deferred to a later sprint, guard the LED call:
```cpp
#if defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, !led_state);
#else
    gpio_put(PICO_DEFAULT_LED_PIN, !led_state);  // Fallback for non-W boards
#endif
```
This also makes the idle hook work on Pico 2 (non-W) boards that have a standard GPIO LED.


### Validation

- LED blinks at 1Hz during normal operation
- LED stops if system hangs (visual diagnostic)

---

## Task 9.6: Verify Core 1 Audio Unaffected

### Problem

Adding FreeRTOS to Core 0 must not affect Core 1 audio timing.

### Fix

No code changes — this is a verification task:

1. Core 1 launched via `multicore_launch_core1()` BEFORE `vTaskStartScheduler()`
2. DMA ISR runs on Core 1 with no FreeRTOS involvement
3. SPSCQueue communication between cores still lock-free
4. No FreeRTOS API calls from Core 1

### Validation

- Audio quality identical to Sprint 8
- Zero underruns during 4-voice playback over 60 seconds
- STATUS CPU% unchanged
- No hardfaults under load

---

## Known Gotchas

These are documented in `07_pico_dsp_architecture.md` and must be verified:

1. **TinyUSB `vTaskDelay` bug** — `vTaskDelay(pdMS_TO_TICKS(1))` delays too long on
   RP2350. Use `tud_task_ext(timeout, false)` or `portYIELD()` in USB task loops.
2. **GPIO init before scheduler** — All `gpio_init()` calls must happen before
   `vTaskStartScheduler()`. Cross-core GPIO init can hang.
3. **Tickless idle unsupported** — Don't enable `configUSE_TICKLESS_IDLE`.
4. **Stack sizes in words** — `configMINIMAL_STACK_SIZE = 256` means 1024 bytes.
5. **Hard-float ABI** — `configENABLE_FPU = 1` ensures context switches preserve FPU
   registers (required with our `-mfloat-abi=hard`).

---

## Definition of Done

- [ ] `external/FreeRTOS-Kernel` submodule added (Raspberry Pi fork)
- [ ] `FreeRTOSConfig.h` created with RP2350-specific settings
- [ ] CMakeLists.txt links FreeRTOS-Kernel-Heap4
- [ ] Serial command handling runs as a FreeRTOS task
- [ ] Idle hook blinks LED at 1Hz
- [ ] Core 1 audio completely unaffected
- [ ] `just test` passes
- [ ] `just test-embedded` passes
- [ ] `just pico-build` succeeds
- [ ] Zero underruns during 4-voice playback over 60 seconds
- [ ] Binary size increase documented (<20KB for FreeRTOS kernel)
