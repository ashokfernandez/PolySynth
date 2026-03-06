#pragma once

#include "types.h"

namespace PolySynthCore {

// ============================================================================
// Voice: Resonance Mapping
// ============================================================================

// Maps UI resonance (0.0-1.0) to biquad filter Q value.
// Q = kResonanceBaseQ + (res^2 * kResonanceScale)
// At res=0: Q=0.5 (gentle rolloff). At res=1: Q=20.5 (sharp peak).
constexpr sample_t kResonanceBaseQ = 0.5;
constexpr sample_t kResonanceScale = 20.0;

// ============================================================================
// Voice: LFO Modulation Scaling
// ============================================================================

// LFO pitch modulation: scales lfoVal * depth to a frequency multiplier.
// At depth=1.0 and lfoVal=1.0: frequency shifts by +/-5% (~0.85 semitones).
constexpr sample_t kLfoPitchScale = 0.05;

// ============================================================================
// Voice: Poly-Mod PWM Scaling
// ============================================================================

// Scales OscB/FilterEnv modulation of pulse width.
// At full modulation (1.0), PW shifts by +/-0.5 around the base PW.
constexpr sample_t kPwmModScale = 0.5;

// ============================================================================
// Voice: Filter Envelope Scaling
// ============================================================================

// Maximum Hz range added by the filter envelope.
// When filterEnvAmount=1.0 and envelope=1.0, cutoff increases by 10,000 Hz.
constexpr sample_t kFilterEnvMaxHz = 10000.0;

// ============================================================================
// Voice: Portamento / Glide
// ============================================================================

// Exponential smoothing time constant for portamento.
// Higher values = faster convergence. 3.0 means ~95% convergence in glideTime seconds.
constexpr sample_t kGlideTimeConstant = 3.0;

// Snap threshold (Hz): when gliding frequency is within this distance of target,
// snap immediately to avoid asymptotic creep.
constexpr sample_t kGlideSnapThresholdHz = 0.01;

// ============================================================================
// Voice: Voice Stealing
// ============================================================================

// Minimum fade time (seconds) when a voice is stolen.
// 20ms prevents audible clicks during voice stealing.
constexpr sample_t kStolenFadeTimeSec = 0.020;

// ============================================================================
// Engine: Chorus FX
// ============================================================================

// Maps UI depth (0.0-1.0) to chorus delay modulation depth in milliseconds.
// At depth=1.0: +/-5ms of delay modulation.
constexpr sample_t kChorusDepthMs = 5.0;

// ============================================================================
// Engine: Delay FX
// ============================================================================

// Maximum delay line length in milliseconds.
constexpr sample_t kMaxDelayMs = 2000.0;

// Conversion factor: UI delay time (0.0-1.2 seconds) to internal milliseconds.
constexpr sample_t kDelayTimeToMs = 1000.0;

// Conversion factor: UI feedback (0.0-0.95) to internal percentage (0-95).
constexpr sample_t kDelayFeedbackScale = 100.0;

// Conversion factor: UI mix (0.0-1.0) to internal percentage (0-100).
constexpr sample_t kDelayMixScale = 100.0;

// ============================================================================
// Engine: Limiter FX
// ============================================================================

// Default lookahead time in milliseconds for the lookahead limiter.
constexpr sample_t kLimiterLookaheadMs = 5.0;

// Default release time in milliseconds for the lookahead limiter.
constexpr sample_t kLimiterReleaseMs = 50.0;

// ============================================================================
// Engine: OscB Fine Tune Scaling
// ============================================================================

// Multiplier to convert UI fine tune (semitones, +/-12) to cents for detune.
constexpr sample_t kFineTuneToCents = 100.0;

static_assert(kResonanceBaseQ > 0.0);
static_assert(kResonanceScale > 0.0);
static_assert(kLfoPitchScale > 0.0);
static_assert(kPwmModScale > 0.0);
static_assert(kFilterEnvMaxHz > 0.0);
static_assert(kGlideTimeConstant > 0.0);
static_assert(kGlideSnapThresholdHz > 0.0);
static_assert(kStolenFadeTimeSec > 0.0);
static_assert(kChorusDepthMs > 0.0);
static_assert(kMaxDelayMs > 0.0);
static_assert(kDelayTimeToMs > 0.0);
static_assert(kDelayFeedbackScale > 0.0);
static_assert(kDelayMixScale > 0.0);
static_assert(kLimiterLookaheadMs > 0.0);
static_assert(kLimiterReleaseMs > 0.0);
static_assert(kFineTuneToCents > 0.0);

} // namespace PolySynthCore
