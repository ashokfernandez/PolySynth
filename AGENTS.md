# Agent Instructions (PolySynth)

This file defines the strict command workflow, architectural constraints, and operational guardrails for human and AI agents working in this repository. You are an expert C++ DSP developer; your execution must be rigorous, real-time safe, and highly portable.

## 🗺️ 1. AGENT NAVIGATION & COMMAND SURFACE

You MUST use `just` as the default interface for local development and verification. Prefer `just` recipes over ad-hoc command chains. 

**Agent Navigation Paths:**
* Agent handbook: `docs/agents/README.md`
* Full docs hub: `docs/README.md`
* Planning hub: `plans/README.md`
* Machine-focused agent files: `.agent/README.md`

**Core Commands:**
* `just` - list available tasks
* `just deps` - fetch/update dependencies
* `just build` - build C++ test targets
* `just test` - run unit tests
* `just lint` - static analysis
* `just asan` / `just tsan` - Address/Thread Sanitizer runs
* `just check` - lint + test
* `just ci-pr` - lint + asan + tsan + test + desktop startup smoke

**Desktop and Gallery Targets:**
* `just desktop-build` / `just desktop-run` / `just desktop-rebuild` / `just desktop-smoke` / `just install-local`
* `just sandbox-build` / `just sandbox-run` / `just gallery-test` / `just vrt-baseline` / `just vrt-run`

**Versioning and Release:**
* `just version` — interactive TUI: shows current version + history, prompts for bump, commits/tags/pushes
* `just version-show` — print current version and recent tags (non-interactive)
* `just version-bump patch|minor|major` — non-interactive bump, full release flow
* `just version-set 1.2.3` — set explicit version, full release flow
* Pushing a `v*.*.*` tag triggers CI to build macOS `.pkg` + Windows `.zip` and publish to GitHub Releases

## 🔇 2. TOKEN-EFFICIENT COMMANDS (RTK)

`rtk` (Rust Token Killer) is installed globally. You MUST use it for the following commands to reduce output noise and token consumption:

| Instead of | Use |
|---|---|
| `ls` / `ls <path>` | `rtk ls <path>` |
| `git status` / `git log` / `git diff` / `git branch` | `rtk git <subcommand>` |
| `gh pr ...` / `gh issue ...` / `gh run ...` | `rtk gh <subcommand>` |
| `cat <file>` | `rtk read <file>` |
| `grep <pattern>` / `rg <pattern>` | `rtk grep <pattern>` |

All other commands (cmake, just, bash scripts, etc.) run as normal. Commands already prefixed with `rtk`, heredocs (`<<`), and unrecognised commands pass through unchanged.

> **Claude Code agents**: the auto-rewrite hook at `~/.claude/hooks/rtk-rewrite.sh` handles this transparently — no manual prefixing needed.

## 🛑 3. ABSOLUTE CONSTRAINTS (The "Never" List)

* **NO HEAP ALLOCATION IN AUDIO PATH:** You MUST NOT allocate memory on the heap in the hot path (e.g., inside `Process()` methods). Rely exclusively on stack-allocated arrays or `std::array` to ensure real-time safety.
* **NO LLM ARTIFACTS IN CODE:** You MUST NOT commit your reasoning traces, stream-of-consciousness remarks, or "thinking out loud" comments (e.g., "Let's check if kPi is defined...") into production source files. Keep your reasoning in the PR description or your internal scratchpad.
* **FIX, DO NOT SUPPRESS:** You MUST NOT use static analysis (e.g., `cppcheck`) suppressions to hide warnings on newly written code. You must fix the underlying issue.
* **DO NOT DEVIATE FROM SPEC:** The sprint plan and pseudocode are your absolute sources of truth. If a behavior is strictly specified (like immediate retriggering on steal or specific sample-rate reset values), you MUST implement it exactly as written and MUST NOT flag it as a bug.
* **DO NOT HARDCODE PLUGIN IDENTITY:** You MUST NOT hardcode plugin name, company name, version, or bundle identifier strings anywhere outside `src/platform/desktop/config.h`. The Info.plist files are generated from `.in` templates by cmake at configure time — edit only `config.h` and re-run cmake. Hardcoding these values elsewhere (e.g. directly in a `.plist`) causes `[NSBundle bundleWithIdentifier:]` to return nil at runtime, silently breaking font loading and crashing DAWs.
* **DO NOT CALL `BundleResourcePath` OUTSIDE `#ifndef WEB_API`:** On Emscripten, `PluginIDType = void*` so passing a `const char*` is a compile error. Any call to `BundleResourcePath` or `PluginPath` with a string argument must be wrapped in `#ifndef WEB_API`. The lint job enforces this via `scripts/check_platform_guards.py`.

