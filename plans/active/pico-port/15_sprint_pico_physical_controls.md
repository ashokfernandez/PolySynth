# Sprint Pico-15: Physical Controls (ADC Knobs, Buttons, Encoder)

**Depends on:** Sprint Pico-9 (FreeRTOS — HID task), Sprint Pico-12 (Presets — button to load presets)
**Approach:** External ADC via SPI, FreeRTOS HID scan task, knob-to-parameter mapping.
**Estimated effort:** 4-5 days (includes hardware prototyping)

> **Why this sprint exists:** Physical knobs, buttons, and an encoder make the synth a
> self-contained instrument that doesn't require a computer. This sprint is last because
> it requires hardware design and procurement — all software-only features come first.

---

## Goal

Add physical potentiometers (knobs), buttons, and a rotary encoder for hands-on control.
The synth becomes playable without a computer.

---

## Hardware Design Prerequisite

**Critical: ADC pin conflict.** The Waveshare Pico-Audio HAT uses GPIO26-28 for I2S
(DIN, BCK, LRCK). These are also the RP2350's only ADC-capable pins (ADC0-2). GPIO29
(ADC3) is used for VSYS sensing on the Pico 2 W.

**Solution: External ADC via SPI.** An MCP3008 (8-channel, 10-bit, SPI ADC) provides
8 analog inputs on any available SPI bus.

| Component | Specification |
|---|---|
| ADC chip | MCP3008 (8-ch, 10-bit, SPI, ~$2) |
| SPI bus | `spi1` on GPIO10 (SCK), GPIO11 (MOSI), GPIO12 (MISO), GPIO13 (CS) |
| SPI clock | 1 MHz (safe at 3.3V; MCP3008 max is 1.35 MHz at 2.7V) |
| Knobs | 8× 10K linear potentiometers |
| Buttons | 3-4 momentary push buttons (GPIO with internal pull-ups) |
| Encoder | 1× rotary encoder with push button (quadrature, any GPIO) |

**Parts list** (must be procured before sprint):
- 1× MCP3008 DIP or breakout board
- 8× 10K linear potentiometers (B10K)
- 3-4× momentary push buttons
- 1× rotary encoder with push button
- Breadboard + jumper wires (prototype)
- Optional: 4-8 LEDs + 220Ω resistors

---

## Task 15.1: External ADC Driver (MCP3008)

### Problem

No ADC pins available (I2S uses GPIO26-28).

### Fix

New file: `src/platform/pico/pico_adc_driver.h/cpp`

MCP3008 SPI driver:
```cpp
class MCP3008 {
public:
    void Init(spi_inst_t* spi, uint cs_pin);
    uint16_t ReadChannel(uint8_t channel);  // Returns 0-1023
};
```

- SPI bus: `spi1` (GPIO10-13, no conflicts with I2S or WiFi)
- Read protocol: 3-byte SPI transaction per channel (start bit, channel select, 10-bit result)

### Validation

- Read all 8 channels, values stable within +/-2 LSB
- Pot sweep gives monotonic 0 → 1023 readings

---

## Task 15.2: HID Scan Task

### Problem

Need to poll controls without blocking audio or other tasks.

### Fix

New file: `src/platform/pico/pico_hid_task.h/cpp`

FreeRTOS task at priority 3 (Normal), stack 512 words (2KB):

Scan loop at 100 Hz (`vTaskDelay(pdMS_TO_TICKS(10))`):
1. Read all 8 ADC channels via MCP3008
2. Exponential smoothing (alpha=0.1) to reject noise
3. Deadzone (+/-2 out of 1023) to prevent jitter
4. On change: map to parameter value, call `app.SetParam()`
5. Read button GPIO states, debounce (20ms)
6. Read encoder quadrature, decode direction

### Validation

- Turning a knob audibly changes the mapped parameter
- No jitter when knob is stationary

---

## Task 15.3: Knob-to-Parameter Mapping

### Problem

Raw ADC values need to map to musically useful parameter ranges.

### Fix

Define mapping table:
```cpp
struct KnobMapping {
    uint8_t adcChannel;
    const char* paramName;
    float minVal, maxVal;
    bool logarithmic;  // true for frequency-domain params
};
```

Suggested 8-knob layout (classic synth panel):

