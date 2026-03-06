# Sprint 7: Table-Driven OnParamChange — Replace 120-line Switch with Metadata Table

## Goal

Replace the 120-line `OnParamChange` switch statement (lines 930-1115 of PolySynth.cpp) and the corresponding 55-line `SyncUIState` reverse-mapping (lines 74-128) with a single parameter metadata table. The constructor's individual `GetParam(...)->InitDouble(...)` calls also consolidate into a loop over the same table.

## Why This Sprint

Currently, adding a new parameter requires editing **5 separate locations** in PolySynth.cpp:
1. `EParams` enum in PolySynth.h
2. Constructor: `GetParam(k...)->InitDouble(...)`
3. `OnParamChange`: switch case
4. `SyncUIState`: `GetParam(k...)->Set(mState.field * scale)`
5. `SynthState.h`: add the field

A metadata table reduces this to **2 locations**: the table entry and the SynthState field. This is a significant maintainability improvement and eliminates an entire class of bugs (forgetting to update one of the 5 locations).

## Prerequisites

- All other sprints should be complete first (this is the highest-risk sprint).
- **Sprint 2 must be complete** — PolySynth_DSP.h is already deleted, so PolySynth.h is the only platform file to modify.

## Background Reading

- `src/platform/desktop/PolySynth.h` — EParams enum (lines 18-63)
- `src/platform/desktop/PolySynth.cpp`:
  - Constructor param init (lines 131-244)
  - SyncUIState (lines 74-128)
  - OnParamChange (lines 930-1115)
- `src/core/SynthState.h` — All parameter fields

---

## Tasks

### Task 7.1: Design the ParamMeta Struct

**What to do:** Create `src/platform/desktop/ParamMeta.h`:

```cpp
#pragma once

#include "../../core/SynthState.h"
#include <cstddef>

// Describes how a UI parameter maps to a SynthState field.
// Used by the constructor, OnParamChange, and SyncUIState to avoid
// hand-writing the same mapping in 3 separate switch statements.
struct ParamMeta {
    // ── Parameter Identity ──
    int paramId;        // EParams enum value (e.g., kParamGain)
    const char* name;   // Display name (e.g., "Gain")
    const char* group;  // Group label (e.g., "ADSR", "Filter") or nullptr
    const char* unit;   // Unit suffix (e.g., "%", "ms", "Hz") or nullptr

    // ── Value Range ──
    double min;
    double max;
    double step;        // UI increment step
    double defaultVal;  // Default value in UI units

    // ── UI ↔ State Mapping ──
    // toState: how to convert UI value to SynthState value
    //   kDivide: state = ui / divisor  (e.g., 50% → 0.5, divisor=100)
    //   kDirect: state = ui            (e.g., cutoff Hz)
    //   kCast:   state = (int)ui       (e.g., waveform enum)
    //   kInvert: state = 1.0 - (ui / divisor)  (e.g., limiter threshold)
    enum class MapKind { kDivide, kDirect, kCast, kInvert };
    MapKind mapKind;
    double divisor;     // Used by kDivide and kInvert

    // ── SynthState Field Access ──
    // Offset of the double/int field within SynthState.
    // We use offsetof() for type-safe field access.
    size_t fieldOffset;
    bool isInt;         // true if the SynthState field is int (not double)

    // ── iPlug2 Shape (optional) ──
    // nullptr = linear, "exp" = ShapeExp, "pow3" = ShapePowCurve(3)
    const char* shape;
};

// IMPORTANT: The table only supports `double` and `int` SynthState fields.
// `bool` fields (oscASync, oscBLoFreq, filterKeyboardTrack, etc.) use
// InitBool/InitEnum and MUST be handled as special cases — never add a
// bool field to kParamTable. The ApplyParamToState/StateToUIValue helpers
// do not handle bool types.
```

### Task 7.2: Build the Parameter Table

**What to do:** In `ParamMeta.h` (or a separate `ParamTable.h`), define the complete table. This is the single source of truth for UI↔State mapping.

