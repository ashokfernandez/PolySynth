# Voice Manager & Theory Engine — Sprint Retrospective Notes

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

### What would have made execution more successful from the start

- A single, explicit "source of truth" section for **behavior invariants per sprint** (e.g., steal semantics, reset semantics, golden invariants).
- A short "review guardrails" checklist in each sprint plan:
  - Which comments are valid improvements now,
  - Which are intentionally deferred,
  - Which would violate sprint invariants.
- A mandatory "if changing runtime behavior, update these artifacts" note (tests + golden policy + migration note).

---

## Sprint 2 (Core Allocation) — Retro placeholders

- _Add notes here during/after Sprint 2 review._

## Sprint 3 (Theory Engine) — Retro placeholders

- _Add notes here during/after Sprint 3 review._

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

---

## Resolved notes

- _Move closed items here with commit/PR references._
