# Spec 005 — Envelope UI Component Review

**Status**: In-progress
**Files under review**: `UI/Controls/Envelope.h`, `UI/Controls/ADSRViewModel.h`
**Plugin-side counterpart**: `src/platform/desktop/PolySynth.h/.cpp` (voice tracking layer)

---

## 1. Purpose

The `Envelope` control is a read-only, hardware-accelerated visualisation of the
plugin's amplitude envelope.  It displays:

1. The static ADSR **shape** — a piecewise-linear path scaled proportionally to
   the current Attack / Decay / Sustain / Release parameter values.
2. Per-voice **animated cursors** — one vertical scan line per active voice,
   tracing the shape in real time and fading with the envelope's amplitude.

It is passive — no user interaction — and must stay completely ignorant of MIDI,
voice engine internals, or polyphony policy.

---

## 2. Component Boundaries

### 2.1 What `Envelope` owns

| Responsibility | Implementation |
|---|---|
| ADSR shape layout | `ADSRViewModel::CalculatePathData()` |
| Rendering (bg, outline, fill, glow, scan lines) | `Envelope::Draw()` |
| Per-voice animation timing | `VoiceTracker` array, `ComputeNoteX()` |
| Animation loop (keep repainting while active) | `IsDirty()` override |

### 2.2 What `Envelope` must NOT own

| Concern | Correct owner |
|---|---|
| MIDI note → voice mapping | `PolySynthPlugin` (`mNoteToUISlot[128]`) |
| Voice-stealing algorithm | `PolySynthPlugin::OnMidiMsgUI()` |
| Polyphony limit enforcement | `PolySynthPlugin` (`mUIMaxVoices`) |
| Note number values | Never enters `Envelope.h` |

---

## 3. Public Interface

### 3.1 Setup (called once at build time)

```cpp
// Pixel bounds of the control (set by iPlug2 layout engine).
void OnResize();  // override — recomputes ViewModel bounds

// ADSR times in seconds (A/D/R: 0.001–1.0 s), sustain as 0.0–1.0 fraction.
void SetADSR(float attackSec, float decaySec, float sustainFrac, float releaseSec);

// Accent colour and fill colour (currently hard-wired to AccentCyan in plugin).
void SetColors(const IColor& stroke, const IColor& fill);
```

### 3.2 Voice events (called on UI thread by plugin)

```cpp
// A voice has started.  slotIdx is 0-based, managed entirely by the plugin.
void OnVoiceOn(int slotIdx);   // 0 ≤ slotIdx < kMaxEnvelopeVoices (16)

// A voice entered its release phase.
void OnVoiceOff(int slotIdx);
```

`OnVoiceOn` records `noteOnTime = Clock::now()`.
`OnVoiceOff` records `noteOffTime = Clock::now()` and sets `released = true`.
Both are no-ops for out-of-range slot indices.

### 3.3 Limits

```cpp
static constexpr int kMaxEnvelopeVoices = 16;
```

The plugin layer must never call `OnVoiceOn` with more than 16 distinct active
slots simultaneously.  Slots above 15 are silently ignored.

---

## 4. Plugin-Side Voice Tracking (counterpart)

`PolySynthPlugin` owns all voice-engine concerns:

| Member | Description |
|---|---|
| `int mNoteToUISlot[128]` | MIDI note → slot index (-1 = unassigned) |
| `int mUISlotNote[16]` | slot → MIDI note (-1 = free or released) |
| `bool mUISlotReleased[16]` | true during release phase |
| `time_point mUISlotOnTime[16]` | wall-clock NoteOn time |
| `time_point mUISlotOffTime[16]` | wall-clock NoteOff time |
| `int mUIMaxVoices` | active polyphony limit, from `kParamPolyphonyLimit` |

**Stealing priority** (in `OnMidiMsgUI`):
1. Pass 1 — steal oldest-released slot (smallest `noteOffTime`) — these are already fading.
2. Pass 2 — steal oldest-sustaining slot (smallest `noteOnTime`) — matches DSP steal mode.

**Initialisation**: all tables are reset to -1/false in `OnUIOpen()`.
**Polyphony updates**: `mUIMaxVoices` is updated in `OnParamChangeUI` whenever
`kParamPolyphonyLimit` changes.