**Critical:** Every value in this table must exactly match the current constructor/OnParamChange/SyncUIState behaviour. Verify each row against the existing code.

```cpp
#include "PolySynth.h" // For EParams enum

// Helper macros for field offset calculation
#define PM_DOUBLE(field) offsetof(PolySynthCore::SynthState, field), false
#define PM_INT(field) offsetof(PolySynthCore::SynthState, field), true

static const ParamMeta kParamTable[] = {
    // paramId, name, group, unit, min, max, step, default, mapKind, divisor, fieldOffset, isInt, shape

    // ── Global ──
    {kParamGain, "Gain", nullptr, "%",
     0., 100., 1.25, 75.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(masterGain), nullptr},

    {kParamNoteGlideTime, "Glide", nullptr, "ms",
     0., 30., 0.1, 0.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(glideTime), nullptr},

    // ── Amp Envelope ──
    {kParamAttack, "Attack", "ADSR", "ms",
     1., 1000., 0.1, 10.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampAttack), "pow3"},

    {kParamDecay, "Decay", "ADSR", "ms",
     1., 1000., 0.1, 100.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampDecay), "pow3"},

    {kParamSustain, "Sustain", "ADSR", "%",
     0., 100., 1., 100.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(ampSustain), nullptr},

    {kParamRelease, "Release", "ADSR", "ms",
     2., 1000., 0.1, 100.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampRelease), nullptr},

    // ── LFO ──
    // NOTE: LFO Shape and LFO Rate use special iPlug2 init methods
    // (InitEnum, InitFrequency). These are handled as special cases.

    {kParamLFODepth, "Dep", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(lfoDepth), nullptr},

    // ── Filter ──
    {kParamFilterCutoff, "Cutoff", "Filter", "Hz",
     20., 20000., 1., 2000.,
     ParamMeta::MapKind::kDirect, 1.0,
     PM_DOUBLE(filterCutoff), "exp"},

    {kParamFilterResonance, "Reso", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(filterResonance), nullptr},

    {kParamFilterEnvAmount, "Contour", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(filterEnvAmount), nullptr},

    // ── Oscillators ──
    // NOTE: OscWave, OscBWave, FilterModel use InitEnum — special cases.

    {kParamOscMix, "Osc Bal", nullptr, "%",
     0., 100., 1., 0.,  // default=0% means fully OscA (mixOscB=0.0, mixOscA=1.0)
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(mixOscB), nullptr},
    // SPECIAL CASE: OscMix writes to mixOscB via the table, but also requires
    // setting mixOscA = 1.0 - mixOscB. This is handled explicitly in OnParamChange
    // after the table write. See Task 7.6.

    {kParamOscPulseWidthA, "PWA", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(oscAPulseWidth), nullptr},

    {kParamOscPulseWidthB, "PWB", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(oscBPulseWidth), nullptr},

    // ── Poly-Mod ──
    {kParamPolyModOscBToFreqA, "FM Depth", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToFreqA), nullptr},

    {kParamPolyModOscBToPWM, "PWM Mod", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToPWM), nullptr},

    {kParamPolyModOscBToFilter, "B-V", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToFilter), nullptr},

    {kParamPolyModFilterEnvToFreqA, "Env FM", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToFreqA), nullptr},

    {kParamPolyModFilterEnvToPWM, "Env PWM", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToPWM), nullptr},

    {kParamPolyModFilterEnvToFilter, "Env VCF", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToFilter), nullptr},

    // ── FX: Chorus ──
    // NOTE: ChorusRate uses InitFrequency — special case.

    {kParamChorusDepth, "Dep", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxChorusDepth), nullptr},

    {kParamChorusMix, "Mix", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxChorusMix), nullptr},

    // ── FX: Delay ──
    // NOTE: DelayTime uses InitMilliseconds — special case.

    {kParamDelayFeedback, "Fbk", nullptr, "%",
     0., 95., 1., 35.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxDelayFeedback), nullptr},

    {kParamDelayMix, "Mix", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxDelayMix), nullptr},

    // ── FX: Limiter ──
    {kParamLimiterThreshold, "Lmt", nullptr, "%",
     0., 100., 1., 5.,
     ParamMeta::MapKind::kInvert, 100.0,
     PM_DOUBLE(fxLimiterThreshold), nullptr},

    // ── Voice Allocation ──
    {kParamPolyphonyLimit, "Poly", nullptr, nullptr,
     1., 16., 1., 8.,
     ParamMeta::MapKind::kCast, 1.0,
     PM_INT(polyphony), nullptr},

    // NOTE: AllocationMode, StealPriority use InitEnum — special cases.

    {kParamUnisonCount, "Uni", nullptr, nullptr,
     1., 8., 1., 1.,
     ParamMeta::MapKind::kCast, 1.0,
     PM_INT(unisonCount), nullptr},

    {kParamUnisonSpread, "Sprd", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(unisonSpread), nullptr},

    {kParamStereoSpread, "Width", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(stereoSpread), nullptr},
};

static constexpr int kParamTableSize = sizeof(kParamTable) / sizeof(kParamTable[0]);

#undef PM_DOUBLE
#undef PM_INT
```

