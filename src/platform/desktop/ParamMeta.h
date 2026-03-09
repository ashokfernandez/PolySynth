#pragma once

#include "PolySynth.h" // EParams enum + SynthState (via transitive include)
#include <cstddef>
#include <type_traits>

static_assert(std::is_standard_layout_v<PolySynthCore::SynthState>,
              "SynthState must be standard-layout for offsetof-based access");

// Describes how a UI parameter maps to a SynthState field.
// Used by the constructor, OnParamChange, and SyncUIState to avoid
// hand-writing the same mapping in 3 separate switch statements.
struct ParamMeta {
  // ── Parameter Identity ──
  int paramId;      // EParams enum value (e.g., kParamGain)
  const char *name; // Display name (e.g., "Gain")
  const char *group; // Group label (e.g., "ADSR", "Filter") or nullptr
  const char *unit;  // Unit suffix (e.g., "%", "ms", "Hz") or nullptr

  // ── Value Range ──
  double min;
  double max;
  double step;       // UI increment step
  double defaultVal; // Default value in UI units

  // ── UI ↔ State Mapping ──
  //   kDivide: state = ui / divisor  (e.g., 50% → 0.5, divisor=100)
  //   kDirect: state = ui            (e.g., cutoff Hz)
  //   kCast:   state = (int)ui       (e.g., polyphony count)
  //   kInvert: state = 1.0 - (ui / divisor)  (e.g., limiter threshold)
  enum class MapKind { kDivide, kDirect, kCast, kInvert };
  MapKind mapKind;
  double divisor; // Used by kDivide and kInvert

  // ── SynthState Field Access ──
  size_t fieldOffset; // offsetof() into SynthState
  bool isInt;         // true if the SynthState field is int (not double)

  // ── iPlug2 Init & Shape ──
  //   kInitDouble:       InitDouble (most params)
  //   kInitInt:          InitInt    (polyphony, unison count)
  //   kInitMilliseconds: InitMilliseconds (glide time)
  enum class InitKind { kInitDouble, kInitInt, kInitMilliseconds };
  InitKind initKind;

  // Only used when initKind == kInitDouble.
  enum class ShapeKind { kLinear, kExp, kPow3 };
  ShapeKind shape;
};

// IMPORTANT: The table only supports `double` and `int` SynthState fields.
// `bool` fields (oscASync, oscBLoFreq, filterKeyboardTrack, etc.) and
// enum/frequency/bool params use InitEnum/InitFrequency/InitBool and MUST
// be handled as special cases — never add them to kParamTable.

// ── Helper macros for field offset calculation ──
#define PM_DOUBLE(field) offsetof(PolySynthCore::SynthState, field), false
#define PM_INT(field) offsetof(PolySynthCore::SynthState, field), true

// clang-format off
static const ParamMeta kParamTable[] = {
    // paramId, name, group, unit, min, max, step, default,
    //   mapKind, divisor, fieldOffset, isInt, initKind, shape

    // ── Global ──
    {kParamGain, "Gain", nullptr, "%",
     0., 100., 1.25, 75.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(masterGain),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamNoteGlideTime, "Glide", nullptr, "ms",
     0., 30., 0.1, 0.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(glideTime),
     ParamMeta::InitKind::kInitMilliseconds, ParamMeta::ShapeKind::kLinear},

    // ── Amp Envelope ──
    {kParamAttack, "Attack", "ADSR", "ms",
     1., 1000., 0.1, 10.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampAttack),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kPow3},

    {kParamDecay, "Decay", "ADSR", "ms",
     1., 1000., 0.1, 100.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampDecay),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kPow3},

    {kParamSustain, "Sustain", "ADSR", "%",
     0., 100., 1., 100.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(ampSustain),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamRelease, "Release", "ADSR", "ms",
     2., 1000., 0.1, 100.,
     ParamMeta::MapKind::kDivide, 1000.0,
     PM_DOUBLE(ampRelease),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── LFO ──
    // NOTE: LFO Shape and LFO Rate use InitEnum/InitFrequency — special cases.

    {kParamLFODepth, "Dep", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(lfoDepth),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── Filter ──
    {kParamFilterCutoff, "Cutoff", "Filter", "Hz",
     20., 20000., 1., 2000.,
     ParamMeta::MapKind::kDirect, 1.0,
     PM_DOUBLE(filterCutoff),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kExp},

    {kParamFilterResonance, "Reso", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(filterResonance),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamFilterEnvAmount, "Contour", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(filterEnvAmount),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── Oscillators ──
    // NOTE: OscWave, OscBWave use InitEnum — special cases.

    {kParamOscMix, "Osc Bal", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(mixOscB),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},
    // SPECIAL CASE: OscMix writes to mixOscB via the table, but also requires
    // setting mixOscA = 1.0 - mixOscB. Handled explicitly in OnParamChange.

    {kParamOscPulseWidthA, "PWA", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(oscAPulseWidth),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamOscPulseWidthB, "PWB", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(oscBPulseWidth),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── Poly-Mod ──
    {kParamPolyModOscBToFreqA, "FM Depth", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToFreqA),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamPolyModOscBToPWM, "PWM Mod", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToPWM),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamPolyModOscBToFilter, "B-V", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModOscBToFilter),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamPolyModFilterEnvToFreqA, "Env FM", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToFreqA),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamPolyModFilterEnvToPWM, "Env PWM", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToPWM),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamPolyModFilterEnvToFilter, "Env VCF", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(polyModFilterEnvToFilter),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── FX: Chorus ──
    // NOTE: ChorusRate uses InitFrequency — special case.

    {kParamChorusDepth, "Dep", nullptr, "%",
     0., 100., 1., 50.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxChorusDepth),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamChorusMix, "Mix", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxChorusMix),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── FX: Delay ──
    // NOTE: DelayTime uses InitMilliseconds — special case (not in table).

    {kParamDelayFeedback, "Fbk", nullptr, "%",
     0., 95., 1., 35.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxDelayFeedback),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamDelayMix, "Mix", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(fxDelayMix),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── FX: Limiter ──
    {kParamLimiterThreshold, "Lmt", nullptr, "%",
     0., 100., 1., 5.,
     ParamMeta::MapKind::kInvert, 100.0,
     PM_DOUBLE(fxLimiterThreshold),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    // ── Voice Allocation ──
    // NOTE: AllocationMode, StealPriority use InitEnum — special cases.

    {kParamPolyphonyLimit, "Poly", nullptr, nullptr,
     1., 16., 1., 8.,
     ParamMeta::MapKind::kCast, 1.0,
     PM_INT(polyphony),
     ParamMeta::InitKind::kInitInt, ParamMeta::ShapeKind::kLinear},

    {kParamUnisonCount, "Uni", nullptr, nullptr,
     1., 8., 1., 1.,
     ParamMeta::MapKind::kCast, 1.0,
     PM_INT(unisonCount),
     ParamMeta::InitKind::kInitInt, ParamMeta::ShapeKind::kLinear},

    {kParamUnisonSpread, "Sprd", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(unisonSpread),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},

    {kParamStereoSpread, "Width", nullptr, "%",
     0., 100., 1., 0.,
     ParamMeta::MapKind::kDivide, 100.0,
     PM_DOUBLE(stereoSpread),
     ParamMeta::InitKind::kInitDouble, ParamMeta::ShapeKind::kLinear},
};
// clang-format on

