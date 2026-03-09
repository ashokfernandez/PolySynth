# Process Improvements

Actionable suggestions distilled from 15 sprint retrospectives across 3 initiatives (Voice Manager, Architecture Review, Native Sandbox Migration).

---

## 1. Write retros at sprint completion, not later

Six Architecture Review retros had to be reconstructed from git history. Retros should be the final step of each sprint PR, written while context is fresh. Add to the sprint Definition of Done:

> ☐ Retro section added to `plans/SPRINT_RETROSPECTIVE_NOTES.md`

---

## 2. Archive completed initiatives immediately

Three completed initiatives sat in `plans/active/` well past completion. Add a checklist item to the final sprint of each initiative:

> ☐ Move initiative folder from `plans/active/` to `plans/archive/`
> ☐ Update `plans/README.md` and `plans/active/README.md`

---

## 3. Use initiative-prefixed sprint names

"Sprint 3" is ambiguous across initiatives. Use `AR-3`, `VM-2`, `NSM-1` prefixes in commit messages, branch names, and retro headers to avoid confusion.

---

## 4. Pre-push verification is non-negotiable

Multiple retros document issues that would have been caught by running `just ci-pr` locally before pushing. This is already in MEMORY.md but should be enforced:

- Add a pre-push hook or make it a PR template checkbox
- Especially for CI/workflow changes: simulate the runner environment locally first

---

## 5. Multi-build-system checklist

When touching headers or dependencies, update ALL build systems. Add to sprint plan templates:

> ☐ CMake (tests/native)
> ☐ Xcode (desktop plugin)
> ☐ WAM makefiles (web builds)

---

## 6. Stop creating redirect stubs

When moving files, just move them. Don't leave "This document moved to..." stubs — git history is the redirect.

---

## 7. Consolidate superseded plans

Don't keep multiple versions of the same plan (e.g., 3 DSP refactor docs). Archive or delete older versions when a new one supersedes them.

---

## 8. Sprint plan validation gate

Before treating a sprint plan as authoritative, validate key claims:

- Build targets actually compile on specified platforms
- API calls reference real methods (not assumed ones)
- Test case data is correct (count semitones, verify values)
