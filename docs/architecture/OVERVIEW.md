# PolySynth Architecture Overview

## Layer Diagram

```
┌─────────────────────────────────────────────────────┐
│  iPlug2 Framework                                   │
│  (Plugin hosting, parameter system, MIDI, UI base)  │
├─────────────────────────────────────────────────────┤
│  Platform Layer     src/platform/desktop/            │
│  PolySynth.h/cpp    Parameter mapping, UI layout,   │
│                     preset I/O, demo sequencer       │
├─────────────────────────────────────────────────────┤
│  Core Layer         src/core/                        │
│  Engine             VoiceManager + FX chain          │
│  VoiceManager       Voice allocation & polyphony     │
│  Voice              Per-voice DSP (osc, filter, env) │
│  SynthState         Canonical parameter state        │
│  PresetManager      JSON serialization               │
├─────────────────────────────────────────────────────┤
│  SEA_DSP            libs/SEA_DSP/                    │
│  (Header-only DSP primitives: oscillators, filters,  │
│   envelopes, LFO, chorus, delay, limiter)            │
└─────────────────────────────────────────────────────┘
```

## Dependency Rules

1. **Core never imports platform.** Files in `src/core/` must not include anything from `src/platform/` or iPlug2 headers.
2. **Platform imports core.** `PolySynth.h` includes `SynthState.h` and (in DSP builds) `Engine.h` or `PolySynth_DSP.h`.
3. **Core imports SEA_DSP.** `VoiceManager.h` and `Engine.h` use SEA_DSP oscillators, filters, envelopes, and effects.
4. **SEA_DSP is fully independent.** No reverse dependencies on core or platform.
5. **UI headers are guarded.** `#if IPLUG_EDITOR` protects all IGraphics includes. DSP code paths must never touch UI headers.

## Build Configurations

| Configuration | IPLUG_DSP | IPLUG_EDITOR | WEB_API | Use Case |
|---|---|---|---|---|
| Desktop Plugin (AU/VST) | 1 | 1 | - | Full plugin with UI + audio |
| WAM Controller | - | 1 | 1 | Web UI only (sends params to processor) |
| WAM Processor | 1 | - | 1 | Web audio only (AudioWorklet) |
| Native Tests (CMake) | - | - | - | Unit tests against core headers |

The `#error` guard in `PolySynth.h` enforces that at least one of `IPLUG_DSP` or `IPLUG_EDITOR` is defined when compiling the plugin.

## File Map

### Core (`src/core/`)

| File | Responsibility |
|---|---|
| `types.h` | Type aliases (`sample_t`), constants (`kPi`, `kMaxBlockSize`), platform assertions |
| `SynthState.h` | Aggregate struct with all synth parameters and inline defaults. Single source of truth. |
| `VoiceManager.h` | `Voice` class (per-voice DSP chain) and `VoiceManager` (allocation, stealing, mixing) |
| `Engine.h` | Top-level DSP coordinator: VoiceManager + Chorus + Delay + Limiter |
| `PresetManager.h/cpp` | JSON serialize/deserialize of SynthState. File I/O helpers. |

### Platform (`src/platform/desktop/`)

| File | Responsibility |
|---|---|
| `PolySynth.h` | Plugin class declaration, parameter enum (`EParams`), control tags |
| `PolySynth.cpp` | Constructor (param init), `OnParamChange`, `SyncUIState`, UI layout, preset dispatch |
| `PolySynth_DSP.h` | DSP wrapper (VoiceManager + FX). Slated for removal — see proposal. |
| `DemoSequencer.h/cpp` | Auto-play demo modes (Mono, Poly, FX) via synthesized MIDI |
| `config.h` | iPlug2 build metadata (plugin name, dimensions, channel config) |

### UI Controls (`UI/Controls/`)

| File | Responsibility |
|---|---|
| `PolyKnob.h` | Custom circular knob control |
| `PolyToggleButton.h` | Binary toggle switch |
| `PolySection.h` | Decorative panel with title and drop shadow |
| `Envelope.h` | Display-only ADSR visualizer |
| `PolyTheme.h` | Centralized color palette, font sizes, layout constants |

### Tests (`tests/`)

| Directory | Contents |
|---|---|
| `tests/unit/` | Catch2 unit tests (one file per class/concern) |
| `tests/demos/` | WAV-generating programs for golden master comparison |
| `tests/utils/` | `WavWriter.h` helper for demo programs |
| `tests/Visual/` | Playwright + Storybook visual regression tests |

### Scripts (`scripts/`)

| Script | Purpose |
|---|---|
| `download_dependencies.sh` | Fetches iPlug2, Catch2, DaisySP |
| `golden_master.py` | Generates and verifies golden master audio files |
| `analyze_audio.py` | Frequency validation of demo audio |
| `build_gallery_wam.sh` | Builds component gallery WAM for Storybook |

## Data Flow: MIDI to Audio

```
MIDI Note On
  → PolySynthPlugin::ProcessMidiMsg()
    → Engine::OnNoteOn(note, velocity)
      → VoiceManager::OnNoteOn()
        → FindFreeVoice() or FindVoiceToSteal()
        → Voice::NoteOn() — set frequency, trigger envelopes

ProcessBlock (per audio buffer)
  → DemoSequencer::Process() — optional auto-play MIDI
  → Engine::UpdateState(mState) — apply current SynthState to DSP
  → Engine::Process() loop (per sample):
      → VoiceManager::Process() — sum all 8 voices
        → Voice::Process() per active voice:
            LFO → pitch/filter/amp modulation
            OscB → poly-mod sources (FM, PWM, filter)
            OscA → modulated by poly-mod
            Mixer (OscA + OscB)
            Filter (Classic/Ladder/Cascade, modulated cutoff)
            Amp Envelope × Velocity × Tremolo
      → Chorus::Process()
      → Delay::Process()
      → Limiter::Process()
  → Write to output buffers
```

## State Synchronization

```
UI Interaction → iPlug2 Parameter → OnParamChange() → mState field update
                                                          ↓
                                              Engine::UpdateState(mState)
                                                          ↓
                                              VoiceManager/Voice setters

Preset Load → PresetManager::Deserialize() → mState = loaded
                                                  ↓
                                          SyncUIState() → all params → UI
```

`mState` (a `SynthState` struct) is the single intermediary. The DSP thread reads it; `OnParamChange` writes it. iPlug2 serializes these calls so no explicit locking is needed.
