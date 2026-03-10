#pragma once

#include <atomic>
#include <cstdint>

#include "Engine.h"
#include "SynthState.h"
#include "SPSCQueue.h"

// ── Audio command sent from main loop to ISR ──────────────────────────────
struct AudioCommand {
    enum Type : uint8_t { NONE, NOTE_ON, NOTE_OFF, UPDATE_STATE, PANIC };
    Type type = NONE;
    uint8_t arg1 = 0;  // note
    uint8_t arg2 = 0;  // velocity
};

// ── Voice-change event sent from ISR to main loop ─────────────────────────
struct VoiceEvent {
    int8_t from;
    int8_t to;
};

// ── PicoSynthApp: owns the DSP engine and all cross-core communication ────
class PicoSynthApp {
public:
    void Init(float sampleRate);

    // Commands (called from Core 0 main loop — enqueue for ISR)
    void NoteOn(uint8_t note, uint8_t velocity);
    void NoteOff(uint8_t note);
    bool SetParam(const char* name, float value);
    bool GetParam(const char* name, float& out) const;
    void PushStateUpdate();
    void Panic();
    void Reset();

    // Called from DMA ISR on Core 1
    void AudioCallback(uint32_t* buffer, uint32_t numFrames);

    // Diagnostics (called from Core 0 main loop)
    int GetActiveVoiceCount() const;
    float ExchangePeakLevel();
    bool DrainVoiceEvent(int8_t& from, int8_t& to);

    // Access for demo/command dispatch
    PolySynthCore::SynthState& GetPendingState() { return mPendingState; }
    const PolySynthCore::SynthState& GetPendingState() const { return mPendingState; }

    // Direct engine access for self-test
    PolySynthCore::Engine& GetEngine() { return mEngine; }

private:
    PolySynthCore::Engine mEngine;

    // State handoff (main → ISR via UPDATE_STATE)
    PolySynthCore::SynthState mPendingState;
    PolySynthCore::SynthState mStagedState;
    std::atomic<bool> mStateConsumed{true};

    // SPSC command queue (main loop → ISR)
    SPSCQueue<AudioCommand, 16> mCommandQueue;

    // Diagnostics (ISR → main loop)
    std::atomic<int> mActiveVoices{0};
    std::atomic<float> mPeakLevel{0.0f};

    // Voice-change event ring buffer (ISR → main loop)
    static constexpr int kVoiceEventBufSize = 32;
    VoiceEvent mVoiceEvents[kVoiceEventBufSize] = {};
    std::atomic<int> mVoiceEventHead{0};
    int mVoiceEventTail = 0;
    int mPrevVoiceCount = 0;  // ISR-private: only accessed from AudioCallback
};
