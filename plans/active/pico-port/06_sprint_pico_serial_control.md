# Sprint Pico-6: Serial Control Interface

**Depends on:** Sprint Pico-4 (Core DSP), Sprint Pico-5 (Effects Architecture)
**Blocks:** Future sprints (dual-core, physical controls, presets)
**Approach:** TDD — protocol parser tests first, then integration
**Estimated effort:** 2–3 days

> **CI Mandate (from Sprint Pico-2):** This sprint's Definition of Done requires:
> 1. `just test` — Desktop tests pass (command parser tests included)
> 2. `just test-embedded` — Layer 1 tests pass (parser works with `sample_t=float`)
> 3. `just pico-build` — RP2350 ARM cross-compile succeeds with parser sources
> 4. Wokwi automation scenario (`test_boot.yaml`) verifies serial protocol:
>    `NOTE_ON 60 100` → `OK:`, `STATUS` → `STATUS:`, `INVALID` → `ERR:`
> 5. 15 command parser Catch2 tests pass in both desktop and embedded configurations

---

## Goal

Implement a USB Serial text-based command protocol to control all synth parameters from a host computer. Create a Python interactive controller script. The result is a fully controllable instrument — playable from any terminal or script.

---

## Prerequisites

- Sprint Pico-4 complete (Engine producing audio, waveform switching works)
- Sprint Pico-5 complete (effects deploy flags, PicoSoftClipper)
- Familiarity with `SynthState.h` (40+ parameters)
- Python 3 + `pyserial` for the control script

---

## Task 6.1: Design Serial Protocol

### What

Define the text-based serial command protocol. This is a design-only task — document the protocol before implementing.

### Protocol Specification

**Format:** ASCII text, newline-terminated (`\n`), max 128 bytes per command.

**Commands:**

| Command | Format | Example | Description |
|---------|--------|---------|-------------|
| `NOTE_ON` | `NOTE_ON <note> <velocity>` | `NOTE_ON 60 100` | Trigger note (MIDI 0-127, velocity 1-127) |
| `NOTE_OFF` | `NOTE_OFF <note>` | `NOTE_OFF 60` | Release note |
| `SET` | `SET <param> <value>` | `SET filterCutoff 2000.0` | Set parameter value |
| `GET` | `GET <param>` | `GET filterCutoff` | Query parameter value |
| `STATUS` | `STATUS` | `STATUS` | Active voices, CPU%, memory, underruns |
| `PANIC` | `PANIC` | `PANIC` | All notes off immediately |
| `RESET` | `RESET` | `RESET` | Reset engine to default state |

**Responses:**

| Prefix | Meaning | Example |
|--------|---------|---------|
| `OK:` | Command succeeded | `OK: NOTE_ON 60 100` |
| `VAL:` | Value response (GET) | `VAL: filterCutoff=2000.000000` |
| `STATUS:` | Status response | `STATUS: voices=2 cpu=12.3 sram=42 underruns=0` |
| `ERR:` | Error | `ERR: unknown param 'foobar'` |

### Parameter Name Mapping

Parameter names in `SET`/`GET` commands map 1:1 to `SynthState` field names. The full list:

```
# Global
masterGain, polyphony, unison, unisonDetune, glideTime

# Voice allocation
allocationMode, stealPriority, unisonCount, unisonSpread, stereoSpread

# Oscillator A
oscAWaveform, oscAFreq, oscAPulseWidth, oscASync

# Oscillator B
oscBWaveform, oscBFreq, oscBFineTune, oscBPulseWidth, oscBLoFreq, oscBKeyboardPoints

# Mixer
mixOscA, mixOscB, mixNoise

# Filter
filterCutoff, filterResonance, filterEnvAmount, filterModel, filterKeyboardTrack

# Filter Envelope
filterAttack, filterDecay, filterSustain, filterRelease

# Amp Envelope
ampAttack, ampDecay, ampSustain, ampRelease

# LFO
lfoShape, lfoRate, lfoDepth

# Poly Mod
polyModOscBToFreqA, polyModOscBToPWM, polyModOscBToFilter
polyModFilterEnvToFreqA, polyModFilterEnvToPWM, polyModFilterEnvToFilter

# Effects
fxChorusRate, fxChorusDepth, fxChorusMix
fxDelayTime, fxDelayFeedback, fxDelayMix
fxLimiterThreshold
```

### Acceptance Criteria

- [ ] Protocol documented with all commands and response formats
- [ ] All 40+ SynthState parameters listed with exact field names
- [ ] Error handling defined (unknown command, unknown param, malformed input)

---

## Task 6.2: Implement Command Parser

### What

Create a zero-allocation command parser that tokenizes input lines and dispatches to the appropriate handler. Uses a static lookup table mapping parameter names to `SynthState` field offsets.

### Files to Create

**`/src/platform/pico/serial_protocol.h`**

