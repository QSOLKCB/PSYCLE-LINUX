// SPDX-License-Identifier: GPL-2.0-or-later
// Project-authored observation harness for the preserved Psycle C++ engine.
// No engine implementation is replaced or patched by this probe.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/player.h>
#include <psycle/core/machinefactory.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

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
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c) << std::dec;
        else out << c;
    }
    out << '"';
    return out.str();
}
std::string state(const psycle::core::CoreSong& song) {
    std::ostringstream out;
    out << "{\"name\":" << quoted(song.name()) << ",\"author\":" << quoted(song.author())
        << ",\"comment\":" << quoted(song.comment()) << ",\"bpm\":" << song.bpm()
        << ",\"tick_speed\":" << song.tick_speed() << ",\"tracks\":" << song.tracks()
        << ",\"machine_slots\":[";
    bool first = true;
    for (int i = 0; i < psycle::core::MAX_MACHINES; ++i) {
        if (song.machine(i)) { if (!first) out << ','; out << i; first = false; }
    }
    out << "]}";
    return out.str();
}
}

int main(int argc, char** argv) {
    if (argc != 4) return 64; // input, format version, absent output path
    const std::string version_text(argv[2]);
    if (version_text != "2" && version_text != "3" && version_text != "4") return 64;
    const int version = std::atoi(argv[2]);
    try {
        using namespace universalis::os::loggers;
        multiplex_logger::singleton().add(stream_logger::default_logger());
        universalis::os::thread_name thread_name("serialization-probe");
        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory = psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);
        {
            psycle::core::CoreSong song;
            player.song(song);
            song.report.connect(&report);
            const bool loaded = song.load(argv[1]);
            const std::string before = state(song);
            const bool saved = loaded && song.save(argv[3], version);
            const std::string after = state(song);
            std::cout << "{\"schema_version\":1,\"load_returned\":" << (loaded ? "true" : "false")
                      << ",\"save_attempted\":" << (loaded ? "true" : "false")
                      << ",\"format_version\":" << version
                      << ",\"save_returned\":" << (saved ? "true" : "false")
                      << ",\"state_before\":" << before << ",\"state_after\":" << after
                      << ",\"reports\":[";
            for (std::size_t i=0; i<reports.size(); ++i) { if (i) std::cout << ','; std::cout << quoted(reports[i]); }
            std::cout << "]}" << std::endl;
        }
        factory.Finalize();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "serialization probe exception: " << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "serialization probe unknown exception" << std::endl;
        return 70;
    }
}
