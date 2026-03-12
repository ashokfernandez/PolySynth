#pragma once

#include "dont_stop.h"
#include "../song_event.h"

namespace pico_song {

inline constexpr const Song* kSongs[] = { &kSong_dont_stop };
inline constexpr int kSongCount = sizeof(kSongs) / sizeof(kSongs[0]);

} // namespace pico_song
