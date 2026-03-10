# Sprint Pico-11: USB MIDI Input

**Depends on:** Sprint Pico-9 (FreeRTOS Migration)
**Blocks:** Sprint Pico-12 (Presets — testing presets with musical input)
**Approach:** TinyUSB composite device (CDC serial + MIDI) as FreeRTOS task.
**Estimated effort:** 3-4 days

> **Why this sprint exists:** The synth is only playable via serial terminal commands.
> USB MIDI makes it a real instrument — plug in any MIDI keyboard and play. TinyUSB is
> already bundled with the Pico SDK, and FreeRTOS (Sprint 9) provides the task
> infrastructure for USB servicing.

---

## Goal

Accept MIDI note and CC messages via USB. The Pico 2 W appears as a USB MIDI device
("PolySynth Pico") when plugged into a computer or connected to a USB MIDI controller.
Serial (CDC) continues to work alongside MIDI on the same USB connection.

---

## Task 11.1: Enable TinyUSB MIDI Composite Device

### Problem

The Pico SDK's `pico_enable_stdio_usb` creates a CDC-only TinyUSB device. Adding MIDI
requires a composite USB device (CDC + MIDI) with custom descriptors.

### Fix

- Add `tinyusb_device` and `tinyusb_board` to CMake `target_link_libraries`
- Create `src/platform/pico/tusb_config.h`:
  - `CFG_TUD_MIDI = 1` (enable MIDI device class)
  - `CFG_TUD_CDC = 1` (keep serial)
  - `CFG_TUSB_OS = OPT_OS_FREERTOS`
  - Appropriate endpoint and buffer sizes
- Create `src/platform/pico/usb_descriptors.c` with composite device descriptor
  (CDC interface + MIDI interface), string descriptors ("PolySynth Pico")
- Replace `pico_enable_stdio_usb(polysynth_pico 1)` with custom TinyUSB configuration

**Gotcha:** CDC and MIDI share the USB peripheral. The Pico SDK's stdio_usb creates
its own TinyUSB CDC device. To add MIDI alongside CDC, you must provide custom USB
descriptors for a composite device. Reference: TinyUSB `examples/device/midi_test` and
`examples/device/cdc_msc`.

### Validation

- `just pico-build` succeeds
- Binary size increase documented
- Device appears as "PolySynth Pico" in system USB device list

---

## Task 11.2: Create MIDI Task

### Problem

TinyUSB requires periodic servicing via `tud_task()`.

### Fix

New files: `src/platform/pico/pico_midi_task.h` and `pico_midi_task.cpp`

FreeRTOS task at priority 4 (same as serial — MIDI is equally time-sensitive):

```cpp
static void midi_task(void* params) {
    auto* app = static_cast<PicoSynthApp*>(params);
    while (true) {
        tud_task_ext(1, false);  // Service USB, 1ms timeout
        while (tud_midi_available()) {
            uint8_t packet[4];
            tud_midi_packet_read(packet);
            dispatch_midi(app, packet);
        }
    }
}
```

Stack: 512 words (2KB) — TinyUSB MIDI parsing is lightweight.

**Gotcha (from architecture doc):** `vTaskDelay(pdMS_TO_TICKS(1))` delays far too long
on RP2350. Use `tud_task_ext(timeout, false)` for USB servicing instead.

### Validation

- Task runs without hardfault
- USB enumeration completes in <3 seconds

---

## Task 11.3: MIDI Message Dispatch

### Problem

MIDI messages need to map to existing `PicoSynthApp` API calls.

### Fix

Map standard MIDI messages:

| MIDI Message | Mapping |
|---|---|
| Note On (0x90) | `app->NoteOn(note, velocity)` |
| Note Off (0x80) | `app->NoteOff(note)` |
| CC 1 (Mod Wheel) | LFO depth |
| CC 7 (Volume) | Master gain |
| CC 64 (Sustain) | `app->GetEngine().OnSustainPedal(value >= 64)` |
| CC 71 (Resonance) | Filter resonance (0-127 → 0.0-1.0) |
| CC 73 (Attack) | Amp attack |
| CC 74 (Cutoff) | Filter cutoff (0-127 → 20-20000 Hz, logarithmic) |
| CC 75 (Decay) | Amp decay |
| CC 123 (All Notes Off) | `app->Panic()` |
| Pitch Bend (0xE0) | Deferred to future sprint (requires Voice.h changes) |

CC-to-parameter mapping stored in a constexpr table for easy extension.

**Multi-producer note:** This is the first sprint with two command producers (serial +
MIDI). The current `SPSCQueue` is single-producer. If both serial and MIDI tasks call
`PicoSynthApp::NoteOn()` concurrently, the queue could corrupt. Options:
1. **FreeRTOS mutex** around `PicoSynthApp` command methods (simplest)
2. **FreeRTOS command queue** between input tasks and SPSC audio queue (cleanest)

Recommend option 1 for Sprint 11, with option 2 as a refactor if Sprint 14 (WiFi/OSC)
adds a third producer.

### Validation

- MIDI keyboard note on/off produces audio
- CC 74 sweeps filter cutoff audibly
- Serial `STATUS` shows correct voice count during MIDI playback

---

## Task 11.4: MIDI Activity Feedback

### Problem

No visual or serial feedback for MIDI activity.

### Fix

- Add atomic MIDI message counter to `PicoSynthApp`
- Extend `STATUS` serial command to show MIDI message count
- Flash LED on MIDI activity (100ms on per message, faster than heartbeat)

### Validation

- `STATUS` shows MIDI message count
- LED blinks on each MIDI note

---

## Task 11.5: Update Self-Test and CI

### Problem

CI must still pass with TinyUSB MIDI enabled.

### Fix

- Add compile-time check that TinyUSB is configured correctly
- Add runtime check: `tud_mounted()` reports USB connection state
- Not a functional MIDI test (would need a MIDI source) — just verifies USB init
- Wokwi CI: TinyUSB MIDI stubs may need `#ifdef` guards if Wokwi RP2040 proxy
  doesn't emulate USB MIDI

### Validation

- `[TEST:ALL_PASSED]` still emitted
- `just test` and `just test-embedded` pass

---

## Hardware Requirements

- Any USB MIDI keyboard or controller (for manual testing)
- USB OTG cable/adapter if connecting a keyboard directly to the Pico
- When using OTG, Pico needs separate power (keyboard may need a powered USB hub)

---

## Risks

1. **USB composite device complexity** — CDC + MIDI requires careful descriptor
   configuration. Many online examples are MIDI-only. Use TinyUSB's composite device
   examples as reference.
2. **Wokwi CI** — Wokwi RP2040 may not emulate USB MIDI. May need stubs for CI.
3. **Multi-producer queue** — First sprint with two producers (serial + MIDI). See
   Task 11.3 for mitigation.

---

## Definition of Done

- [ ] TinyUSB MIDI device class enabled alongside CDC serial
- [ ] MIDI task runs as FreeRTOS task on Core 0
- [ ] Note On/Off from MIDI keyboard produces audio
- [ ] At least 6 CC mappings functional (volume, cutoff, resonance, attack, decay, mod wheel)
- [ ] Sustain pedal (CC 64) works
- [ ] Multi-producer safety addressed (mutex or equivalent)
- [ ] Serial STATUS shows MIDI activity
- [ ] `just test` and `just test-embedded` pass
- [ ] `just pico-build` succeeds
- [ ] Audio unaffected (zero underruns during MIDI playback)
- [ ] Binary size increase documented
