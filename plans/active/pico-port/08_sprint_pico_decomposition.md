# Sprint Pico-8: Architecture Decomposition

**Depends on:** Sprint Pico-7 (Performance Foundations)
**Blocks:** Sprint Pico-9 (FreeRTOS Migration)
**Approach:** Extract modules from main.cpp into clean, testable components. No new features.
**Estimated effort:** 2-3 days

> **Why this sprint exists:** The current main.cpp is a 670-line monolith that mixes
> audio callbacks, command dispatch, demo sequencing, state handoff, self-tests, and
> the main loop. This makes it impossible to add FreeRTOS tasks, test subsystems
> independently, or maintain the codebase as features grow.

---

## Goal

Break `src/platform/pico/main.cpp` into well-defined modules with clear ownership
boundaries. After this sprint, each module can be tested and modified independently,
and the code is ready for FreeRTOS task conversion in Sprint Pico-9.

---

## Task 8.1: Create PicoSynthApp Class

### Problem

There is no equivalent of the desktop's iPlug2 `PolySynth` class on Pico. The Engine,
SynthState, and command queue are all loose statics in main.cpp with no encapsulation.

### Fix

Create `src/platform/pico/pico_synth_app.h/.cpp`:

```cpp
class PicoSynthApp {
public:
    void Init(sample_t sampleRate);  // sample_t = float on embedded, double on desktop

    // Command interface (called from Core 0)
    void NoteOn(uint8_t note, uint8_t velocity);
    void NoteOff(uint8_t note);
    void SetParam(const char* name, float value);
    float GetParam(const char* name);
    void Panic();
    void Reset();

    // Audio interface (called from Core 1 DMA ISR)
    void AudioCallback(uint32_t* buffer, uint32_t numFrames);

    // Diagnostics
    int GetActiveVoiceCount() const;
    float GetCpuPercent() const;
    float GetPeakLevel();

private:
    PolySynthCore::Engine mEngine;
    PolySynthCore::SynthState mPendingState;
    PolySynthCore::SynthState mStagedState;
    // ... SPSC queue, atomics, state handoff
};
```

This mirrors what iPlug2's `PolySynth` class does for desktop. (iPlug2 is the audio
plugin framework used for the desktop build — it provides a clean application class
that owns the Engine, manages state transfer, and bridges I/O.) All the audio callback
logic (gain staging, soft clip, SSAT, PackI2S) moves here.

### Validation

- `just pico-build` succeeds
- `main()` is reduced to init + main loop

---

## Task 8.2: Extract Command Dispatcher

### Problem

The serial command parsing (`command_parser.h`) and the parameter dispatch (20+ strcmp
chains in main.cpp) are interleaved. The strcmp dispatch will diverge from `SynthState`
as parameters are added.

### Fix

Create `src/platform/pico/pico_command_dispatch.h/.cpp`:

```cpp
namespace pico_commands {
    // Process a parsed command against the app
    void Dispatch(const ParsedCommand& cmd, PicoSynthApp& app);

    // Format a response string (STATUS, GET, etc.)
    void FormatResponse(char* buf, size_t bufLen, const ParsedCommand& cmd,
                        const PicoSynthApp& app);
}
```

The command parser (`command_parser.h`) stays as-is — it's already clean and tested.
Only the dispatch logic (the big switch/strcmp block) moves out of main.cpp.

### Validation

- All existing serial commands still work
- Existing `command_parser` Catch2 tests still pass
- `just test-embedded` passes

---

## Task 8.3: Extract Demo Sequence

### Problem

The demo sequence state machine (~120 lines) lives in main.cpp with timing logic
coupled to the main loop's polling rate.

### Fix

Create `src/platform/pico/pico_demo.h/.cpp`:

```cpp
class PicoDemo {
public:
    void Start(PicoSynthApp& app);
    void Stop(PicoSynthApp& app);
    bool IsRunning() const;

    // Call periodically from main loop (or later, from a FreeRTOS task)
    void Update(uint32_t nowMs, PicoSynthApp& app);

private:
    // Demo step table, current step, timing
};
```

Uses absolute timestamps (`to_ms_since_boot(get_absolute_time())`) so it works
whether called from a busy loop or a FreeRTOS task.

Also add an explicit `default` case to `demo_phase_name()` to silence compiler warnings
about missing default in non-void functions (flagged in PR #65 review — the trailing
`return "?"` handles it but some compilers still warn).

### Validation

- `DEMO` serial command still works
- `STOP` still stops the demo
- Demo timing unchanged
- No compiler warnings from `demo_phase_name`

---

## Task 8.4: Extract Self-Test Module

### Problem

The boot self-test (~60 lines) runs inline in main.cpp and mixes hardware checks
(I2S, LED) with DSP validation (oscillator output test).

### Fix

Create `src/platform/pico/pico_self_test.h/.cpp`:

```cpp
namespace pico_self_test {
    struct TestResult {
        bool passed;
        const char* message;
    };

    TestResult RunAll(PicoSynthApp& app);
}
```

### Validation

- Boot self-test still runs and reports via serial
- `[TEST:ALL_PASSED]` marker still emitted (Wokwi CI depends on this)

---

## Task 8.5: Clean Up main.cpp

### Problem

After extraction, main.cpp should be a thin shell.

### Fix

Final main.cpp structure:

```cpp
#include "pico_synth_app.h"
#include "pico_command_dispatch.h"
#include "pico_demo.h"
#include "pico_self_test.h"

static PicoSynthApp s_app;

static void core1_audio_entry() {
    // FPSCR setup (from Sprint 7)
    s_app.StartAudio();
    while (true) __wfi();
}

int main() {
    // Hardware init
    // Self-test
    // Launch Core 1
    // Main loop: serial poll → dispatch → demo update
}
```

Target: under 120 lines total. Ready for FreeRTOS conversion in Sprint 9.

### Validation

- `just pico-build` succeeds
- `just test-embedded` passes
- All serial commands work
- Audio quality unchanged from Sprint 7

---

## Task 8.6: Replace Custom SPSC Queue with SPSCQueue.h

### Problem

main.cpp contains a hand-rolled SPSC queue (arrays + atomic head/tail) that duplicates
the tested `SPSCQueue.h` already in `src/core/`.

### Fix

Replace the custom `s_cmdQueue`/`s_cmdHead`/`s_cmdTail` with `SPSCQueue<AudioCommand>`.
This eliminates duplication and uses the same battle-tested implementation the desktop
build uses.

### Validation

- Command queue still works under load
- No dropped commands during rapid NOTE_ON/OFF sequences
- `just test-embedded` passes

---

## Definition of Done

- [ ] `PicoSynthApp` class owns Engine, state, and audio callback
- [ ] Command dispatch extracted from main.cpp
- [ ] Demo sequence extracted as `PicoDemo`
- [ ] Self-test extracted as module
- [ ] Custom SPSC queue replaced with `SPSCQueue.h`
- [ ] main.cpp is <120 lines
- [ ] `just test` passes
- [ ] `just test-embedded` passes
- [ ] `just pico-build` succeeds
- [ ] Audio quality unchanged from Sprint 7
- [ ] All serial commands functional
- [ ] `[TEST:ALL_PASSED]` still emitted for Wokwi CI
