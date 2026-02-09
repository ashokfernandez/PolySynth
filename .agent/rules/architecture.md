# Architecture: "The Hub & Spokes"

Follow these strict architectural rules to ensure the codebase remains maintainable and portable to embedded hardware.

## 1. Core Principles
- **Agnotic Core ("The Hub")**: All DSP and business logic MUST reside in `src/core`.
- **Platform Adapters ("The Spokes")**: Platform-specific code (iPlug2, Daisy, etc.) MUST reside in `src/platform`.
- **Minimal Dependencies**: `src/core` should only depend on the C++17 Standard Library (strictly limited). No platform headers (Windows.h, Cocoa.h) allowed here.

## 2. Directory Structure
- `src/core/`: Agnostic DSP.
    - `Engine.h`: The main entry point for the DSP engine.
    - `VoiceManager.h`: Handles polyphony and note stealing.
    - `oscillator/`, `filter/`, `modulation/`: Modular DSP components.
- `src/platform/`:
    - `desktop/`: iPlug2 implementation for AU/VST3/Standalone.
    - `daisy/`: (Future) Hardware implementation.

## 3. Communication Rules
- **UI -> Audio Thread**: Use `std::atomic` for simple parameters. Use lock-free FIFOs for complex events (e.g., loading a sample).
- **Audio Thread -> UI**: Use lock-free FIFOs for visualizations (meters, scope).
- **Forbidden**: Never use mutexes, semaphores, or any blocking primitives in the audio thread.

## 4. Initialization
- All memory MUST be pre-allocated during the `Init()` phase.
- Objects should be fixed-size (e.g., `std::array` instead of `std::vector`) where possible to avoid runtime allocations.
