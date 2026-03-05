#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include "PolySynth_DSP.h"
#include <vector>

namespace iplug {

class DemoSequencer {
public:
  enum class Mode { Off = 0, Mono, Poly, FX };

  void SetMode(Mode mode, double sampleRate);

  // No-callback overload (defined in .cpp)
  void Process(int nFrames, double sampleRate, PolySynthDSP &dsp);

  // Templated overload with callback (must be in header)
  template <typename Callback>
  void Process(int nFrames, double sampleRate, PolySynthDSP &dsp,
               Callback&& uiCallback) {
    ProcessImpl(nFrames, sampleRate, dsp, static_cast<Callback&&>(uiCallback));
  }

  Mode GetMode() const { return mMode; }

private:
  template <typename Callback>
  void ProcessImpl(int nFrames, double sampleRate, PolySynthDSP &dsp,
                   Callback&& uiCallback);

  Mode mMode = Mode::Off;
  long long mSampleCounter = 0;
  int mNoteIndex = 0;
};

// Template implementation must be in header
template <typename Callback>
void DemoSequencer::ProcessImpl(int nFrames, double sampleRate,
                                PolySynthDSP &dsp, Callback&& uiCallback) {
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
        dsp.ProcessMidiMsg(msg);
        uiCallback(msg);
      }
      msg.MakeNoteOnMsg(currNote, 100, 0);
      dsp.ProcessMidiMsg(msg);
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
          dsp.ProcessMidiMsg(msg);
          uiCallback(msg);
        }
      }
      for (int i = 0; i < 3; i++) {
        msg.MakeNoteOnMsg(chords[currIdx][i], 90, 0);
        dsp.ProcessMidiMsg(msg);
        uiCallback(msg);
      }
    }
  }
}

} // namespace iplug
