#pragma once

#include <sea_dsp/sea_platform.h>
#include <stdint.h>
#include <type_traits>

namespace PolySynthCore {

// Fixed precision mode can be defined here if needed
#ifdef POLYSYNTH_USE_FLOAT
using sample_t = float;
#else
using sample_t = double;
#endif

static_assert(std::is_same_v<sample_t, sea::Real>,
              "sample_t and sea::Real must be the same type");

// Constants
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
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
  Sustain,    // Sprint 1: transition deferred; set in later sprint when phase tracking is added
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

} // namespace PolySynthCore
