// SPDX-License-Identifier: GPL-2.0-or-later
// Fixed-frame Phase 6C candidate render for the shared Sampulse witness.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/player.h>
#include <psycle/core/sequencer.h>
#include <psycle/core/internal_machines.h>
#include <psycle/core/xminstrument.h>
#include <psycle/audiodrivers/audiodriver.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

        const char* requested_threads = std::getenv("PSYCLE_THREADS");
        if (requested_threads == 0 || std::string(requested_threads) != "1") {
            std::cerr << "candidate render requires PSYCLE_THREADS=1\n";
            return 69;
        }

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;

            // The retained r12005 Psy3Filter::LoadEINSv1() loads historical
            // Sampulse state into existing slots via rInstrument()/SampleData()
            // but CoreSong leaves those vectors empty. Preallocate empty slot 0
            // in the project-owned harness only; the EINS bytes must overwrite
            // both placeholders before playback.
            psycle::core::XMInstrument preload_instrument;
            psycle::core::XMInstrument::WaveData preload_sample;
            song.m_Instruments.SetInst(preload_instrument, 0);
            song.m_rWaveLayers.SetSample(preload_sample, 0);

            player.song(song);
            if (!song.load(argv[1])) {
                std::cerr << "shared Sampulse witness load failed\n";
                result = 65;
            } else if (
                !song.rInstrument(0).IsEnabled()
                || song.SampleData(0).WaveLength() != 512
                || song.rInstrument(0).NoteToSample(60).second != 0
            ) {
                std::cerr << "historical EINS state did not overwrite preload slots\n";
                result = 68;
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

                    // The frozen r12005 Sequencer::Work() keeps global-event
                    // last_pos only within one callback, so this observation must
                    // span the complete command-bearing window in one Sequencer call.
                    // Do not route that large count through Player::Work(): the
                    // historical Player callback buffer is sized for driver-era
                    // callbacks while Master writes two interleaved floats per frame.
                    // Give Master an explicitly sized harness buffer instead, then
                    // use the frozen Sequencer and Player::process() path unchanged.
                    std::vector<float> master_output(
                        static_cast<std::size_t>(target_frames) * 2u, 0.0f);
                    psycle::core::Master& master =
                        static_cast<psycle::core::Master&>(
                            *song.machine(psycle::core::MASTER_INDEX));
                    master._pMasterSamples = master_output.data();

                    psycle::core::Sequencer sequencer;
                    sequencer.set_song(song);
                    sequencer.set_time_info(player.timeInfo());
                    sequencer.set_player(player);
                    sequencer.Work(target_frames);

                    const int rendered = target_frames;
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
                            << ",\"target_beats\":4.25"
                            << ",\"target_frames\":" << target_frames
                            << ",\"threads\":1"
                            << ",\"sequencer_work_calls\":1"
                            << ",\"player_work_direct\":false"
                            << ",\"master_buffer_float_count\":"
                            << master_output.size()
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