| Knob | Channel | Parameter | Range | Curve |
|---|---|---|---|---|
| 0 | ADC0 | Filter Cutoff | 20-20000 Hz | Log |
| 1 | ADC1 | Filter Resonance | 0-1 | Linear |
| 2 | ADC2 | Filter Env Amount | 0-1 | Linear |
| 3 | ADC3 | Amp Attack | 0.001-5s | Log |
| 4 | ADC4 | Amp Decay | 0.001-5s | Log |
| 5 | ADC5 | Amp Sustain | 0-1 | Linear |
| 6 | ADC6 | Amp Release | 0.001-5s | Log |
| 7 | ADC7 | LFO Rate | 0.1-20 Hz | Log |

Logarithmic mapping: `val = min * pow(max/min, normalized)`. Use a constexpr lookup
table to avoid runtime `pow()` (same pattern as Sprint 10's MIDI-to-freq table).

### Validation

- Each knob controls its assigned parameter audibly
- Logarithmic knobs feel natural (even sweep across perceptual range)

---

## Task 15.4: Buttons and Encoder

### Problem

Need discrete controls for preset navigation and mode selection.

### Fix

Buttons wired directly to GPIO with internal pull-ups (any free GPIO 0-9, 14-22):

| Button | GPIO | Function |
|---|---|---|
| Preset Up | GPIO14 | Load next preset slot |
| Preset Down | GPIO15 | Load previous preset slot |
| Panic | GPIO16 | All notes off |
| Waveform | GPIO17 | Cycle OscA waveform (Saw→Square→Tri→Sine) |

Rotary encoder (quadrature + push button):
- GPIO18 = Encoder A, GPIO19 = Encoder B, GPIO20 = Push
- Rotate: fine-adjust the "focused" parameter
- Push: cycle which parameter is focused

Debounce: 20ms via FreeRTOS software timer (not busy-wait).

### Validation

- Button press triggers expected action instantly
- Encoder rotation smooth, no missed steps or double-counts

---

## Task 15.5: LED Feedback

### Problem

No visual feedback for synth state.

### Fix

Use available GPIO for status LEDs:
- **Voice activity LED:** PWM brightness proportional to active voice count
- **Preset indicator:** Binary display on 4 LEDs (0-15 = 4 bits), or blink patterns
  on the onboard CYW43 LED
- **Clipping indicator:** Brief flash when peak level > 0.95

PWM via Pico SDK `hardware/pwm.h` on a free PWM slice.

### Validation

- LEDs respond to voice activity and preset changes
- Clipping LED flashes when output is hot

---

## Task 15.6: Serial Feedback

### Problem

Should be able to see knob changes on serial for debugging.

### Fix

When a knob changes a parameter, print on serial (rate-limited to 10/sec):
```
[KNOB] filterCutoff = 1234.5
```

Extend `STATUS` to show current knob positions.

### Validation

- Serial shows knob changes
- `STATUS` includes knob values

---

## Risks

1. **ADC pin conflict (mitigated)** — External MCP3008 via SPI avoids the GPIO26-28
   I2S conflict. Well-understood solution.
2. **Noise on breadboard** — Long wires pick up noise. Exponential smoothing + deadzone
   mitigate. PCB would eliminate.
3. **SPI bus contention** — If a future sprint adds an SPI display, it shares the bus.
   Use FreeRTOS mutex on SPI.
4. **Hardware lead time** — Parts must be procured and wired before sprint starts. Plan
   1-2 weeks lead time.
5. **Multi-producer safety** — HID task adds another command producer. Must use the
   FreeRTOS command queue established in Sprint 14.

---

## Definition of Done

- [ ] MCP3008 SPI driver reads 8 channels reliably
- [ ] HID task scans at 100Hz with smoothing and deadzone
- [ ] 8 knob-to-parameter mappings functional with correct curves
- [ ] At least 3 buttons functional (preset up/down, panic)
- [ ] Rotary encoder works for fine parameter adjustment
- [ ] LED feedback for voice activity
- [ ] Serial feedback for knob changes (rate-limited)
- [ ] No audio glitches during knob sweeps
- [ ] `just test` and `just test-embedded` pass
- [ ] `just pico-build` succeeds
- [ ] Control response: <20ms from knob turn to audible change
