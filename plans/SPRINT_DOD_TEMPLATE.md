# Sprint Definition of Done — Template

Copy this checklist into every sprint PR. All items must be checked before merge.

---

## Universal DoD

- [ ] `just ci-pr` passes (lint + ASan + TSan + tests + desktop smoke)
- [ ] `just vrt-run` passes (visual regression)
- [ ] All sprint plan tasks complete — no partial sprints
- [ ] No new compiler warnings introduced
- [ ] No LLM reasoning traces in committed code
- [ ] Multi-build-system sync: CMake, Xcode, and any platform-specific configs updated
- [ ] Retro section added to `plans/SPRINT_RETROSPECTIVE_NOTES.md` (with Quality Scorecard)
- [ ] Golden masters: verified unchanged or regenerated with explanation

## Sprint-Specific DoD

<!-- Copy the sprint plan's specific DoD items here -->

- [ ] ...
