# Learnings: HTML Canvas → iPlug2/Skia UI Translation

Converting an HTML Canvas/CSS prototype to a hardware-accelerated iPlug2 control
involves a cluster of non-obvious API mismatches.  Each section below records a
specific mistake made and the fix.

---

## Path continuity: PathMoveTo mid-path breaks fills and multi-segment strokes

**Context**: Drawing an ADSR shape with a gap (e.g., omitting the sustain
plateau line by using `PathMoveTo` for the sustain segment).

**Mistake**: Used `PathMoveTo` for the sustain segment to "skip" it visually:
```cpp
g.PathLineTo(decaySustainNode.x, decaySustainNode.y);
g.PathMoveTo(releaseStartNode.x, releaseStartNode.y); // <-- breaks path
g.PathLineTo(releaseNode.x, drawRect.B);
```

**Fix**: Use `PathLineTo` for every segment.  If you want to visually suppress a
segment, use a separate draw call with a matching background colour or simply
accept the line is there:
```cpp
g.PathLineTo(decaySustainNode.x, decaySustainNode.y);
g.PathLineTo(releaseStartNode.x, releaseStartNode.y); // sustain plateau
g.PathLineTo(releaseNode.x, drawRect.B);
```

**Why**: `PathMoveTo` starts a new sub-path.  iPlug2/Skia treats each sub-path
independently, so `PathFill` produces disconnected polygons and `PathStroke` on
a single call won't join segments across the break.

---

## Clip region reset: use empty IRECT, not save/restore

**Context**: Clipping the reveal layer (fill + stroke) to the area left of the
furthest active voice cursor.

**Mistake**: Tried `ctx.save()` / `ctx.restore()` style thinking:
```cpp
// No equivalent of ctx.restore() — does not exist in this API.
```

**Fix**: Set the clip region with `PathClipRegion(IRECT(...))`, then reset it
with an empty IRECT:
```cpp
g.PathClipRegion(IRECT(mRECT.L, mRECT.T, maxVoiceX, mRECT.B));
// ... draw clipped layers ...
g.PathClipRegion(IRECT()); // reset — equivalent of ctx.restore()
```

**Why**: iPlug2's Skia backend implements clip reset as `canvas->restore()` when
it receives an empty IRECT.  An empty IRECT is the designated sentinel.

---

## Animation loop: override IsDirty(), never call SetDirty() inside Draw()

**Context**: Keeping the control animating every frame while a note is active.

**Mistake**: Called `SetDirty(false)` inside `Draw()` hoping to re-trigger the
next frame:
```cpp
void Draw(IGraphics& g) override {
    // ... render ...
    SetDirty(false); // DOES NOTHING — iPlug2 calls SetClean() after Draw()
}
```

**Fix**: Override `IsDirty()` to return `true` while any voice is active:
```cpp
bool IsDirty() override {
    if (IControl::IsDirty()) return true;
    for (int i = 0; i < kMaxEnvelopeVoices; i++)
        if (mVoices[i].active) return true;
    return false;
}
```

**Why**: iPlug2 calls `SetClean()` immediately after `Draw()` returns, so any
`SetDirty` inside `Draw` is wiped.  `IsDirty()` is queried before each frame;
returning `true` causes iPlug2 to schedule another repaint.  `OnIdle()` is not
reliably virtual in all iPlug2 builds, so this is the correct pattern.

---

## X-axis layout: normalise time segments, don't use raw seconds as proportions

**Context**: Mapping A/D/S/R parameter values to pixel X positions.

**Mistake**: Treated raw parameter values (0.0–1.0 seconds) directly as
proportions of the display width:
```cpp
float attackX = left + width * mAttack; // 0.1s attack → 10% of width
```
This caused huge sustain gaps when A+D+R didn't sum to 1.0, and the shape
looked broken at extreme settings.

**Fix**: Scale all three time segments proportionally to the total time, with a
fixed slot reserved for sustain:
```cpp
float T_total   = std::max(0.001f, mAttack + mDecay + mRelease);
float T_display = T_total * 1.25f;   // sustain = 20% of display
float scale     = width / T_display;

float attackX       = left  + mAttack              * scale;
float decayX        = attackX + mDecay             * scale;
float releaseStartX = decayX  + T_total * 0.25f    * scale; // always 20% of width
float releaseX      = right;                                 // always fills to edge
```

**Why**: With arbitrary param values the segments will never fill the display
using raw mapping.  Proportional scaling ensures A, D, and R always occupy their
fair share of the width relative to each other, and sustain is a fixed visual
slot so the shape always fills the control cleanly.

---

## Y-axis: sustain level is inverted in screen space

**Context**: Computing `sustainY` from a 0–1 fraction where 1.0 = full amplitude.

**Mistake**: `sustainY = top + sustainFrac * height`
This puts high sustain near the top and low sustain near the bottom — correct
mathematically but visually the ADSR peak and sustain swap positions.

**Fix**:
```cpp
float sustainY = bottom - (sustainFrac * height);
```

**Why**: Screen Y increases downward.  `bottom` is silence (0 amplitude), `top`
is full amplitude.  Subtracting moves up as level increases.

---

