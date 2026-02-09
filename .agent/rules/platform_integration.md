# Platform Integration & UI Binding

Rules for integrating the Agnotsic Core with platform-specific wrappers and the Web UI.

## 1. iPlug2 Integration (Desktop)
- Use `PolySynth_DSP.h` as the bridge between the Core `Engine` and iPlug2.
- Parameters MUST be defined in `config.h` and registered in the `PolySynth` constructor.
- Use `SetParamFromUI()` and `OnParamChange()` to handle state synchronization.

## 2. Web UI (WebView)
- The UI is built using HTML/Vanilla JS/CSS in `src/platform/desktop/resources/web/`.
- **Bidirectional Binding**: 
    - JS knobs send values to C++ via `SAMPLER_NAME.app.setParam(id, value)`.
    - C++ updates JS knobs via `SAMPLER_NAME.app.sendControlValueFromDelegate(id, value)`.
- Use `index.html` unique IDs for all interactive elements to facilitate testing.

## 3. UI Initialization
- The UI must be initialized with the current DSP state on load.
- Avoid large data transfers in the UI thread; only sync changed parameters.
