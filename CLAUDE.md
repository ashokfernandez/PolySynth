# PolySynth — Agent Instructions

## Required Reading

Before making changes, read the relevant doc:

| Doc | When to read |
|-----|-------------|
| [`docs/architecture/DESIGN_PRINCIPLES.md`](docs/architecture/DESIGN_PRINCIPLES.md) | **Always** — 10 non-negotiable rules |
| [`docs/architecture/OVERVIEW.md`](docs/architecture/OVERVIEW.md) | Understanding the codebase |
| [`docs/architecture/TESTING_GUIDE.md`](docs/architecture/TESTING_GUIDE.md) | Writing or modifying tests |
| [`docs/architecture/ADDING_A_PARAMETER.md`](docs/architecture/ADDING_A_PARAMETER.md) | Adding a SynthState field |
| [`plans/active/architecture-review/pr_guidelines.md`](plans/active/architecture-review/pr_guidelines.md) | Opening a PR |
| [`plans/SPRINT_RETROSPECTIVE_NOTES.md`](plans/SPRINT_RETROSPECTIVE_NOTES.md) | Learning from past mistakes |

## Build Commands

**Always use `just` commands.** Never call cmake/make/scripts directly.

```
just build          # Compile tests (Ninja + cppcheck)
just test           # Run unit tests
just check          # Lint + tests
just ci-pr          # Full PR gate (lint + ASan + TSan + tests + desktop smoke + VRT)
just vrt-baseline   # Regenerate VRT baselines
just vrt-run        # Verify VRT against baselines
just desktop-build  # Build desktop plugin
just sandbox-build  # Build ComponentGallery
```

**Before pushing any PR branch:** run `just ci-pr` to verify the full gate passes.

## Hard Rules (violations = PR rejection)

1. **Skia only.** Graphics backend is IGRAPHICS_SKIA everywhere. Never use NanoVG.
2. **No allocations in audio path.** Zero `new`/`malloc`/`vector::push_back`/exceptions/locks in code called from `ProcessBlock`.
3. **No magic numbers.** Name constants or add inline derivation comments. DSP constants go in `DspConstants.h`.
4. **No bare `==` on floats in tests.** Use `Approx(x).margin(eps)`.
5. **SynthState is the single source of truth.** Defaults are inline member initializers. `Reset()` is `*this = SynthState{}`.
6. **Serialization must cover every field.** `Test_SynthState.cpp` round-trip test enforces this.
7. **Layer boundaries: SEA_DSP ← Core ← Platform.** Core code must never include iPlug2 headers.
8. **VRT uses Skia pixel readback.** `VRTCaptureHelper.h` enforces `#error` if IGRAPHICS_SKIA is missing.

## Code Style

- camelCase for members/variables, PascalCase for classes/types
- `sample_t` (from `types.h`) for all audio signal values, not bare `double`
- `#if IPLUG_EDITOR` guards on all `GetUI()` calls
- No `using namespace` in headers
- Hot path (`Voice::Process`, `VoiceManager::Process`) stays header-only + `inline`
