#pragma once

#include "command_parser.h"

class PicoSynthApp;
class PicoDemo;

namespace pico_commands {

void Dispatch(const pico_serial::Command& cmd, PicoSynthApp& app, PicoDemo& demo);

} // namespace pico_commands
