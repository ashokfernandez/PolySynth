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

## Sprint 4 (UI Integration) — Retro placeholders

- _Add notes here during/after Sprint 4 review._

---

## Resolved notes

- _Move closed items here with commit/PR references._
