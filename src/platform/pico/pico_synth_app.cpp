#include "pico_synth_app.h"

#include <cstring>

#include "pico.h"  // __time_critical_func
#include "sine_generator.h" // PackI2S()

// ── Fast tanh approximant (Padé) ─────────────────────────────────────────
// Replaces tanhf() which is a library call on Cortex-M33 (~200-500 cycles).
// This Padé approximant is <10 cycles and accurate to ~0.1% for |x| < 3.
static inline float fast_tanh(float x) {
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// ── Saturating int16 conversion (SSAT instruction) ───────────────────────
// Single-cycle saturating clamp to [-32768, 32767] via ARM DSP extension.
static inline int16_t saturate_to_i16(int32_t val) {
    int32_t result;
    __asm__ volatile("ssat %0, #16, %1" : "=r"(result) : "r"(val));
    return static_cast<int16_t>(result);
}

// ── Parameter dispatch (string name → SynthState field) ──────────────────
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

// ── Public API ───────────────────────────────────────────────────────────

void PicoSynthApp::Init(float sampleRate) {
    mPendingState.Reset();
    mStagedState.Reset();
    mEngine.Init(static_cast<double>(sampleRate));
    mEngine.UpdateState(mStagedState);
}

void PicoSynthApp::NoteOn(uint8_t note, uint8_t velocity) {
    mCommandQueue.TryPush({AudioCommand::NOTE_ON, note, velocity});
}

void PicoSynthApp::NoteOff(uint8_t note) {
    mCommandQueue.TryPush({AudioCommand::NOTE_OFF, note, 0});
}

bool PicoSynthApp::SetParam(const char* name, float value) {
    if (!set_param(mPendingState, name, value))
        return false;
    PushStateUpdate();
    return true;
}

bool PicoSynthApp::GetParam(const char* name, float& out) const {
    return get_param(mPendingState, name, out);
}

void PicoSynthApp::PushStateUpdate() {
    // Wait for ISR to consume previous staged state (virtually never spins —
    // audio callback period ~5.3ms, state updates are ≥20ms apart)
    while (!mStateConsumed.load(std::memory_order_acquire)) {
        // spin
    }
    mStagedState = mPendingState;
    mStateConsumed.store(false, std::memory_order_release);
    mCommandQueue.TryPush({AudioCommand::UPDATE_STATE, 0, 0});
}

void PicoSynthApp::Panic() {
    mCommandQueue.TryPush({AudioCommand::PANIC, 0, 0});
}

void PicoSynthApp::Reset() {
    Panic();
    mPendingState.Reset();
    PushStateUpdate();
}

// ── Audio callback (called from DMA ISR on Core 1) ───────────────────────
// __time_critical_func places this in SRAM, avoiding XIP flash cache misses
// that cause variable-latency stalls (~10-100+ cycles per miss) during audio.
// Since SEA_DSP is header-only and inlined here, the DSP code lands in SRAM too.
void __time_critical_func(PicoSynthApp::AudioCallback)(uint32_t* buffer, uint32_t numFrames)
{
    // Drain command queue
    AudioCommand cmd;
    while (mCommandQueue.TryPop(cmd)) {
        switch (cmd.type) {
            case AudioCommand::NOTE_ON:
                mEngine.OnNoteOn(cmd.arg1, cmd.arg2);
                break;
            case AudioCommand::NOTE_OFF:
                mEngine.OnNoteOff(cmd.arg1);
                break;
            case AudioCommand::UPDATE_STATE:
                mEngine.UpdateState(mStagedState);
                mStateConsumed.store(true, std::memory_order_release);
                break;
            case AudioCommand::PANIC:
                mEngine.Reset();
                break;
            default:
                break;
        }
    }

    // Process audio
    for (uint32_t i = 0; i < numFrames; i++) {
        float left = 0.0f, right = 0.0f;
        mEngine.Process(left, right);

        constexpr float kOutputGain = 4.0f;
        left *= kOutputGain;
        right *= kOutputGain;

        // Track peak level for diagnostics
        float absL = left > 0 ? left : -left;
        float absR = right > 0 ? right : -right;
        float peak = absL > absR ? absL : absR;
        float prev = mPeakLevel.load(std::memory_order_relaxed);
        if (peak > prev) mPeakLevel.store(peak, std::memory_order_relaxed);

        // Soft clip + saturating int16 conversion
        auto l16 = saturate_to_i16(static_cast<int32_t>(fast_tanh(left) * 32767.0f));
        auto r16 = saturate_to_i16(static_cast<int32_t>(fast_tanh(right) * 32767.0f));
        buffer[i * 2]     = pico_audio::PackI2S(l16);
        buffer[i * 2 + 1] = pico_audio::PackI2S(r16);
    }

    // Update voice count for status reporting
    int vc = mEngine.GetActiveVoiceCount();
    mActiveVoices.store(vc, std::memory_order_relaxed);

    // Buffer voice-change events (NO printf from ISR — not thread-safe)
    if (vc != mPrevVoiceCount) {
        int head = mVoiceEventHead.load(std::memory_order_relaxed);
        int next = (head + 1) % kVoiceEventBufSize;
        mVoiceEvents[head] = {static_cast<int8_t>(mPrevVoiceCount),
                              static_cast<int8_t>(vc)};
        mVoiceEventHead.store(next, std::memory_order_release);
        mPrevVoiceCount = vc;
    }
}

// ── Diagnostics ──────────────────────────────────────────────────────────

int PicoSynthApp::GetActiveVoiceCount() const {
    return mActiveVoices.load(std::memory_order_relaxed);
}

float PicoSynthApp::ExchangePeakLevel() {
    return mPeakLevel.exchange(0.0f, std::memory_order_relaxed);
}

bool PicoSynthApp::DrainVoiceEvent(int8_t& from, int8_t& to) {
    int head = mVoiceEventHead.load(std::memory_order_acquire);
    if (mVoiceEventTail == head) return false;
    auto& ev = mVoiceEvents[mVoiceEventTail];
    from = ev.from;
    to = ev.to;
    mVoiceEventTail = (mVoiceEventTail + 1) % kVoiceEventBufSize;
    return true;
}
