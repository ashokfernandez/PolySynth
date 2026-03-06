#include "DemoSequencer.h"

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

void DemoSequencer::Process(int nFrames, double sampleRate,
                            std::function<void(const IMidiMsg&)> midiCallback) {
  ProcessImpl(nFrames, sampleRate, midiCallback, [](const IMidiMsg&) {});
}

} // namespace iplug