### Task 7.3: Identify Special Cases

**These parameters cannot use the generic table** and must be handled individually:

1. **`kParamLFOShape`** — Uses `InitEnum` with string labels `{"Sin", "Tri", "Sqr", "Saw"}`
2. **`kParamLFORateHz`** — Uses `InitFrequency` (logarithmic scale)
3. **`kParamOscWave`** — Uses `InitEnum` with `{"SAW", "SQR", "TRI", "SIN"}`
4. **`kParamOscBWave`** — Uses `InitEnum`
5. **`kParamFilterModel`** — Uses `InitEnum` with `{"CL", "LD", "P12", "P24"}`
6. **`kParamChorusRate`** — Uses `InitFrequency`
7. **`kParamDelayTime`** — Uses `InitMilliseconds`
8. **`kParamAllocationMode`** — Uses `InitEnum`
9. **`kParamStealPriority`** — Uses `InitEnum`
10. **`kParamPresetSelect`** — Uses `InitEnum` with 16 preset slots
11. **`kParamDemoMono/Poly/FX`** — Uses `InitBool` with toggle logic
12. **`kParamOscMix`** — Sets TWO fields: `mixOscA = 1.0 - val/100` and `mixOscB = val/100`

These special cases get their own explicit code blocks (not the table loop).

### Task 7.4: Implement Helper Functions

**What to do:** Add these helpers to `ParamMeta.h` or as inline functions in PolySynth.cpp:

```cpp
// Write a UI value to the corresponding SynthState field.
inline void ApplyParamToState(const ParamMeta& meta, double uiValue,
                              PolySynthCore::SynthState& state) {
    char* base = reinterpret_cast<char*>(&state);
    double stateValue;
    switch (meta.mapKind) {
    case ParamMeta::MapKind::kDivide:
        stateValue = uiValue / meta.divisor;
        break;
    case ParamMeta::MapKind::kDirect:
        stateValue = uiValue;
        break;
    case ParamMeta::MapKind::kCast:
        *reinterpret_cast<int*>(base + meta.fieldOffset) = static_cast<int>(uiValue);
        return;
    case ParamMeta::MapKind::kInvert:
        stateValue = 1.0 - (uiValue / meta.divisor);
        break;
    }
    *reinterpret_cast<double*>(base + meta.fieldOffset) = stateValue;
}

// Read a SynthState field and convert to UI value.
inline double StateToUIValue(const ParamMeta& meta,
                             const PolySynthCore::SynthState& state) {
    const char* base = reinterpret_cast<const char*>(&state);
    if (meta.isInt) {
        int intVal = *reinterpret_cast<const int*>(base + meta.fieldOffset);
        return static_cast<double>(intVal);
    }
    double stateValue = *reinterpret_cast<const double*>(base + meta.fieldOffset);
    switch (meta.mapKind) {
    case ParamMeta::MapKind::kDivide:
        return stateValue * meta.divisor;
    case ParamMeta::MapKind::kDirect:
        return stateValue;
    case ParamMeta::MapKind::kCast:
        return stateValue;
    case ParamMeta::MapKind::kInvert:
        return (1.0 - stateValue) * meta.divisor;
    }
    return stateValue;
}

// Find the table entry for a given param ID, or nullptr.
inline const ParamMeta* FindParamMeta(int paramId) {
    for (int i = 0; i < kParamTableSize; ++i) {
        if (kParamTable[i].paramId == paramId)
            return &kParamTable[i];
    }
    return nullptr;
}
```

