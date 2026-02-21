# Testing Setup Guide

This guide is the operator-focused setup for local testing workflows.

## Prerequisites

- `just`
- CMake 3.14+
- Python 3.10+
- Ninja (recommended)
- Platform toolchain:
  - macOS: Xcode command line tools
  - Linux: GCC/Clang + standard build tools
  - Windows: MSVC build tools

## Bootstrap

```bash
just deps
```

## Core Test Commands

```bash
just test            # Catch2 unit tests
just asan            # AddressSanitizer + UBSan
just tsan            # ThreadSanitizer
just check           # lint + unit tests
just ci-pr           # local PR gate (lint + asan + tsan + tests)
```

## Native UI Visual Regression (VRT)

```bash
just sandbox-build   # build PolySynthGallery native sandbox
just vrt-run         # compare captures to baselines
just vrt-baseline    # regenerate baselines intentionally
just gallery-test    # convenience alias: sandbox-build + vrt-run
```

Outputs:
- Baselines: `tests/Visual/baselines/`
- Current captures: `tests/Visual/current/`
- Diff reports: `tests/Visual/output/`

## Migration Contract Validation

```bash
bash scripts/tasks/check_migration_complete.sh
```

This must pass before considering the native sandbox migration complete.

## Logs and Failure Triage

All `just` tasks write full logs under `.artifacts/logs/<run-id>/`.

```bash
just logs-latest
POLYSYNTH_VERBOSE=1 just test
```
