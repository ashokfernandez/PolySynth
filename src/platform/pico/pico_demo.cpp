#include "pico_demo.h"
#include "pico_synth_app.h"
#include <array>
#include <cstdio>

#include "types.h"  // kMaxVoices

void PicoDemo::Start(uint64_t nowUs) {
    mRunning = true;
    mPhase = Phase::SAW_NOTE;
    mPhaseStartUs = nowUs;
    mPhaseInitDone = false;
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
        mPhaseInitDone = false;  // Reset one-shot guard on every transition
    };

    switch (mPhase) {
        case Phase::SAW_NOTE:
            if (!mPhaseInitDone) {
                mPhaseInitDone = true;
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
            if (!mPhaseInitDone) {
                mPhaseInitDone = true;
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

        case Phase::CHORD: {
            if (!mPhaseInitDone) {
                mPhaseInitDone = true;
                mChordNotesAdded = 0;
                auto& st = app.GetPendingState();
                st.oscAWaveform = 0;  // Saw
                st.filterCutoff = 8000.0f;
                printf("[DEMO] CHORD: polyphony=%d\n", st.polyphony);
                app.PushStateUpdate();
                app.NoteOn(48, 100);   // C3
                app.NoteOn(55, 100);   // G3
                app.NoteOn(60, 100);   // C4
                app.NoteOn(67, 100);   // G4
                printf("[DEMO] CHORD: sent 4 NoteOns\n");
            }
            // 200ms later, dump per-voice state
            if (mChordNotesAdded == 0 && elapsedS >= 0.2f) {
                mChordNotesAdded = 1;
                printf("[DEMO] voices=%d | ", app.GetActiveVoiceCount());
                std::array<int, 4> notes{};
                int count = app.GetEngine().GetHeldNotes(notes);
                printf("held(%d):", count);
                for (int i = 0; i < count; i++) printf(" %d", notes[i]);
                printf("\n");
                // Per-voice AUDIO peak (actual oscillator output, not just envelope)
                printf("[DEMO] audio peak: v0=%.3f v1=%.3f v2=%.3f v3=%.3f\n",
                       static_cast<double>(app.GetVoicePeak(0)),
                       static_cast<double>(app.GetVoicePeak(1)),
                       static_cast<double>(app.GetVoicePeak(2)),
                       static_cast<double>(app.GetVoicePeak(3)));
                // Per-voice frequency diagnostics
                for (int i = 0; i < 4; i++) {
                    printf("[DEMO] v%d: note=%d freq=%.1f\n",
                           i,
                           app.GetVoiceNote(i),
                           static_cast<double>(app.GetVoiceFreq(i)));
                }
            }
            if (elapsedS >= 3.0f) {
                advance(Phase::FILTER_SWEEP);
            }
            break;
        }

        case Phase::FILTER_SWEEP: {
            if (nowUs - mLastStatePushUs >= 20000ULL) {
                mLastStatePushUs = nowUs;
                float t = elapsedS / 3.0f;
                if (t > 1.0f) t = 1.0f;
                float cutoff = 200.0f + t * (8000.0f - 200.0f);
                auto& st = app.GetPendingState();
                st.filterCutoff = cutoff;
                app.PushStateUpdate();
            }
            if (elapsedS >= 3.0f) {
                advance(Phase::CHORD_OFF);
            }
            break;
        }

        case Phase::CHORD_OFF:
            if (!mPhaseInitDone) {
                mPhaseInitDone = true;
                app.NoteOff(48);
                app.NoteOff(55);
                app.NoteOff(60);
                app.NoteOff(67);
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
