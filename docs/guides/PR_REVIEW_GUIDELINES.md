# PolySynth Sprint PR Guidelines

## PR Template

Every sprint PR must follow this template:

```markdown
## <Initiative>-N: [Sprint Title]

### Summary
- [1-3 bullet points describing what changed and why]

### Changes
- [ ] List every file added, modified, or deleted
- [ ] Categorise: NEW / MODIFIED / DELETED

### Testing Evidence
- [ ] `just build` — compiles cleanly (paste output or screenshot)
- [ ] `just test` — all tests pass (paste test count summary)
- [ ] `just check` — lint + tests pass
- [ ] `just asan` — no memory errors (paste summary)
- [ ] `just tsan` — no data races (paste summary)
- [ ] Golden masters: `python3 scripts/golden_master.py --verify` (paste result)

### Platform Evidence (Sprints 2, 7 only)
- [ ] `just desktop-build` — iPlug2 desktop build compiles
- [ ] `just desktop-smoke` — Desktop app launches without crash

### Definition of Done Checklist
[Copy from `plans/SPRINT_DOD_TEMPLATE.md` and add sprint-specific items]

### Reviewer Notes
- Any decisions made during implementation that deviate from the sprint doc
- Any follow-up work identified during implementation
```

---

## Review Checklist (for PR Reviewers)

### Code Quality
- [ ] No new compiler warnings introduced
- [ ] No `#include` cycles or unnecessary header dependencies
- [ ] All new code follows existing naming conventions (camelCase members, PascalCase classes)
- [ ] Header-only pattern maintained for core/ DSP code (Design Principle #8)
- [ ] No `virtual` keyword in core/ code (Design Principle #5)
- [ ] No heap allocations in audio-path code (`new`, `malloc`, `std::vector::push_back`)
- [ ] No exceptions, locks, or blocking calls in audio-path code
- [ ] No magic numbers — numeric constants are named or commented (Design Principle #9)
- [ ] No bare `==` on floats in tests — use `Approx` with explicit margin (Design Principle #10)

### Testing
- [ ] Every new function/method has at least one test
- [ ] Edge cases tested (zero values, max values, NaN guards)
- [ ] All existing tests still pass (no regressions)
- [ ] Test names clearly describe what they verify

### Architecture
- [ ] Core code has zero iPlug2 dependencies (check includes)
- [ ] No `using namespace` in headers
- [ ] `static_assert(std::is_aggregate_v<SynthState>)` still compiles
- [ ] SPSCQueue pattern unchanged (lock-free UI→audio boundary)

### Documentation
- [ ] New constants have one-line comments explaining their purpose
- [ ] New test files have a brief header comment explaining what they test
- [ ] Sprint doc's Definition of Done is fully satisfied

---

## Branch Naming

All sprint branches follow: `sprint/<prefix>-N-short-description`

The prefix identifies the initiative (e.g., `AR` for Architecture Review, `pico` for Pico Port).

Examples:
- `sprint/AR-1-extract-voice-header`
- `sprint/AR-3-magic-numbers-to-constants`
- `sprint/pico-1-cmake-skeleton`

---

## Merge Policy

1. **Squash merge** each sprint into `main` with a clean commit message
2. **CI must be green** — all `just check` + `just asan` + `just tsan` pass
3. **One approval** required from a team member who did not write the code
4. **Golden master delta** — if golden masters changed, the PR must explain why and include regenerated files
5. **No partial sprints** — all tasks in the sprint doc must be complete before merge

---

## Sprint Dependency Graph

Each initiative defines its own dependency graph in its `00_overview.md` file under `plans/active/<initiative>/`. Consult that file before starting work on any sprint to understand ordering constraints.

---

## Cross-Sprint Fix Policy

If during Sprint N you discover a bug or needed adjustment in a file modified by an earlier Sprint M:

1. **Small fix (< 10 lines, same area):** Include the fix in Sprint N's PR. Note it in the PR description under a "### Fixes from earlier sprints" heading with a brief explanation.

2. **Larger fix (> 10 lines, different concern):** Create a separate hotfix PR targeting `main` with the branch name `hotfix/sprint-M-description`. This keeps Sprint N's PR focused and reviewable.

3. **Sprint doc correction:** If the sprint doc itself had an error (wrong API, wrong line number), fix the doc in the same PR and note it. Sprint docs are living documents — corrections are expected.

4. **Never silently change behaviour from an earlier sprint** without documenting it in the PR. If a fix changes audio output, regenerate golden masters and explain why.
