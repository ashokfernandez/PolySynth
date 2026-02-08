#include "../../src/core/Engine.h"
#include "../utils/WavWriter.h"
#include <cmath>
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Engine Demo (Sawtooth)..." << std::endl;

  PolySynth::Engine engine;
  double sr = 48000.0;
  engine.Init(sr);

  // Play Note A4
  engine.OnNoteOn(69, 100);

  // 2 Seconds
  int seconds = 2;
  int frames = (int)(sr * seconds);

  std::vector<float> outputBuffer;
  outputBuffer.resize(frames);

  // Process
  int blockSize = 512;
  int currentFrame = 0;

  PolySynth::sample_t *outPtrs[2];
  PolySynth::sample_t left[512];
  PolySynth::sample_t right[512];
  outPtrs[0] = left;
  outPtrs[1] = right;

  while (currentFrame < frames) {
    int todo = std::min(blockSize, frames - currentFrame);

    engine.Process(nullptr, outPtrs, todo, 2);

    for (int i = 0; i < todo; ++i) {
      outputBuffer[currentFrame + i] = (float)left[i];
    }

    currentFrame += todo;
  }

  // Use the new util
  PolySynth::WavWriter::Write("demo_engine_saw.wav", outputBuffer, (int)sr);

  return 0;
}
