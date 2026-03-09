# Platform Integration Rules

## Supported Platforms

| Platform | Location | Build System | Notes |
|---|---|---|---|
| **Desktop** (AU/VST3/Standalone) | `src/platform/desktop/` | CMake + Xcode | iPlug2 + Skia + Metal |
| **Pico** (RP2350 embedded) | `src/platform/pico/` | CMake + Pico SDK | In progress — see `plans/active/pico-port/` |

## Desktop (iPlug2)

- Parameters MUST be defined in `config.h` and registered in the `PolySynth` constructor.
- Use `OnParamChange()` to update `mState` from parameter changes.
- UI controls are wired in `PolySynth::OnLayout()` using iPlug2's `IControl` classes.
- Graphics backend is **Skia** (never NanoVG). Set via `IGRAPHICS_SKIA` in build configs.
- State sync flow: UI → `OnParamChange()` → `mState` → `ProcessBlock()` reads `mState`.

## Pico (RP2350 Embedded)

- Uses the Pico SDK (CMake-based). No iPlug2 dependency.
- Core DSP code (`src/core/`) is shared with desktop — the agnostic core principle applies.
- Platform adapter in `src/platform/pico/` bridges core Engine to hardware I/O.
- Memory constraints: 520 KB SRAM. All allocations must be static/stack. No STL containers that heap-allocate.
- See `plans/active/pico-port/00_overview.md` for the full initiative plan.

## Deleted Platforms

- **Web UI** (HTML/React/npm) — removed. The desktop plugin uses native iPlug2/Skia controls.
- **`PolySynth_DSP.h`** — deleted. DSP bridging is now handled directly in `PolySynth.h/cpp`.

## UI Initialization

- The UI must be initialized with the current DSP state on load.
- Avoid large data transfers in the UI thread; only sync changed parameters.
