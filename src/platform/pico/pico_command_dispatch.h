#pragma once

#include "command_parser.h"

class PicoSynthApp;
class PicoDemo;
class SongPlayer;

namespace pico_commands {

void Dispatch(const pico_serial::Command& cmd, PicoSynthApp& app, PicoDemo& demo,
              SongPlayer& songPlayer);

} // namespace pico_commands
