// Unit tests for the Pico serial command parser.
// Runs on desktop (no Pico SDK dependency).

#include "catch.hpp"
#include "command_parser.h"

using pico_serial::Command;
using pico_serial::CommandParser;

static Command feed_line(CommandParser& p, const char* line) {
    for (const char* c = line; *c; ++c) {
        p.Feed(*c);
    }
    REQUIRE(p.Feed('\n'));
    return p.Parse();
}

TEST_CASE("CommandParser: NOTE_ON with note and velocity", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "NOTE_ON 60 100");
    REQUIRE(cmd.type == Command::NOTE_ON);
    REQUIRE(cmd.intArg1 == 60);
    REQUIRE(cmd.intArg2 == 100);
}

TEST_CASE("CommandParser: NOTE_ON with default velocity", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "NOTE_ON 72");
    REQUIRE(cmd.type == Command::NOTE_ON);
    REQUIRE(cmd.intArg1 == 72);
    REQUIRE(cmd.intArg2 == 100);  // default
}

TEST_CASE("CommandParser: NOTE_OFF", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "NOTE_OFF 60");
    REQUIRE(cmd.type == Command::NOTE_OFF);
    REQUIRE(cmd.intArg1 == 60);
}

TEST_CASE("CommandParser: SET param", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "SET filterCutoff 2000.0");
    REQUIRE(cmd.type == Command::SET);
    REQUIRE(std::strcmp(cmd.strArg, "filterCutoff") == 0);
    REQUIRE(cmd.floatArg == Approx(2000.0));
}

TEST_CASE("CommandParser: GET param", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "GET masterGain");
    REQUIRE(cmd.type == Command::GET);
    REQUIRE(std::strcmp(cmd.strArg, "masterGain") == 0);
}

TEST_CASE("CommandParser: STATUS", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "STATUS");
    REQUIRE(cmd.type == Command::STATUS);
}

TEST_CASE("CommandParser: PANIC", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "PANIC");
    REQUIRE(cmd.type == Command::PANIC);
}

TEST_CASE("CommandParser: RESET", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "RESET");
    REQUIRE(cmd.type == Command::RESET);
}

TEST_CASE("CommandParser: DEMO", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "DEMO");
    REQUIRE(cmd.type == Command::DEMO);
}

TEST_CASE("CommandParser: STOP", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "STOP");
    REQUIRE(cmd.type == Command::STOP);
}

TEST_CASE("CommandParser: unknown command", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "FOOBAR 123");
    REQUIRE(cmd.type == Command::UNKNOWN);
}

TEST_CASE("CommandParser: empty line ignored", "[CommandParser]") {
    CommandParser p;
    // Feeding just a newline should return false (no command ready)
    REQUIRE_FALSE(p.Feed('\n'));
}

TEST_CASE("CommandParser: buffer overflow protection", "[CommandParser]") {
    CommandParser p;
    // Feed >128 chars, should not crash
    for (int i = 0; i < 200; i++) {
        p.Feed('A');
    }
    REQUIRE(p.Feed('\n'));
    auto cmd = p.Parse();
    // Should parse as UNKNOWN since the truncated line won't match any command
    REQUIRE(cmd.type == Command::UNKNOWN);
}

TEST_CASE("CommandParser: multiple commands in sequence", "[CommandParser]") {
    CommandParser p;
    auto cmd1 = feed_line(p, "NOTE_ON 60 80");
    REQUIRE(cmd1.type == Command::NOTE_ON);
    REQUIRE(cmd1.intArg1 == 60);

    auto cmd2 = feed_line(p, "NOTE_OFF 60");
    REQUIRE(cmd2.type == Command::NOTE_OFF);
    REQUIRE(cmd2.intArg1 == 60);
}

TEST_CASE("CommandParser: SET integer param", "[CommandParser]") {
    CommandParser p;
    auto cmd = feed_line(p, "SET oscAWaveform 1");
    REQUIRE(cmd.type == Command::SET);
    REQUIRE(std::strcmp(cmd.strArg, "oscAWaveform") == 0);
    REQUIRE(cmd.floatArg == Approx(1.0));
}