static constexpr int kParamTableSize =
    sizeof(kParamTable) / sizeof(kParamTable[0]);

#undef PM_DOUBLE
#undef PM_INT

// ── Verify every EParams is handled ──
// kParamTableSize = table-driven params (28)
// 9 = special enum/frequency/milliseconds params:
//     LFOShape, LFORateHz, OscWave, OscBWave, FilterModel,
//     ChorusRate, DelayTime, AllocationMode, StealPriority
// 4 = non-synth params: PresetSelect, DemoMono, DemoPoly, DemoFX
static_assert(kParamTableSize + 9 + 4 == kNumParams,
              "Every EParams must be handled by either the table or "
              "special cases — update kParamTable or the special-case count");

// ── Helper Functions ──

// Write a UI value to the corresponding SynthState field.
inline void ApplyParamToState(const ParamMeta &meta, double uiValue,
                              PolySynthCore::SynthState &state) {
  char *base = reinterpret_cast<char *>(&state);
  double stateValue;
  switch (meta.mapKind) {
  case ParamMeta::MapKind::kDivide:
    stateValue = uiValue / meta.divisor;
    break;
  case ParamMeta::MapKind::kDirect:
    stateValue = uiValue;
    break;
  case ParamMeta::MapKind::kCast:
    *reinterpret_cast<int *>(base + meta.fieldOffset) =
        static_cast<int>(uiValue);
    return;
  case ParamMeta::MapKind::kInvert:
    stateValue = 1.0 - (uiValue / meta.divisor);
    break;
  }
  *reinterpret_cast<double *>(base + meta.fieldOffset) = stateValue;
}

// Read a SynthState field and convert to UI value.
inline double StateToUIValue(const ParamMeta &meta,
                             const PolySynthCore::SynthState &state) {
  const char *base = reinterpret_cast<const char *>(&state);
  if (meta.isInt) {
    int intVal = *reinterpret_cast<const int *>(base + meta.fieldOffset);
    return static_cast<double>(intVal);
  }
  double stateValue =
      *reinterpret_cast<const double *>(base + meta.fieldOffset);
  switch (meta.mapKind) {
  case ParamMeta::MapKind::kDivide:
    return stateValue * meta.divisor;
  case ParamMeta::MapKind::kDirect:
    return stateValue;
  case ParamMeta::MapKind::kCast:
    return stateValue; // unreachable for int fields (handled above)
  case ParamMeta::MapKind::kInvert:
    return (1.0 - stateValue) * meta.divisor;
  }
  return stateValue;
}

// O(1) lookup: paramId → table entry (or nullptr for special-case params).
// Lazily built on first call; the table has kNumParams slots.
inline const ParamMeta *FindParamMeta(int paramId) {
  static const ParamMeta *lookup[kNumParams] = {};
  static bool initialized = false;
  if (!initialized) {
    for (int i = 0; i < kParamTableSize; ++i)
      lookup[kParamTable[i].paramId] = &kParamTable[i];
    initialized = true;
  }
  if (paramId < 0 || paramId >= kNumParams)
    return nullptr;
  return lookup[paramId];
}
