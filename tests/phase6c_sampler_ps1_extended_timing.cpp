// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime audio probe for frozen-candidate PS1 E-D3/E-C3 timing.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/instrument.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/player.h>
#include <psycle/core/playertimeinfo.h>
#include <psycle/core/sampler.h>
#include <psycle/core/sequencer.h>
#include <psycle/core/internal_machines.h>
#include <psycle/core/constants.h>
#include <psycle/core/pattern.h>
#include <psycle/core/patternevent.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace {
const int kSampleRate = 44100;
const int kRenderFrames = 22050;
const float kActiveThreshold = 64.0f;

struct LoadedEvent {
    double position;
    int track;
    psycle::core::PatternEvent event;
};

struct AudioObservation {
    bool active;
    int first_active_frame;
    int last_active_frame;
    int active_frame_count;
    double final_play_beat;
};

std::vector<LoadedEvent> loaded_events(psycle::core::CoreSong& song) {
    std::vector<LoadedEvent> result;
    psycle::core::Sequence& sequence = song.sequence();
    for (psycle::core::Sequence::iterator line_it = sequence.begin();
         line_it != sequence.end(); ++line_it) {
        psycle::core::SequenceLine* line = *line_it;
        for (psycle::core::SequenceLine::iterator entry_it = line->begin();
             entry_it != line->end(); ++entry_it) {
            psycle::core::Pattern& pattern = entry_it->second->pattern();
            if (pattern.id() < 0) continue;
            for (psycle::core::Pattern::iterator it = pattern.begin();
                 it != pattern.end(); ++it) {
                result.push_back(LoadedEvent{
                    it->first.first, it->first.second, it->second
                });
            }
            return result;
        }
    }
    return result;
}

bool inject_pcm(
    psycle::core::CoreSong& song,
    const char* pcm_path)
{
    std::ifstream input(pcm_path, std::ios::binary | std::ios::ate);
    const std::streamoff size =
        input ? static_cast<std::streamoff>(input.tellg()) : -1;
    if (!input || size != kSampleRate * 2)
        return false;
    input.seekg(0, std::ios::beg);
    std::vector<unsigned char> bytes(
        static_cast<std::size_t>(size), 0u);
    input.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(bytes.size()));
    if (!input)
        return false;

    psycle::core::Instrument* instrument = song._pInstrument[0];
    if (instrument == 0 ||
        instrument->waveLength != 0 ||
        instrument->waveDataL != 0)
        return false;

    instrument->waveLength = kSampleRate;
    instrument->waveVolume = 100;
    instrument->waveLoopStart = 0;
    instrument->waveLoopEnd = 0;
    instrument->waveTune = 0;
    instrument->waveFinetune = 0;
    instrument->waveLoopType = false;
    instrument->waveStereo = false;
    instrument->waveDataL = new std::int16_t[kSampleRate];
    instrument->waveDataR = 0;
    for (std::size_t frame = 0;
         frame < static_cast<std::size_t>(kSampleRate); ++frame) {
        const std::uint16_t bits =
            static_cast<std::uint16_t>(bytes[frame * 2u])
            | (static_cast<std::uint16_t>(bytes[frame * 2u + 1u]) << 8);
        instrument->waveDataL[frame] = static_cast<std::int16_t>(bits);
    }
    return true;
}

AudioObservation render_through_production_path(
    psycle::core::CoreSong& song,
    psycle::core::Player& player)
{
    std::vector<float> master_output(
        static_cast<std::size_t>(kRenderFrames) * 2u, 0.0f);
    psycle::core::Master& master =
        static_cast<psycle::core::Master&>(
            *song.machine(psycle::core::MASTER_INDEX));
    master._pMasterSamples = master_output.data();

    player.start(0.0);

    psycle::core::Sequencer sequencer;
    sequencer.set_song(song);
    sequencer.set_time_info(player.timeInfo());
    sequencer.set_player(player);
    sequencer.Work(kRenderFrames);

    const double final_play_beat = player.playPos();
    player.stop();

    int first = -1;
    int last = -1;
    int count = 0;
    for (int frame = 0; frame < kRenderFrames; ++frame) {
        const float left =
            master_output[static_cast<std::size_t>(frame) * 2u];
        const float right =
            master_output[static_cast<std::size_t>(frame) * 2u + 1u];
        if (std::fabs(left) > kActiveThreshold ||
            std::fabs(right) > kActiveThreshold) {
            if (first < 0) first = frame;
            last = frame;
            ++count;
        }
    }

    AudioObservation observation;
    observation.active = first >= 0;
    observation.first_active_frame = first;
    observation.last_active_frame = last;
    observation.active_frame_count = count;
    observation.final_play_beat = final_play_beat;
    return observation;
}

void emit_frame(const char* key, int value) {
    std::cout << ",\"" << key << "\":";
    if (value < 0) std::cout << "null";
    else std::cout << value;
}