## IColor: alpha is first argument, range is 0–255 (not 0.0–1.0)

**Context**: Setting per-frame opacity on a scan line or gradient stop.

**Mistake**: Passed alpha as 0.0–1.0 float directly:
```cpp
IColor c(0.5f, 255, 100, 100); // wrong — first arg truncated to 0
```

**Fix**:
```cpp
float alpha = 0.5f;
IColor c(static_cast<int>(alpha * 255), r, g, b);
```

**Why**: `IColor(int a, int r, int g, int b)` — all arguments are 0–255
integers.  The constructor does not accept floats; passing 0.5f silently
truncates to 0 (fully transparent).

---

## Gradients: use IPattern::CreateLinearGradient with color stop pairs

**Context**: Trailing glow gradient behind the furthest active voice cursor.

**HTML equivalent**:
```js
const grad = ctx.createLinearGradient(x0, y0, x1, y1);
grad.addColorStop(0, 'transparent');
grad.addColorStop(1, 'rgba(0,200,200,0.4)');
ctx.fillStyle = grad;
```

**iPlug2 equivalent**:
```cpp
IPattern glow = IPattern::CreateLinearGradient(
    x0, y0, x1, y1,
    { {COLOR_TRANSPARENT, 0.0f},
      {IColor(46, 0, 180, 180), 1.0f} });
g.PathClear();
buildFillPath();
g.PathFill(glow);
```

**Why**: `IPattern` is iPlug2's gradient descriptor.  It accepts an initialiser
list of `{IColor, float position}` pairs.  Positions are 0.0–1.0.  The pattern
is passed to `PathFill` instead of a flat colour.

---

## Separation of concerns: voice tracking belongs in the plugin, not the UI control

**Context**: Showing per-voice cursors on the envelope with the correct polyphony
limit enforced.

**Mistake**: Put MIDI note → voice mapping, voice stealing, and polyphony limit
all inside `Envelope.h`:
```cpp
void OnNoteOn(int midiNote) {
    // voice stealing, mMaxVoices check, mNotes[midiNote] assignment...
}
```

**Fix**: The `Envelope` control only accepts slot indices (0..N-1):
```cpp
void OnVoiceOn(int slotIdx);   // plugin decided the slot
void OnVoiceOff(int slotIdx);
```

The plugin (`OnMidiMsgUI`) owns:
- `mNoteToUISlot[128]` — MIDI note → slot index
- `mUISlotNote[16]` — slot → MIDI note
- voice-stealing algorithm (prefer oldest-released, then oldest-sustaining)
- `mUIMaxVoices` (from `kParamPolyphonyLimit`)

**Why**: A UI control has no business knowing about MIDI note numbers, DSP voice
allocation policy, or polyphony limits.  When voice logic lives in the control,
changing the stealing policy requires touching rendering code.  Clean separation
lets both evolve independently.

---

## DemoSequencer bypass: demo notes skip the plugin's MIDI hook

**Context**: Demo sequencer notes not appearing in the envelope animation.

**Mistake**: `DemoSequencer::Process()` called `dsp.ProcessMidiMsg(msg)` directly,
bypassing `PolySynthPlugin::ProcessMidiMsg()` where `SendMidiMsgFromDelegate`
was hooked.  Result: demo notes reached the DSP but never reached the UI.

**Fix**: Add a `uiCallback` parameter to `DemoSequencer::Process()` and call it
for every MIDI message:
```cpp
void Process(int nFrames, double sampleRate, PolySynthDSP& dsp,
             const std::function<void(const IMidiMsg&)>& uiCallback = nullptr);

// Inside Process:
dsp.ProcessMidiMsg(msg);
if (uiCallback) uiCallback(msg);
```

In `ProcessBlock`:
```cpp
mDemoSequencer.Process(nFrames, GetSampleRate(), mDSP,
    [this](const IMidiMsg& msg) { SendMidiMsgFromDelegate(msg); });
```

**Why**: `SendMidiMsgFromDelegate` queues MIDI to the UI thread safely.  Any
MIDI path that bypasses the plugin's `ProcessMidiMsg` must explicitly invoke
this callback, or the UI never sees the notes.

---

## mUISlotNote is not cleared when release animation ends

**Context**: Slot table compaction after a voice's release animation completes.

**Known issue** (not yet fixed): When `ComputeNoteX` returns -1.0f (release
complete), `Envelope` marks `mVoices[slot].active = false` but
`PolySynthPlugin::mUISlotNote[slot]` is never cleared.  Over many note cycles,
all 16 slots fill with stale entries.  Voice stealing still works (it prefers
released slots), but the table is never naturally compacted.

**Recommended fix**: In `OnMidiMsgUI`, after counting occupied slots, first
expire any whose release has elapsed:
```cpp
float r = GetParam(kParamRelease)->Value() / 1000.f;
auto now = Clock::now();
for (int i = 0; i < kUIMaxVoices; i++) {
    if (mUISlotNote[i] >= 0 && mUISlotReleased[i]) {
        float elapsed = std::chrono::duration<float>(now - mUISlotOffTime[i]).count();
        if (elapsed >= r) mUISlotNote[i] = -1;
    }
}
```
