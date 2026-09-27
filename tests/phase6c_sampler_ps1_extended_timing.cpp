// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime probe for frozen-candidate PS1 E-D3/E-C3 sample-counter timing.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/instrument.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/player.h>
#include <psycle/core/playertimeinfo.h>
#include <psycle/core/sampler.h>
#include <psycle/core/pattern.h>
#include <psycle/core/patternevent.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
struct LoadedEvent {
    double position;
    int track;
    psycle::core::PatternEvent event;
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

int find_voice(psycle::core::Sampler& sampler, int channel) {
    for (int i = 0; i < sampler._numVoices; ++i) {
        if (sampler._voices[i]._channel == channel)
            return i;
    }
    return -1;
}

bool consume_before_boundary(
    psycle::core::Sampler& sampler,
    int voice,
    int target_samples)
{
    int remaining = target_samples - 1;
    while (remaining > 0) {
        const int chunk = remaining > 128 ? 128 : remaining;
        sampler.VoiceWork(0, chunk, voice);
        remaining -= chunk;
    }
    return true;
}

bool inject_pcm(
    psycle::core::CoreSong& song,
    const char* pcm_path)
{
    std::ifstream input(pcm_path, std::ios::binary | std::ios::ate);
    const std::streamoff size =
        input ? static_cast<std::streamoff>(input.tellg()) : -1;
    if (!input || size != 44100 * 2)
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

    instrument->waveLength = 44100;
    instrument->waveVolume = 100;
    instrument->waveLoopStart = 0;
    instrument->waveLoopEnd = 0;
    instrument->waveTune = 0;
    instrument->waveFinetune = 0;
    instrument->waveLoopType = false;
    instrument->waveStereo = false;
    instrument->waveDataL = new std::int16_t[44100];
    instrument->waveDataR = 0;
    for (std::size_t frame = 0; frame < 44100u; ++frame) {
        const std::uint16_t bits =
            static_cast<std::uint16_t>(bytes[frame * 2u])
            | (static_cast<std::uint16_t>(bytes[frame * 2u + 1u]) << 8);
        instrument->waveDataL[frame] = static_cast<std::int16_t>(bits);
    }
    return true;
}

void emit_common(
    const char* variant,
    psycle::core::CoreSong& song,
    const psycle::core::PlayerTimeInfo& info,
    int target)
{
    std::cout << std::setprecision(15)
              << "{\"schema_version\":1"
              << ",\"variant\":\"" << variant << "\""
              << ",\"load_returned\":true"
              << ",\"bpm\":" << song.bpm()
              << ",\"tick_speed\":" << song.tick_speed()
              << ",\"is_ticks\":" << (song.is_ticks() ? "true" : "false")
              << ",\"sample_rate\":44100"
              << ",\"samples_per_beat\":" << info.samplesPerBeat()
              << ",\"samples_per_tick\":" << info.samplesPerTick()
              << ",\"timing_parameter\":3"
              << ",\"trigger_samples\":" << target;
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
        if (threads == 0 || std::string(threads) != "1")
            return 69;

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
                psycle::core::PlayerTimeInfo info;
                info.setSampleRate(44100);
                info.setBpm(song.bpm());
                info.setTicksSpeed(song.tick_speed(), song.is_ticks());
                player.timeInfo(info);

                if (song.bpm() != 120.0f ||
                    song.tick_speed() != 4 ||
                    !song.is_ticks()) {
                    std::cerr << "loaded timing metadata changed\n";
                    result = 67;
                } else {
                    psycle::core::Sampler* sampler =
                        dynamic_cast<psycle::core::Sampler*>(song.machine(0));
                    if (sampler == 0) {
                        std::cerr << "loaded PS1 Sampler missing\n";
                        result = 68;
                    } else {
                        const int target = static_cast<int>(
                            (info.samplesPerTick() / 6.0) * 3.0);
                        const std::vector<LoadedEvent> events =
                            loaded_events(song);

                        if (variant == "delay") {
                            psycle::core::PatternEvent delayed;
                            bool found = false;
                            double position = -1.0;
                            for (std::size_t i = 0; i < events.size(); ++i) {
                                if (events[i].event.command() == 0x0e &&
                                    events[i].event.parameter() == 0xd3) {
                                    delayed = events[i].event;
                                    position = events[i].position;
                                    found = true;
                                }
                            }
                            if (!found || position != 0.0) {
                                std::cerr << "loaded E-D3 event missing\n";
                                result = 70;
                            } else {
                                sampler->Tick(0, delayed);
                                const int voice = find_voice(*sampler, 0);
                                if (voice < 0 ||
                                    sampler->_voices[voice]._triggerNoteDelay != target ||
                                    sampler->_voices[voice]._envelope._stage !=
                                        psycle::core::ENV_OFF) {
                                    std::cerr << "E-D3 trigger setup mismatch\n";
                                    result = 71;
                                } else {
                                    consume_before_boundary(*sampler, voice, target);
                                    const bool before =
                                        sampler->_voices[voice]._sampleCounter == target - 1 &&
                                        sampler->_voices[voice]._triggerNoteDelay == target &&
                                        sampler->_voices[voice]._envelope._stage ==
                                            psycle::core::ENV_OFF;
                                    sampler->VoiceWork(0, 1, voice);
                                    const bool fired =
                                        sampler->_voices[voice]._sampleCounter == target &&
                                        sampler->_voices[voice]._triggerNoteDelay == 0 &&
                                        sampler->_voices[voice]._envelope._stage ==
                                            psycle::core::ENV_ATTACK;
                                    emit_common("delay", song, info, target);
                                    std::cout
                                        << ",\"command\":\"E-D3\""
                                        << ",\"event_position_beats\":" << position
                                        << ",\"before_boundary_preserved\":"
                                        << (before ? "true" : "false")
                                        << ",\"trigger_fired_at_boundary\":"
                                        << (fired ? "true" : "false")
                                        << ",\"absolute_trigger_beats\":"
                                        << (position + target / info.samplesPerBeat())
                                        << "}" << std::endl;
                                    if (!before || !fired) result = 72;
                                }
                            }
                        } else {
                            psycle::core::PatternEvent note;
                            psycle::core::PatternEvent noteoff;
                            bool have_note = false;
                            bool have_noteoff = false;
                            double command_position = -1.0;
                            for (std::size_t i = 0; i < events.size(); ++i) {
                                if (events[i].event.note() == 60 &&
                                    events[i].event.command() == 0) {
                                    note = events[i].event;
                                    have_note = true;
                                } else if (
                                    events[i].event.command() == 0x0e &&
                                    events[i].event.parameter() == 0xc3) {
                                    noteoff = events[i].event;
                                    command_position = events[i].position;
                                    have_noteoff = true;
                                }
                            }
                            if (!have_note || !have_noteoff ||
                                command_position != 0.25) {
                                std::cerr << "loaded E-C3 sequence missing\n";
                                result = 73;
                            } else {
                                sampler->Tick(0, note);
                                const int voice = find_voice(*sampler, 0);
                                if (voice < 0 ||
                                    sampler->_voices[voice]._envelope._stage ==
                                        psycle::core::ENV_OFF) {
                                    std::cerr << "E-C3 setup note did not start\n";
                                    result = 74;
                                } else {
                                    sampler->Tick(0, noteoff);
                                    const bool armed =
                                        sampler->_voices[voice]._triggerNoteOff == target;
                                    consume_before_boundary(*sampler, voice, target);
                                    const bool before =
                                        sampler->_voices[voice]._sampleCounter == target - 1 &&
                                        sampler->_voices[voice]._triggerNoteOff == target &&
                                        sampler->_voices[voice]._envelope._stage !=
                                            psycle::core::ENV_RELEASE;
                                    sampler->VoiceWork(0, 1, voice);
                                    const bool fired =
                                        sampler->_voices[voice]._sampleCounter == target &&
                                        sampler->_voices[voice]._triggerNoteOff == 0 &&
                                        sampler->_voices[voice]._envelope._stage ==
                                            psycle::core::ENV_RELEASE;
                                    emit_common("noteoff", song, info, target);
                                    std::cout
                                        << ",\"command\":\"E-C3\""
                                        << ",\"command_position_beats\":"
                                        << command_position
                                        << ",\"trigger_armed\":"
                                        << (armed ? "true" : "false")
                                        << ",\"before_boundary_preserved\":"
                                        << (before ? "true" : "false")
                                        << ",\"trigger_fired_at_boundary\":"
                                        << (fired ? "true" : "false")
                                        << ",\"absolute_trigger_beats\":"
                                        << (command_position +
                                            target / info.samplesPerBeat())
                                        << "}" << std::endl;
                                    if (!armed || !before || !fired) result = 75;
                                }
                            }
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
