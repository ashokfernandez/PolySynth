# UI Controls Library

This directory contains custom UI controls used by PolySynth.

The ComponentGallery/Storybook surface is intentionally limited to controls used in the current PolySynth UI:
- Envelope
- PolyKnob
- PolySection
- PolyToggleButton
- SectionFrame
- LCDPanel
- PresetSaveButton

Legacy iPlug demo controls (plain knob/fader/button/switch variants) are intentionally excluded from the gallery.

## Envelope

ADSR envelope visualizer (`Envelope.h`).

```cpp
auto* envelope = new Envelope(IRECT(x, y, x+400, y+150));
envelope->SetADSR(0.2f, 0.4f, 0.6f, 0.5f);
pGraphics->AttachControl(envelope);
```

## PolyKnob

Themed rotary knob with accent arc, label, and value readout (`PolyKnob.h`).

```cpp
auto* knob = new PolyKnob(IRECT(x, y, x+80, y+116), paramIdx, "Cutoff");
knob->SetAccent(PolyTheme::AccentCyan);
pGraphics->AttachControl(knob);
```

## PolySection

Cached section panel with title, sheen, and border treatment (`PolySection.h`).

```cpp
pGraphics->AttachControl(new PolySection(
    IRECT(x, y, x+300, y+200), "FILTER"));
```

## PolyToggleButton

Latching toggle button with themed active/inactive states (`PolyToggleButton.h`).

```cpp
pGraphics->AttachControl(new PolyToggleButton(
    IRECT(x, y, x+80, y+28), paramIdx, "MONO"));
```

## SectionFrame

Simple framed grouping control (`SectionFrame.h`).

```cpp
pGraphics->AttachControl(new SectionFrame(
    IRECT(x, y, x+380, y+220),
    "MOD MATRIX",
    PolyTheme::SectionBorder,
    PolyTheme::TextDark,
    PolyTheme::ControlBG));
```

## LCDPanel

LCD-style display panel treatment (`LCDPanel.h`).

```cpp
const IRECT panel(x, y, x+320, y+72);
pGraphics->AttachControl(new LCDPanel(panel, PolyTheme::LCDBackground));
```

## PresetSaveButton

Stateful save button showing dirty/clean preset state (`PresetSaveButton.h`).

```cpp
auto* save = new PresetSaveButton(
    IRECT(x, y, x+120, y+34),
    [](IControl* caller) { (void)caller; });
save->SetHasUnsavedChanges(true);
pGraphics->AttachControl(save);
```

## Visual Validation

```bash
just gallery-build
just gallery-view
just gallery-test
```
