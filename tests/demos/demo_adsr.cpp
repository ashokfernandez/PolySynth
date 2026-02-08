#include "../../src/core/modulation/ADSREnvelope.h"
#include <fstream>
#include <iostream>
#include <vector>

int main() {
  std::cout << "Generating ADSR Demo CSV..." << std::endl;

  PolySynthCore::ADSREnvelope adsr;
  double sr = 48000.0;
  adsr.Init(sr);

  // A=0.1, D=0.2, S=0.5, R=0.5
  adsr.SetParams(0.1, 0.2, 0.5, 0.5);

  std::ofstream file("demo_adsr.csv");
  file << "Time,Level\n";

  // 1. Note On at 0.0s
  adsr.NoteOn();

  // Run for 1.0s (Sustain holds)
  for (int i = 0; i < 48000; i++) {
    double t = (double)i / sr;
    PolySynthCore::sample_t level = adsr.Process();
    file << t << "," << level << "\n";
  }

  // 2. Note Off at 1.0s
  adsr.NoteOff();

  // Run for 1.0s (Release)
  for (int i = 0; i < 48000; i++) {
    double t = 1.0 + (double)i / sr;
    PolySynthCore::sample_t level = adsr.Process();
    file << t << "," << level << "\n";
  }

  std::cout << "Wrote demo_adsr.csv" << std::endl;
  return 0;
}
