#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace PolySynth {

struct WavHeader {
  char riff[4] = {'R', 'I', 'F', 'F'};
  uint32_t overallSize;
  char wave[4] = {'W', 'A', 'V', 'E'};
  char fmtChunkMarker[4] = {'f', 'm', 't', ' '};
  uint32_t lengthOfFmt = 16;
  uint16_t formatType = 1; // PCM
  uint16_t channels;
  uint32_t sampleRate;
  uint32_t byteRate;
  uint16_t blockAlign;
  uint16_t bitsPerSample;
  char dataChunkHeader[4] = {'d', 'a', 't', 'a'};
  uint32_t dataSize;
};

class WavWriter {
public:
  static void Write(const std::string &filename,
                    const std::vector<float> &buffer, int sampleRate,
                    int channels = 1) {
    std::ofstream file(filename, std::ios::binary);

    WavHeader header;
    header.channels = (uint16_t)channels;
    header.sampleRate = (uint32_t)sampleRate;
    header.bitsPerSample = 16;
    header.blockAlign = header.channels * header.bitsPerSample / 8;
    header.byteRate = header.sampleRate * header.blockAlign;
    header.dataSize = (uint32_t)(buffer.size() * (header.bitsPerSample / 8));
    header.overallSize = header.dataSize + 36;

    file.write((const char *)&header, sizeof(WavHeader));

    for (float sample : buffer) {
      // Soft clip
      sample = std::max(-1.0f, std::min(1.0f, sample));

      int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
      file.write((const char *)&pcm, sizeof(int16_t));
    }

    std::cout << "Generated: " << filename << std::endl;
  }
};

} // namespace PolySynth