### Task 7.5: Refactor Constructor

**What to do:** Replace the individual `GetParam(k...)->InitDouble(...)` calls with a loop:

```cpp
// Table-driven parameter registration
for (int i = 0; i < kParamTableSize; ++i) {
    const auto& m = kParamTable[i];
    if (m.shape && strcmp(m.shape, "exp") == 0) {
        GetParam(m.paramId)->InitDouble(m.name, m.defaultVal, m.min, m.max,
                                         m.step, m.unit ? m.unit : "",
                                         IParam::kFlagsNone,
                                         m.group ? m.group : "",
                                         IParam::ShapeExp());
    } else if (m.shape && strcmp(m.shape, "pow3") == 0) {
        GetParam(m.paramId)->InitDouble(m.name, m.defaultVal, m.min, m.max,
                                         m.step, m.unit ? m.unit : "",
                                         IParam::kFlagsNone,
                                         m.group ? m.group : "",
                                         IParam::ShapePowCurve(3.));
    } else {
        GetParam(m.paramId)->InitDouble(m.name, m.defaultVal, m.min, m.max,
                                         m.step, m.unit ? m.unit : "");
    }
}

// Special cases: enum, frequency, milliseconds, bool params
GetParam(kParamLFOShape)->InitEnum("LFO", state.lfoShape, {"Sin", "Tri", "Sqr", "Saw"});
GetParam(kParamLFORateHz)->InitFrequency("Rate", state.lfoRate, 0.01, 40.);
// ... (keep all InitEnum/InitFrequency/InitMilliseconds/InitBool calls as-is)
```

### Task 7.6: Refactor OnParamChange

**What to do:** Replace the 120-line switch with:

```cpp
void PolySynthPlugin::OnParamChange(int paramIdx) {
    if (mIsUpdatingUI)
        return;

    mIsDirty = true;
#if IPLUG_EDITOR
    if (auto *pUI = GetUI()) {
        if (auto *pBtn = pUI->GetControlWithTag(kCtrlTagSaveBtn)) {
            ((PresetSaveButton *)pBtn)->SetHasUnsavedChanges(true);
        }
    }
#endif

    double value = GetParam(paramIdx)->Value();

    // ── Table-driven parameters ──
    const ParamMeta* meta = FindParamMeta(paramIdx);
    if (meta) {
        ApplyParamToState(*meta, value, mState);

        // Special: OscMix also sets mixOscA
        if (paramIdx == kParamOscMix) {
            mState.mixOscA = 1.0 - mState.mixOscB;
        }

        mStateQueue.TryPush(mState);
        return;
    }

    // ── Special cases (enums, demos, presets) ──
    switch (paramIdx) {
    case kParamLFOShape:
        mState.lfoShape = static_cast<int>(value);
        break;
    case kParamLFORateHz:
        mState.lfoRate = value;
        break;
    case kParamOscWave:
        mState.oscAWaveform = static_cast<int>(value);
        break;
    case kParamOscBWave:
        mState.oscBWaveform = static_cast<int>(value);
        break;
    case kParamFilterModel:
        mState.filterModel = static_cast<int>(value);
        break;
    case kParamChorusRate:
        mState.fxChorusRate = value;
        break;
    case kParamDelayTime:
        mState.fxDelayTime = value / 1000.0;
        break;
    case kParamAllocationMode:
        mState.allocationMode = static_cast<int>(value);
        break;
    case kParamStealPriority:
        mState.stealPriority = static_cast<int>(value);
        break;
    case kParamPresetSelect:
        mIsDirty = false;
        OnMessage(kMsgTagLoadPreset, (int)value, 0, nullptr);
        return; // Don't push state for preset selection
    case kParamDemoMono:
    case kParamDemoPoly:
    case kParamDemoFX:
        // Keep demo button logic as-is (unchanged from original)
        HandleDemoButton(paramIdx, value);
        return;
    default:
        break;
    }

    mStateQueue.TryPush(mState);
}
```

