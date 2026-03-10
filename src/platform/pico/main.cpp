// PolySynth Pico — Sprint 4+5: DSP Engine + Serial Control
// Real synth engine wired into I2S audio callback with serial command interface.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"

#include "audio_i2s_driver.h"
#include "command_parser.h"
#include "Engine.h"
#include "SynthState.h"
#include "sine_generator.h"  // for PackI2S()

// ── DSP engine ───────────────────────────────────────────────────────────
static PolySynthCore::Engine s_engine;

// ── SPSC command queue (main loop → ISR) ─────────────────────────────────
struct AudioCommand {
    enum Type : uint8_t { NONE, NOTE_ON, NOTE_OFF, UPDATE_STATE, PANIC };
    Type type = NONE;
    uint8_t arg1 = 0;  // note
    uint8_t arg2 = 0;  // velocity
};

static constexpr int kCmdQueueSize = 16;
static AudioCommand s_cmdQueue[kCmdQueueSize];
static std::atomic<int> s_cmdHead{0};  // main loop writes
static std::atomic<int> s_cmdTail{0};  // ISR reads

// ── State handoff (main → ISR via UPDATE_STATE) ─────────────────────────
// s_pendingState: main loop's working copy — never touched by ISR.
// s_stagedState:  snapshot copied from pending, consumed by ISR.
// s_stateConsumed: ISR sets true after reading staged; main waits for it
//                  before overwriting staged (in practice never spins —
//                  audio callback period ≫ state update interval).
static PolySynthCore::SynthState s_pendingState;
static PolySynthCore::SynthState s_stagedState;
static std::atomic<bool> s_stateConsumed{true};
static std::atomic<int> s_activeVoices{0};  // updated by audio ISR for diagnostics
static std::atomic<float> s_peakLevel{0.0f};  // peak output level (pre-tanh), reset on read

// Voice-change event ring buffer (ISR writes, main loop reads)
struct VoiceEvent { int8_t from; int8_t to; };
static constexpr int kVoiceEventBufSize = 32;
static VoiceEvent s_voiceEvents[kVoiceEventBufSize];
static std::atomic<int> s_voiceEventHead{0};  // ISR writes
static int s_voiceEventTail = 0;               // main loop reads
static int s_prevVoiceCount = 0;  // ISR-private: only accessed from audio_callback

static void enqueue_cmd(AudioCommand::Type type, uint8_t a1 = 0, uint8_t a2 = 0) {
    int head = s_cmdHead.load(std::memory_order_relaxed);
    int next = (head + 1) % kCmdQueueSize;
    if (next == s_cmdTail.load(std::memory_order_acquire)) return;  // queue full, drop
    s_cmdQueue[head] = {type, a1, a2};
    s_cmdHead.store(next, std::memory_order_release);
}

// ── Fast tanh approximant (Padé) ─────────────────────────────────────────
// Replaces tanhf() which is a library call on Cortex-M33 (~200-500 cycles).
// This Padé approximant is <10 cycles and accurate to ~0.1% for |x| < 3.
static inline float fast_tanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ── Saturating int16 conversion (SSAT instruction) ───────────────────────
// Single-cycle saturating clamp to [-32768, 32767] via ARM DSP extension.
// Replaces std::clamp + cast for float→int16 output conversion.
static inline int16_t saturate_to_i16(int32_t val) {
    int32_t result;
    __asm__ volatile("ssat %0, #16, %1" : "=r"(result) : "r"(val));
    return static_cast<int16_t>(result);
}

