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
just desktop-smoke   # desktop launch smoke test (UI boots then exits)
just ci-pr           # local PR gate (lint + asan + tsan + tests + desktop-smoke)
```

## Golden Master Audio Tests

```bash
just golden-verify             # verify desktop golden masters
just golden-verify-embedded    # verify embedded (Pico-equivalent) golden masters
just golden-verify-arm         # verify ARM golden masters (requires QEMU + ARM toolchain)

just golden-generate           # regenerate desktop references (after intentional DSP changes)
just golden-generate-embedded  # regenerate embedded references
just golden-generate-arm       # regenerate ARM references
```

References stored in `tests/golden/{desktop-x86_64,embedded-x86_64,embedded-armv7}/` (Git LFS).

## Pico Embedded Tests

```bash
just test-embedded       # unit tests with Pico compile flags (float, 4 voices, no FX)
just pico-build          # build RP2350 firmware (.uf2)
just pico-build-emu      # build for Wokwi emulation (RP2040 proxy)
just pico-emu-test       # run firmware in Wokwi (requires wokwi-cli + WOKWI_CLI_TOKEN)
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

## Logs and Failure Triage

All `just` tasks write full logs under `.artifacts/logs/<run-id>/`.

```bash
just logs-latest
POLYSYNTH_VERBOSE=1 just test
```