```cpp
#pragma once

// PolySynth Pico — Serial Command Protocol
// Text-based serial interface for controlling the synthesizer.
// See sprint plan 06 (Pico-6) for protocol specification.

#include <cstdint>

namespace pico_serial {

// Maximum command line length (including null terminator)
static constexpr uint32_t kMaxCommandLength = 128;

// Command types
enum class CommandType : uint8_t {
    NoteOn,
    NoteOff,
    Set,
    Get,
    Status,
    Panic,
    Reset,
    Unknown
};

// Parsed command result
struct ParsedCommand {
    CommandType type = CommandType::Unknown;
    int intArg1 = 0;      // note number or param index
    int intArg2 = 0;      // velocity
    float floatArg = 0.0f; // parameter value for SET
    int paramIndex = -1;    // index into SynthState field table
};

} // namespace pico_serial
```

**`/src/platform/pico/command_parser.h`**

```cpp
#pragma once

#include "serial_protocol.h"

#include <cstdint>

// Forward declaration — avoid including full SynthState in parser header
namespace PolySynthCore { struct SynthState; }

namespace pico_serial {

// Parameter descriptor — maps name to offset in SynthState
struct ParamDescriptor {
    const char* name;
    uint32_t offset;    // byte offset into SynthState struct
    bool isFloat;       // true = float/double, false = int/bool
};

// Initialize the command parser. Call once at startup.
void InitParser();

// Feed a single character to the line buffer.
// Returns true when a complete line is ready (newline received).
bool FeedChar(char c);

// Parse the current line buffer into a command.
// Returns a ParsedCommand with type and arguments.
ParsedCommand ParseLine();

// Get a parameter value from SynthState by index.
// Writes the value as a string into outBuf (max outBufLen chars).
void GetParamValue(const PolySynthCore::SynthState& state, int paramIndex,
                   char* outBuf, uint32_t outBufLen);

// Set a parameter value in SynthState by index.
// Returns true on success, false if index is invalid.
bool SetParamValue(PolySynthCore::SynthState& state, int paramIndex, float value);

// Get parameter name by index (for error messages).
const char* GetParamName(int paramIndex);

// Get total number of registered parameters.
int GetParamCount();

} // namespace pico_serial
```

**`/src/platform/pico/command_parser.cpp`**

