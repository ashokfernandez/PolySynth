#include "audio_i2s_driver.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/time.h"

// Generated PIO header (created by pioasm during CMake build)
#include "i2s.pio.h"

namespace pico_audio {

// ── State ────────────────────────────────────────────────────────────────
static uint32_t s_buffer[kNumBuffers][kBufferWords];  // ~4 KB total
static volatile uint32_t s_active_buffer = 0;          // Which buffer DMA is sending
static AudioCallback s_callback = nullptr;
static volatile uint32_t s_underrun_count = 0;
static volatile uint32_t s_last_fill_time_us = 0;

static PIO s_pio = pio0;
static uint s_sm = 0;           // PIO state machine index
static int s_dma_channel_a = -1;
static int s_dma_channel_b = -1;

// ── Pin configuration (Waveshare Pico-Audio HAT) ────────────────────────
static constexpr uint PIN_DIN  = 26;  // I2S data
static constexpr uint PIN_BCK  = 27;  // Bit clock (side-set pin 0)
static constexpr uint PIN_LRCK = 28;  // Word select (side-set pin 1)

// ── DMA interrupt handler ───────────────────────────────────────────────
// Called when one DMA channel completes — triggers fill of the next buffer.
static void dma_irq_handler()
{
    uint32_t start_us = time_us_32();

    // Determine which channel completed and acknowledge
    uint32_t fill_idx;
    if (dma_channel_get_irq0_status(s_dma_channel_a)) {
        dma_channel_acknowledge_irq0(s_dma_channel_a);
        fill_idx = 0;
        // Reset channel A for next cycle — DMA does NOT auto-reset
        // read_addr or trans_count on chain trigger
        dma_channel_set_read_addr(s_dma_channel_a, s_buffer[0], false);
        dma_channel_set_trans_count(s_dma_channel_a, kBufferWords, false);
    } else if (dma_channel_get_irq0_status(s_dma_channel_b)) {
        dma_channel_acknowledge_irq0(s_dma_channel_b);
        fill_idx = 1;
        // Reset channel B for next cycle
        dma_channel_set_read_addr(s_dma_channel_b, s_buffer[1], false);
        dma_channel_set_trans_count(s_dma_channel_b, kBufferWords, false);
    } else {
        return;  // Spurious interrupt
    }

    // Fill the buffer that just finished sending (it's now free)
    if (s_callback) {
        s_callback(s_buffer[fill_idx], kBufferFrames);
    }

    s_last_fill_time_us = time_us_32() - start_us;
}

// ── Public API ──────────────────────────────────────────────────────────
bool Init(AudioCallback callback)
{
    if (!callback) return false;
    s_callback = callback;

    // ── PIO setup ────────────────────────────────────────────────────
    uint offset = pio_add_program(s_pio, &i2s_out_program);
    s_sm = pio_claim_unused_sm(s_pio, true);

    // Configure PIO state machine
    pio_sm_config cfg = i2s_out_program_get_default_config(offset);

    // DIN: output on GPIO26 (1 pin for 'out')
    sm_config_set_out_pins(&cfg, PIN_DIN, 1);
    pio_gpio_init(s_pio, PIN_DIN);
    pio_sm_set_consecutive_pindirs(s_pio, s_sm, PIN_DIN, 1, true);

    // BCK + LRCK: side-set on GPIO27-28 (2 pins)
    sm_config_set_sideset_pins(&cfg, PIN_BCK);
    pio_gpio_init(s_pio, PIN_BCK);
    pio_gpio_init(s_pio, PIN_LRCK);
    pio_sm_set_consecutive_pindirs(s_pio, s_sm, PIN_BCK, 2, true);

    // Autopull: shift out MSB-first, 32-bit threshold, shift right
    sm_config_set_out_shift(&cfg, false, true, 32);

    // Clock divider: PIO clock = system_clock / divider
    // Each I2S frame = 64 PIO cycles (32 per channel, 2 instructions per bit)
    // Target: 48000 frames/sec × 64 cycles/frame × 2 instructions/cycle = 6.144 MHz PIO rate
    float sys_clk = (float)clock_get_hz(clk_sys);
    float pio_clk = (float)kSampleRate * 64.0f * 2.0f;  // 2 PIO cycles per bit period
    sm_config_set_clkdiv(&cfg, sys_clk / pio_clk);

    pio_sm_init(s_pio, s_sm, offset + i2s_out_offset_entry_point, &cfg);

    // ── DMA setup (ping-pong double buffer) ─────────────────────────
    s_dma_channel_a = dma_claim_unused_channel(true);
    s_dma_channel_b = dma_claim_unused_channel(true);

    // Channel A config: sends buffer 0, chains to channel B
    dma_channel_config dma_cfg_a = dma_channel_get_default_config(s_dma_channel_a);
    channel_config_set_transfer_data_size(&dma_cfg_a, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg_a, true);
    channel_config_set_write_increment(&dma_cfg_a, false);
    channel_config_set_dreq(&dma_cfg_a, pio_get_dreq(s_pio, s_sm, true));
    channel_config_set_chain_to(&dma_cfg_a, s_dma_channel_b);

    dma_channel_configure(
        s_dma_channel_a,
        &dma_cfg_a,
        &s_pio->txf[s_sm],           // Write to PIO TX FIFO
        s_buffer[0],                   // Read from buffer 0
        kBufferWords,                  // Transfer count: 32-bit words (one per L/R sample)
        false                          // Don't start yet
    );

    // Channel B config: sends buffer 1, chains to channel A
    dma_channel_config dma_cfg_b = dma_channel_get_default_config(s_dma_channel_b);
    channel_config_set_transfer_data_size(&dma_cfg_b, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_cfg_b, true);
    channel_config_set_write_increment(&dma_cfg_b, false);
    channel_config_set_dreq(&dma_cfg_b, pio_get_dreq(s_pio, s_sm, true));
    channel_config_set_chain_to(&dma_cfg_b, s_dma_channel_a);

    dma_channel_configure(
        s_dma_channel_b,
        &dma_cfg_b,
        &s_pio->txf[s_sm],           // Write to PIO TX FIFO
        s_buffer[1],                   // Read from buffer 1
        kBufferWords,                  // Transfer count: 32-bit words (one per L/R sample)
        false                          // Don't start yet
    );

    // Enable DMA interrupt on both channels (for refill notification)
    dma_channel_set_irq0_enabled(s_dma_channel_a, true);
    dma_channel_set_irq0_enabled(s_dma_channel_b, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Pre-fill both buffers with silence
    for (uint32_t i = 0; i < kBufferWords; i++) {
        s_buffer[0][i] = 0;
        s_buffer[1][i] = 0;
    }

    return true;
}

void Start()
{
    // Pre-fill buffer 0 with audio data
    if (s_callback) {
        s_callback(s_buffer[0], kBufferFrames);
    }

    // Start PIO state machine
    pio_sm_set_enabled(s_pio, s_sm, true);

    // Start DMA channel A (will chain to B automatically)
    dma_channel_start(s_dma_channel_a);
}

void Stop()
{
    pio_sm_set_enabled(s_pio, s_sm, false);
    dma_channel_abort(s_dma_channel_a);
    dma_channel_abort(s_dma_channel_b);
}

uint32_t GetUnderrunCount()
{
    return s_underrun_count;
}

uint32_t GetLastFillTimeUs()
{
    return s_last_fill_time_us;
}

} // namespace pico_audio
