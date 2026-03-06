#pragma once
// Shared test helpers for Voice and Engine unit tests.
// Used by Test_Voice.cpp, Test_FilterModels.cpp, Test_ParameterBoundaries.cpp, etc.

#include "Voice.h"
#include <cmath>

namespace TestHelpers {

inline double processNSamples(PolySynthCore::Voice& voice, int n) {
    double last = 0.0;
    for (int i = 0; i < n; ++i) {
        last = voice.Process();
    }
    return last;
}

inline double processMaxAbs(PolySynthCore::Voice& voice, int n) {
    double maxAbs = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        double a = std::abs(s);
        if (a > maxAbs) maxAbs = a;
    }
    return maxAbs;
}

inline bool processAllFinite(PolySynthCore::Voice& voice, int n) {
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        if (!std::isfinite(s)) return false;
    }
    return true;
}

inline double processRMS(PolySynthCore::Voice& voice, int n) {
    double sumSq = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = voice.Process();
        sumSq += s * s;
    }
    return std::sqrt(sumSq / n);
}

inline bool isValidAudio(double value, double maxMagnitude = 10.0) {
    return std::isfinite(value) && std::abs(value) <= maxMagnitude;
}

} // namespace TestHelpers