```cpp
#include "command_parser.h"
#include "SynthState.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace pico_serial {

// ── Line buffer ─────────────────────────────────────────────────────────
static char s_line_buffer[kMaxCommandLength];
static uint32_t s_line_pos = 0;

// ── Parameter table ─────────────────────────────────────────────────────
// Maps parameter names to byte offsets in SynthState.
// Using offsetof() for portable, type-safe offset calculation.
#define PARAM_ENTRY(name, field, isFloat) \
    { name, static_cast<uint32_t>(offsetof(PolySynthCore::SynthState, field)), isFloat }

static const ParamDescriptor s_params[] = {
    // Global
    PARAM_ENTRY("masterGain",      masterGain,      true),
    PARAM_ENTRY("polyphony",       polyphony,       false),
    PARAM_ENTRY("unison",          unison,          false),
    PARAM_ENTRY("unisonDetune",    unisonDetune,    true),
    PARAM_ENTRY("glideTime",       glideTime,       true),

    // Voice allocation
    PARAM_ENTRY("allocationMode",  allocationMode,  false),
    PARAM_ENTRY("stealPriority",   stealPriority,   false),
    PARAM_ENTRY("unisonCount",     unisonCount,     false),
    PARAM_ENTRY("unisonSpread",    unisonSpread,    true),
    PARAM_ENTRY("stereoSpread",    stereoSpread,    true),

    // Oscillator A
    PARAM_ENTRY("oscAWaveform",    oscAWaveform,    false),
    PARAM_ENTRY("oscAFreq",        oscAFreq,        true),
    PARAM_ENTRY("oscAPulseWidth",  oscAPulseWidth,  true),
    PARAM_ENTRY("oscASync",        oscASync,        false),

    // Oscillator B
    PARAM_ENTRY("oscBWaveform",    oscBWaveform,    false),
    PARAM_ENTRY("oscBFreq",        oscBFreq,        true),
    PARAM_ENTRY("oscBFineTune",    oscBFineTune,    true),
    PARAM_ENTRY("oscBPulseWidth",  oscBPulseWidth,  true),
    PARAM_ENTRY("oscBLoFreq",      oscBLoFreq,      false),
    PARAM_ENTRY("oscBKeyboardPoints", oscBKeyboardPoints, false),

    // Mixer
    PARAM_ENTRY("mixOscA",         mixOscA,         true),
    PARAM_ENTRY("mixOscB",         mixOscB,         true),
    PARAM_ENTRY("mixNoise",        mixNoise,        true),

    // Filter
    PARAM_ENTRY("filterCutoff",    filterCutoff,    true),
    PARAM_ENTRY("filterResonance", filterResonance, true),
    PARAM_ENTRY("filterEnvAmount", filterEnvAmount, true),
    PARAM_ENTRY("filterModel",     filterModel,     false),
    PARAM_ENTRY("filterKeyboardTrack", filterKeyboardTrack, false),

    // Filter Envelope
    PARAM_ENTRY("filterAttack",    filterAttack,    true),
    PARAM_ENTRY("filterDecay",     filterDecay,     true),
    PARAM_ENTRY("filterSustain",   filterSustain,   true),
    PARAM_ENTRY("filterRelease",   filterRelease,   true),

    // Amp Envelope
    PARAM_ENTRY("ampAttack",       ampAttack,       true),
    PARAM_ENTRY("ampDecay",        ampDecay,        true),
    PARAM_ENTRY("ampSustain",      ampSustain,      true),
    PARAM_ENTRY("ampRelease",      ampRelease,      true),

    // LFO
    PARAM_ENTRY("lfoShape",        lfoShape,        false),
    PARAM_ENTRY("lfoRate",         lfoRate,         true),
    PARAM_ENTRY("lfoDepth",        lfoDepth,        true),

    // Poly Mod
    PARAM_ENTRY("polyModOscBToFreqA",       polyModOscBToFreqA,       true),
    PARAM_ENTRY("polyModOscBToPWM",         polyModOscBToPWM,         true),
    PARAM_ENTRY("polyModOscBToFilter",      polyModOscBToFilter,      true),
    PARAM_ENTRY("polyModFilterEnvToFreqA",  polyModFilterEnvToFreqA,  true),
    PARAM_ENTRY("polyModFilterEnvToPWM",    polyModFilterEnvToPWM,    true),
    PARAM_ENTRY("polyModFilterEnvToFilter", polyModFilterEnvToFilter, true),

    // Effects
    PARAM_ENTRY("fxChorusRate",        fxChorusRate,        true),
    PARAM_ENTRY("fxChorusDepth",       fxChorusDepth,       true),
    PARAM_ENTRY("fxChorusMix",         fxChorusMix,         true),
    PARAM_ENTRY("fxDelayTime",         fxDelayTime,         true),
    PARAM_ENTRY("fxDelayFeedback",     fxDelayFeedback,     true),
    PARAM_ENTRY("fxDelayMix",          fxDelayMix,          true),
    PARAM_ENTRY("fxLimiterThreshold",  fxLimiterThreshold,  true),
};

#undef PARAM_ENTRY

static constexpr int kParamCount = sizeof(s_params) / sizeof(s_params[0]);

// ── Parser implementation ───────────────────────────────────────────────
void InitParser()
{
    s_line_pos = 0;
    s_line_buffer[0] = '\0';
}

bool FeedChar(char c)
{
    if (c == '\n' || c == '\r') {
        if (s_line_pos > 0) {
            s_line_buffer[s_line_pos] = '\0';
            return true;  // Line ready
        }
        return false;  // Empty line
    }

    if (s_line_pos < kMaxCommandLength - 1) {
        s_line_buffer[s_line_pos++] = c;
    }
    // Silently truncate if line exceeds max length
    return false;
}

// Find parameter index by name. Returns -1 if not found.
static int FindParam(const char* name)
{
    for (int i = 0; i < kParamCount; i++) {
        if (strcmp(s_params[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

ParsedCommand ParseLine()
{
    ParsedCommand cmd;
    cmd.type = CommandType::Unknown;

    // Tokenize: command is first word
    char* saveptr = nullptr;
    char* token = strtok_r(s_line_buffer, " \t", &saveptr);
    if (!token) {
        s_line_pos = 0;
        return cmd;
    }

    if (strcmp(token, "NOTE_ON") == 0) {
        cmd.type = CommandType::NoteOn;
        char* noteStr = strtok_r(nullptr, " \t", &saveptr);
        char* velStr  = strtok_r(nullptr, " \t", &saveptr);
        if (noteStr && velStr) {
            cmd.intArg1 = atoi(noteStr);   // note
            cmd.intArg2 = atoi(velStr);    // velocity
        } else {
            cmd.type = CommandType::Unknown;  // Missing arguments
        }
    }
    else if (strcmp(token, "NOTE_OFF") == 0) {
        cmd.type = CommandType::NoteOff;
        char* noteStr = strtok_r(nullptr, " \t", &saveptr);
        if (noteStr) {
            cmd.intArg1 = atoi(noteStr);
        } else {
            cmd.type = CommandType::Unknown;
        }
    }
    else if (strcmp(token, "SET") == 0) {
        cmd.type = CommandType::Set;
        char* paramStr = strtok_r(nullptr, " \t", &saveptr);
        char* valueStr = strtok_r(nullptr, " \t", &saveptr);
        if (paramStr && valueStr) {
            cmd.paramIndex = FindParam(paramStr);
            cmd.floatArg = strtof(valueStr, nullptr);
        } else {
            cmd.type = CommandType::Unknown;
        }
    }
    else if (strcmp(token, "GET") == 0) {
        cmd.type = CommandType::Get;
        char* paramStr = strtok_r(nullptr, " \t", &saveptr);
        if (paramStr) {
            cmd.paramIndex = FindParam(paramStr);
        } else {
            cmd.type = CommandType::Unknown;
        }
    }
    else if (strcmp(token, "STATUS") == 0) {
        cmd.type = CommandType::Status;
    }
    else if (strcmp(token, "PANIC") == 0) {
        cmd.type = CommandType::Panic;
    }
    else if (strcmp(token, "RESET") == 0) {
        cmd.type = CommandType::Reset;
    }

    s_line_pos = 0;  // Reset line buffer for next command
    return cmd;
}

void GetParamValue(const PolySynthCore::SynthState& state, int paramIndex,
                   char* outBuf, uint32_t outBufLen)
{
    if (paramIndex < 0 || paramIndex >= kParamCount) {
        snprintf(outBuf, outBufLen, "ERR: invalid param index");
        return;
    }

    const auto& desc = s_params[paramIndex];
    const auto* base = reinterpret_cast<const uint8_t*>(&state);

    if (desc.isFloat) {
        double val;
        memcpy(&val, base + desc.offset, sizeof(double));
        snprintf(outBuf, outBufLen, "VAL: %s=%f", desc.name, val);
    } else {
        // Integer/bool fields in SynthState are int or bool (both fit in int)
        int val;
        memcpy(&val, base + desc.offset, sizeof(int));
        snprintf(outBuf, outBufLen, "VAL: %s=%d", desc.name, val);
    }
}

bool SetParamValue(PolySynthCore::SynthState& state, int paramIndex, float value)
{
    if (paramIndex < 0 || paramIndex >= kParamCount) {
        return false;
    }

    const auto& desc = s_params[paramIndex];
    auto* base = reinterpret_cast<uint8_t*>(&state);

    if (desc.isFloat) {
        double dval = static_cast<double>(value);
        memcpy(base + desc.offset, &dval, sizeof(double));
    } else {
        int ival = static_cast<int>(value);
        memcpy(base + desc.offset, &ival, sizeof(int));
    }

    return true;
}

const char* GetParamName(int paramIndex)
{
    if (paramIndex < 0 || paramIndex >= kParamCount) {
        return "(invalid)";
    }
    return s_params[paramIndex].name;
}

int GetParamCount()
{
    return kParamCount;
}

} // namespace pico_serial
```