void emit_common(
    const char* variant,
    psycle::core::CoreSong& song,
    const psycle::core::PlayerTimeInfo& info,
    int target,
    const AudioObservation& audio)
{
    std::cout << std::setprecision(
                  std::numeric_limits<double>::max_digits10)
              << "{\"schema_version\":1"
              << ",\"variant\":\"" << variant << "\""
              << ",\"load_returned\":true"
              << ",\"bpm\":" << song.bpm()
              << ",\"tick_speed\":" << song.tick_speed()
              << ",\"is_ticks\":" << (song.is_ticks() ? "true" : "false")
              << ",\"sample_rate\":" << kSampleRate
              << ",\"samples_per_beat\":" << info.samplesPerBeat()
              << ",\"samples_per_tick\":" << info.samplesPerTick()
              << ",\"timing_parameter\":3"
              << ",\"trigger_samples\":" << target
              << ",\"processing_path\":"
                 "\"sequencer-player-production-blocks\""
              << ",\"player_max_work_block_samples\":"
              << psycle::core::MAX_BUFFER_LENGTH
              << ",\"render_frames\":" << kRenderFrames
              << ",\"active_threshold_abs_float\":"
              << kActiveThreshold
              << ",\"audio_active\":"
              << (audio.active ? "true" : "false")
              << ",\"active_frame_count\":"
              << audio.active_frame_count
              << ",\"final_play_beat\":"
              << audio.final_play_beat;
    emit_frame("first_active_frame", audio.first_active_frame);
    emit_frame("last_active_frame", audio.last_active_frame);
}
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0]
                  << " VARIANT CANDIDATE_PSY3 PCM16LE\n";
        return 64;
    }
    const std::string variant(argv[1]);
    if (variant != "delay" && variant != "noteoff")
        return 64;

    try {
        using namespace universalis::os::loggers;
        static stream_logger diagnostics(std::cerr);
        multiplex_logger::singleton().add(diagnostics);
        universalis::os::thread_name thread_name("ps1-extended-timing");

        const char* threads = std::getenv("PSYCLE_THREADS");
        if (threads == 0 || std::string(threads) != "1") {
            std::cerr << "candidate timing probe requires PSYCLE_THREADS=1\n";
            return 69;
        }

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;
            player.song(song);
            if (!song.load(argv[2])) {
                std::cout
                    << "{\"schema_version\":1,\"load_returned\":false}"
                    << std::endl;
                result = 65;
            } else if (!inject_pcm(song, argv[3])) {
                std::cerr << "candidate PCM injection failed\n";
                result = 66;
            } else {
                psycle::core::PlayerTimeInfo& info = player.timeInfo();
                info.setSampleRate(kSampleRate);
                info.setBpm(song.bpm());
                info.setTicksSpeed(song.tick_speed(), song.is_ticks());

                if (song.bpm() != 120.0f ||
                    song.tick_speed() != 4 ||
                    !song.is_ticks()) {
                    std::cerr << "loaded timing metadata changed\n";
                    result = 67;
                } else if (
                    dynamic_cast<psycle::core::Sampler*>(song.machine(0)) == 0
                ) {
                    std::cerr << "loaded PS1 Sampler missing\n";
                    result = 68;
                } else {
                    const int target = static_cast<int>(
                        (info.samplesPerTick() / 6.0) * 3.0);
                    const std::vector<LoadedEvent> events =
                        loaded_events(song);
                    bool fixture_ok = false;
                    double command_position = -1.0;

                    if (variant == "delay") {
                        for (std::size_t i = 0; i < events.size(); ++i) {
                            if (events[i].position == 0.0 &&
                                events[i].event.note() == 60 &&
                                events[i].event.command() == 0x0e &&
                                events[i].event.parameter() == 0xd3) {
                                fixture_ok = true;
                                command_position = events[i].position;
                            }
                        }
                        if (!fixture_ok) {
                            std::cerr << "loaded E-D3 event missing\n";
                            result = 70;
                        }
                    } else {
                        bool have_note = false;
                        bool have_noteoff = false;
                        for (std::size_t i = 0; i < events.size(); ++i) {
                            if (events[i].position == 0.0 &&
                                events[i].event.note() == 60 &&
                                events[i].event.command() == 0) {
                                have_note = true;
                            } else if (
                                events[i].position == 0.25 &&
                                events[i].event.command() == 0x0e &&
                                events[i].event.parameter() == 0xc3) {
                                have_noteoff = true;
                                command_position = events[i].position;
                            }
                        }
                        fixture_ok = have_note && have_noteoff;
                        if (!fixture_ok) {
                            std::cerr << "loaded E-C3 sequence missing\n";
                            result = 73;
                        }
                    }

                    if (result == 0) {
                        const AudioObservation audio =
                            render_through_production_path(song, player);
                        emit_common(
                            variant.c_str(), song, info, target, audio);
                        if (variant == "delay") {
                            std::cout
                                << ",\"command\":\"E-D3\""
                                << ",\"event_position_beats\":"
                                << command_position
                                << ",\"semantic_boundary_frame\":"
                                << target
                                << ",\"absolute_trigger_beats\":"
                                << (command_position +
                                    static_cast<double>(target) /
                                    static_cast<double>(info.samplesPerBeat()))
                                << "}" << std::endl;
                        } else {
                            const int command_frame = static_cast<int>(
                                command_position * info.samplesPerBeat());
                            std::cout
                                << ",\"command\":\"E-C3\""
                                << ",\"command_position_beats\":"
                                << command_position
                                << ",\"command_position_frame\":"
                                << command_frame
                                << ",\"semantic_boundary_frame\":"
                                << (command_frame + target)
                                << ",\"absolute_trigger_beats\":"
                                << (command_position +
                                    static_cast<double>(target) /
                                    static_cast<double>(info.samplesPerBeat()))
                                << "}" << std::endl;
                    }
                    }
                }
            }
        }
        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "PS1 extended timing probe exception: "
                  << error.what() << std::endl;
        return 76;
    } catch (...) {
        std::cerr << "PS1 extended timing probe unknown exception\n";
        return 76;
    }
}