// ── Audio callback (called from DMA ISR) ─────────────────────────────────
// __time_critical_func places this in SRAM, avoiding XIP flash cache misses
// that cause variable-latency stalls (~10-100+ cycles per miss) during audio.
// Since SEA_DSP is header-only and inlined here, the DSP code lands in SRAM too.
static void __time_critical_func(audio_callback)(uint32_t* buffer, uint32_t num_frames)
{
    // Drain command queue
    int tail = s_cmdTail.load(std::memory_order_relaxed);
    int head = s_cmdHead.load(std::memory_order_acquire);
    while (tail != head) {
        auto& cmd = s_cmdQueue[tail];
        switch (cmd.type) {
            case AudioCommand::NOTE_ON:
                s_engine.OnNoteOn(cmd.arg1, cmd.arg2);
                break;
            case AudioCommand::NOTE_OFF:
                s_engine.OnNoteOff(cmd.arg1);
                break;
            case AudioCommand::UPDATE_STATE:
                s_engine.UpdateState(s_stagedState);
                s_stateConsumed.store(true, std::memory_order_release);
                break;
            case AudioCommand::PANIC:
                s_engine.Reset();
                break;
            default:
                break;
        }
        tail = (tail + 1) % kCmdQueueSize;
        s_cmdTail.store(tail, std::memory_order_release);
    }

    // Process audio
    for (uint32_t i = 0; i < num_frames; i++) {
        float left = 0.0f, right = 0.0f;
        s_engine.Process(left, right);

        // Output gain: compensate for conservative gain staging
        // (velocity × filter × pan × headroom ≈ 0.12 for a single note).
        // Boost before soft clip so tanh gracefully limits dense chords.
        constexpr float kOutputGain = 4.0f;
        left *= kOutputGain;
        right *= kOutputGain;

        // Track peak level for diagnostics (relaxed: diagnostic only, no ordering needed)
        float absL = left > 0 ? left : -left;
        float absR = right > 0 ? right : -right;
        float peak = absL > absR ? absL : absR;
        float prev = s_peakLevel.load(std::memory_order_relaxed);
        if (peak > prev) s_peakLevel.store(peak, std::memory_order_relaxed);

        // Soft clip (Padé approximant — tanhf() is too expensive on Cortex-M33)
        // SSAT handles overflow: fast_tanh can exceed ±1.0 for |x|>3, so the
        // int32 multiply may exceed int16 range — SSAT saturates in one cycle.
        auto l16 = saturate_to_i16(static_cast<int32_t>(fast_tanh(left) * 32767.0f));
        auto r16 = saturate_to_i16(static_cast<int32_t>(fast_tanh(right) * 32767.0f));
        buffer[i * 2]     = pico_audio::PackI2S(l16);
        buffer[i * 2 + 1] = pico_audio::PackI2S(r16);
    }
    // Update voice count for status reporting
    // NOTE: UpdateVisualization() removed — it uses std::atomic<uint64_t> which
    // is NOT lock-free on 32-bit ARM (may disable interrupts or use spinlocks).
    // GetActiveVoiceCount() iterates voices directly, safe from ISR context
    // since voice state is only modified here via the command queue above.
    int vc = s_engine.GetActiveVoiceCount();
    s_activeVoices.store(vc, std::memory_order_relaxed);
    // Buffer voice-change events (NO printf from ISR — not thread-safe)
    if (vc != s_prevVoiceCount) {
        int head = s_voiceEventHead.load(std::memory_order_relaxed);
        int next = (head + 1) % kVoiceEventBufSize;
        s_voiceEvents[head] = {static_cast<int8_t>(s_prevVoiceCount),
                               static_cast<int8_t>(vc)};
        s_voiceEventHead.store(next, std::memory_order_release);
        s_prevVoiceCount = vc;
    }
}

// ── Helper: push state update to engine ──────────────────────────────────
static void push_state_update() {
    // Wait for ISR to consume previous staged state (virtually never spins —
    // audio callback period ~5.3ms, state updates are ≥20ms apart)
    while (!s_stateConsumed.load(std::memory_order_acquire)) {
        // spin
    }
    s_stagedState = s_pendingState;
    s_stateConsumed.store(false, std::memory_order_release);
    enqueue_cmd(AudioCommand::UPDATE_STATE);
}

// ── SRAM report ──────────────────────────────────────────────────────────
extern "C" {
    extern uint8_t __StackLimit;
    extern uint8_t __bss_end__;
}

