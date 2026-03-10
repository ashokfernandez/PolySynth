#pragma once

#include <cstdint>

class PicoSynthApp;

class PicoDemo {
public:
    void Start(uint64_t nowUs);
    void Stop();
    bool IsRunning() const { return mRunning; }
    const char* CurrentPhaseName() const;
    void Update(uint64_t nowUs, PicoSynthApp& app);

private:
    enum class Phase : uint8_t {
        SAW_NOTE,       // 0-2s: C4 saw
        SAW_OFF,        // 2-2.5s: silence
        SQUARE_NOTE,    // 2.5-4.5s: E4 square
        SQUARE_OFF,     // 4.5-5s: silence
        CHORD,          // 5-8s: 4-voice chord
        FILTER_SWEEP,   // 8-11s: filter sweep
        CHORD_OFF,      // 11-13s: silence
        DONE            // loop back to SAW_NOTE
    };

    bool mRunning = false;
    Phase mPhase = Phase::SAW_NOTE;
    uint64_t mPhaseStartUs = 0;
    uint64_t mLastStatePushUs = 0;
};