### Implementation Notes for Juniors

**Why `offsetof()` and `memcpy()`?**
We need a way to generically read/write any field in `SynthState` by name. `offsetof()` gives us the byte offset of each field, and `memcpy()` is the type-safe way to read/write at arbitrary offsets. This avoids a giant switch statement for 40+ parameters.

**Why `strtok_r()` (not `strtok()`)?**
`strtok()` uses global state and is not reentrant. `strtok_r()` is the reentrant version. While we're single-threaded on Pico, it's good practice and avoids issues if we ever add a second core.

**Why no heap allocations?**
The parser uses a fixed-size line buffer and static parameter table. No `std::string`, no `std::map`, no `new`. This is critical for embedded code — deterministic memory usage.

### TDD: Unit Tests

**Create `/tests/unit/Test_CommandParser.cpp`:**

```cpp
#include "../../src/platform/pico/command_parser.h"
#include "../../src/core/SynthState.h"
#include "catch.hpp"
#include <cstring>

// Helper: feed a complete line to the parser
static pico_serial::ParsedCommand parse_command(const char* cmd)
{
    pico_serial::InitParser();
    for (const char* p = cmd; *p; p++) {
        pico_serial::FeedChar(*p);
    }
    pico_serial::FeedChar('\n');
    return pico_serial::ParseLine();
}

TEST_CASE("CommandParser: NOTE_ON parsed correctly", "[CommandParser]") {
    auto cmd = parse_command("NOTE_ON 60 100");
    REQUIRE(cmd.type == pico_serial::CommandType::NoteOn);
    REQUIRE(cmd.intArg1 == 60);
    REQUIRE(cmd.intArg2 == 100);
}

TEST_CASE("CommandParser: NOTE_OFF parsed correctly", "[CommandParser]") {
    auto cmd = parse_command("NOTE_OFF 60");
    REQUIRE(cmd.type == pico_serial::CommandType::NoteOff);
    REQUIRE(cmd.intArg1 == 60);
}

TEST_CASE("CommandParser: SET with float value", "[CommandParser]") {
    auto cmd = parse_command("SET filterCutoff 2000.0");
    REQUIRE(cmd.type == pico_serial::CommandType::Set);
    REQUIRE(cmd.paramIndex >= 0);
    REQUIRE(cmd.floatArg == Approx(2000.0f));
    REQUIRE(std::strcmp(pico_serial::GetParamName(cmd.paramIndex), "filterCutoff") == 0);
}

TEST_CASE("CommandParser: SET with integer value", "[CommandParser]") {
    auto cmd = parse_command("SET oscAWaveform 1");
    REQUIRE(cmd.type == pico_serial::CommandType::Set);
    REQUIRE(cmd.paramIndex >= 0);
    REQUIRE(cmd.floatArg == Approx(1.0f));
}

TEST_CASE("CommandParser: GET valid parameter", "[CommandParser]") {
    auto cmd = parse_command("GET masterGain");
    REQUIRE(cmd.type == pico_serial::CommandType::Get);
    REQUIRE(cmd.paramIndex >= 0);
}

TEST_CASE("CommandParser: SET unknown param returns valid type but invalid index", "[CommandParser]") {
    auto cmd = parse_command("SET nonExistentParam 1.0");
    REQUIRE(cmd.type == pico_serial::CommandType::Set);
    REQUIRE(cmd.paramIndex == -1);  // Unknown parameter
}

TEST_CASE("CommandParser: STATUS command", "[CommandParser]") {
    auto cmd = parse_command("STATUS");
    REQUIRE(cmd.type == pico_serial::CommandType::Status);
}

TEST_CASE("CommandParser: PANIC command", "[CommandParser]") {
    auto cmd = parse_command("PANIC");
    REQUIRE(cmd.type == pico_serial::CommandType::Panic);
}

TEST_CASE("CommandParser: RESET command", "[CommandParser]") {
    auto cmd = parse_command("RESET");
    REQUIRE(cmd.type == pico_serial::CommandType::Reset);
}

TEST_CASE("CommandParser: unknown command", "[CommandParser]") {
    auto cmd = parse_command("FOOBAR 123");
    REQUIRE(cmd.type == pico_serial::CommandType::Unknown);
}

TEST_CASE("CommandParser: empty line", "[CommandParser]") {
    pico_serial::InitParser();
    REQUIRE_FALSE(pico_serial::FeedChar('\n'));  // Empty line, nothing to parse
}

TEST_CASE("CommandParser: SetParamValue and GetParamValue roundtrip", "[CommandParser]") {
    PolySynthCore::SynthState state;
    state.Reset();

    // Find filterCutoff index
    int idx = -1;
    for (int i = 0; i < pico_serial::GetParamCount(); i++) {
        if (std::strcmp(pico_serial::GetParamName(i), "filterCutoff") == 0) {
            idx = i;
            break;
        }
    }
    REQUIRE(idx >= 0);

    // Set value
    REQUIRE(pico_serial::SetParamValue(state, idx, 5000.0f));

    // Read back
    char buf[128];
    pico_serial::GetParamValue(state, idx, buf, sizeof(buf));

    // Should contain "5000"
    REQUIRE(std::strstr(buf, "5000") != nullptr);
}

TEST_CASE("CommandParser: all SynthState params are registered", "[CommandParser]") {
    // Verify that the parameter count matches expectations
    // SynthState has ~51 fields (counted from the struct definition)
    REQUIRE(pico_serial::GetParamCount() >= 40);
}

TEST_CASE("CommandParser: line buffer handles max length", "[CommandParser]") {
    pico_serial::InitParser();

    // Feed exactly kMaxCommandLength - 1 characters (should not crash)
    for (uint32_t i = 0; i < pico_serial::kMaxCommandLength + 10; i++) {
        pico_serial::FeedChar('A');
    }
    pico_serial::FeedChar('\n');

    // Should parse as unknown (not a valid command, but shouldn't crash)
    auto cmd = pico_serial::ParseLine();
    // Just verify no crash — command will be Unknown since "AAAA..." isn't valid
    REQUIRE(cmd.type == pico_serial::CommandType::Unknown);
}

TEST_CASE("CommandParser: NOTE_ON missing arguments", "[CommandParser]") {
    auto cmd = parse_command("NOTE_ON");
    REQUIRE(cmd.type == pico_serial::CommandType::Unknown);
}

TEST_CASE("CommandParser: NOTE_ON partial arguments", "[CommandParser]") {
    auto cmd = parse_command("NOTE_ON 60");
    REQUIRE(cmd.type == pico_serial::CommandType::Unknown);
}
```