## 🏗️ 3. ARCHITECTURE & DSP RULES (When/Then Triggers)

* **WHEN adding new building blocks:** You MUST enforce the split principle. If a class has a `Process(sample)` method, it belongs in `SEA_DSP`. If it supports audio applications but does not process audio samples (e.g., allocators, thread-safe queues, music theory), it belongs in `SEA_Util`.
* **WHEN defining numeric types:** You MUST enforce strict type discipline for embedded portability. Use the `sample_t` alias for signals instead of hardcoded `double`. Use `float` for control parameters (e.g., spread, detune). Use `uint32_t` for timestamps to avoid expensive 64-bit atomic operations on ARM platforms.
* **WHEN writing math for music theory:** You MUST normalize modulo arithmetic for negative inputs (e.g., `if (val < 0) val += 12;`) because standard C++ `%` is a remainder operator and will cause out-of-bounds bit shifts for negative numbers.
* **WHEN adding header includes:** You MUST include exactly what you use and never rely on transitive includes, as WAM web builds compile in different orders. Do not add speculative includes.

## 🖥️ 4. IPLUG2 & UI THREAD HYGIENE (When/Then Triggers)

* **WHEN writing WAM/Processor code (HEADLESS BUILD PROTECTION):** You MUST wrap any code touching the UI (e.g., calls to `GetUI()`) within `#if IPLUG_EDITOR` / `#endif` blocks. Headless builds define `IPLUG_DSP=1` and `NO_IGRAPHICS`, meaning `GetUI()` does not exist and will fail compilation if unguarded.
* **WHEN displaying DSP states (UI/AUDIO THREAD ISOLATION):** You MUST NOT iterate non-atomic DSP structures (like `VoiceManager::mVoices`) directly from the UI thread, such as within `OnIdle()`. You must use atomic mirrors or lock-free SPSC ring buffers for visual data.
* **WHEN building UI components:** You MUST NOT perform direct drawing calls (e.g., `g->FillRect`) within `OnLayout()`. Custom graphics must be encapsulated exclusively within `IControl::Draw()` methods.
* **WHEN styling typography:** You MUST explicitly set fonts and colors using a centralized theme/style object (e.g., `PolyTheme`). Hardcoding strings for fonts causes immediate launch crashes.

## 📜 5. SCRIPT ORGANIZATION & LOGGING

* Root-level helper scripts have been retired. Non-trivial script implementations now live in `scripts/tasks/`.
* Shared command logging helpers live in `scripts/lib/cli.sh`, and the command execution wrapper lives in `scripts/cli.sh`.
* You MUST NOT reintroduce old root scripts unless explicitly requested. If you need lower-level behavior, call scripts in `scripts/tasks/` directly.
* Full stdout/stderr is always written to `.artifacts/logs/<run-id>/`. Terminal output is compact (`PASS`/`FAIL` + duration). On failure, an issue excerpt is printed, with a path to the full log. 
* **When reporting failures in conversation, you MUST include the log path from `.artifacts/logs/...`.**
* To view logs, use: `just logs-latest` or `POLYSYNTH_VERBOSE=1 just <task>`.