---

## 5. Visual Specification

### 5.1 Layout

```
┌────────────────────────────────────────────────────────────┐
│  (10 px padding on all sides)                              │
│                                                            │
│         ·                                                  │
│        /|                                                  │
│       / |                                                  │
│      /  \_______________                                   │
│     /                  \                                   │
│    /                    \                                  │
│___/                      \________________________________│
│  A       D               S              R                  │
└────────────────────────────────────────────────────────────┘
```

### 5.2 X-axis layout — proportional time scaling

All four ADSR segments share the display width proportionally.
Sustain is a fixed 20% slot regardless of individual A/D/R values.

```
T_total   = Attack + Decay + Release          (in seconds)
T_display = T_total × 1.25                   (25% headroom → 20% of display for sustain)
scale     = displayWidth / T_display

attackX       = left  + Attack  × scale
decayX        = attackX + Decay × scale
releaseStartX = decayX  + (T_total × 0.25) × scale   (always 20% of width)
releaseX      = right edge                             (always fills the control)
```

This ensures the envelope always spans the full width cleanly — no large gaps
when sustain or release times are extreme.

### 5.3 Y-axis

```
sustainY = bottom - (sustainFrac × height)
attackY  = top    (full amplitude peak)
```

Y is screen-space: top = highest amplitude, bottom = silence.

### 5.4 Render layers (painter's order)

| Layer | Style | Description |
|---|---|---|
| Background | `PolyTheme::ControlBG`, rounded | Panel fill |
| Border | `PolyTheme::SectionBorder`, 1 px | Panel outline |
| Faint outline | `Theme::Faint` (α ≈ 0.12), 2 px | Always-visible ghost shape |
| Reveal fill | `Theme::Fill` (α ≈ 0.08), clipped | Filled area left of furthest cursor |
| Reveal stroke | `Theme::Dark`, 3 px, clipped | Bright stroke within revealed area |
| Glow gradient | `Theme::Glow` (α ≈ 0.18), clipped | Linear gradient left→right, 80 px wide, trailing the furthest cursor |
| Scan lines | `Theme::Base` at Y-faded α, 2 px | Per-voice vertical line from curveY→bottom |

The **reveal clip region** is `IRECT(left, top, maxVoiceX, bottom)` where
`maxVoiceX` is the rightmost active cursor.  It is reset with
`PathClipRegion(IRECT())` after drawing.

### 5.5 Scan line (cursor) behaviour

Each active voice gets one vertical scan line:

```
scanX  = ComputeNoteX(voice)     // wall-clock elapsed → proportional X on path
curveY = ComputeEnvelopeYAtX(x)  // piecewise-linear Y on ADSR shape at scanX

alpha  = (bottom - curveY) / height   // 0 at silence, 1 at full amplitude
lineColor = Theme::Base with alpha applied
DrawLine(lineColor, scanX, curveY, scanX, bottom, 2 px)
```

**Fade semantics**:
- Attack peak → cursor is fully opaque (alpha ≈ 1).
- Sustain hold → cursor holds at sustain alpha (e.g. sustain=0.5 → alpha=0.5).
- Release ramp → cursor dims and vanishes as amplitude → 0 at the bottom.
- When `ComputeNoteX` returns -1 (release complete), the slot is marked
  `active = false` and removed from the next render pass.

### 5.6 Colour palette (HSL base: 182°, S=1.0, L=0.42)

| Theme token | Alpha | Role |
|---|---|---|
| `Base` | 255 (fully opaque) | Scan line head |
| `Dark` | 255 (L − 0.25) | Revealed stroke |
| `Faint` | ~31 (0.12 × 255) | Ghost outline |
| `Fill` | ~20 (0.08 × 255) | Revealed fill |
| `Glow` | ~46 (0.18 × 255) | Trailing glow gradient |

---

## 6. ADSRViewModel — Coordinate Calculator

`ADSRViewModel` is a pure-data helper (no iPlug2 rendering calls) that converts
parameter values into screen-space pixel coordinates.

### 6.1 Key output — `ADSRPathData`

