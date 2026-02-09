#pragma once

#include "../../types.h"
#include <cmath>

namespace PolySynthCore {

class TPTIntegrator {
public:
  TPTIntegrator() = default;

  void Init(double sampleRate) {
    mSampleRate = sampleRate;
    Reset();
  }

  void Reset() { s = 0.0; }

  // Prepare the integrator for the current sample
  // cutoff: Cutoff frequency in Hz
  inline void Prepare(double cutoff) {
    double wd = kTwoPi * cutoff;
    double T = 1.0 / mSampleRate;
    double wa = (2.0 / T) * std::tan(wd * T / 2.0);
    g = wa * T / 2.0;
  }

  // Process the input sample and return the output
  // in: Input sample
  inline sample_t Process(sample_t in) {
    // y = g * in + s
    sample_t y = (sample_t)(g * in + s);

    // Update state
    // s = g * in + y -> This is from the TPT structure diagram, but simpler:
    // s_new = 2*y - s_old? No, typically s update is:
    // s[n+1] = 2*y[n] - s[n] (Trapezoidal rule form) check
    // Let's verify standard TPT update:
    // y = (g*x + s) / (1 + g) -> Wait, this is for 1-pole filter.
    // For pure integrator: y = g*x + s.
    // Update: s = g*x + y = 2*y - s?
    // Let's stick to the prompt description: "State Update: After each sample,
    // update s (or z1)." VaFilterDesign book: s[n+1] = 2*y[n] - s[n]

    // Wait, the prompt says instantaneous response y = g * x + s.
    // This is the output of the integrator block itself.
    // The update is indeed s = g * x + y.

    s = (sample_t)(g * in + y);
    return y;
  }

  // Get the instantaneous gain G
  inline double GetG() const { return g; }

  // Get the current state S
  inline double GetS() const { return s; }

  // Set the state S (needed for some ladder implementations)
  inline void SetS(double newState) { s = newState; }

private:
  double mSampleRate = 44100.0;
  double s = 0.0; // State
  double g = 0.0; // Instantaneous gain
};

} // namespace PolySynthCore
