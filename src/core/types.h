#pragma once

#include <sea_dsp/sea_platform.h>
#include <stdint.h>
#include <type_traits>

namespace PolySynthCore {

// Fixed precision mode: enabled by POLYSYNTH_USE_FLOAT or iPlug2's
// SAMPLE_TYPE_FLOAT (set automatically for WAM/WASM builds).
#if defined(POLYSYNTH_USE_FLOAT) || defined(SAMPLE_TYPE_FLOAT)
using sample_t = float;
#else
using sample_t = double;
#endif

static_assert(std::is_same_v<sample_t, sea::Real>,
              "sample_t and sea::Real must be the same type");

// Constants — use sample_t to avoid double-precision on embedded (Cortex-M33 has
// single-precision FPU only; double ops fall to software emulation ~10x slower).
constexpr sample_t kPi = static_cast<sample_t>(3.14159265358979323846);
constexpr sample_t kTwoPi = sample_t(2) * kPi;
constexpr int kMaxBlockSize = 4096;

// Compile-time voice count — override with -DPOLYSYNTH_MAX_VOICES=N
#ifndef POLYSYNTH_MAX_VOICES
#define POLYSYNTH_MAX_VOICES 16
#endif
constexpr int kMaxVoices = POLYSYNTH_MAX_VOICES;

// Voice lifecycle states
enum class VoiceState : uint8_t {
  Idle = 0,
  Attack,
  Sustain, // Sprint 1: transition deferred; set in later sprint when phase
           // tracking is added
  Release,
  Stolen
};

// Snapshot of a single voice for UI display (read from audio thread)
struct VoiceRenderState {
  uint8_t voiceID = 0;
  VoiceState state = VoiceState::Idle;
  int note = -1;
  int velocity = 0;
  float currentPitch = 0.0f;
  float panPosition = 0.0f;
  float amplitude = 0.0f;
  float phaseIncrement = 0.0f;  // Oscillator A phase increment (diagnostic)
};

// Event sent from audio thread to UI thread via SPSCRingBuffer.
// Used in Sprint 2 for audio→UI event passing.
struct VoiceEvent {
  enum class Type : uint8_t { NoteOn, NoteOff, Steal, StateChange };
  Type type = Type::NoteOn;
  uint8_t voiceID = 0;
  int note = -1;
  int velocity = 0;
  float pitch = 0.0f;
  float pan = 0.0f;
};

// Note: AllocationMode and StealPriority enums are defined in
// sea_util/sea_voice_allocator.h and not aliased here to avoid circular deps.
// See sea::AllocationMode and sea::StealPriority.

} // namespace PolySynthCore
