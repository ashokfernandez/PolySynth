# PolySynth Architecture Refactoring — Sprint Plans

## Overview

This folder contains fully groomed sprint documents for the PolySynth architecture review and refactoring. Each sprint is independently deliverable and designed so any developer can pick it up, implement the changes, and verify the results.

## Sprint Summary

| Sprint | Title | Risk | Effort | Key Deliverable |
|--------|-------|------|--------|-----------------|
| **1** | [Extract Voice to Own Header](01_sprint_extract_voice.md) | Low | Small | Voice.h + 12 isolated Voice tests |
| **2** | [Eliminate PolySynth_DSP](02_sprint_eliminate_polysynth_dsp.md) | Low | Small | Delete adapter, plugin owns Engine directly |
| **3** | [Extract Magic Numbers](03_sprint_magic_numbers.md) | Low | Small | DspConstants.h with 13+ named constants |
| **4** | [Strengthen Test Pyramid](04_sprint_test_pyramid.md) | Low | Medium | 22+ new tests (filters, boundaries, stress, unused fields) |
| **5** | [Performance Caching](05_sprint_performance_caching.md) | Medium | Small | Cache pow/exp/sqrt in setters (may regenerate golden masters) |
| **6** | [LFO Tests & Documentation](06_sprint_lfo_tests_and_docs.md) | Low | Medium | 7 LFO tests + architecture doc updates |
| **7** | [Table-Driven OnParamChange](07_sprint_table_driven_params.md) | High | Large | ParamMeta table replaces 120-line switch |

## Dependency Graph

```
Sprint 1 (Voice extraction)     ─── enables ──→ Sprint 3 (Magic Numbers)
                                                     └──→ Sprint 5 (Performance)
                                ─── enables ──→ Sprint 4 (Test Pyramid)
                                ─── enables ──→ Sprint 6 (LFO Tests)

Sprint 2 (PolySynth_DSP removal)    independent

Sprint 7 (Table-Driven Params)      independent but do LAST (highest risk)
```

## Recommended Execution Order

```
Phase 1 (parallel):  Sprint 1 + Sprint 2    (no overlapping files)
Phase 2 (parallel):  Sprint 3 + Sprint 4    (after Sprint 1)
Phase 3:             Sprint 5               (after Sprint 3)
Phase 4:             Sprint 6               (after Sprint 1, ideally after all others for docs)
Phase 5:             Sprint 7               (standalone, highest risk, do last)
```

## What Each Sprint Delivers

### Sprint 1: Foundation for Testing
- **Before:** Voice class trapped inside VoiceManager.h — can't test in isolation
- **After:** Voice.h is independently includable, 12 new test cases verify all Voice features

### Sprint 2: Clean Platform Boundary
- **Before:** Unnecessary PolySynth_DSP adapter pulls iPlug2 headers into DSP-adjacent code
- **After:** Plugin owns Engine directly, MIDI dispatch is 8 inline lines

### Sprint 3: Self-Documenting DSP Code
- **Before:** 15+ magic numbers like `0.05`, `10000.0`, `0.020` scattered through hot path
- **After:** Every tuning parameter is a named constant with a comment explaining its purpose

### Sprint 4: Comprehensive Safety Net
- **Before:** No filter model tests, no boundary tests, no stress tests, no unused-field guards
- **After:** 22+ new tests ensuring filters are stable, parameters are safe at extremes, voice stealing works under stress, and unused fields don't accidentally affect audio

### Sprint 5: Embedded-Ready Performance
- **Before:** `std::pow`, `std::exp`, `std::sqrt` called per-sample per-voice in audio loop
- **After:** All expensive math cached in setter methods, only recomputed when parameters change

### Sprint 6: Complete Documentation
- **Before:** LFO routing untested, architecture docs outdated after 5 sprints of changes
- **After:** 7 LFO tests, Voice::Process() has signal-flow comments, architecture docs match reality

### Sprint 7: Maintainable Parameter System
- **Before:** Adding a parameter requires editing 5 locations in PolySynth.cpp
- **After:** Adding a parameter requires editing 2 locations (table entry + SynthState field)

## Cumulative Impact

| Metric | Before | After All Sprints |
|--------|--------|-------------------|
| Test cases | ~32 | ~75+ |
| Core files independently testable | 3 (Engine, SynthState, PresetManager) | 4 (+Voice) |
| Magic numbers in hot path | ~15 | 0 |
| Per-sample expensive math calls | 5 (pow, exp, sqrt × multiple) | 0 |
| Lines in OnParamChange | 120 | ~20 |
| Files to edit for new parameter | 5 | 2 |
| Stale architecture docs | Yes | No |
| iPlug2 headers in DSP-adjacent code | Yes (PolySynth_DSP) | No |

## PR Guidelines

See [PR_REVIEW_GUIDELINES.md](../../../docs/guides/PR_REVIEW_GUIDELINES.md) for:
- PR template (required for every sprint)
- Review checklist
- Branch naming conventions
- Merge policy
- Sprint dependency enforcement

## Verification Commands

Every sprint must pass this before merge:
```bash
just build          # CMake test build
just test           # All unit tests pass
just check          # lint + test
just asan           # No memory errors
just tsan           # No data races
python3 scripts/golden_master.py --verify  # Audio unchanged (or regenerated with justification)
```
