# Learnings: iPlug2 Audio↔UI Threading

---

## SendMidiMsgFromDelegate is the only safe way to send MIDI to the UI thread

**Context**: Any MIDI generated on the audio thread (DSP) that needs to drive
UI animation.

**Mistake**: Calling UI methods directly from `ProcessBlock` or from DSP helper
classes called within `ProcessBlock`.

**Fix**: Call `SendMidiMsgFromDelegate(msg)` from `ProcessMidiMsg` (which runs
on the audio thread).  iPlug2 queues the message and delivers it to
`OnMidiMsgUI` on the UI thread:
```cpp
// Audio thread
void PolySynthPlugin::ProcessMidiMsg(const IMidiMsg& msg) {
    mDSP.ProcessMidiMsg(msg);
    SendMidiMsgFromDelegate(msg); // safely queued to UI thread
}

// UI thread
void PolySynthPlugin::OnMidiMsgUI(const IMidiMsg& msg) {
    // safe to touch UI controls here
}
```

**Why**: The audio thread and UI thread run concurrently.  Touching UI objects
from the audio thread causes data races and crashes.  `SendMidiMsgFromDelegate`
is the designated iPlug2 lock-free crossing point.

---

## OnMessage for keyboard widget notes must also route through voice tracking

**Context**: The virtual keyboard widget sends notes via `OnMessage(kMsgTagNoteOn, ...)`,
not via `ProcessMidiMsg`, so they can bypass any routing added in `ProcessMidiMsg`.

**Mistake**: Updated `OnMidiMsgUI` for demo sequencer notes but forgot the
`OnMessage` path, leaving `pEnv->OnNoteOn(ctrlTag)` calls pointing to the old
`Envelope` API.

**Fix**: Route keyboard `OnMessage` notes through `OnMidiMsgUI` by constructing
an `IMidiMsg` and calling the method directly:
```cpp
else if (msgTag == kMsgTagNoteOn) {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(ctrlTag, 100, 0);
    mDSP.ProcessMidiMsg(msg);
    OnMidiMsgUI(msg); // reuse same voice-tracking path
    return true;
}
```

**Why**: `OnMidiMsgUI` is called on the UI thread; so is `OnMessage` in the WDGI
build.  Constructing an `IMidiMsg` and calling `OnMidiMsgUI` directly is safe
and ensures all note sources share the same voice-slot tracking logic.

---

## DemoSequencer runs on the audio thread — it cannot touch the UI

**Context**: Adding per-note animation driven by demo sequencer playback.

**Known trap**: `DemoSequencer::Process()` is called from `ProcessBlock` (audio
thread).  Any direct UI call from inside `Process()` is unsafe.

**Pattern**: Pass a `std::function<void(const IMidiMsg&)>` callback to
`Process()`.  The plugin provides a lambda that calls `SendMidiMsgFromDelegate`:
```cpp
mDemoSequencer.Process(nFrames, GetSampleRate(), mDSP,
    [this](const IMidiMsg& msg) { SendMidiMsgFromDelegate(msg); });
```

The callback is invoked on the audio thread but only enqueues the message — no
UI access.  The UI receives it later via `OnMidiMsgUI`.
