#include "../../src/core/VoiceManager.h"
#include "../utils/WavWriter.h"
#include <iostream>
#include <vector>

int main() {
  std::cout << "Rendering Poly Chords Demo..." << std::endl;

  PolySynth::VoiceManager vm;
  double sr = 48000.0;
  vm.Init(sr);

  // Duration: 2 seconds
  int duration = 2 * (int)sr;
  std::vector<float> buffer;
  buffer.reserve(duration);

  // 0.0s: C Major Chord (C4, E4, G4)
  vm.OnNoteOn(60, 100);
  vm.OnNoteOn(64, 100);
  vm.OnNoteOn(67, 100);

  for (int i = 0; i < sr; i++) { // 1 second hold
    buffer.push_back((float)vm.Process());
  }

  // 1.0s: Release
  vm.OnNoteOff(60);
  vm.OnNoteOff(64);
  vm.OnNoteOff(67);

  // Process Release tail
  for (int i = 0; i < sr; i++) {
    buffer.push_back((float)vm.Process());
  }

  PolySynth::WavWriter::Write("demo_poly_chords.wav", buffer, (int)sr);
  return 0;
}
