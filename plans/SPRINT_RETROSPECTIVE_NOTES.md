# PolySynth — Sprint Retrospective Notes

This file captures cross-sprint review/retro feedback that should inform future implementation.

## How to use this file

- Add notes as bullets under the relevant sprint section.
- Keep each note actionable (what to change, where, and why).
- When a note is resolved, move it to a `Resolved` subsection and reference the commit/PR.
- Prefer clarifying plan docs over relying on reviewer tribal knowledge.

---

## Sprint 1 (Scaffolding) — Retro

### Open follow-ups for future sprints

1. **Clarify `VoiceManager::Reset()` sample-rate contract in plan docs**
   - Ambiguity observed: should `Reset()` restore to canonical 44.1k defaults for reproducibility, or to runtime `mSampleRate` for physical correctness?
   - This decision materially affects golden-master stability and future reviewer expectations.
   - **Action:** add explicit policy text in `00_overview.md` and/or Sprint 2/4 docs.

2. **Document steal-path behavior more explicitly where `OnNoteOn()` is specified**
   - Design decision says immediate retrigger on normal steal, and `StartSteal()` for polyphony-limit reduction only.
   - Some review feedback expected fade on normal steal.
   - **Action:** repeat this rule directly in Sprint 2 `OnNoteOn()` pseudocode block and acceptance criteria to prevent misinterpretation.

3. **Golden-master expectations should call out behavior-changing fixes path**
   - The current plan says goldens unchanged until later, but review comments may still request behavior changes that invalidate goldens.
   - **Action:** add a short decision tree in Sprint docs: "if behavioral fix conflicts with current goldens, choose X and document Y."

4. **Make “test harness prerequisites” explicit**
   - Local test runs depend on `json.hpp` in `external/iPlug2/Dependencies/Extras/nlohmann/`.
   - **Action:** add one line to test setup docs indicating dependency/bootstrap expectation.

5. **`VoiceManager::mSampleRate` stored but never read**
   - **Problem:** `Init()` saved `mSampleRate` into the VoiceManager, but `Reset()` used the literal `44100.0` and nothing else ever read the field. Dead storage.
   - **Action:** Identified in review as a redundant field; `Reset()` using `44100.0` is correct per the plan spec, so `mSampleRate` on `VoiceManager` itself is unused. Noted for cleanup in a future refactor.
   - **Lesson:** If a field is stored but never consumed, it either needs a consumer or should be removed. A stored-but-unread field is a sign the design changed mid-implementation.

6. **Two blank lines before `GetVoiceStates()` / `GetHeldNotes()` made public methods look misplaced**
   - **Problem:** The two new public methods were appended between `IsNoteActive()` and the `private:` keyword with no section marker, making it look like they had slipped in accidentally.
   - **Action:** Added a `// --- Sprint 1: Voice state query helpers ---` comment to signal intent.
   - **Lesson:** When adding new public methods to an existing class, either place them with the existing public group or add a section comment. Don't leave orphaned methods floating before `private:`.

7. **Reviewer incorrectly flagged `StartSteal()` absence and `Reset()` sample-rate as bugs**
   - **Problem:** Two review findings in round 1 were wrong: (a) `StartSteal()` not being called in normal `OnNoteOn()` steal path is intentional per spec; (b) `Reset()` using `44100.0` literally is exactly what the plan's pseudocode specifies.
   - **Action:** Both were retracted in round 2 after re-reading the spec. The retro note in the PR itself documents the misreading.
   - **Lesson:** Re-read the plan's pseudocode before flagging deviations. The spec may be more prescriptive than expected on seemingly incidental details. Adding a "review guardrails" section to each sprint plan would prevent this.

### What would have made execution more successful from the start

- A single, explicit "source of truth" section for **behavior invariants per sprint** (e.g., steal semantics, reset semantics, golden invariants).
- A short "review guardrails" checklist in each sprint plan:
  - Which comments are valid improvements now,
  - Which are intentionally deferred,
  - Which would violate sprint invariants.
- A mandatory "if changing runtime behavior, update these artifacts" note (tests + golden policy + migration note).

---

## Sprint 2 (Core Allocation) — Retro

### Issues & Resolutions

