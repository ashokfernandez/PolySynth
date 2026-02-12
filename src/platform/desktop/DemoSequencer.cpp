#include "DemoSequencer.h"
#include "PolySynth_DSP.h"

namespace iplug {

void DemoSequencer::SetMode(Mode mode, double sampleRate, PolySynthDSP &dsp) {
  mMode = mode;
  if (mMode == Mode::Off) {
    dsp.Reset(sampleRate, 0);
    return;
  }
  mSampleCounter =
      static_cast<long long>(sampleRate * 0.25); // Trigger immediately
  mNoteIndex = -1;
  dsp.Reset(sampleRate, 0);
}

void DemoSequencer::Process(int nFrames, double sampleRate, PolySynthDSP &dsp) {
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
      }
      msg.MakeNoteOnMsg(currNote, 100, 0);
      dsp.ProcessMidiMsg(msg);
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
        }
      }
      for (int i = 0; i < 3; i++) {
        msg.MakeNoteOnMsg(chords[currIdx][i], 90, 0);
        dsp.ProcessMidiMsg(msg);
      }
    }
  }
}

} // namespace iplug
