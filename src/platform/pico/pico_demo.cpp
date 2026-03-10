#include "pico_demo.h"
#include "pico_synth_app.h"

void PicoDemo::Start(uint64_t nowUs) {
    mRunning = true;
    mPhase = Phase::SAW_NOTE;
    mPhaseStartUs = nowUs;
}

void PicoDemo::Stop() {
    mRunning = false;
}

const char* PicoDemo::CurrentPhaseName() const {
    switch (mPhase) {
        case Phase::SAW_NOTE:     return "SAW";
        case Phase::SAW_OFF:      return "SAW_OFF";
        case Phase::SQUARE_NOTE:  return "SQUARE";
        case Phase::SQUARE_OFF:   return "SQ_OFF";
        case Phase::CHORD:        return "CHORD";
        case Phase::FILTER_SWEEP: return "SWEEP";
        case Phase::CHORD_OFF:    return "CHD_OFF";
        case Phase::DONE:         return "DONE";
        default:                  return "?";
    }
}

void PicoDemo::Update(uint64_t nowUs, PicoSynthApp& app) {
    if (!mRunning) return;

    uint64_t elapsedUs = nowUs - mPhaseStartUs;
    float elapsedS = static_cast<float>(elapsedUs) / 1000000.0f;

    auto advance = [&](Phase next) {
        mPhase = next;
        mPhaseStartUs = nowUs;
    };

    switch (mPhase) {
        case Phase::SAW_NOTE:
            if (elapsedS < 0.001f) {
                auto& st = app.GetPendingState();
                st.Reset();
                st.oscAWaveform = 0;  // Saw
                app.PushStateUpdate();
                app.NoteOn(60, 100);
            }
            if (elapsedS >= 2.0f) {
                app.NoteOff(60);
                advance(Phase::SAW_OFF);
            }
            break;

        case Phase::SAW_OFF:
            if (elapsedS >= 0.5f) {
                advance(Phase::SQUARE_NOTE);
            }
            break;

        case Phase::SQUARE_NOTE:
            if (elapsedS < 0.001f) {
                auto& st = app.GetPendingState();
                st.oscAWaveform = 1;  // Square
                app.PushStateUpdate();
                app.NoteOn(64, 100);
            }
            if (elapsedS >= 2.0f) {
                app.NoteOff(64);
                advance(Phase::SQUARE_OFF);
            }
            break;

        case Phase::SQUARE_OFF:
            if (elapsedS >= 0.5f) {
                advance(Phase::CHORD);
            }
            break;

        case Phase::CHORD:
            if (elapsedS < 0.001f) {
                auto& st = app.GetPendingState();
                st.oscAWaveform = 0;  // Saw
                st.filterCutoff = 2000.0;
                app.PushStateUpdate();
                app.NoteOn(60, 90);
                app.NoteOn(64, 85);
                app.NoteOn(67, 80);
                app.NoteOn(72, 75);
            }
            if (elapsedS >= 3.0f) {
                advance(Phase::FILTER_SWEEP);
            }
            break;

        case Phase::FILTER_SWEEP: {
            if (nowUs - mLastStatePushUs >= 20000ULL) {
                mLastStatePushUs = nowUs;
                float t = elapsedS / 3.0f;
                if (t > 1.0f) t = 1.0f;
                float cutoff = 200.0f + t * (8000.0f - 200.0f);
                auto& st = app.GetPendingState();
                st.filterCutoff = static_cast<double>(cutoff);
                app.PushStateUpdate();
            }
            if (elapsedS >= 3.0f) {
                advance(Phase::CHORD_OFF);
            }
            break;
        }

        case Phase::CHORD_OFF:
            if (elapsedS < 0.001f) {
                app.NoteOff(60);
                app.NoteOff(64);
                app.NoteOff(67);
                app.NoteOff(72);
            }
            if (elapsedS >= 2.0f) {
                advance(Phase::DONE);
            }
            break;

        case Phase::DONE:
            Start(nowUs);  // loop
            break;
    }
}
