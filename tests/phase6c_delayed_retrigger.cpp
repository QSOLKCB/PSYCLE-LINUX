// SPDX-License-Identifier: GPL-2.0-or-later
// Project-authored observation harness for delayed/retrigger command scheduling.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/player.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/playertimeinfo.h>
#include <psycle/core/sampler.h>
#include <psycle/core/pattern.h>
#include <psycle/core/patternevent.h>
#include <psycle/core/commands.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

#define private public
#include <psycle/core/sequencer.h>
#undef private

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::vector<std::string> reports;

void report(const std::string& message, const std::string& title) {
    reports.push_back(title + ": " + message);
}

std::string quoted(const std::string& text) {
    std::ostringstream out;
    out << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') out << '\\' << c;
        else if (c < 32) {
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << int(c) << std::dec;
        } else out << c;
    }
    out << '"';
    return out.str();
}

struct Capture {
    double offset;
    int track;
    int note;
    int command;
    int parameter;
};

class RecordingSampler : public psycle::core::Sampler {
public:
    RecordingSampler(psycle::core::MachineCallbacks* callbacks, int id)
        : Sampler(callbacks, id) {}

    void AddEvent(
        double offset, int track,
        const psycle::core::PatternEvent& event) override {
        captures.push_back(Capture{
            offset, track, event.note(), event.command(), event.parameter()
        });
    }

    std::vector<Capture> captures;
};

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

void emit_captures(const std::vector<Capture>& captures) {
    std::cout << '[';
    for (std::size_t i = 0; i < captures.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << "{\"offset\":" << captures[i].offset
                  << ",\"track\":" << captures[i].track
                  << ",\"note\":" << captures[i].note
                  << ",\"command\":" << captures[i].command
                  << ",\"parameter\":" << captures[i].parameter << '}';
    }
    std::cout << ']';
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 64;

    try {
        using namespace universalis::os::loggers;
        static stream_logger diagnostics(std::cerr);
        multiplex_logger::singleton().add(diagnostics);
        universalis::os::thread_name thread_name("delayed-retrigger-probe");

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;
            player.song(song);
            song.report.connect(&report);
            if (!song.load(argv[1])) {
                std::cout << "{\"schema_version\":1,\"load_returned\":false}"
                          << std::endl;
                result = 65;
            } else {
                std::vector<LoadedEvent> events = loaded_events(song);
                RecordingSampler* recorder = new RecordingSampler(
                    factory.getCallbacks(), 0);
                recorder->Init();
                song.ReplaceMachine(recorder, 0);

                psycle::core::PlayerTimeInfo info;
                info.setSampleRate(44100);
                info.setBpm(song.bpm());
                info.setTicksSpeed(song.tick_speed(), song.is_ticks());

                psycle::core::Sequencer sequencer;
                sequencer.set_song(song);
                sequencer.set_time_info(info);
                sequencer.set_player(player);

                std::vector<Capture> note_delay, retrigger, retr_cont;
                double marker_position = -1.0;
                int loaded_note_delay_parameter = -1;
                int loaded_retrigger_parameter = -1;
                int loaded_retr_cont_parameter = -1;
                int loaded_extended_parameter = -1;

                for (std::size_t i = 0; i < events.size(); ++i) {
                    psycle::core::PatternEvent event = events[i].event;
                    if (event.command() == psycle::core::commandtypes::NOTE_DELAY) {
                        loaded_note_delay_parameter = event.parameter();
                        recorder->captures.clear();
                        sequencer.execute_notes(events[i].position, events[i].track, event);
                        note_delay = recorder->captures;
                    } else if (event.command() == psycle::core::commandtypes::RETRIGGER) {
                        loaded_retrigger_parameter = event.parameter();
                        recorder->captures.clear();
                        sequencer.execute_notes(events[i].position, events[i].track, event);
                        retrigger = recorder->captures;
                    } else if (event.command() == psycle::core::commandtypes::RETR_CONT) {
                        loaded_retr_cont_parameter = event.parameter();
                        recorder->captures.clear();
                        sequencer.execute_notes(events[i].position, events[i].track, event);
                        retr_cont = recorder->captures;
                    } else if (event.command() == psycle::core::commandtypes::EXTENDED) {
                        loaded_extended_parameter = event.parameter();
                    } else if (event.note() == 55 && events[i].track == 4) {
                        marker_position = events[i].position;
                    }
                }

                std::cout << std::setprecision(15)
                          << "{\"schema_version\":1"
                          << ",\"load_returned\":true"
                          << ",\"song_name\":" << quoted(song.name())
                          << ",\"bpm\":" << song.bpm()
                          << ",\"tick_speed\":" << song.tick_speed()
                          << ",\"is_ticks\":" << (song.is_ticks() ? "true" : "false")
                          << ",\"sample_rate\":44100"
                          << ",\"samples_per_beat\":" << info.samplesPerBeat()
                          << ",\"samples_per_tick\":" << info.samplesPerTick()
                          << ",\"loaded_parameters\":{"
                          << "\"note_delay\":" << loaded_note_delay_parameter
                          << ",\"retrigger\":" << loaded_retrigger_parameter
                          << ",\"retr_cont\":" << loaded_retr_cont_parameter
                          << ",\"extended_lpb\":" << loaded_extended_parameter
                          << "}"
                          << ",\"marker_position_after_extended\":" << marker_position
                          << ",\"note_delay_events\":";
                emit_captures(note_delay);
                std::cout << ",\"retrigger_events\":";
                emit_captures(retrigger);
                std::cout << ",\"retr_cont_events\":";
                emit_captures(retr_cont);
                std::cout << ",\"reports\":[";
                for (std::size_t i = 0; i < reports.size(); ++i) {
                    if (i) std::cout << ',';
                    std::cout << quoted(reports[i]);
                }
                std::cout << "]}" << std::endl;
            }
        }

        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "delayed-retrigger probe exception: " << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "delayed-retrigger probe unknown exception" << std::endl;
        return 70;
    }
}