**Note:** Extract the demo button logic into a private helper `HandleDemoButton(int paramIdx, double value)` to keep OnParamChange clean.

### Task 7.7: Refactor SyncUIState

**What to do:** Replace the 55-line method with:

```cpp
void PolySynthPlugin::SyncUIState() {
    mIsUpdatingUI = true;

    // Table-driven sync
    for (int i = 0; i < kParamTableSize; ++i) {
        const auto& m = kParamTable[i];
        GetParam(m.paramId)->Set(StateToUIValue(m, mState));
    }

    // Special cases: enums and direct-value params
    GetParam(kParamLFOShape)->Set(static_cast<double>(mState.lfoShape));
    GetParam(kParamLFORateHz)->Set(mState.lfoRate);
    GetParam(kParamOscWave)->Set(static_cast<double>(mState.oscAWaveform));
    GetParam(kParamOscBWave)->Set(static_cast<double>(mState.oscBWaveform));
    GetParam(kParamFilterModel)->Set(static_cast<double>(mState.filterModel));
    GetParam(kParamChorusRate)->Set(mState.fxChorusRate);
    GetParam(kParamDelayTime)->Set(mState.fxDelayTime * 1000.0);
    GetParam(kParamAllocationMode)->Set(static_cast<double>(mState.allocationMode));
    GetParam(kParamStealPriority)->Set(static_cast<double>(mState.stealPriority));

    // Demo buttons
    GetParam(kParamDemoMono)->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::Mono ? 1.0 : 0.0);
    GetParam(kParamDemoPoly)->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::Poly ? 1.0 : 0.0);
    GetParam(kParamDemoFX)->Set(mDemoSequencer.GetMode() == DemoSequencer::Mode::FX ? 1.0 : 0.0);

    for (int i = 0; i < kNumParams; ++i) {
        SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }

    mIsUpdatingUI = false;
    mStateQueue.TryPush(mState);
}
```

### Task 7.8: Write Coverage Test — Every EParams Has a Handler

**What to do:** Add a test that verifies every `EParams` value (except demo/preset params) is either in the table or in the special-case switch:

This test goes in `tests/unit/Test_ParamTable.cpp`:
```cpp
#define CATCH_CONFIG_PREFIX_ALL
#include "catch.hpp"

// We can't include ParamMeta.h directly in the test build (it needs iPlug2).
// Instead, test that the EParams enum values are contiguous and counted correctly.
// The actual coverage test runs as part of desktop-build validation.

// If ParamMeta.h is available in the test build:
// Verify kParamTableSize + kSpecialCaseCount == kNumParams - kNumDemoParams
```

Alternatively, add a `static_assert` in PolySynth.cpp:
```cpp
// Verify every parameter is handled.
// kParamTableSize = table-driven params
// 9 = special enum/frequency params (LFOShape, LFORateHz, OscWave, OscBWave,
//     FilterModel, ChorusRate, DelayTime, AllocationMode, StealPriority)
// 4 = non-synth params (PresetSelect, DemoMono, DemoPoly, DemoFX)
static_assert(kParamTableSize + 9 + 4 == kNumParams,
              "Every EParams must be handled by either the table or special cases");
```

### Task 7.9: Extract HandleDemoButton Helper

**What to do:** Move the demo button toggle logic (currently ~40 lines across 3 switch cases) into a private method:

```cpp
// In PolySynth.h, add declaration:
void HandleDemoButton(int paramIdx, double value);

// In PolySynth.cpp, implement:
void PolySynthPlugin::HandleDemoButton(int paramIdx, double value) {
    if (value <= 0.5) return; // Ignore release

    // ... existing toggle logic for kParamDemoMono/kParamDemoPoly/kParamDemoFX ...
}
```

