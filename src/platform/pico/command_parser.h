#pragma once
// Serial command parser for PolySynth Pico.
// Zero heap allocation: 128-byte static line buffer, character-at-a-time feed.

#include <cstdint>
#include <cstring>

namespace pico_serial {

struct Command {
    enum Type : uint8_t {
        NOTE_ON, NOTE_OFF, SET, GET, STATUS, PANIC, RESET, DEMO, STOP, UNKNOWN
    };
    Type type = UNKNOWN;
    int intArg1 = 0;      // note number
    int intArg2 = 0;      // velocity
    char strArg[32] = {};  // parameter name
    double floatArg = 0.0; // parameter value
};

class CommandParser {
public:
    // Feed a character. Returns true when a complete line is ready (newline received).
    bool Feed(char c);

    // Parse the buffered line into a Command. Call after Feed() returns true.
    // Resets the internal buffer so the parser is ready for the next line.
    Command Parse();

    // Get the raw line buffer (for diagnostics).
    const char* GetLine() const { return mBuf; }

private:
    static constexpr int kMaxLineLen = 128;
    char mBuf[kMaxLineLen] = {};
    int mPos = 0;
};

} // namespace pico_serial