```cpp
struct ADSRPathData {
  Point attackNode;        // peak of attack (attackX, top)
  Point attackControlPoint;// Bezier handle (unused in linear draw path)
  Point decaySustainNode;  // bottom of decay / start of sustain (decayX, sustainY)
  Point decayControlPoint; // Bezier handle (unused)
  Point releaseStartNode;  // end of sustain plateau (releaseStartX, sustainY)
  Point releaseNode;       // end of release (right, bottom)
  Point releaseControlPoint;// Bezier handle (unused)
};
```

Note: control points are computed and stored but the current `Envelope` draw
path uses `PathLineTo` (straight lines), ignoring the Bezier handles.  They are
reserved for a future curved-segment option.

### 6.2 Input units

| Parameter | Unit | Range |
|---|---|---|
| `a` (attack) | seconds | 0.001 – 1.0 |
| `d` (decay) | seconds | 0.001 – 1.0 |
| `s` (sustain) | fraction | 0.0 – 1.0 |
| `r` (release) | seconds | 0.001 – 1.0 |

Plugin converts from UI param units: `GetParam(kParamAttack)->Value() / 1000.f`
(ms → s), `GetParam(kParamSustain)->Value() / 100.f` (% → fraction).

---

## 7. Animation Loop Mechanism

iPlug2 calls `SetClean()` after every `Draw()`, so calling `SetDirty()` from
within `Draw()` has no effect.  Instead, `IsDirty()` is overridden to return
`true` while any voice slot is active:

```cpp
bool IsDirty() override {
    if (IControl::IsDirty()) return true;
    for (int i = 0; i < kMaxEnvelopeVoices; i++)
        if (mVoices[i].active) return true;
    return false;
}
```

This causes iPlug2 to repaint the control every frame while any note is playing,
with no reliance on a timer or `OnIdle`.

---

## 8. Known Issues / Review Flags

### 8.1 Unit test assertions — RESOLVED

Tests updated to match the proportional scaling formula.  All 103 test cases
pass (48762 assertions).

### 8.2 Test build compilation — RESOLVED

Mock stubs extended with `PathClipRegion`, `IPattern`, and virtual `IsDirty`.

### 8.3 Alpha values — RESOLVED

Tests updated to match the code's deliberately reduced alpha values
(Faint=0.12, Fill=0.08, Glow=0.18).

### 8.4 Bezier control points unused

`ADSRPathData` carries `attackControlPoint`, `decayControlPoint`, and
`releaseControlPoint` but `Envelope::Draw()` draws straight lines only.
Reserved for a future curved-segment rendering mode.

### 8.5 `mUISlotNote` not cleared when release animation ends

Slot table is never compacted after release animations complete.  Assessed as
cosmetic — voice stealing handles stale entries correctly.

### 8.6 Stolen voices `OnVoiceOff` — RESOLVED

`pEnv->OnVoiceOff(toSteal)` now called before clearing stolen slots, preventing
ghost cursors when multiple slots are stolen simultaneously.

### 8.7 Time arrays reset in `OnUIOpen` — RESOLVED

`mUISlotOnTime` and `mUISlotOffTime` now zero-filled on UI open.

---

## 9. Parameter Flow Summary

```
kParamAttack    → GetParam()->Value() / 1000.f → Envelope::SetADSR(a, ...)
kParamDecay     → GetParam()->Value() / 1000.f → Envelope::SetADSR(., d, .)
kParamSustain   → GetParam()->Value() / 100.f  → Envelope::SetADSR(., ., s, .)
kParamRelease   → GetParam()->Value() / 1000.f → Envelope::SetADSR(., ., ., r)
kParamPolyphonyLimit → Int()                   → mUIMaxVoices (plugin layer only)

MIDI NoteOn  → OnMidiMsgUI → steal if needed → OnVoiceOn(slot)
MIDI NoteOff → OnMidiMsgUI → mark released  → OnVoiceOff(slot)
DemoSequencer → dsp.ProcessMidiMsg + uiCallback → SendMidiMsgFromDelegate → OnMidiMsgUI
Keyboard widget → OnMessage(kMsgTagNoteOn) → OnMidiMsgUI (reuse path)
```
