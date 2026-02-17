#include "../../src/core/VoiceManager.h"
#include "catch.hpp"

TEST_CASE("Voice starts in Idle state", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    REQUIRE(voice.IsActive() == false);
}

TEST_CASE("Voice transitions Idle -> Attack on NoteOn", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    REQUIRE(voice.IsActive() == true);
    REQUIRE(voice.GetNote() == 60);
    REQUIRE(voice.GetTimestamp() == 1);
}

TEST_CASE("Voice transitions Attack -> Release on NoteOff", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.NoteOff();
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Release);
}

TEST_CASE("Voice transitions Release -> Idle when envelope finishes", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.NoteOff();
    for (int i = 0; i < 48000; i++) {
        voice.Process();
    }
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    REQUIRE(voice.IsActive() == false);
}

TEST_CASE("Voice StartSteal triggers Stolen state with fade-out", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    voice.StartSteal();
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Stolen);
}

TEST_CASE("Stolen voice completes fade in ~2ms", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    for (int i = 0; i < 10; i++) voice.Process();

    voice.StartSteal();
    int samples = 0;
    while (voice.GetVoiceState() == PolySynthCore::VoiceState::Stolen && samples < 200) {
        voice.Process();
        samples++;
    }
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Idle);
    REQUIRE(samples <= 100);
    REQUIRE(samples >= 90);
}

TEST_CASE("Re-trigger: NoteOn during Attack transitions back to Attack", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 0);
    voice.NoteOn(60, 100, 1);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    voice.NoteOn(64, 80, 2);
    REQUIRE(voice.GetVoiceState() == PolySynthCore::VoiceState::Attack);
    REQUIRE(voice.GetNote() == 64);
    REQUIRE(voice.GetTimestamp() == 2);
}

TEST_CASE("VoiceManager timestamp increments on each NoteOn", "[VoiceStateMachine]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    REQUIRE(vm.GetGlobalTimestamp() == 0);

    vm.OnNoteOn(60, 100);
    REQUIRE(vm.GetGlobalTimestamp() == 1);

    vm.OnNoteOn(64, 100);
    REQUIRE(vm.GetGlobalTimestamp() == 2);

    vm.OnNoteOn(67, 100);
    REQUIRE(vm.GetGlobalTimestamp() == 3);

    auto states = vm.GetVoiceStates();
    int activeCount = 0;
    for (const auto& s : states) {
        if (s.state != PolySynthCore::VoiceState::Idle) {
            REQUIRE(s.state == PolySynthCore::VoiceState::Attack);
            activeCount++;
        }
    }
    REQUIRE(activeCount == 3);
}

TEST_CASE("GetRenderState reflects current voice state", "[VoiceStateMachine]") {
    PolySynthCore::Voice voice;
    voice.Init(48000.0, 5);

    auto rs = voice.GetRenderState();
    REQUIRE(rs.voiceID == 5);
    REQUIRE(rs.state == PolySynthCore::VoiceState::Idle);
    REQUIRE(rs.note == -1);

    voice.NoteOn(72, 127, 42);
    rs = voice.GetRenderState();
    REQUIRE(rs.state == PolySynthCore::VoiceState::Attack);
    REQUIRE(rs.note == 72);
    REQUIRE(rs.velocity == 127);
    REQUIRE(rs.voiceID == 5);
}

TEST_CASE("GetHeldNotes deduplicates same pitch across voices", "[VoiceStateMachine]") {
    PolySynthCore::VoiceManager vm;
    vm.Init(48000.0);

    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(60, 100);
    vm.OnNoteOn(64, 100);

    std::array<int, PolySynthCore::kMaxVoices> notes{};
    int count = vm.GetHeldNotes(notes);

    REQUIRE(count == 2);
    bool has60 = false, has64 = false;
    for (int i = 0; i < count; i++) {
        if (notes[i] == 60) has60 = true;
        if (notes[i] == 64) has64 = true;
    }
    REQUIRE(has60);
    REQUIRE(has64);
}