1. **Active-count scan scope mismatch in `AllocateSlot`**
   - **Problem:** The active-voice count loop iterated over `MaxSlots` (the full array), but the free-slot search only scanned `[0, mPolyphonyLimit)`. If voices above the polyphony limit were still fading out after a limit reduction, they were counted as active but invisible to the search — causing `AllocateSlot` to return -1 (no slot available) even when slots inside the limit were free.
   - **Action:** Both loops now bound to `mPolyphonyLimit`. `EnforcePolyphonyLimit` intentionally still scans `MaxSlots` — it needs to discover voices above the limit in order to kill them. This asymmetry is deliberate and commented.
   - **Lesson:** When `AllocateSlot` and `EnforcePolyphonyLimit` share the same array but have different scan semantics, document the difference explicitly or it will look like a bug on re-read.

2. **Unused includes (`<cmath>`, `<type_traits>`) shipped without the code that justified them**
   - **Problem:** Both headers were added to `sea_voice_allocator.h` in anticipation of `static_assert` trait checks and `std::pow` calls that were never written. The spec's allowed-include list (`<array>`, `<cstdint>`, `<algorithm>`) was violated.
   - **Action:** `<cmath>` removed. `<type_traits>` kept once the `static_assert` checks were actually implemented.
   - **Lesson:** Don't add includes speculatively. Add them when the code that requires them lands in the same commit.

3. **Static_assert trait requirements promised but not implemented**
   - **Problem:** The class comment and the spec both stated trait requirements would be "checked at compile time via static_assert in methods," but no `static_assert` calls existed. Users passing a non-conforming type would get cryptic template errors.
   - **Action:** Four `static_assert(std::is_invocable_r_v<...>)` checks added at class scope covering `IsActive`, `GetTimestamp`, `GetPitch`, and `StartSteal`.
   - **Lesson:** If a spec says "checked at compile time," treat that as a deliverable, not a suggestion. Unfulfilled static_asserts are tech debt that actively harms future users of the template.

4. **Unused variable warning from `killCount` in `VoiceManager::SetPolyphonyLimit`**
   - **Problem:** `int killCount = mAllocator.EnforcePolyphonyLimit(...)` stored the return value in a variable that was never read, producing a `-Wunused-variable` warning and noisy inline comments explaining the design decision.
   - **Action:** Return value discarded (call used as a statement), comment replaced with a single-line explanation.
   - **Lesson:** Design-diary reasoning belongs in retro notes or PR descriptions, not in production source comments. Inline comments should describe *what* and *why*, not *why I wasn't sure*.

5. **`ShouldHold(int note)` silently ignored its parameter**
   - **Problem:** The method accepted a MIDI note number but discarded it entirely, returning `mSustainDown` unconditionally. This generated an unused-parameter warning and left the API misleading — callers might expect per-note sustain logic.
   - **Action:** Added `(void)note;` suppression and a comment: "Note-agnostic sustain for now."
   - **Lesson:** When a parameter is intentionally unused, suppress the warning explicitly and document why. Silence is ambiguous — it looks like an oversight.

6. **Task 2.2 (enum aliases in `types.h`) delivered as comment-only, not checked off**
   - **Problem:** The plan specified adding a comment block to `types.h` noting where `AllocationMode` and `StealPriority` live. This was an open acceptance criterion that wasn't in the initial diff.
   - **Action:** Comment block added explaining aliases are not duplicated to avoid circular includes.
   - **Lesson:** Comment-only tasks are easy to skip because they feel trivial. Include them in the PR checklist explicitly.

### What worked well

- **`EnforcePolyphonyLimit` using `std::sort` + stack-allocated `Victim` array** was a clean, RT-safe approach — no heap, linear time for small `MaxSlots`, and easily auditable.
- **Deletion of `FindFreeVoice` / `FindVoiceToSteal`** with no legacy wrapper left behind kept the codebase clean. The allocator fully replaced the old logic with no dead code.
- **Demo targets gaining `SEA_Util` linkage** was caught proactively — `VoiceManager.h` now transitively pulls in `sea_voice_allocator.h`, so any TU that includes VoiceManager needs the library. All demo targets were updated in the same PR.

### Technical Debt / Future Improvements