**Add to `/tests/CMakeLists.txt`:**
- Add `unit/Test_CommandParser.cpp` to `UNIT_TEST_SOURCES`
- Add `${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/pico/command_parser.cpp` to sources
- Add `${CMAKE_CURRENT_SOURCE_DIR}/../src/platform/pico` to include directories

### Acceptance Criteria

- [ ] All 15 command parser tests pass
- [ ] Zero heap allocations in parser code
- [ ] All 40+ SynthState parameters registered in lookup table
- [ ] Handles malformed input without crash (overflow, missing args)
- [ ] `strtof()` used for float parsing (not `atof()` which returns double)

---

## Task 6.3: Integrate Serial Control in Main Loop

### What

Add the serial command processing loop to `main.cpp`. Commands are parsed in the main loop between audio buffer fills — not in the audio ISR.

### File to Modify

**`/src/platform/pico/main.cpp`** — Replace the automated demo sequence with an interactive control loop:

```cpp
// In main(), after audio starts:

    // ── Initialize serial command parser ────────────────────────────
    pico_serial::InitParser();
    printf("Serial control ready. Type 'STATUS' for help.\n");
    printf("> ");

    // ── Main control loop ───────────────────────────────────────────
    uint32_t status_timer = 0;
    while (true) {
        // Poll for serial input (non-blocking)
        int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            char c = static_cast<char>(ch);

            // Echo character back (terminal feedback)
            putchar(c);

            if (pico_serial::FeedChar(c)) {
                putchar('\n');  // Newline after command

                auto cmd = pico_serial::ParseLine();
                char response[128];

                switch (cmd.type) {
                case pico_serial::CommandType::NoteOn:
                    if (cmd.intArg1 >= 0 && cmd.intArg1 <= 127 &&
                        cmd.intArg2 >= 1 && cmd.intArg2 <= 127) {
                        s_engine.OnNoteOn(cmd.intArg1, cmd.intArg2);
                        printf("OK: NOTE_ON %d %d\n", cmd.intArg1, cmd.intArg2);
                    } else {
                        printf("ERR: note 0-127, velocity 1-127\n");
                    }
                    break;

                case pico_serial::CommandType::NoteOff:
                    if (cmd.intArg1 >= 0 && cmd.intArg1 <= 127) {
                        s_engine.OnNoteOff(cmd.intArg1);
                        printf("OK: NOTE_OFF %d\n", cmd.intArg1);
                    } else {
                        printf("ERR: note 0-127\n");
                    }
                    break;

                case pico_serial::CommandType::Set:
                    if (cmd.paramIndex >= 0) {
                        pico_serial::SetParamValue(s_state, cmd.paramIndex, cmd.floatArg);
                        s_engine.UpdateState(s_state);
                        printf("OK: SET %s %f\n",
                               pico_serial::GetParamName(cmd.paramIndex),
                               static_cast<double>(cmd.floatArg));
                    } else {
                        printf("ERR: unknown parameter\n");
                    }
                    break;

                case pico_serial::CommandType::Get:
                    if (cmd.paramIndex >= 0) {
                        pico_serial::GetParamValue(s_state, cmd.paramIndex,
                                                   response, sizeof(response));
                        printf("%s\n", response);
                    } else {
                        printf("ERR: unknown parameter\n");
                    }
                    break;

                case pico_serial::CommandType::Status: {
                    float process_us = static_cast<float>(s_process_time_us);
                    float buffer_us = static_cast<float>(pico_audio::kBufferFrames) *
                                      1000000.0f /
                                      static_cast<float>(pico_audio::kSampleRate);
                    float cpu_pct = (process_us / buffer_us) * 100.0f;
                    printf("STATUS: voices=%d cpu=%.1f underruns=%lu\n",
                           POLYSYNTH_MAX_VOICES,
                           static_cast<double>(cpu_pct),
                           static_cast<unsigned long>(pico_audio::GetUnderrunCount()));
                    break;
                }

                case pico_serial::CommandType::Panic:
                    for (int n = 0; n < 128; n++) {
                        s_engine.OnNoteOff(n);
                    }
                    printf("OK: PANIC — all notes off\n");
                    break;

                case pico_serial::CommandType::Reset:
                    for (int n = 0; n < 128; n++) {
                        s_engine.OnNoteOff(n);
                    }
                    s_state.Reset();
                    s_engine.UpdateState(s_state);
                    printf("OK: RESET to defaults\n");
                    break;

                case pico_serial::CommandType::Unknown:
                    printf("ERR: unknown command. Try: NOTE_ON, NOTE_OFF, SET, GET, STATUS, PANIC, RESET\n");
                    break;
                }

                printf("> ");
            }
        }

        // Periodic status (every 10 seconds)
        sleep_ms(1);
        status_timer++;
        if (status_timer >= 10000) {
            status_timer = 0;
            // LED heartbeat
            static bool led = false;
            led = !led;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);
        }
    }
```

