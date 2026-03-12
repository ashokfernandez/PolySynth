#include "pico_command_dispatch.h"
#include "pico_synth_app.h"
#include "pico_demo.h"
#include "song_player.h"
#include "songs/song_registry.h"
#include "audio_i2s_driver.h"
#include "Engine.h"

#include <cstdio>
#include <cstring>

#include "pico/time.h"

namespace pico_commands {

void Dispatch(const pico_serial::Command& cmd, PicoSynthApp& app, PicoDemo& demo,
              SongPlayer& songPlayer) {
    using Type = pico_serial::Command::Type;

    switch (cmd.type) {
        case Type::NOTE_ON:
            app.NoteOn(static_cast<uint8_t>(cmd.intArg1),
                       static_cast<uint8_t>(cmd.intArg2));
            printf("OK: NOTE_ON %d %d\n", cmd.intArg1, cmd.intArg2);
            break;

        case Type::NOTE_OFF:
            app.NoteOff(static_cast<uint8_t>(cmd.intArg1));
            printf("OK: NOTE_OFF %d\n", cmd.intArg1);
            break;

        case Type::SET:
            if (app.SetParam(cmd.strArg, cmd.floatArg)) {
                printf("OK: SET %s=%.4f\n", cmd.strArg, cmd.floatArg);
            } else {
                printf("ERR: unknown param '%s'\n", cmd.strArg);
            }
            break;

        case Type::GET: {
            float val = 0.0f;
            if (app.GetParam(cmd.strArg, val)) {
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
                   app.GetActiveVoiceCount(),
                   static_cast<double>(cpu_percent),
                   static_cast<unsigned>(sizeof(PolySynthCore::Engine)),
                   static_cast<unsigned long>(pico_audio::GetUnderrunCount()));
            break;
        }

        case Type::PANIC:
            app.Panic();
            printf("OK: PANIC\n");
            break;

        case Type::RESET:
            app.Reset();
            printf("OK: RESET\n");
            break;

        case Type::DEMO:
            songPlayer.Stop(app);  // Stop song if playing
            demo.Start(time_us_64());
            printf("OK: DEMO started\n");
            break;

        case Type::STOP:
            demo.Stop();
            songPlayer.Stop(app);
            app.Panic();
            printf("OK: STOP\n");
            break;

        case Type::SONG: {
            if (std::strcmp(cmd.strArg, "STOP") == 0) {
                songPlayer.Stop(app);
            } else {
                int index = cmd.intArg1;
                if (index < 0 || index >= pico_song::kSongCount) {
                    printf("ERR: song index %d out of range (0-%d)\n",
                           index, pico_song::kSongCount - 1);
                } else {
                    demo.Stop();  // Stop demo if running
                    songPlayer.Play(*pico_song::kSongs[index], app);
                }
            }
            break;
        }

        case Type::UNKNOWN:
            printf("ERR: unknown command\n");
            break;
    }
}

} // namespace pico_commands