- **`ShouldHold` is note-agnostic.** Per-note sustain (where only certain notes are held by the pedal) would require tracking which notes were active at pedal-down time. This is the correct model for legato and half-pedal techniques. Tracked for a future sprint.
- **`mRoundRobinIndex` is not reset on `Init()`.** If `VoiceManager::Init()` is called mid-session, cycle-mode allocation will continue from wherever the round-robin index was, rather than from 0. Low priority but worth noting.

## Sprint 3 (Theory Engine) — Retro

### Issues & Resolutions

1. **Negative Modulo Undefined Behavior**
   - **Problem:** `note % 12` in C++ returns a negative value for negative inputs (e.g., `-1 % 12 == -1`), which caused out-of-bounds bit shifts when building the pitch class mask.
   - **Action:** Implemented `if (pc < 0) pc += 12;` normalization to ensure all pitch classes are in the [0, 11] range.
   - **Lesson:** Standard C++ `%` is a remainder operator, not a mathematical modulo. Always normalize for circular arithmetic like pitch classes.

2. **Aesthetic Accuracy vs. Template Matching**
   - **Problem:** Pure bitmask subset matching (`(rotated & tpl) == tpl`) was too permissive. A 12-note chromatic cluster would match *every* chord template simultaneously.
   - **Action:** Introduced a scoring system that penalizes "noise" (extra notes not in the template).
   - **Lesson:** Chord detection in a musical context requires "functional harmony" filtering—rejecting matches where the signal-to-noise ratio is too low (e.g., clusters).

3. **Inversion Priority**
   - **Problem:** Root position chords were sometimes shadowed by inversions with more intervals (e.g. a 7th chord inversion matching over a triad).
   - **Action:** Added a +2 score bonus for "Root == Bass" to prefer root-position triads over complex inversions when the match is ambiguous.

4. **Plan spec had wrong notes for the Bm7b5 test case**
   - **Problem:** The spec's Bm7b5 test used `{59, 62, 65, 70}` — but B(59) to Bb(70) is 11 semitones (a major 7th), not the 10-semitone minor 7th that defines a half-diminished chord. The test would have failed.
   - **Action:** Implementation correctly used `{59, 62, 65, 69}` (B, D, F, A — 10 semitones from B). The spec had a wrong note.
   - **Lesson:** Music theory test cases should be double-checked by counting semitone intervals, not just copying note numbers from memory. A quick sanity check (note - root = expected interval) catches these before they reach a PR.

### What worked well

- **TDD for Music Theory:** Writing the 12 major triads test case up-front immediately caught a one-off error in the rotation logic that would have been hard to debug purely by reading the code.
- **Stateless/All-Static Design:** The engine is extremely easy to test and integrate because it has no side effects and requires no initialization. This fits the `sea::` library goal of "building blocks."
- **Bass-note bonus (+2) for root-position scoring** cleanly resolved augmented chord ambiguity (C/E/Ab aug all produce the same bitmask — the bonus reliably picks the root matching the lowest sounding note).
- **`bestScore <= 0` gate** means a chord with as many extraneous notes as template notes scores exactly 0 and is rejected as invalid — the right behaviour for dense clusters without a separate cluster-detection pass.

### Technical Debt / Future Improvements

- **Omitted 5ths:** Current templates require the perfect 5th. In jazz/pop, the 5th is often omitted. A future improvement could allow "essential tone" matching (Root + 3rd + 7th).
- **Scale Detection:** The bitmask logic here is a perfect foundation for a `ScaleEngine` (detecting the most likely key/mode of a sequence).
- **Augmented chord inversions are inherently ambiguous.** C aug in first inversion `{E, Ab, C}` is reported as "Eaug" (root position) because all three enharmonic roots score equally and the bass-note bonus picks E. This is consistent with how most notation software handles it, but worth documenting for future UI work.
- **`extraBits` uses XOR not AND-NOT** — since the match condition guarantees the template is a subset of the rotated mask, these are equivalent. Worth a comment for future readers who may find the XOR surprising.

## Sprint 4 (UI Integration & Advanced Voice Management) — Retro

### Issues & Resolutions

1. **Launch Crash due to Font Name Mismatches**
   - **Problem:** Application crashed immediately on launch with an assertion failure in `IGraphicsNanoVG`.
   - **Cause:** Fonts were loaded as "Regular" and "Bold", but UI controls (`PolySection`, `PolyKnob`, etc.) were hardcoded to request "Roboto-Regular" and "Roboto-Bold".
   - **Action:** Standardize font resource names across the entire project and explicitly set font styles in the main `synthStyle` object.
   - **Lesson:** Always use a centralized theme/style object for typography to avoid hardcoding strings in multiple UI classes.

