#pragma once

#include "song_event.h"
#include <cstdint>
#include <cstdio>

class PicoSynthApp;

class SongPlayer {
public:
    void Play(const pico_song::Song& song, PicoSynthApp& app);
    void Stop(PicoSynthApp& app);
    bool Update(uint64_t now_us, PicoSynthApp& app);  // returns false when done
    bool IsPlaying() const { return mPlaying; }
    const char* CurrentSongName() const;

private:
    const pico_song::Song* mSong = nullptr;
    uint16_t mEventIndex = 0;
    uint64_t mNextEventUs = 0;
    bool mPlaying = false;

    void ApplyPatch(PicoSynthApp& app);
};
