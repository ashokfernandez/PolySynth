# Agent Instructions (PolySynth)

This file defines the strict command workflow, architectural constraints, and operational guardrails for human and AI agents working in this repository. You are an expert C++ DSP developer; your execution must be rigorous, real-time safe, and highly portable.

## 🗺️ 1. Agent Navigation & Command Surface

You MUST use `just` as the default interface for local development and verification. Prefer `just` recipes over ad-hoc command chains.

Agent navigation paths:

- Agent handbook: [`docs/agents/README.md`](docs/agents/README.md)
- Full docs hub: [`docs/README.md`](docs/README.md)
- Planning hub: [`plans/README.md`](plans/README.md)
- Machine-focused agent files: [`.agent/README.md`](.agent/README.md)

Core commands:

- `just` - list available tasks
- `just deps` - fetch/update dependencies
- `just build` - build C++ test targets
- `just test` - run unit tests
- `just lint` - static analysis
- `just asan` / `just tsan` - Address/Thread Sanitizer runs
- `just check` - lint + test
- `just ci-pr` - lint + asan + tsan + test

Desktop and gallery targets:

- `just desktop-build` / `just desktop-run` / `just desktop-rebuild` / `just install-local`
- `just gallery-build` / `just gallery-view` / `just gallery-test` / `just gallery-pages-build` / `just gallery-pages-view`

## 🛑 2. Absolute Constraints (The "Never" List)

- `NO HEAP ALLOCATION IN AUDIO PATH`: You MUST NOT allocate memory on the heap in the hot path (e.g., inside `Process()` methods). Rely exclusively on stack-allocated arrays or `std::array` to ensure real-time safety.
- `NO LLM ARTIFACTS IN CODE`: You MUST NOT commit reasoning traces, stream-of-consciousness remarks, or "thinking out loud" comments (e.g., "Let's check if kPi is defined...") into production source files. Keep reasoning in PR description or scratchpad.
- `FIX, DO NOT SUPPRESS`: You MUST NOT use cppcheck suppressions to hide static analysis warnings on newly written code. You must fix the underlying issue.
- `DO NOT DEVIATE FROM SPEC`: The sprint plan and pseudocode are sources of truth. If behavior is strictly specified (e.g., immediate retrigger on steal, specific sample-rate reset values), you MUST implement exactly as written and MUST NOT flag it as a bug.

## 🏗️ 3. Architecture & DSP Rules (When/Then Triggers)

- `WHEN adding new building blocks`: You MUST enforce the split principle. If a class has a `Process(sample)` method, it belongs in SEA_DSP. If it supports audio applications but does not process audio samples (e.g., allocators, thread-safe queues, music theory), it belongs in SEA_Util.
- `WHEN defining numeric types`: You MUST enforce strict type discipline for embedded portability. Use the `sample_t` alias for signals instead of hardcoded `double`. Use `float` for control parameters (e.g., spread, detune). Use `uint32_t` for timestamps to avoid expensive 64-bit atomic operations on ARM platforms.
- `WHEN writing math for music theory`: You MUST normalize modulo arithmetic for negative inputs (e.g., `if (val < 0) val += 12;`) because standard C++ `%` is a remainder operator and can cause out-of-bounds bit shifts for negative numbers.
- `WHEN adding header includes`: You MUST include exactly what you use and never rely on transitive includes, since WAM web builds compile in different orders.

## 🖥️ 4. iPlug2 & UI Thread Hygiene (When/Then Triggers)

- `WHEN writing WAM/processor code (HEADLESS BUILD PROTECTION)`: You MUST wrap any code touching the UI (e.g., calls to `GetUI()`) within `#if IPLUG_EDITOR` / `#endif` blocks. Headless builds (e.g., WAM processor) define `IPLUG_DSP=1` and `NO_IGRAPHICS`, so `GetUI()` does not exist and will fail compilation if unguarded.
- `WHEN displaying DSP states (UI/AUDIO THREAD ISOLATION)`: You MUST NOT iterate non-atomic DSP structures (like `VoiceManager::mVoices`) directly from the UI thread, including in `OnIdle()`. Use atomic mirrors or lock-free SPSC ring buffers for visual data.
- `WHEN building UI components`: You MUST NOT perform direct drawing calls (e.g., `g->FillRect`) within `OnLayout()`. Custom graphics must be encapsulated exclusively within `IControl::Draw()` methods.
- `WHEN styling typography`: You MUST explicitly set fonts and colors using a centralized theme/style object (e.g., `PolyTheme`). Hardcoded font strings can cause launch crashes.

## 📜 5. Script Organization & Logging

- Root-level helper scripts have been retired.
- Non-trivial script implementations now live in `scripts/tasks/`.
- Shared command logging helpers live in `scripts/lib/cli.sh`.
- Command execution wrapper lives in `scripts/cli.sh`.
- You MUST NOT reintroduce old root scripts unless explicitly requested.
- If lower-level behavior is needed, call scripts in `scripts/tasks/` directly.

`just` commands run through the shared CLI wrapper:

- Full stdout/stderr is always written to `.artifacts/logs/<run-id>/`.
- Terminal output is compact (`PASS/FAIL` + duration).
- On failure, an issue excerpt is printed, with path to the full log.

When reporting failures in conversation, you MUST include the log path from `.artifacts/logs/...`.

To view logs:

- `just logs-latest`
- `POLYSYNTH_VERBOSE=1 just <task>`

## 📋 6. Agent Pre-Flight Checklist

Before concluding a task, running `just ci-pr`, or submitting a PR, you MUST output a checklist verifying:

- [ ] `Test-Driven Development`: Have you written Catch2 unit tests defining expected behavior before writing implementation?
- [ ] `Build Systems Synced`: Have you updated all parallel build targets? Adding a library dependency requires updating native CMake, desktop Xcode, and WAM makefiles.
- [ ] `Variable Hygiene`: Have you verified all stored variables are actually read? Have you discarded unused return values as statements? Are intentionally unused parameters explicitly suppressed with `(void)param;`?
- [ ] `Compile-Time Checks`: If the spec mentions trait requirements checked at compile time, have you implemented the `static_assert` calls?
- [ ] `Golden Master Policy`: If your fix changes runtime behavior, have you checked the decision tree for Golden Masters?