2. **Compilation Failure from Preprocessor Directives**
   - **Problem:** Build failed with `#endif without #if`.
   - **Cause:** Accidental insertion of `#endif` during rapid iteration on `PolySynth.cpp`.
   - **Action:** Improved build hygiene and carefully audited the preprocessor stack.

3. **UI Drawing Safety in Layout Code**
   - **Problem:** Direct drawing calls (e.g., `g->FillRect`) within `OnLayout` can lead to crashes or undefined behavior in certain graphics backends.
   - **Action:** Created a regression test script `scripts/check_ui_safety.py` to prevent direct drawing in layout functions.
   - **Lesson:** Custom graphics should always be encapsulated in `IControl::Draw` methods, with `OnLayout` only handling attachment and positioning.

4. **Voice Stealing "Crunch" Artifacts**
   - **Problem:** Audible clicks when voices were stolen during polyphony limit reduction.
   - **Action:** Increased the voice stealing crossfade/fade-out duration (2ms -> 20ms) in the DSP core.
   - **Lesson:** UI-driven state changes (like polyphony limit) can require smoother ramping than real-time MIDI-driven voice stealing.

5. **UI Layout Alignment & Polish**
   - **Problem:** Preset selector and Save button alignment was inconsistent; version text was cluttering the footer.
   - **Action:** Refined `BuildHeader` and `BuildFooter` with consistent centering logic and removed non-essential text elements.

11. **Thread Safety in UI Visualization (Active Voice Count & Held Notes)**
    - **Problem:** `OnIdle()` (UI thread) was reading `GetActiveVoiceCount()` and `GetHeldNotes()` directly from the DSP object, iterating over voices that are modified by the Audio thread. This is a race condition.
    - **Action:** Implemented atomic caching in `PolySynthDSP`. The Audio thread updates `std::atomic` variables (`mVisualActiveVoiceCount` and bitmasks for `mVisualHeldNotes`) at the end of `ProcessBlock`. The UI thread reads these atomics.
    - **Lesson:** Never iterate non-atomic DSP structures (like `VoiceManager::mVoices`) directly from the UI thread. Use atomic mirrors or lock-free queues for visual data.

### CI Stabilization (post-implementation review by separate agent)

A separate agent was brought in to fix CI failures on PR #39 after the Sprint 4 implementation agent had finished. Three CI jobs were failing: Build Gallery WAM, Build PolySynth WAM Demo, and Build Component Gallery Storybook. The fixes required four commits across five files. Below is what was found and what could have been done better.

#### Issues found

6. **Missing `#include "PolyTheme.h"` in `Envelope.h`**
   - **Problem:** `Envelope.h` referenced `PolyTheme::ControlBG`, `PolyTheme::AccentCyan`, etc. but never included the header. The desktop Xcode build happened to pull it in transitively, but the WAM web builds (Gallery and Storybook) compiled `Envelope.h` in a different include order where the transitive path didn't exist.
   - **Action:** Added `#include "PolyTheme.h"` to `Envelope.h`.
   - **Lesson:** Every header should include what it uses directly. Never rely on transitive includes — they break when compilation order or include paths change between build targets.

7. **Missing `SEA_Util` include path in WAM build makefiles**
   - **Problem:** `VoiceManager.h` (added in Sprint 2) includes `<sea_util/sea_voice_allocator.h>`, but `PolySynth-web.mk` only had `SEA_DSP` in its `WAM_CFLAGS` and `WEB_CFLAGS`. The WAM processor build couldn't find the header.
   - **Action:** Added `SEA_UTIL_INCLUDE` variable and appended `-I$(SEA_UTIL_INCLUDE)` to both `WAM_CFLAGS` and `WEB_CFLAGS`.
   - **Lesson:** When a new library dependency is added to the core (Sprint 2 added `SEA_Util`), *all* build targets that compile core headers must be updated — not just CMake/native builds. The WAM makefile-based builds are a separate build system that must be maintained in parallel. This was noted as working well in Sprint 2's retro ("Demo targets gaining `SEA_Util` linkage was caught proactively"), but the WAM web makefiles were missed.

