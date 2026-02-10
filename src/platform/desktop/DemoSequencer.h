#pragma once
#include "IPlug_include_in_plug_hdr.h"
#include <vector>

class PolySynthDSP;

namespace iplug {

class DemoSequencer {
public:
  enum class Mode { Off = 0, Mono, Poly, FX };

  void SetMode(Mode mode, double sampleRate, PolySynthDSP &dsp);
  void Process(int nFrames, double sampleRate, PolySynthDSP &dsp);
  Mode GetMode() const { return mMode; }

private:
  Mode mMode = Mode::Off;
  long long mSampleCounter = 0;
  int mNoteIndex = 0;
};

} // namespace iplug