### Implementation Notes for Juniors

**Why `getchar_timeout_us(0)` instead of `getchar()`?**
`getchar()` blocks until a character arrives — this would freeze the entire Pico, including the LED heartbeat and any control-rate processing. `getchar_timeout_us(0)` returns immediately with `PICO_ERROR_TIMEOUT` if no character is available, allowing the main loop to continue.

**Why is serial parsing in the main loop, not the audio ISR?**
The audio ISR (DMA interrupt handler) must complete within 5.33ms (one buffer period). Serial parsing takes ~10-50μs — fast, but we don't want it competing with audio. The main loop has unlimited time between audio callbacks.

**Thread safety:**
This is single-threaded bare-metal code. The audio callback runs in an interrupt, but `SynthState` is only written from the main loop and read from the audio callback. Since `SynthState` is a plain struct and updates are at control rate (~188Hz), there's no race condition. The worst case is a parameter update being partially applied during one buffer — this produces a one-buffer glitch, which is inaudible (5.33ms).

### Acceptance Criteria

- [ ] `NOTE_ON 60 100` via serial produces Middle C
- [ ] `NOTE_OFF 60` stops note with release envelope
- [ ] `SET filterCutoff 500.0` audibly changes filter in real-time
- [ ] `SET oscAWaveform 1` changes to square wave
- [ ] `STATUS` returns accurate CPU% and underrun count
- [ ] `PANIC` silences all audio immediately
- [ ] `RESET` returns to default state
- [ ] `GET masterGain` returns current value
- [ ] Invalid commands return `ERR:` without crash

---

## Task 6.4: Create Python Interactive Controller

### What

Create a Python script that provides an interactive REPL and convenience functions for controlling the Pico synth over serial.

### File to Create

**`/tools/pico_control.py`**

