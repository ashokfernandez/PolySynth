// Golden master test for the full Pico signal chain.
// Verifies that Engine → gain → fast_tanh → int16 saturation → I2S packing
// produces deterministic output matching a stored reference.
//
// The reference is generated on first run (or with GENERATE_GOLDEN defined)
// and verified on subsequent runs. This catches regressions in:
//   - Float-precision DSP (embedded config)
//   - Post-processing chain (gain, soft-clip, saturation, I2S packing)

#include "catch.hpp"
#include "PicoSignalChain.h"
#include "WavWriter.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

namespace {

// Path to the golden reference binary file (relative to working directory).
// CI runs from tests/build/ or tests/build_embedded/.
static const char* kGoldenRefPath = "../golden_embedded/pico_signal_chain.bin";
static const char* kGoldenWavPath = "../golden_embedded/pico_signal_chain.wav";

static constexpr float kSampleRate = 48000.0f;
static constexpr uint32_t kNoteOnFrames = 24000;   // 0.5s
static constexpr uint32_t kReleaseFrames = 24000;   // 0.5s
static constexpr uint32_t kTotalFrames = kNoteOnFrames + kReleaseFrames;

// Render a known note sequence through the full signal chain.
std::vector<uint32_t> RenderReference() {
    PolySynthCore::Engine engine;
    engine.Init(static_cast<double>(kSampleRate));

    // Use default patch state — saw wave, default ADSR, default filter.
    engine.OnNoteOn(69, 100); // A4

    // Render note-on phase
    auto buffer = PicoSignalChain::RenderToI2S(engine, kNoteOnFrames);

    // Note off + release phase
    engine.OnNoteOff(69);
    auto release = PicoSignalChain::RenderToI2S(engine, kReleaseFrames);

    buffer.insert(buffer.end(), release.begin(), release.end());
    return buffer;
}

bool FileExists(const char* path) {
    std::ifstream f(path);
    return f.good();
}

std::vector<uint32_t> ReadBinaryGolden(const char* path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return {};
    auto size = f.tellg();
    f.seekg(0);
    std::vector<uint32_t> data(static_cast<size_t>(size) / sizeof(uint32_t));
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

void WriteBinaryGolden(const char* path, const std::vector<uint32_t>& data) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size() * sizeof(uint32_t)));
}

// Also write a WAV for human inspection.
void WriteWavFromI2S(const char* path, const std::vector<uint32_t>& buffer,
                     uint32_t numFrames) {
    auto samples = PicoSignalChain::UnpackI2SLeft(buffer, numFrames);
    std::vector<float> floatSamples(numFrames);
    for (uint32_t i = 0; i < numFrames; i++) {
        floatSamples[i] = static_cast<float>(samples[i]) / 32768.0f;
    }
    PolySynth::WavWriter::Write(path, floatSamples,
                                static_cast<int>(kSampleRate));
}

} // namespace

// ─── Golden master: full Pico signal chain ───────────────────────────────

TEST_CASE("Pico signal chain golden master", "[PicoGoldenMaster][embedded]")
{
    auto current = RenderReference();
    REQUIRE(current.size() == kTotalFrames * 2); // stereo

    SECTION("Output is non-silent") {
        // At least some samples should be non-zero during note-on phase.
        bool hasNonZero = false;
        for (uint32_t i = 0; i < kNoteOnFrames * 2 && !hasNonZero; i++) {
            if (current[i] != 0) hasNonZero = true;
        }
        REQUIRE(hasNonZero);
    }

    SECTION("Verify against golden reference") {
        if (!FileExists(kGoldenRefPath)) {
            // First run — generate the golden reference.
            WriteBinaryGolden(kGoldenRefPath, current);
            WriteWavFromI2S(kGoldenWavPath, current, kTotalFrames);
            WARN("Golden reference generated: " << kGoldenRefPath);
            WARN("Inspect WAV: " << kGoldenWavPath);
            // Don't fail — the next run will verify.
            SUCCEED("Generated golden reference (first run)");
            return;
        }

        auto golden = ReadBinaryGolden(kGoldenRefPath);
        REQUIRE(golden.size() == current.size());

        // Exact binary match — the signal chain is fully deterministic.
        uint32_t mismatches = 0;
        uint32_t firstMismatchIdx = 0;
        for (size_t i = 0; i < golden.size(); i++) {
            if (golden[i] != current[i]) {
                if (mismatches == 0) firstMismatchIdx = static_cast<uint32_t>(i);
                mismatches++;
            }
        }

        if (mismatches > 0) {
            // Write the current output for diff analysis.
            WriteBinaryGolden("pico_signal_chain_current.bin", current);
            WriteWavFromI2S("pico_signal_chain_current.wav", current, kTotalFrames);
            FAIL("Signal chain mismatch: " << mismatches << " of "
                 << golden.size() << " words differ (first at index "
                 << firstMismatchIdx << "). "
                 << "Current output written to pico_signal_chain_current.wav");
        }
    }
}

// ─── Verify signal chain components individually ─────────────────────────

TEST_CASE("PicoSignalChain fast_tanh matches expected values", "[PicoGoldenMaster]")
{
    // Verify the Padé approximant against known values.
    CHECK(PicoSignalChain::fast_tanh(0.0f) == Approx(0.0f));
    CHECK(PicoSignalChain::fast_tanh(1.0f) == Approx(std::tanh(1.0f)).epsilon(0.025));
    CHECK(PicoSignalChain::fast_tanh(-1.0f) == Approx(std::tanh(-1.0f)).epsilon(0.025));
    CHECK(PicoSignalChain::fast_tanh(3.0f) == Approx(std::tanh(3.0f)).epsilon(0.05));

    // Monotonicity in [0, 4]
    float prev = 0.0f;
    for (float x = 0.01f; x <= 4.0f; x += 0.01f) {
        float y = PicoSignalChain::fast_tanh(x);
        CHECK(y >= prev);
        prev = y;
    }
}

TEST_CASE("PicoSignalChain saturate_to_i16 clamps correctly", "[PicoGoldenMaster]")
{
    CHECK(PicoSignalChain::saturate_to_i16(0) == 0);
    CHECK(PicoSignalChain::saturate_to_i16(32767) == 32767);
    CHECK(PicoSignalChain::saturate_to_i16(32768) == 32767);   // clamped
    CHECK(PicoSignalChain::saturate_to_i16(100000) == 32767);  // clamped
    CHECK(PicoSignalChain::saturate_to_i16(-32768) == -32768);
    CHECK(PicoSignalChain::saturate_to_i16(-32769) == -32768); // clamped
    CHECK(PicoSignalChain::saturate_to_i16(-100000) == -32768);
}
