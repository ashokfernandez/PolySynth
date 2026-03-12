#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

// ── PolySynth Pico FreeRTOS Configuration ──────────────────────────────────
// FreeRTOS runs on Core 0 ONLY. Core 1 stays bare-metal (DMA ISR audio).
// See plans/active/pico-port/09_sprint_pico_freertos.md for rationale.

// ── Core scheduling ────────────────────────────────────────────────────────
#define configNUMBER_OF_CORES               1       // Core 0 only — avoids RP2350 SMP spinlock bugs
#define configTICK_CORE                     0
#define configTICK_RATE_HZ                  1000
#define configMAX_PRIORITIES                6
#define configMINIMAL_STACK_SIZE            256     // In words, NOT bytes (= 1024 bytes)
#define configUSE_PREEMPTION                1
#define configUSE_TIME_SLICING              1       // Same-priority tasks share CPU

// ── Heap ───────────────────────────────────────────────────────────────────
// Using heap_4: best-fit with coalescing. Budget breakdown:
//   Serial task stack:      4 KB (1024 words)
//   USB MIDI task (future): 4 KB
//   WiFi task (future):     8 KB
//   Queue/semaphore alloc:  2 KB
//   Headroom:              30 KB
// Total: 48KB = <10% of 520KB SRAM
#define configTOTAL_HEAP_SIZE               ( 48 * 1024 )

// ── RP2350 / Cortex-M33 specific ──────────────────────────────────────────
#define configENABLE_FPU                    1       // Save/restore FPU regs on context switch (required: hard-float ABI)
#define configENABLE_MPU                    0       // Not supported on this port
#define configENABLE_TRUSTZONE              0       // Not supported on this port
#define configRUN_FREERTOS_SECURE_ONLY      1       // RP2350 runs in secure mode only
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 16     // Only tested value on RP2350
#define configCPU_CLOCK_HZ                  150000000  // RP2350 default 150 MHz

// ── Pico SDK interop (both required) ──────────────────────────────────────
#define configSUPPORT_PICO_SYNC_INTEROP     1       // SDK mutex/semaphore interop
#define configSUPPORT_PICO_TIME_INTEROP     1       // SDK sleep_ms() interop

// ── Features ──────────────────────────────────────────────────────────────
#define configUSE_IDLE_HOOK                 1       // LED heartbeat in idle
#define configUSE_TICK_HOOK                 0
#define configUSE_MUTEXES                   1
#define configUSE_COUNTING_SEMAPHORES       1
#define configUSE_TASK_NOTIFICATIONS        1
#define configUSE_TIMERS                    1       // Software timers for periodic status
#define configTIMER_TASK_PRIORITY           2
#define configTIMER_QUEUE_LENGTH            10
#define configTIMER_TASK_STACK_DEPTH        configMINIMAL_STACK_SIZE

// ── Debug (disable for release by setting to 0) ──────────────────────────
#define configCHECK_FOR_STACK_OVERFLOW      2       // Full pattern check
#define configASSERT( x )                   if( !( x ) ) { __asm volatile("bkpt #0"); }

// ── Required by SMP port but unused with configNUMBER_OF_CORES=1 ─────────
#define configUSE_CORE_AFFINITY             0
#define configUSE_PASSIVE_IDLE_HOOK         0

// ── Tick type ────────────────────────────────────────────────────────────
#define configTICK_TYPE_WIDTH_IN_BITS       TICK_TYPE_WIDTH_32_BITS

// ── Tickless idle — NOT supported on RP2350 ──────────────────────────────
#define configUSE_TICKLESS_IDLE             0

// ── Required defines ─────────────────────────────────────────────────────
#define configSUPPORT_STATIC_ALLOCATION     0
#define configSUPPORT_DYNAMIC_ALLOCATION    1

// ── Stack overflow hook declaration ──────────────────────────────────────
#define configRECORD_STACK_HIGH_ADDRESS      1

// ── Include optional API functions ───────────────────────────────────────
#define INCLUDE_vTaskDelay                  1
#define INCLUDE_vTaskDelayUntil             1
#define INCLUDE_vTaskPrioritySet            1
#define INCLUDE_uxTaskPriorityGet           1
#define INCLUDE_vTaskDelete                 1
#define INCLUDE_vTaskSuspend                1
#define INCLUDE_xTaskGetSchedulerState      1
#define INCLUDE_uxTaskGetStackHighWaterMark  1
#define INCLUDE_xTimerPendFunctionCall      1

#endif /* FREERTOS_CONFIG_H */