```python
#!/usr/bin/env python3
"""
PolySynth Pico — Interactive Serial Controller

Usage:
    python3 tools/pico_control.py [--port /dev/ttyACM0] [--baud 115200]

Interactive commands:
    note_on(60, 100)        # Play Middle C at velocity 100
    note_off(60)            # Release Middle C
    play_chord([60,64,67])  # Play C major chord
    stop_chord([60,64,67])  # Release C major chord
    set_param("filterCutoff", 2000)
    get_param("filterCutoff")
    sweep_filter(200, 10000, 5.0)  # Sweep filter cutoff over 5 seconds
    status()                # Show synth status
    panic()                 # All notes off
    reset()                 # Reset to defaults
"""

import serial
import time
import sys
import argparse
import threading


class PicoSynth:
    """Serial controller for PolySynth Pico."""

    def __init__(self, port="/dev/ttyACM0", baud=115200, timeout=1.0):
        self.ser = serial.Serial(port, baud, timeout=timeout)
        time.sleep(0.5)  # Wait for USB enumeration
        self._flush()
        print(f"Connected to {port} at {baud} baud")

    def _flush(self):
        """Flush any pending data."""
        self.ser.reset_input_buffer()

    def _send(self, command: str) -> str:
        """Send a command and return the response."""
        self.ser.write(f"{command}\n".encode())
        self.ser.flush()
        time.sleep(0.05)  # Give Pico time to respond

        response = ""
        while self.ser.in_waiting:
            response += self.ser.read(self.ser.in_waiting).decode(errors="replace")
            time.sleep(0.01)

        return response.strip()

    def note_on(self, note: int, velocity: int = 100) -> str:
        """Play a note. note: 0-127, velocity: 1-127."""
        return self._send(f"NOTE_ON {note} {velocity}")

    def note_off(self, note: int) -> str:
        """Release a note."""
        return self._send(f"NOTE_OFF {note}")

    def play_chord(self, notes: list, velocity: int = 100):
        """Play a chord (list of MIDI notes)."""
        for note in notes:
            self.note_on(note, velocity)
            time.sleep(0.01)  # Small delay between notes
        print(f"Playing chord: {notes}")

    def stop_chord(self, notes: list):
        """Release a chord."""
        for note in notes:
            self.note_off(note)
            time.sleep(0.01)
        print(f"Released chord: {notes}")

    def set_param(self, name: str, value: float) -> str:
        """Set a synth parameter."""
        return self._send(f"SET {name} {value}")

    def get_param(self, name: str) -> str:
        """Get a synth parameter value."""
        return self._send(f"GET {name}")

    def status(self) -> str:
        """Get synth status."""
        resp = self._send("STATUS")
        print(resp)
        return resp

    def panic(self) -> str:
        """All notes off."""
        return self._send("PANIC")

    def reset(self) -> str:
        """Reset to defaults."""
        return self._send("RESET")

    def sweep_filter(self, start_hz: float, end_hz: float, duration_s: float,
                     steps: int = 100):
        """Sweep filter cutoff over time."""
        print(f"Sweeping filter {start_hz} -> {end_hz} Hz over {duration_s}s")
        for i in range(steps + 1):
            t = i / steps
            # Logarithmic interpolation for perceptually even sweep
            freq = start_hz * ((end_hz / start_hz) ** t)
            self.set_param("filterCutoff", freq)
            time.sleep(duration_s / steps)
        print("Sweep complete")

    def demo(self):
        """Run a quick demo sequence."""
        print("=== PolySynth Pico Demo ===")

        print("\n1. Playing C major chord...")
        self.play_chord([60, 64, 67, 72])
        time.sleep(1.0)

        print("\n2. Sweeping filter cutoff...")
        self.sweep_filter(200, 8000, 3.0)

        print("\n3. Changing to square wave...")
        self.set_param("oscAWaveform", 1)
        time.sleep(1.0)

        print("\n4. Sweeping filter back down...")
        self.sweep_filter(8000, 200, 3.0)

        print("\n5. Releasing chord...")
        self.stop_chord([60, 64, 67, 72])
        time.sleep(0.5)

        print("\n6. Resetting...")
        self.reset()

        print("\n=== Demo complete ===")

    def close(self):
        """Close serial connection."""
        self.ser.close()


def main():
    parser = argparse.ArgumentParser(description="PolySynth Pico Controller")
    parser.add_argument("--port", default="/dev/ttyACM0", help="Serial port")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--demo", action="store_true", help="Run demo sequence")
    args = parser.parse_args()

    synth = PicoSynth(port=args.port, baud=args.baud)

    if args.demo:
        synth.demo()
        synth.close()
        return

    # Interactive REPL
    print("\nPolySynth Pico Interactive Controller")
    print("Commands: note_on(60,100), note_off(60), play_chord([60,64,67]),")
    print("          set_param('filterCutoff', 2000), status(), panic(), reset()")
    print("          sweep_filter(200, 10000, 5.0), demo()")
    print("Type 'quit' to exit.\n")

    # Make synth available in the interactive namespace
    import code
    namespace = {
        "synth": synth,
        "note_on": synth.note_on,
        "note_off": synth.note_off,
        "play_chord": synth.play_chord,
        "stop_chord": synth.stop_chord,
        "set_param": synth.set_param,
        "get_param": synth.get_param,
        "status": synth.status,
        "panic": synth.panic,
        "reset": synth.reset,
        "sweep_filter": synth.sweep_filter,
        "demo": synth.demo,
    }
    code.interact(local=namespace, banner="", exitmsg="Goodbye!")
    synth.close()


if __name__ == "__main__":
    main()
```

### Acceptance Criteria

- [ ] `python3 tools/pico_control.py` connects to Pico
- [ ] `note_on(60, 100)` produces Middle C
- [ ] `play_chord([60, 64, 67])` plays C major chord
- [ ] `sweep_filter(200, 10000, 5.0)` sweeps filter audibly
- [ ] `demo()` runs full demo sequence without errors
- [ ] `panic()` silences audio
- [ ] `quit` exits cleanly

---

## Task 6.5: Update CMakeLists.txt

### What