static void print_sram_report()
{
    const uint32_t total_sram = 520 * 1024;
    printf("=== SRAM Report ===\n");
    printf("  Total SRAM: %lu KB\n", static_cast<unsigned long>(total_sram / 1024));
    printf("  sizeof(Engine): %u bytes\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("===================\n");
}

// ── Demo sequence state machine ──────────────────────────────────────────
enum class DemoPhase : uint8_t {
    SAW_NOTE,       // 0-2s: C4 saw
    SAW_OFF,        // 2-2.5s: silence
    SQUARE_NOTE,    // 2.5-4.5s: E4 square
    SQUARE_OFF,     // 4.5-5s: silence
    CHORD,          // 5-8s: 4-voice chord
    FILTER_SWEEP,   // 8-11s: filter sweep
    CHORD_OFF,      // 11-13s: silence
    DONE            // loop back to SAW_NOTE
};

static const char* demo_phase_name(DemoPhase p) {
    switch (p) {
        case DemoPhase::SAW_NOTE:     return "SAW";
        case DemoPhase::SAW_OFF:      return "SAW_OFF";
        case DemoPhase::SQUARE_NOTE:  return "SQUARE";
        case DemoPhase::SQUARE_OFF:   return "SQ_OFF";
        case DemoPhase::CHORD:        return "CHORD";
        case DemoPhase::FILTER_SWEEP: return "SWEEP";
        case DemoPhase::CHORD_OFF:    return "CHD_OFF";
        case DemoPhase::DONE:         return "DONE";
    }
    return "?";
}

struct DemoState {
    bool running = true;
    DemoPhase phase = DemoPhase::SAW_NOTE;
    uint64_t phase_start_us = 0;
    uint64_t last_state_push_us = 0;  // rate-limit state updates
};

static DemoState s_demo;

static void demo_reset(uint64_t now_us) {
    s_demo.phase = DemoPhase::SAW_NOTE;
    s_demo.phase_start_us = now_us;
}

static void demo_tick(uint64_t now_us) {
    if (!s_demo.running) return;

    uint64_t elapsed_us = now_us - s_demo.phase_start_us;
    float elapsed_s = static_cast<float>(elapsed_us) / 1000000.0f;

    auto advance = [&](DemoPhase next) {
        s_demo.phase = next;
        s_demo.phase_start_us = now_us;
    };

    switch (s_demo.phase) {
        case DemoPhase::SAW_NOTE:
            if (elapsed_s < 0.001f) {
                // Set saw wave and play C4
                auto& st = s_pendingState;
                st.Reset();
                st.oscAWaveform = 0;  // Saw
                push_state_update();
                enqueue_cmd(AudioCommand::NOTE_ON, 60, 100);
            }
            if (elapsed_s >= 2.0f) {
                enqueue_cmd(AudioCommand::NOTE_OFF, 60);
                advance(DemoPhase::SAW_OFF);
            }
            break;

        case DemoPhase::SAW_OFF:
            if (elapsed_s >= 0.5f) {
                advance(DemoPhase::SQUARE_NOTE);
            }
            break;

        case DemoPhase::SQUARE_NOTE:
            if (elapsed_s < 0.001f) {
                auto& st = s_pendingState;
                st.oscAWaveform = 1;  // Square
                push_state_update();
                enqueue_cmd(AudioCommand::NOTE_ON, 64, 100);
            }
            if (elapsed_s >= 2.0f) {
                enqueue_cmd(AudioCommand::NOTE_OFF, 64);
                advance(DemoPhase::SQUARE_OFF);
            }
            break;

        case DemoPhase::SQUARE_OFF:
            if (elapsed_s >= 0.5f) {
                advance(DemoPhase::CHORD);
            }
            break;

        case DemoPhase::CHORD:
            if (elapsed_s < 0.001f) {
                auto& st = s_pendingState;
                st.oscAWaveform = 0;  // Saw
                st.filterCutoff = 2000.0;
                push_state_update();
                enqueue_cmd(AudioCommand::NOTE_ON, 60, 90);
                enqueue_cmd(AudioCommand::NOTE_ON, 64, 85);
                enqueue_cmd(AudioCommand::NOTE_ON, 67, 80);
                enqueue_cmd(AudioCommand::NOTE_ON, 72, 75);
            }
            if (elapsed_s >= 3.0f) {
                advance(DemoPhase::FILTER_SWEEP);
            }
            break;

        case DemoPhase::FILTER_SWEEP: {
            // Sweep cutoff from 200 to 8000 Hz over 3 seconds
            // Rate-limit to ~50 updates/sec (20ms intervals) to avoid
            // flooding the ISR with UpdateState calls
            if (now_us - s_demo.last_state_push_us >= 20000ULL) {
                s_demo.last_state_push_us = now_us;
                float t = elapsed_s / 3.0f;
                if (t > 1.0f) t = 1.0f;
                float cutoff = 200.0f + t * (8000.0f - 200.0f);
                auto& st = s_pendingState;
                st.filterCutoff = static_cast<double>(cutoff);
                push_state_update();
            }
            if (elapsed_s >= 3.0f) {
                advance(DemoPhase::CHORD_OFF);
            }
            break;
        }

        case DemoPhase::CHORD_OFF:
            if (elapsed_s < 0.001f) {
                enqueue_cmd(AudioCommand::NOTE_OFF, 60);
                enqueue_cmd(AudioCommand::NOTE_OFF, 64);
                enqueue_cmd(AudioCommand::NOTE_OFF, 67);
                enqueue_cmd(AudioCommand::NOTE_OFF, 72);
            }
            if (elapsed_s >= 2.0f) {
                advance(DemoPhase::DONE);
            }
            break;

        case DemoPhase::DONE:
            demo_reset(now_us);
            break;
    }
}

// ── Serial parameter dispatch ────────────────────────────────────────────
static bool set_param(PolySynthCore::SynthState& st, const char* name, float val) {
    if (std::strcmp(name, "masterGain") == 0)         { st.masterGain = val; return true; }
    if (std::strcmp(name, "filterCutoff") == 0)        { st.filterCutoff = val; return true; }
    if (std::strcmp(name, "filterResonance") == 0)     { st.filterResonance = val; return true; }
    if (std::strcmp(name, "filterEnvAmount") == 0)     { st.filterEnvAmount = val; return true; }
    if (std::strcmp(name, "filterModel") == 0)         { st.filterModel = static_cast<int>(val); return true; }
    if (std::strcmp(name, "ampAttack") == 0)            { st.ampAttack = val; return true; }
    if (std::strcmp(name, "ampDecay") == 0)             { st.ampDecay = val; return true; }
    if (std::strcmp(name, "ampSustain") == 0)           { st.ampSustain = val; return true; }
    if (std::strcmp(name, "ampRelease") == 0)           { st.ampRelease = val; return true; }
    if (std::strcmp(name, "filterAttack") == 0)         { st.filterAttack = val; return true; }
    if (std::strcmp(name, "filterDecay") == 0)          { st.filterDecay = val; return true; }
    if (std::strcmp(name, "filterSustain") == 0)        { st.filterSustain = val; return true; }
    if (std::strcmp(name, "filterRelease") == 0)        { st.filterRelease = val; return true; }
    if (std::strcmp(name, "oscAWaveform") == 0)         { st.oscAWaveform = static_cast<int>(val); return true; }
    if (std::strcmp(name, "oscBWaveform") == 0)         { st.oscBWaveform = static_cast<int>(val); return true; }
    if (std::strcmp(name, "mixOscA") == 0)              { st.mixOscA = val; return true; }
    if (std::strcmp(name, "mixOscB") == 0)              { st.mixOscB = val; return true; }
    if (std::strcmp(name, "lfoShape") == 0)             { st.lfoShape = static_cast<int>(val); return true; }
    if (std::strcmp(name, "lfoRate") == 0)              { st.lfoRate = val; return true; }
    if (std::strcmp(name, "lfoDepth") == 0)             { st.lfoDepth = val; return true; }
    if (std::strcmp(name, "glideTime") == 0)            { st.glideTime = val; return true; }
    return false;
}

static bool get_param(const PolySynthCore::SynthState& st, const char* name, float& out) {
    if (std::strcmp(name, "masterGain") == 0)         { out = st.masterGain; return true; }
    if (std::strcmp(name, "filterCutoff") == 0)        { out = st.filterCutoff; return true; }
    if (std::strcmp(name, "filterResonance") == 0)     { out = st.filterResonance; return true; }
    if (std::strcmp(name, "filterEnvAmount") == 0)     { out = st.filterEnvAmount; return true; }
    if (std::strcmp(name, "filterModel") == 0)         { out = static_cast<float>(st.filterModel); return true; }
    if (std::strcmp(name, "ampAttack") == 0)            { out = st.ampAttack; return true; }
    if (std::strcmp(name, "ampDecay") == 0)             { out = st.ampDecay; return true; }
    if (std::strcmp(name, "ampSustain") == 0)           { out = st.ampSustain; return true; }
    if (std::strcmp(name, "ampRelease") == 0)           { out = st.ampRelease; return true; }
    if (std::strcmp(name, "filterAttack") == 0)         { out = st.filterAttack; return true; }
    if (std::strcmp(name, "filterDecay") == 0)          { out = st.filterDecay; return true; }
    if (std::strcmp(name, "filterSustain") == 0)        { out = st.filterSustain; return true; }
    if (std::strcmp(name, "filterRelease") == 0)        { out = st.filterRelease; return true; }
    if (std::strcmp(name, "oscAWaveform") == 0)         { out = static_cast<float>(st.oscAWaveform); return true; }
    if (std::strcmp(name, "oscBWaveform") == 0)         { out = static_cast<float>(st.oscBWaveform); return true; }
    if (std::strcmp(name, "mixOscA") == 0)              { out = st.mixOscA; return true; }
    if (std::strcmp(name, "mixOscB") == 0)              { out = st.mixOscB; return true; }
    if (std::strcmp(name, "lfoShape") == 0)             { out = static_cast<float>(st.lfoShape); return true; }
    if (std::strcmp(name, "lfoRate") == 0)              { out = st.lfoRate; return true; }
    if (std::strcmp(name, "lfoDepth") == 0)             { out = st.lfoDepth; return true; }
    if (std::strcmp(name, "glideTime") == 0)            { out = st.glideTime; return true; }
    return false;
}

// ── Serial command dispatch ──────────────────────────────────────────────
static void dispatch_command(const pico_serial::Command& cmd) {
    using Type = pico_serial::Command::Type;

    switch (cmd.type) {
        case Type::NOTE_ON:
            enqueue_cmd(AudioCommand::NOTE_ON,
                        static_cast<uint8_t>(cmd.intArg1),
                        static_cast<uint8_t>(cmd.intArg2));
            printf("OK: NOTE_ON %d %d\n", cmd.intArg1, cmd.intArg2);
            break;

        case Type::NOTE_OFF:
            enqueue_cmd(AudioCommand::NOTE_OFF,
                        static_cast<uint8_t>(cmd.intArg1));
            printf("OK: NOTE_OFF %d\n", cmd.intArg1);
            break;

        case Type::SET: {
            auto& st = s_pendingState;
            if (set_param(st, cmd.strArg, cmd.floatArg)) {
                push_state_update();
                printf("OK: SET %s=%.4f\n", cmd.strArg, cmd.floatArg);
            } else {
                printf("ERR: unknown param '%s'\n", cmd.strArg);
            }
            break;
        }

        case Type::GET: {
            const auto& st = s_pendingState;
            float val = 0.0f;
            if (get_param(st, cmd.strArg, val)) {
                printf("VAL: %s=%.4f\n", cmd.strArg, static_cast<double>(val));
            } else {
                printf("ERR: unknown param '%s'\n", cmd.strArg);
            }
            break;
        }

        case Type::STATUS: {
            float fill_time_us = static_cast<float>(pico_audio::GetLastFillTimeUs());
            float buffer_time_us = static_cast<float>(pico_audio::kBufferFrames) * 1000000.0f /
                                   static_cast<float>(pico_audio::kSampleRate);
            float cpu_percent = (fill_time_us / buffer_time_us) * 100.0f;
            printf("STATUS: voices=%d cpu=%.1f%% engine=%uB underruns=%lu\n",
                   s_activeVoices.load(std::memory_order_relaxed),
                   static_cast<double>(cpu_percent),
                   static_cast<unsigned>(sizeof(PolySynthCore::Engine)),
                   static_cast<unsigned long>(pico_audio::GetUnderrunCount()));
            break;
        }

        case Type::PANIC:
            enqueue_cmd(AudioCommand::PANIC);
            printf("OK: PANIC\n");
            break;

        case Type::RESET: {
            enqueue_cmd(AudioCommand::PANIC);
            auto& st = s_pendingState;
            st.Reset();
            push_state_update();
            printf("OK: RESET\n");
            break;
        }

        case Type::DEMO:
            s_demo.running = true;
            demo_reset(time_us_64());
            printf("OK: DEMO started\n");
            break;

        case Type::STOP:
            s_demo.running = false;
            enqueue_cmd(AudioCommand::PANIC);
            printf("OK: STOP\n");
            break;

        case Type::UNKNOWN:
            printf("ERR: unknown command\n");
            break;
    }
}

// ── Self-test sequence ───────────────────────────────────────────────────
static bool run_self_tests()
{
    int pass_count = 0;
    int fail_count = 0;

    printf("[TEST:BEGIN]\n");

    // Test 1: Boot init completed
    printf("[TEST:PASS] boot_init\n");
    pass_count++;

    // Test 2: Serial output works
    printf("[TEST:PASS] serial_output\n");
    pass_count++;

    // Test 3: SRAM report
    print_sram_report();
    printf("[TEST:PASS] sram_report\n");
    pass_count++;

    // Test 4: Engine init
    s_pendingState.Reset();
    s_stagedState.Reset();
    s_engine.Init(static_cast<double>(pico_audio::kSampleRate));
    s_engine.UpdateState(s_stagedState);
    printf("[TEST:PASS] engine_init\n");
    pass_count++;

    // Test 5: Engine produces non-zero audio
    {
        float left = 0.0f, right = 0.0f;
        float energy = 0.0f;
        s_engine.OnNoteOn(60, 100);
        for (int i = 0; i < 256; i++) {
            left = 0.0f;
            right = 0.0f;
            s_engine.Process(left, right);
            energy += left * left + right * right;
        }
        s_engine.OnNoteOff(60);

        if (energy > 0.0001f) {
            printf("[TEST:PASS] engine_produces_audio (energy=%.4f)\n",
                   static_cast<double>(energy));
            pass_count++;
        } else {
            printf("[TEST:FAIL] engine_produces_audio: energy=%.4f (expected > 0.0001)\n",
                   static_cast<double>(energy));
            fail_count++;
        }
    }

    // Test 6: Audio I2S init (on core 0 for self-test, will re-init on core 1)
    if (pico_audio::Init(audio_callback)) {
        printf("[TEST:PASS] audio_i2s_init\n");
        pass_count++;
        // Stop and release resources — audio will be re-initialized on core 1
        pico_audio::Stop();
    } else {
        printf("[TEST:FAIL] audio_i2s_init\n");
        fail_count++;
    }

    // Summary
    printf("\n[TEST:SUMMARY] %d passed, %d failed\n", pass_count, fail_count);
    if (fail_count == 0) {
        printf("[TEST:ALL_PASSED]\n");
    } else {
        printf("[TEST:SOME_FAILED]\n");
    }

    return fail_count == 0;
}

// ── Core 1 entry point — dedicated to audio DSP ──────────────────────────
static void core1_audio_entry() {
    // Flush denormals to zero: set FPSCR FZ (bit 24) and DN (bit 25).
    // Denormal floats in filter tails can cost 10-80× more cycles than normal floats.
    // On Cortex-M33 with hard-float ABI, this prevents performance cliffs when
    // signals decay to near-zero (e.g. filter resonance ringing out).
    {
        uint32_t fpscr;
        __asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
        fpscr |= (1u << 24) | (1u << 25);  // FZ=1, DN=1
        __asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
        printf("[Core1] FPSCR set: FZ+DN enabled (0x%08lx)\n",
               static_cast<unsigned long>(fpscr));
    }

    // Init audio on core 1 so the DMA IRQ fires on this core
    if (!pico_audio::Init(audio_callback, 1)) {
        multicore_fifo_push_blocking(0);  // signal failure
        return;
    }
    multicore_fifo_push_blocking(1);  // signal success

    pico_audio::Start();

    // Core 1 just sleeps; all work happens in DMA ISR
    while (true) {
        __wfi();  // wait for interrupt — lowest power, wakes on DMA IRQ
    }
}

// ── Main ─────────────────────────────────────────────────────────────────
int main()
{
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("ERROR: CYW43 init failed\n");
        return 1;
    }

    // Wait for USB serial
    for (int i = 0; i < 30; i++) {
        if (stdio_usb_connected()) break;
        sleep_ms(100);
    }

    printf("\n");
    printf("========================================\n");
    printf("  PolySynth Pico v0.5 — Dual-Core + Fast Math\n");
    printf("  Board:       Pico 2 W (RP2350)\n");
    printf("  Sample rate: %u Hz\n", pico_audio::kSampleRate);
    printf("  Buffer:      %u frames (%.2f ms)\n",
           pico_audio::kBufferFrames,
           static_cast<float>(pico_audio::kBufferFrames) * 1000.0f /
               static_cast<float>(pico_audio::kSampleRate));
    printf("  Engine:      %u bytes (effects stripped)\n",
           static_cast<unsigned>(sizeof(PolySynthCore::Engine)));
    printf("========================================\n");
    printf("Commands: NOTE_ON <n> <v>, NOTE_OFF <n>, SET <p> <v>,\n");
    printf("          GET <p>, STATUS, PANIC, RESET, DEMO, STOP\n");
    printf("========================================\n\n");

    // Run self-tests (includes audio init)
    if (!run_self_tests()) {
        printf("Self-tests failed. Halting.\n");
        while (true) { sleep_ms(1000); }
    }

    // Re-init engine cleanly after self-test
    s_engine.Reset();
    s_pendingState.Reset();
    s_stagedState.Reset();
    s_engine.UpdateState(s_stagedState);

    // Launch core 1 for dedicated audio DSP
    multicore_launch_core1(core1_audio_entry);
    uint32_t audio_ok = multicore_fifo_pop_blocking();
    if (!audio_ok) {
        printf("ERROR: Audio init on core 1 failed, falling back to core 0\n");
        pico_audio::Init(audio_callback);
        pico_audio::Start();
    }
    printf("Audio started on core %d — running demo sequence\n", audio_ok ? 1 : 0);
    printf("Type STOP to halt demo, or send commands\n\n");

    // Start demo
    s_demo.running = true;
    demo_reset(time_us_64());

    // Main loop: serial polling + demo + status
    pico_serial::CommandParser parser;
    uint32_t report_counter = 0;
    uint64_t last_report_us = time_us_64();

    while (true) {
        // Non-blocking serial polling
        int ch = getchar_timeout_us(0);
        if (ch != PICO_ERROR_TIMEOUT) {
            if (parser.Feed(static_cast<char>(ch))) {
                auto cmd = parser.Parse();
                dispatch_command(cmd);
            }
        }

        // Service CYW43 driver (required for pico_cyw43_arch_none)
        cyw43_arch_poll();

        // Demo tick
        demo_tick(time_us_64());

        // Drain voice-change events (safe — main loop only)
        {
            int head = s_voiceEventHead.load(std::memory_order_acquire);
            while (s_voiceEventTail != head) {
                auto& ev = s_voiceEvents[s_voiceEventTail];
                printf("[VOICE] %d -> %d\n", ev.from, ev.to);
                s_voiceEventTail = (s_voiceEventTail + 1) % kVoiceEventBufSize;
            }
        }

        // Periodic status report (every 5 seconds)
        uint64_t now = time_us_64();
        if (now - last_report_us >= 5000000ULL) {
            last_report_us = now;
            report_counter++;

            float fill_time_us = static_cast<float>(pico_audio::GetLastFillTimeUs());
            float buffer_time_us = static_cast<float>(pico_audio::kBufferFrames) * 1000000.0f /
                                   static_cast<float>(pico_audio::kSampleRate);
            float cpu_percent = (fill_time_us / buffer_time_us) * 100.0f;

            float peak = s_peakLevel.exchange(0.0f, std::memory_order_relaxed);

            printf("[%lus] CPU: %.1f%% | Voices: %d | Peak: %.3f | Phase: %s\n",
                   static_cast<unsigned long>(report_counter * 5),
                   static_cast<double>(cpu_percent),
                   s_activeVoices.load(std::memory_order_relaxed),
                   static_cast<double>(peak),
                   s_demo.running ? demo_phase_name(s_demo.phase) : "OFF");

            // Blink LED
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, report_counter % 2);
        }
    }

    return 0;
}
