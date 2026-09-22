// SPDX-License-Identifier: GPL-2.0-or-later
// Fixed-frame Phase 6C candidate render for the shared Sampulse witness.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/player.h>
#include <psycle/audiodrivers/audiodriver.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

#include <cmath>
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: " << argv[0] << " FIXTURE OUTPUT_WAV\n";
        return 64;
    }

    try {
        using namespace universalis::os::loggers;
        static stream_logger diagnostics(std::cerr);
        multiplex_logger::singleton().add(diagnostics);
        universalis::os::thread_name thread_name("phase6c-sampulse-render");

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;
            player.song(song);
            if (!song.load(argv[1])) {
                std::cerr << "shared Sampulse witness load failed\n";
                result = 65;
            } else {
                psycle::audiodrivers::AudioDriverSettings settings(
                    player.driver().playbackSettings());
                settings.setSamplesPerSec(44100);
                settings.setBitDepth(16);
                settings.setChannelMode(0);
                settings.setBlockFrames(2048);
                player.driver().setPlaybackSettings(settings);

                player.setFileName(argv[2]);
                player.startRecording();
                if (!player.recording()) {
                    std::cerr << "candidate recording did not start\n";
                    result = 66;
                } else {
                    player.start(0.0);

                    const double beat_frames = 44100.0 * 60.0 / 137.0;
                    const int target_frames =
                        static_cast<int>(std::ceil(4.25 * beat_frames));
                    int rendered = 0;
                    while (rendered < target_frames) {
                        const int remaining = target_frames - rendered;
                        const int amount = remaining < 2048 ? remaining : 2048;
                        player.Work(amount);
                        rendered += amount;
                    }

                    const double final_beat = player.playPos();
                    player.stop();
                    player.stopRecording();

                    std::ifstream output(argv[2], std::ios::binary | std::ios::ate);
                    if (!output || output.tellg() <= 44) {
                        std::cerr << "candidate render output missing or empty\n";
                        result = 67;
                    } else {
                        std::cout
                            << "{\"schema_version\":1"
                            << ",\"fixed_frame_render\":true"
                            << ",\"sample_rate\":44100"
                            << ",\"channels\":1"
                            << ",\"bits_per_sample\":16"
                            << ",\"target_frames\":" << target_frames
                            << ",\"final_play_beat\":" << final_beat
                            << "}" << std::endl;
                    }
                }
            }
        }

        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "candidate Sampulse render exception: "
                  << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "candidate Sampulse render unknown exception\n";
        return 70;
    }
}