## 🎨 6. UI ITERATION & HUMAN FEEDBACK PROTOCOL

As an AI, you cannot visually inspect the rendered iPlug2 GUI. When a human provides UI feedback, you MUST follow this defensive protocol:

* **WHEN a human reports visual overlap, clipping, or bad alignment:** You MUST NOT blindly guess pixel offsets or hardcode magic numbers for `IRECT` math. You MUST search the codebase for established layout patterns (e.g., `AttachStackedControl()`, `BuildHeader()`) and adapt them.
* **WHEN a human reports a crash immediately on UI launch:** You MUST immediately verify that the code uses explicit font styles from the centralized `PolyTheme` object.
* **WHEN a human reports a Web/WAM build failure after UI changes:** You MUST audit the preprocessor stack for unguarded `GetUI()` calls and wrap them in `#if IPLUG_EDITOR` / `#endif`.

## 🧪 7. BUG FIXING & REGRESSION PROTOCOL (Test-Driven Resolution)

Your goal is to permanently immunize the repository against recurring failures. You MUST follow a strict Test-Driven Development (TDD) loop for all bug reports.

* **Step 1 (Reproduce):** You MUST NOT write the fix first. You MUST first write a new Catch2 unit test in the appropriate `tests/unit/` file that reproduces the exact bug or regression.
* **Step 2 (Verify Failure):** You MUST run `just test` to prove that the new test accurately catches the failure.
* **Step 3 (Implement Fix):** You MUST implement the code fix until the newly written test passes.
* **Step 4 (Document the Gap):** You MUST document the testing gap in `plans/SPRINT_RETROSPECTIVE_NOTES.md` using the Problem/Action/Lesson format.
* **For UI/Build specific regressions:** If Catch2 tests do not apply, you MUST update or write an automated script (like `scripts/check_ui_safety.py`) to catch the structural rule violation before compilation.

## 📝 8. CONTINUOUS LEARNING & KNOWLEDGE CAPTURE

You MUST act as a steward of the project's institutional memory. 

* **WHEN to document:** You MUST capture a learning whenever you resolve a build/UI failure, debug a complex DSP issue, receive an architectural correction, or apply the Bug Fixing Protocol (Section 7).
* **WHERE to document:** You MUST append your learnings to `plans/SPRINT_RETROSPECTIVE_NOTES.md` under the current sprint's "Retro" or "Issues & Resolutions" section. 
* **HOW to document:** You MUST use the exact three-part format:
  * **Problem:** A concise explanation of what failed or was ambiguous.
  * **Action:** What you changed, where, and why to fix it.
  * **Lesson:** A generalized, prompt-friendly rule designed to prevent the mistake in the future.

## 📋 9. AGENT PRE-FLIGHT CHECKLIST

Before concluding your task, running `just ci-pr`, or submitting a PR, you MUST output a checklist verifying the following:

1.  [ ] **Test-Driven Development:** Have you written Catch2 unit tests defining expected behavior *before* writing the implementation?
2.  [ ] **Build Systems Synced:** Have you updated all parallel build targets? Adding a library dependency requires updating native CMake, desktop Xcode, AND WAM makefiles simultaneously.
3.  [ ] **Variable Hygiene:** Have you verified all stored variables are actually read? Have you discarded unused return values as statements? Are intentionally unused parameters explicitly suppressed with `(void)param;`?
4.  [ ] **Compile-Time Checks:** If the spec mentions trait requirements checked at compile time, have you actually implemented the `static_assert` calls?
5.  [ ] **Golden Master Policy:** If your fix changes runtime behavior, have you checked the decision tree for Golden Masters?
6.  [ ] **Desktop Startup Smoke:** Have you run `just desktop-smoke` (or CI equivalent) to verify the desktop app reaches UI-loaded state without crashing?
