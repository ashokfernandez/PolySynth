# Adding a Parameter to PolySynth

This is the most common type of change. A new synth parameter touches multiple files in a specific order. Missing any step causes bugs that may be silent — the checklist below is designed so that automated tests catch most omissions.

---

## Checklist

### Step 1: Add the field to SynthState

**File:** `src/core/SynthState.h`

Add the field with an inline default value in the appropriate section:

```cpp
// --- Filter (VCF) -------------------------
double filterCutoff = 2000.0; // Hz
double filterResonance = 0.0; // 0.0 to 1.0
double myNewParam = 0.5;      // 0.0 to 1.0, description of what it does
```

**Rules:**
- Always provide an inline default value. This IS the canonical default.
- Add a comment with the valid range and unit.
- Do not add a constructor — SynthState must remain an aggregate (`static_assert` enforces this).

**If you skip this:** Nothing compiles that references the field. Hard to miss.

### Step 2: Add serialization

**File:** `src/core/PresetManager.cpp`

Add to both `Serialize` and `Deserialize`:

```cpp
// In Serialize:
SERIALIZE(j, state, myNewParam);

// In Deserialize:
DESERIALIZE(j, outState, myNewParam);
```

Place it in the same section as related fields (Filter, LFO, FX, etc.).

**If you skip this:** The round-trip test in `Test_SynthState.cpp` will fail. This is caught automatically.

### Step 3: Add the parameter enum

**File:** `src/platform/desktop/PolySynth.h`

Add an entry to the `EParams` enum, before `kNumParams`:

```cpp
enum EParams {
    // ...existing params...
    kParamMyNewParam,
    kNumParams
};
```

**If you skip this:** The constructor won't reference it, so no UI control will exist. Obvious at runtime.

### Step 4: Initialize the parameter in the constructor

**File:** `src/platform/desktop/PolySynth.cpp` (constructor)

Add parameter initialization using the SynthState default:

```cpp
GetParam(kParamMyNewParam)
    ->InitDouble("Label", state.myNewParam * kToPercentage, 0., 100., 1., "%");
```

Use the appropriate `Init*` method:
- `InitDouble` for continuous values (with scaling: `kToPercentage = 100.0`, `kToMs = 1000.0`)
- `InitEnum` for discrete choices
- `InitBool` for toggles
- `InitFrequency` for Hz values with logarithmic curves
- `InitMilliseconds` for time values

**If you skip this:** The parameter exists in the enum but has no name, range, or default. UI will show garbage.

### Step 5: Add OnParamChange handler

**File:** `src/platform/desktop/PolySynth.cpp` (inside `OnParamChange`, `#if IPLUG_DSP` block)

Add a case to the switch statement:

```cpp
case kParamMyNewParam:
    mState.myNewParam = value / kToPercentage;  // Reverse the display scaling
    break;
```

The conversion must be the exact inverse of what you used in Step 4.

**If you skip this:** The UI control moves but the synth state never updates. Silent bug — no test catches this automatically.

### Step 6: Add SyncUIState handler

**File:** `src/platform/desktop/PolySynth.cpp` (inside `SyncUIState`, `#if IPLUG_DSP` block)

Add the reverse mapping:

```cpp
GetParam(kParamMyNewParam)->Set(mState.myNewParam * kToPercentage);
```

This must use the same scaling as Step 4, applied to the SynthState value.

**If you skip this:** Preset loads won't update this parameter's UI control. The knob will show stale values after loading a preset.

### Step 7: Wire into the DSP engine

**File:** `src/core/Engine.h` (or `VoiceManager.h` / `Voice.h` as appropriate)

Add a setter that the engine's `UpdateState` method will call:

```cpp
// In Engine::UpdateState(const SynthState& state):
mVoiceManager.SetMyNewParam(state.myNewParam);
```

Or, if it affects FX:
```cpp
mChorus.SetMyNewParam(state.myNewParam);
```

**If you skip this:** The state field updates, presets save/load it, but it has zero audio effect. The "unimplemented field" test pattern (if present) would catch this.

### Step 8: Add a UI control

**File:** `src/platform/desktop/PolySynth.cpp` (inside the appropriate `BuildXxx` method)

```cpp
AttachStackedControl(g, innerRect, kParamMyNewParam, "LABEL", style);
```

**If you skip this:** Parameter works via MIDI/automation but has no on-screen control. Noticeable but not a crash.

### Step 9: Update tests

**File:** `tests/unit/Test_SynthState.cpp`

In the "Reset restores all fields" test, add:
```cpp
modified.myNewParam = 0.99;  // Non-default value
```
And in the verification section:
```cpp
CATCH_CHECK(modified.myNewParam == fresh.myNewParam);
```

In the "serialization round-trip" test, add:
```cpp
original.myNewParam = 0.42;  // Distinct non-default
```
And in the verification:
```cpp
CATCH_CHECK(loaded.myNewParam == original.myNewParam);
```

### Step 10: Verify

```bash
# Build and run unit tests
cd tests && mkdir -p build && cd build && cmake .. && make && ./run_tests

# Verify golden masters (should pass if audio didn't change, or regenerate if it did)
cd ../.. && python3 scripts/golden_master.py --verify
```

---

## What the Tests Catch Automatically

| Omission | Caught by |
|---|---|
| Missing `SERIALIZE` / `DESERIALIZE` | Round-trip test in `Test_SynthState.cpp` |
| Field not reset by `Reset()` | Reset-equals-default test in `Test_SynthState.cpp` |
| Adding a constructor to SynthState | `static_assert(is_aggregate_v<SynthState>)` |
| Unintended audio change | `golden_master.py --verify` in CI |
| Missing build flag guard | WAM build in CI (compiles with IPLUG_EDITOR only) |

## What Tests Do NOT Catch

| Omission | How to catch it |
|---|---|
| Missing `OnParamChange` case | Manual testing — move the knob, listen |
| Missing `SyncUIState` line | Load a preset, check if the knob updates |
| Wrong scaling (×100 vs ×1000) | Manual testing, or a future parameter metadata test |
| Missing UI control | Visual inspection |

---

## Quick Reference: Scaling Conventions

| SynthState unit | Display unit | Multiplier | Example |
|---|---|---|---|
| 0.0 - 1.0 | 0 - 100% | `kToPercentage = 100.0` | Gain, resonance, mix |
| Seconds | Milliseconds | `kToMs = 1000.0` | Attack, decay, delay time |
| Hz | Hz | 1.0 (no scaling) | Cutoff, LFO rate |
| Enum (int) | Enum (int) | 1.0 (cast to int) | Waveform, filter model |
| Inverted 0-1 | 0-100% | `(1.0 - value) * kToPercentage` | Limiter threshold |
