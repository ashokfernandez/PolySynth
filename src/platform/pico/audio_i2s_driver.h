#pragma once

#include <cstdint>

// Audio callback signature: fill the buffer with left-justified 32-bit stereo samples.
// buffer: pointer to uint32_t array of size (2 * num_frames) — [L0,R0,L1,R1,...]
//   Each sample: int16 audio data << 16 (upper 16 bits = data, lower 16 = zero padding)
// num_frames: number of stereo frames to fill
using AudioCallback = void (*)(uint32_t* buffer, uint32_t num_frames);

namespace pico_audio {

// Audio buffer configuration
static constexpr uint32_t kSampleRate     = 48000;
static constexpr uint32_t kBufferFrames   = 256;    // Stereo frames per DMA buffer
static constexpr uint32_t kBufferWords    = kBufferFrames * 2;  // Total 32-bit words per buffer (L+R per frame)
static constexpr uint32_t kNumBuffers     = 2;      // Double-buffering (ping-pong)

// Buffer latency: 256 frames / 48000 Hz = 5.33 ms per buffer
// Total latency: 2 × 5.33 ms = 10.67 ms (double-buffered)

// Initialize PIO I2S and DMA. Must be called once at startup.
// callback: function called from DMA ISR to fill the next buffer.
// dma_irq: which DMA IRQ to use (0 or 1). Use 1 when running on core 1
//          so the IRQ fires on that core.
// Returns true on success.
bool Init(AudioCallback callback, int dma_irq = 0);

// Start audio playback. Call after Init().
void Start();

// Stop audio playback.
void Stop();

// Get DMA underrun count (for diagnostics).
uint32_t GetUnderrunCount();

// Get current buffer fill time in microseconds (for CPU% calculation).
uint32_t GetLastFillTimeUs();

} // namespace pico_audio