8. **Unguarded `GetUI()` calls in `PolySynth.cpp` for WAM processor build**
   - **Problem:** `OnIdle()` and `OnParamChange()` called `GetUI()` and cast to `PresetSaveButton*` without `#if IPLUG_EDITOR` guards. The WAM processor build defines `IPLUG_DSP=1` with `NO_IGRAPHICS`, so `GetUI()` doesn't exist and `PresetSaveButton` is not included.
   - **Action:** Wrapped the editor-only blocks in `#if IPLUG_EDITOR` / `#endif`.
   - **Lesson:** iPlug2 compiles the same `.cpp` file in multiple configurations (editor+DSP for controller, DSP-only for processor). Any code that touches UI must be guarded. The Sprint 4 agent should have audited all `GetUI()` calls against the preprocessor stack before committing.

9. **Dead code and copy-paste artifacts in `VoiceManager.h`**
   - **Problem:** Three issues were being hidden by cppcheck suppressions added during the CI hardening pipeline:
     - Duplicate `rs.panPosition = mPanPosition;` line (copy-paste)
     - `pan` variable computed then immediately re-assigned the same expression, surrounded by 20+ lines of "thinking out loud" comments
     - Local `const double kPi` shadowing the identical `kPi` from `types.h`, with 7 lines of stream-of-consciousness comments debating which constant to use
   - **Action:** Removed dead code, removed rambling comments, removed 3 unnecessary suppressions (`redundantAssignment`, `redundantInitialization`, `shadowVariable`).
   - **Lesson:** LLM-generated "thinking out loud" comments (e.g., "Let's check if kPi is defined in types.h? It's likely M_PI. Plan said kPi.") must never be committed to production source. These are reasoning traces, not documentation. The implementing agent should have cleaned up its draft comments before finalizing the PR. Similarly, duplicate lines should have been caught by a self-review pass or by running cppcheck locally before pushing.

10. **New cppcheck 2.19 checks not covered by suppressions**
    - **Problem:** `suspiciousFloatingPointCast` (test code) and `dangerousTypeCast` (WAV writer) are new checks in cppcheck 2.19 that weren't in the CI's older version. They would break CI on a future cppcheck upgrade.
    - **Action:** Added targeted suppressions for both in test/utility code only.
    - **Lesson:** When adding a CI static analysis gate, pin the tool version or periodically run with the latest version locally to catch new checks before they surprise CI.

#### Process observations

- **The implementing agent never ran the WAM web build.** The CI had three distinct web build jobs (Gallery WAM, PolySynth WAM Demo, Component Gallery Storybook) and all three failed. A local `emmake` build or even a dry-run syntax check with `clang++ -fsyntax-only` using the WAM build flags would have caught issues 6, 7, and 8 before pushing.
- **The implementing agent never ran cppcheck locally.** The dead code (issue 9) was already flagged by the static analysis pipeline, but instead of fixing the issues, suppressions were added to baseline them. This is appropriate for pre-existing issues in code you don't own, but not for code you just wrote in the same PR.
- **Multiple build systems require parallel maintenance.** This project has CMake (for native tests), Xcode (for desktop plugin), and Makefiles (for WAM web builds). Adding a dependency or header in one build system does not propagate to the others. Future sprint plans should include a checklist item: "Update all build targets: CMake, Xcode, WAM makefiles."
- **`#if IPLUG_EDITOR` guards are a recurring source of WAM build failures.** Sprint 4's retro item #2 already noted a preprocessor issue. A pre-commit check that scans for unguarded `GetUI()` calls (similar to the existing `check_ui_safety.py`) would prevent this class of error entirely.

### Embedded Portability Refactor (Sprint 4 Follow-up)

