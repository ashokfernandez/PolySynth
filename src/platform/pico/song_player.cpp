#include "song_player.h"
#include "pico_synth_app.h"

void SongPlayer::Play(const pico_song::Song& song, PicoSynthApp& app) {
    // Stop any current playback
    if (mPlaying) {
        app.Panic();
    }

    mSong = &song;
    mEventIndex = 0;
    mPlaying = true;

    // Clear held notes and set up patch
    app.Panic();
    ApplyPatch(app);

    // Schedule first event
    // delta_ms of first event is delay before first note
    mNextEventUs = 0;  // Will be set on first Update() call
    printf("[SONG] Playing: %s (%.0f BPM, %u events)\n",
           song.name, static_cast<double>(song.bpm), song.eventCount);
}

void SongPlayer::Stop(PicoSynthApp& app) {
    if (mPlaying) {
        mPlaying = false;
        app.Panic();
        printf("[SONG] Stopped\n");
    }
}

bool SongPlayer::Update(uint64_t now_us, PicoSynthApp& app) {
    if (!mPlaying || !mSong) return false;

    // First call after Play(): set the initial time reference
    if (mNextEventUs == 0) {
        mNextEventUs = now_us + static_cast<uint64_t>(mSong->events[0].delta_ms) * 1000ULL;
    }

    // Process all events that are due
    while (now_us >= mNextEventUs && mEventIndex < mSong->eventCount) {
        const auto& ev = mSong->events[mEventIndex];

        switch (ev.type) {
            case pico_song::EventType::NoteOn:
                app.NoteOn(ev.data1, ev.data2);
                break;

            case pico_song::EventType::NoteOff:
                app.NoteOff(ev.data1);
                break;

            case pico_song::EventType::Param: {
                // Map data2 (0-255) to 0.0-1.0, apply to param by ID
                // Reserved for future use — param IDs not yet defined
                break;
            }

            case pico_song::EventType::End:
                // Loop: reset to beginning
                app.Panic();
                mEventIndex = 0;
                mNextEventUs = now_us + static_cast<uint64_t>(mSong->events[0].delta_ms) * 1000ULL;
                printf("[SONG] Looping: %s\n", mSong->name);
                return true;
        }

        mEventIndex++;

        // Schedule next event
        if (mEventIndex < mSong->eventCount) {
            mNextEventUs += static_cast<uint64_t>(mSong->events[mEventIndex].delta_ms) * 1000ULL;
        } else {
            // Ran past end without End sentinel — loop
            app.Panic();
            mEventIndex = 0;
            mNextEventUs = now_us + static_cast<uint64_t>(mSong->events[0].delta_ms) * 1000ULL;
            printf("[SONG] Looping: %s\n", mSong->name);
            return true;
        }
    }

    return mPlaying;
}

const char* SongPlayer::CurrentSongName() const {
    if (mSong) return mSong->name;
    return "none";
}

void SongPlayer::ApplyPatch(PicoSynthApp& app) {
    auto& st = app.GetPendingState();
    st.Reset();

    // OscA: Saw — rich harmonics for bass and lead
    st.oscAWaveform = 0;
    st.mixOscA = 1.0f;
    st.mixOscB = 0.0f;

    // Filter: Ladder, lower cutoff so envelope sweep is audible, keyboard tracking
    st.filterModel = 1;
    st.filterCutoff = 1800.0f;
    st.filterResonance = 0.1f;
    st.filterEnvAmount = 0.45f;
    st.filterKeyboardTrack = true;

    // Amp ADSR: snappy attack, moderate sustain, quick release
    st.ampAttack = 0.005f;
    st.ampDecay = 0.15f;
    st.ampSustain = 0.7f;
    st.ampRelease = 0.2f;

    // Filter ADSR: slight pluck on each note
    st.filterAttack = 0.01f;
    st.filterDecay = 0.2f;
    st.filterSustain = 0.3f;
    st.filterRelease = 0.15f;

    // Master gain
    st.masterGain = 0.8f;

    app.PushStateUpdate();
    printf("[SONG] Patch applied: saw/ladder/funky\n");
}
