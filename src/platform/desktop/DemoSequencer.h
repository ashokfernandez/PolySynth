#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include <vector>

namespace iplug {

class DemoSequencer {
public:
  enum class Mode { Off = 0, Mono, Poly, FX };

  void SetMode(Mode mode, double sampleRate);

  // No-callback overload (defined in .cpp)
  void Process(int nFrames, double sampleRate,
               std::function<void(const IMidiMsg&)> midiCallback);

  // Templated overload with separate MIDI and UI callbacks
  template <typename MidiCb, typename UICb>
  void Process(int nFrames, double sampleRate, MidiCb&& midiCallback,
               UICb&& uiCallback) {
    ProcessImpl(nFrames, sampleRate, static_cast<MidiCb&&>(midiCallback),
                static_cast<UICb&&>(uiCallback));
  }

  Mode GetMode() const { return mMode; }

private:
  template <typename MidiCb, typename UICb>
  void ProcessImpl(int nFrames, double sampleRate, MidiCb&& midiCallback,
                   UICb&& uiCallback);

  Mode mMode = Mode::Off;
  long long mSampleCounter = 0;
  int mNoteIndex = 0;
};

// Template implementation must be in header
template <typename MidiCb, typename UICb>
void DemoSequencer::ProcessImpl(int nFrames, double sampleRate,
                                MidiCb&& midiCallback, UICb&& uiCallback) {
  if (mMode == Mode::Off || sampleRate <= 0.0)
    return;

  long long samplesPerStep = static_cast<long long>(sampleRate * 0.25);
  if (samplesPerStep <= 0)
    return;
  mSampleCounter += nFrames;

  if (mSampleCounter >= samplesPerStep) {
    mSampleCounter = 0;
    mNoteIndex++;

    IMidiMsg msg;
    if (mMode == Mode::Mono) {
      const int seq[] = {48, 55, 60, 67};
      int prevNote = seq[(mNoteIndex - 1 + 4) % 4];
      int currNote = seq[mNoteIndex % 4];

      if (mNoteIndex > 0) {
        msg.MakeNoteOffMsg(prevNote, 0);
        midiCallback(msg);
        uiCallback(msg);
      }
      msg.MakeNoteOnMsg(currNote, 100, 0);
      midiCallback(msg);
      uiCallback(msg);
    } else if (mMode == Mode::Poly || mMode == Mode::FX) {
      const int chords[4][3] = {
          {60, 64, 67}, // C
          {60, 65, 69}, // F
          {59, 62, 67}, // G
          {60, 64, 67}  // C
      };

      int prevIdx = (mNoteIndex - 1 + 4) % 4;
      int currIdx = mNoteIndex % 4;

      if (mNoteIndex > 0) {
        for (int i = 0; i < 3; i++) {
          msg.MakeNoteOffMsg(chords[prevIdx][i], 0);
          midiCallback(msg);
          uiCallback(msg);
        }
      }
      for (int i = 0; i < 3; i++) {
        msg.MakeNoteOnMsg(chords[currIdx][i], 90, 0);
        midiCallback(msg);
        uiCallback(msg);
      }
    }
  }
}

} // namespace iplug
