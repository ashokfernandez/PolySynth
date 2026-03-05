#include "DemoSequencer.h"
#include "PolySynth_DSP.h"

namespace iplug {

void DemoSequencer::SetMode(Mode mode, double sampleRate) {
  mMode = mode;
  if (mMode == Mode::Off) {
    return;
  }
  mSampleCounter =
      static_cast<long long>(sampleRate * 0.25); // Trigger immediately
  mNoteIndex = -1;
}

void DemoSequencer::Process(int nFrames, double sampleRate, PolySynthDSP &dsp) {
  ProcessImpl(nFrames, sampleRate, dsp, [](const IMidiMsg&) {});
}

} // namespace iplug