Add the command parser source file to the Pico build.

### File to Modify

**`/src/platform/pico/CMakeLists.txt`** — Add to executable sources:

```cmake
add_executable(polysynth_pico
    main.cpp
    audio_i2s_driver.cpp
    command_parser.cpp
)
```

### Acceptance Criteria

- [ ] `just pico-build` succeeds with command parser included
- [ ] No new warnings

---

## Definition of Done

- [ ] `NOTE_ON 60 100` via serial produces Middle C
- [ ] `NOTE_OFF 60` stops note with release envelope
- [ ] `SET filterCutoff 500.0` audibly changes filter in real-time
- [ ] `SET oscAWaveform 1` changes to square wave
- [ ] All 40+ SynthState parameters controllable via `SET`
- [ ] `STATUS` returns accurate voice count and CPU%
- [ ] `PANIC` silences all audio immediately
- [ ] `RESET` returns to default state
- [ ] Python script plays a 4-note chord and sweeps filter
- [ ] No stuck notes, crashes, or underruns during 5 min interactive use
- [ ] 15 command parser unit tests pass
- [ ] `just build && just test` (desktop) — zero regressions
- [ ] `just pico-build` produces valid `.uf2`

---

## Files Summary

| Action | File | Purpose |
|--------|------|---------|
| Create | `/src/platform/pico/serial_protocol.h` | Protocol types and constants |
| Create | `/src/platform/pico/command_parser.h` | Parser API |
| Create | `/src/platform/pico/command_parser.cpp` | Parser implementation |
| Create | `/tests/unit/Test_CommandParser.cpp` | 15 parser unit tests |
| Create | `/tools/pico_control.py` | Python interactive controller |
| Modify | `/src/platform/pico/main.cpp` | Serial control loop |
| Modify | `/src/platform/pico/CMakeLists.txt` | Add parser source |
| Modify | `/tests/CMakeLists.txt` | Add parser test + include path |

---

## Testing Guide

### Interactive Testing (On Hardware)

```bash
# Connect to Pico serial
minicom -D /dev/ttyACM0 -b 115200

# Basic test sequence:
NOTE_ON 60 100      # Should hear Middle C
STATUS              # Should show voices=4, cpu < 30%
SET filterCutoff 500 # Should hear filter close
SET filterCutoff 8000 # Should hear filter open
SET oscAWaveform 1  # Should hear square wave
NOTE_OFF 60         # Should hear release tail
PANIC               # Silence

# Polyphony test:
NOTE_ON 60 100
NOTE_ON 64 90
NOTE_ON 67 80
NOTE_ON 72 85       # 4-voice chord
NOTE_ON 76 100      # 5th note — should trigger voice stealing
PANIC

# Error handling:
FOOBAR              # Should return ERR:
NOTE_ON             # Should return ERR: (missing args)
SET nonexistent 1.0 # Should return ERR: unknown parameter
```

### Python Testing

```bash
# Demo mode (automated sequence)
python3 tools/pico_control.py --demo

# Interactive mode
python3 tools/pico_control.py
>>> note_on(60, 100)
>>> play_chord([60, 64, 67, 72])
>>> sweep_filter(200, 10000, 5.0)
>>> panic()
>>> quit
```

### Stress Testing

```bash
# Rapid parameter changes (100 per second for 5 seconds)
python3 -c "
import serial, time
s = serial.Serial('/dev/ttyACM0', 115200)
s.write(b'NOTE_ON 60 100\n')
for i in range(500):
    freq = 200 + (i % 100) * 80
    s.write(f'SET filterCutoff {freq}\n'.encode())
    time.sleep(0.01)
s.write(b'STATUS\n')
time.sleep(0.5)
print(s.read_all().decode())
s.close()
"
# Verify: STATUS shows 0 underruns, no crash
```

---

## Troubleshooting Guide

### Serial shows nothing after boot
→ Wait 3 seconds for USB enumeration. Check `ls /dev/ttyACM*`.

### "ERR: unknown parameter"
→ Check spelling — parameter names are case-sensitive and must match `SynthState` field names exactly. Run `GET masterGain` to verify connection.

### Note stuck after crash
→ Disconnect and reconnect USB. The Pico will reboot. Or add a hardware reset button.

### Python script can't connect
→ Ensure no other program (minicom, screen) has the serial port open. Only one program can use the port at a time.

### Parameter change has no audible effect
→ Some parameters only affect new notes (e.g., `oscAWaveform`). Try `PANIC` then `NOTE_ON 60 100` after changing.

---

## Review Checklist for Reviewers

- [ ] No heap allocations in command_parser.cpp (no `std::string`, no `new`)
- [ ] `offsetof()` usage is valid for aggregate `SynthState` struct
- [ ] All `SynthState` fields are registered in the parameter table
- [ ] Line buffer overflow is handled (truncation, not crash)
- [ ] `strtok_r()` used (reentrant), not `strtok()`
- [ ] `getchar_timeout_us(0)` used (non-blocking), not `getchar()`
- [ ] MIDI note range validated (0-127) before passing to engine
- [ ] Python script handles serial connection errors gracefully
- [ ] Desktop tests include command parser tests and pass
- [ ] No Pico SDK dependency in command_parser.h/.cpp (testable on desktop)