13. **Hardcoded `double` and `uint64_t` types hindered embedded porting**
    - **Problem:** `VoiceManager` and `sea_voice_allocator` used `double` for audio/control signals and `uint64_t` for timestamps. This is suboptimal for embedded platforms (e.g. Cortex-M7) where `float` is native and `double` is software-emulated or slower, and 64-bit atomics are expensive.
    - **Action:** Refactored the entire voice architecture to use `sample_t` (configurable via `types.h`) for signals and `uint32_t` for timestamps. Updated `sea::VoiceAllocator` to use `float` for control parameters (spread, detune) as extreme precision isn't needed there.
    - **Lesson:** When designing a DSP library for both desktop and embedded (`SEA_DSP`/`SEA_Util`), strict type discipline is required. Use `sample_t` alias everywhere. Avoid `uint64_t` for sample counters unless the wrap-around time (< 24 hours at 48kHz for uint32) is a genuine problem. (Here, `uint32_t` voice ages/timestamps are relative and short-lived, so wrap-around is manageable or irrelevant).

14. **Ninja migration exposed platform-specific ASan runtime options**
    - **Problem:** During Ninja pipeline validation, `just asan-ninja` aborted on macOS because `ASAN_OPTIONS=detect_leaks=1` is unsupported by the Apple sanitizer runtime.
    - **Action:** Updated `scripts/dev.sh` to build ASan options dynamically: include `detect_leaks=1` only on Linux, while keeping `halt_on_error` and `print_stacktrace` across platforms. CI remains unchanged and still enforces leak checks on Linux.
    - **Lesson:** Sanitizer runtime flags are not universally portable. Keep sanitizer policy strict in Linux CI, but gate unsupported options in local developer scripts by host OS.

15. **Storybook gallery 404 due to incomplete build artifact set**
    - **Problem:** Component-gallery stories load iframe targets like `gallery/knob/index.html`, but CI built only `ComponentGallery/build-web/index.html` via `scripts/build_gallery_wam.sh`. Deployed Pages lacked per-component subdirectories, causing `/component-gallery/gallery/*/index.html` 404s.
    - **Action:** Switched gallery build paths to `scripts/tasks/build_gallery.sh` (which now runs `scripts/build_all_galleries.sh`), updated CI workflows to use that task, and added `scripts/tasks/check_gallery_story_assets.py` to assert each story target has a corresponding built page.
    - **Lesson:** If Storybook uses `staticDirs` for iframe content, CI must build the full static source tree, not just a base bundle. Add an explicit structural check so missing iframe targets fail the pipeline before deploy.

16. **Gallery drift left obsolete components in deployed pages**
    - **Problem:** The gallery still exposed legacy iPlug demo variants (knob/fader/button/switch/etc.) and `gallery-pages-build` copied into `docs/component-gallery` without cleaning, so removed components persisted as stale folders.
    - **Action:** Reduced gallery variants/stories/specs to the PolySynth-relevant controls only (`envelope`, `polyknob`, `polysection`, `polytoggle`, `sectionframe`, `lcdpanel`, `presetsavebutton`) and updated `build_gallery_pages.sh` to clear `docs/component-gallery` before publishing.
    - **Lesson:** Static-site publish steps must replace the destination directory, not overlay it. If the component surface is curated, enforce it in one place (stories + build variant list) and keep smoke tests aligned with those IDs.

17. **Release download UI can drift if generator and deployed page diverge**
   - **Problem:** `docs/index.html` is generated by `scripts/generate_test_report.py`. Adding a manual download section directly in the generated file risks being overwritten the next time report generation runs.
   - **Action:** Added release-download UI in the generator template and in the current docs page, plus CI generation of `docs/downloads.json` from GitHub Releases in `package-demo-site`.
   - **Lesson:** For generated documentation pages, update the source generator first and treat the generated HTML as derived output to avoid regressions.

---

## Resolved notes

- **Sprint 1 scaffolding** — all issues resolved and merged in PR #36 (squash commit to main).
  - `mLastAmpEnvVal` populated, fade-gain order fixed, `Sustain` enum commented, `VoiceEvent` annotated, README conventions completed, timestamp test strengthened, poly test uses `kMaxVoices` constants.
- **Sprint 2 core allocation** — all issues resolved across two review rounds, merged in PR #37.
  - Active-count bounds bug fixed, dead includes removed, static_assert trait checks added, unused variable cleaned up, `ShouldHold` parameter suppressed, Task 2.2 comment block added.
- **Sprint 3 theory engine** — clean implementation, merged in PR #38 in a single review pass.
  - Plan's Bm7b5 note error caught and corrected in implementation. Three extra robustness tests (negative notes, large count, large MIDI values) added beyond spec.