---

## Cleanup & Style Improvements

1. **Remove redundant comments** — The old OnParamChange had no comments (each case was self-documenting). The new table makes the mapping even more explicit.
2. **Consistent ordering** — The kParamTable entries should follow the same order as the EParams enum.
3. **Update ADDING_A_PARAMETER.md** — Currently describes 9+ steps. Update to reflect the new workflow:
   - Step 1: Add field to SynthState
   - Step 2: Add enum to EParams
   - Step 3: Add entry to kParamTable (or special-case if enum/frequency)
   - Step 4: Update UI layout if needed
   - Step 5: Wire in Engine::UpdateState if needed
   - Step 6: Update tests

---

## Files Changed

| File | Action | Description |
|------|--------|-------------|
| `src/platform/desktop/ParamMeta.h` | **NEW** | ParamMeta struct, table, helper functions |
| `src/platform/desktop/PolySynth.h` | **MODIFIED** | Add HandleDemoButton declaration |
| `src/platform/desktop/PolySynth.cpp` | **MODIFIED** | Refactor constructor, OnParamChange, SyncUIState |
| `docs/architecture/ADDING_A_PARAMETER.md` | **MODIFIED** | Updated workflow for table-driven params |

---

## Testing Instructions

```bash
just build                                    # Test build (may not cover platform code)
just test                                     # All tests pass
just desktop-build                            # CRITICAL: desktop build must compile
just desktop-smoke                            # CRITICAL: plugin launches without crash
python3 scripts/golden_master.py --verify     # Audio unchanged
```

### Manual verification checklist
After building the desktop plugin:
1. Open in a DAW or standalone
2. Verify each knob/slider still controls its parameter (spot-check 5-6)
3. Load a preset → verify all parameters restore correctly
4. Tweak parameters → save preset → reload → verify round-trip
5. Demo buttons still work (Mono, Poly, FX)
6. Sustain pedal still works via MIDI

---

## Definition of Done

- [ ] `ParamMeta.h` exists with the complete parameter table
- [ ] OnParamChange switch reduced to ≤20 lines (table lookup + special cases)
- [ ] SyncUIState reduced to a single loop + special cases
- [ ] Constructor uses table loop for `InitDouble` parameters
- [ ] `static_assert` verifies every EParams is handled
- [ ] Demo button logic extracted to `HandleDemoButton()`
- [ ] `ADDING_A_PARAMETER.md` updated with new workflow
- [ ] `just build` — clean compilation
- [ ] `just test` — all tests pass
- [ ] `just desktop-build` — desktop build compiles
- [ ] `just desktop-smoke` — plugin launches without crash
- [ ] `python3 scripts/golden_master.py --verify` — golden masters unchanged
- [ ] Manual spot-check: 5+ parameters verified in running plugin

---

## Common Pitfalls

1. **offsetof with non-POD types** — `SynthState` is an aggregate (verified by `static_assert`), so `offsetof` is safe. But some compilers warn. Use `__builtin_offsetof` if needed.
2. **OscMix dual-field update** — `kParamOscMix` maps to `mixOscB` but also sets `mixOscA = 1.0 - mixOscB`. This MUST be handled as a special case after the table write.
3. **Limiter threshold inversion** — UI shows 0-100% where 0% = no limiting (threshold=1.0). The `kInvert` map kind handles this but verify carefully.
4. **Enum params need InitEnum, not InitDouble** — The table only handles `InitDouble` params. Enum params (LFOShape, OscWave, etc.) keep their individual `InitEnum` calls.
5. **reinterpret_cast alignment** — SynthState fields are naturally aligned (all doubles at 8-byte boundaries, ints at 4-byte). `offsetof` guarantees correct alignment.
6. **Don't break the SPSC queue** — Every code path in OnParamChange (both table and special cases) must end with `mStateQueue.TryPush(mState)` — except preset selection which triggers its own load path.
