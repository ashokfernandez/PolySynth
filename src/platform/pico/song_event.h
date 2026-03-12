#pragma once
#include <cstdint>

namespace pico_song {

enum class EventType : uint8_t {
    NoteOn  = 0,   // data1=note, data2=velocity
    NoteOff = 1,   // data1=note, data2=unused
    Param   = 2,   // data1=param_id, data2=value (0-255 → 0.0-1.0)
    End     = 3,   // sentinel — stop playback
};

// Compact event: 5 bytes. A 60s song at 120 BPM ≈ 2000-3000 events ≈ 10-15KB.
struct Event {
    uint16_t delta_ms;   // ms since previous event (max 65535 = ~65s gap)
    EventType type;
    uint8_t data1;
    uint8_t data2;
};

// Song metadata + pointer to event data
struct Song {
    const char* name;
    float bpm;
    const Event* events;
    uint16_t eventCount;
};

} // namespace pico_song
