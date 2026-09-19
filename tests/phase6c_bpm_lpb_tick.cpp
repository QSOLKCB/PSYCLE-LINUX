// SPDX-License-Identifier: GPL-2.0-or-later
// Project-authored observation harness for the preserved Psycle C++ engine.
// It loads one timing fixture and reports model/timing values without playback.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/player.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/playertimeinfo.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>

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

std::vector<double> marker_positions(const psycle::core::CoreSong& song) {
    const psycle::core::Sequence& sequence = song.sequence();
    for (psycle::core::Sequence::const_iterator line_it = sequence.begin();
         line_it != sequence.end(); ++line_it) {
        const psycle::core::SequenceLine* line = *line_it;
        for (psycle::core::SequenceLine::const_iterator entry_it = line->begin();
             entry_it != line->end(); ++entry_it) {
            const psycle::core::Pattern& pattern = entry_it->second->pattern();
            if (pattern.id() < 0) continue;
            std::vector<double> positions;
            for (psycle::core::Pattern::const_iterator event = pattern.begin();
                 event != pattern.end(); ++event) {
                if (event->first.second == 0) positions.push_back(event->first.first);
            }
            if (!positions.empty()) return positions;
        }
    }
    return std::vector<double>();
}

void emit_timing(const psycle::core::CoreSong& song) {
    const std::vector<double> positions = marker_positions(song);
    double derived_lpb = 0.0;
    if (positions.size() >= 2) {
        const double spacing = positions[1] - positions[0];
        if (spacing > 0.0) derived_lpb = 1.0 / spacing;
    }

    std::cout << std::setprecision(15)
              << "{\"schema_version\":1"
              << ",\"load_returned\":true"
              << ",\"song_name\":" << quoted(song.name())
              << ",\"bpm\":" << song.bpm()
              << ",\"tick_speed\":" << song.tick_speed()
              << ",\"is_ticks\":" << (song.is_ticks() ? "true" : "false")
              << ",\"marker_positions\":[";
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << positions[i];
    }
    std::cout << "]"
              << ",\"derived_lpb\":" << derived_lpb
              << ",\"sample_rates\":[";

    const int rates[] = {44100, 48000};
    for (std::size_t i = 0; i < sizeof(rates) / sizeof(rates[0]); ++i) {
        psycle::core::PlayerTimeInfo time_info;
        time_info.setSampleRate(rates[i]);
        time_info.setBpm(song.bpm());
        time_info.setTicksSpeed(song.tick_speed(), song.is_ticks());
        if (i) std::cout << ',';
        std::cout << "{\"sample_rate\":" << rates[i]
                  << ",\"samples_per_beat\":" << time_info.samplesPerBeat()
                  << ",\"samples_per_tick\":" << time_info.samplesPerTick()
                  << ",\"samples_per_fixture_line\":"
                  << (time_info.samplesPerBeat() / derived_lpb)
                  << '}';
    }

    std::cout << "],\"reports\":[";
    for (std::size_t i = 0; i < reports.size(); ++i) {
        if (i) std::cout << ',';
        std::cout << quoted(reports[i]);
    }
    std::cout << "]}" << std::endl;
}
}

int main(int argc, char** argv) {
    if (argc != 2) return 64;

    try {
        using namespace universalis::os::loggers;
        static stream_logger diagnostics(std::cerr);
        multiplex_logger::singleton().add(diagnostics);
        universalis::os::thread_name thread_name("bpm-lpb-tick-probe");

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;
            player.song(song);
            song.report.connect(&report);
            const bool loaded = song.load(argv[1]);
            if (!loaded) {
                std::cout << "{\"schema_version\":1,\"load_returned\":false"
                          << ",\"reports\":[";
                for (std::size_t i = 0; i < reports.size(); ++i) {
                    if (i) std::cout << ',';
                    std::cout << quoted(reports[i]);
                }
                std::cout << "]}" << std::endl;
                result = 65;
            } else {
                emit_timing(song);
            }
        }

        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "BPM/LPB/tick probe exception: " << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "BPM/LPB/tick probe unknown exception" << std::endl;
        return 70;
    }
}
