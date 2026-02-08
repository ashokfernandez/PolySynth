#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Engine Demo (Poly/ADSR)..." << std::endl;

  PolySynthCore::Engine engine;
  double sr = 48000.0;
  engine.Init(sr);

  // Note On at 0.0
  // A4 (69)
  engine.OnNoteOn(69, 100);

  // Run for 1.0s (Attack 0.01 + Decay 0.1 -> Sustain)
  // Should hear pluck + sustain
  int sn1 = (int)(1.0 * sr);

  std::vector<float> outputBuffer;
  outputBuffer.reserve(sr * 2); // 2 sec total

  PolySynthCore::sample_t *outPtrs[2];
  PolySynthCore::sample_t left[512];
  PolySynthCore::sample_t right[512];
  outPtrs[0] = left;
  outPtrs[1] = right;

  auto processBlock = [&](int frames) {
    int current = 0;
    while (current < frames) {
      int todo = std::min(512, frames - current);
      engine.Process(nullptr, outPtrs, todo, 2);
      for (int i = 0; i < todo; i++)
        outputBuffer.push_back((float)left[i]);
      current += todo;
    }
  };

  processBlock(sn1);

  // Note Off at 1.0s
  engine.OnNoteOff(69);

  // Run for 1.0s (Release 0.2)
  processBlock(sn1);

  PolySynth::WavWriter::Write("demo_engine_poly.wav", outputBuffer, (int)sr);

  return 0;
}
