# Adding a Parameter to PolySynth

This is the most common type of change. Thanks to the table-driven parameter system (Sprint 7), adding a new parameter now requires editing only **2-3 locations** instead of the previous 5.

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

### Step 3: Add the parameter enum + table entry

**File:** `src/platform/desktop/PolySynth.h` — add to `EParams` enum:

```cpp
enum EParams {
    // ...existing params...
    kParamMyNewParam,
    kNumParams
};
```

**File:** `src/platform/desktop/ParamMeta.h` — add a table entry:

For a standard `InitDouble` parameter (most common):
```cpp
{kParamMyNewParam, "Label", nullptr, "%",
 0., 100., 1., 50.,
 ParamMeta::MapKind::kDivide, 100.0,
 PM_DOUBLE(myNewParam),
 ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},
```

Available shapes: `kLinear` (default), `kExp` (ShapeExp), `kPow3` (ShapePowCurve(3)).

For an `InitInt` parameter:
```cpp
{kParamMyNewParam, "Label", nullptr, nullptr,
 1., 16., 1., 8.,
 ParamMeta::MapKind::kCast, 1.0,
 PM_INT(myNewParam),
 ParamMeta::InitKind::kInitInt, ParamMeta::ShapeKind::kLinear},
```

For an `InitMilliseconds` parameter:
```cpp
{kParamMyNewParam, "Label", nullptr, "ms",
 0., 1000., 0.1, 100.,
 ParamMeta::MapKind::kDivide, 1000.0,
 PM_DOUBLE(myNewParam),
 ParamMeta::InitKind::kInitMilliseconds, ParamMeta::ShapeKind::kLinear},
```

The `static_assert` at the bottom of `ParamMeta.h` will fail if you add an enum entry but forget the table entry (or vice versa).

**Special-case params:** If the parameter needs `InitEnum`, `InitFrequency`, or `InitBool`, it cannot use the table. Instead:
1. Add the enum entry to `EParams`
2. Add explicit `Init*` call in the constructor (special cases section)
3. Add explicit case in `OnParamChange` (special cases switch)
4. Add explicit `Set()` call in `SyncUIState` (special cases section)
5. Update the `static_assert` count in `ParamMeta.h`

### Step 4: Wire into the DSP engine

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

**If you skip this:** The state field updates, presets save/load it, but it has zero audio effect.

### Step 5: Add a UI control

**File:** `src/platform/desktop/PolySynth.cpp` (inside the appropriate `BuildXxx` method)

```cpp
AttachStackedControl(g, innerRect, kParamMyNewParam, "LABEL", style);
```

**If you skip this:** Parameter works via MIDI/automation but has no on-screen control.

### Step 6: Update tests

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

### Step 7: Verify

```bash
just check       # lint + unit tests
just ci-pr       # full PR gate (includes desktop build + smoke test)
just vrt-run     # VRT verification
```

---

## What the Tests Catch Automatically

| Omission | Caught by |
|---|---|
| Missing `SERIALIZE` / `DESERIALIZE` | Round-trip test in `Test_SynthState.cpp` |
| Field not reset by `Reset()` | Reset-equals-default test in `Test_SynthState.cpp` |
| Adding a constructor to SynthState | `static_assert(is_aggregate_v<SynthState>)` |
| Missing table entry or wrong special-case count | `static_assert` in `ParamMeta.h` |
| Unintended audio change | `golden_master.py --verify` in CI |
| Missing build flag guard | WAM build in CI (compiles with IPLUG_EDITOR only) |

## What Tests Do NOT Catch

| Omission | How to catch it |
|---|---|
| Wrong scaling (MapKind or divisor) | Manual testing — move the knob, listen |
| Missing UI control | Visual inspection |
| Missing DSP engine wiring | Manual testing — parameter has no audio effect |

---

## Quick Reference: MapKind Conventions

| SynthState unit | Display unit | MapKind | Divisor | Example |
|---|---|---|---|---|
| 0.0 - 1.0 | 0 - 100% | `kDivide` | 100.0 | Gain, resonance, mix |
| Seconds | Milliseconds | `kDivide` | 1000.0 | Attack, decay, glide |
| Hz | Hz | `kDirect` | 1.0 | Cutoff |
| Enum (int) | Enum (int) | `kCast` | 1.0 | Polyphony, unison count |
| Inverted 0-1 | 0-100% | `kInvert` | 100.0 | Limiter threshold |
