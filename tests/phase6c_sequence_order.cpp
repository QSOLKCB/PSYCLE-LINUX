// SPDX-License-Identifier: GPL-2.0-or-later
// Project-authored observation harness for the preserved Psycle C++ engine.
// The probe only loads a fixture and reports the public sequence model.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/player.h>
#include <psycle/core/machinefactory.h>
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

void emit_sequence(const psycle::core::CoreSong& song) {
    const psycle::core::Sequence& sequence = song.sequence();
    std::cout << "{\"schema_version\":1"
              << ",\"load_returned\":true"
              << ",\"song_name\":" << quoted(song.name())
              << ",\"sequence_lines\":[";

    bool first_line = true;
    int line_index = 0;
    for (psycle::core::Sequence::const_iterator line_it = sequence.begin();
         line_it != sequence.end(); ++line_it, ++line_index) {
        const psycle::core::SequenceLine* line = *line_it;
        if (!first_line) std::cout << ',';
        first_line = false;
        std::cout << "{\"line_index\":" << line_index << ",\"entries\":[";

        bool first_entry = true;
        for (psycle::core::SequenceLine::const_iterator entry_it = line->begin();
             entry_it != line->end(); ++entry_it) {
            const psycle::core::SequenceEntry* entry = entry_it->second;
            if (!first_entry) std::cout << ',';
            first_entry = false;
            std::cout << "{\"position\":" << std::setprecision(12)
                      << entry_it->first
                      << ",\"pattern_id\":" << entry->pattern().id()
                      << ",\"pattern_name\":"
                      << quoted(entry->pattern().name()) << '}';
        }
        std::cout << "]}";
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
        universalis::os::thread_name thread_name("sequence-order-probe");

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
                emit_sequence(song);
            }
        }

        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "sequence order probe exception: " << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "sequence order probe unknown exception" << std::endl;
        return 70;
    }
}
