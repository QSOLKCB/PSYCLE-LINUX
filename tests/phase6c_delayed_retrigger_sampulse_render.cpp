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
#include "phase6c-render-provenance.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

static std::uint32_t rotr32(std::uint32_t value, unsigned int count) {
    return (value >> count) | (value << (32u - count));
}

static std::string sha256_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return std::string();
    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    const std::uint64_t bit_length =
        static_cast<std::uint64_t>(data.size()) * 8u;
    data.push_back(0x80u);
    while ((data.size() % 64u) != 56u) data.push_back(0u);
    for (int shift = 56; shift >= 0; shift -= 8) {
        data.push_back(
            static_cast<unsigned char>((bit_length >> shift) & 0xffu));
    }

    static const std::uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    std::uint32_t h[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };

    for (std::size_t offset = 0; offset < data.size(); offset += 64u) {
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            const std::size_t p = offset + static_cast<std::size_t>(i) * 4u;
            w[i] =
                (static_cast<std::uint32_t>(data[p]) << 24)
                | (static_cast<std::uint32_t>(data[p + 1]) << 16)
                | (static_cast<std::uint32_t>(data[p + 2]) << 8)
                | static_cast<std::uint32_t>(data[p + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 =
                rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18)
                ^ (w[i - 15] >> 3);
            const std::uint32_t s1 =
                rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19)
                ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        std::uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 =
                rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            const std::uint32_t ch = (e & f) ^ ((~e) & g);
            const std::uint32_t temp1 = hh + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 =
                rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + maj;
            hh = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }

    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) hex << std::setw(8) << h[i];
    return hex.str();
}

static std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
        const unsigned char ch = static_cast<unsigned char>(*it);
        if (ch == '\\' || ch == '"') {
            out << '\\' << static_cast<char>(ch);
        } else if (ch == '\n') {
            out << "\\n";
        } else if (ch == '\r') {
            out << "\\r";
        } else if (ch == '\t') {
            out << "\\t";
        } else if (ch < 0x20u) {
            out << "\\u00" << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<unsigned int>(ch) << std::dec;
        } else {
            out << static_cast<char>(ch);
        }
    }
    return out.str();
}

static void print_compiled_provenance() {
    std::cout
        << "{\"schema_version\":1"
        << ",\"renderer_source_sha256\":\""
        << PHASE6C_RENDER_SOURCE_SHA256 << "\""
        << ",\"renderer_project_sha256\":\""
        << PHASE6C_RENDER_PROJECT_SHA256 << "\""
        << ",\"engine_sequencer_sha256\":\""
        << PHASE6C_ENGINE_SEQUENCER_SHA256 << "\""
        << ",\"engine_psy3_loader_sha256\":\""
        << PHASE6C_ENGINE_PSY3_LOADER_SHA256 << "\""
        << ",\"engine_xmsampler_sha256\":\""
        << PHASE6C_ENGINE_XMSAMPLER_SHA256 << "\""
        << "}" << std::endl;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--phase6c-provenance") {
        print_compiled_provenance();
        return 0;
    }
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

        std::ifstream input_fixture(argv[1], std::ios::binary | std::ios::ate);
        std::streamoff input_size = static_cast<std::streamoff>(-1);
        if (input_fixture) {
            input_size = static_cast<std::streamoff>(input_fixture.tellg());
        }
        if (!input_fixture || input_size <= 20) {
            std::cerr << "candidate fixture input missing or empty\n";
            return 72;
        }
        input_fixture.close();
        const std::string input_sha256 = sha256_file(argv[1]);
        if (input_sha256.size() != 64u) {
            std::cerr << "candidate fixture SHA-256 failed\n";
            return 72;
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
            } else if (sha256_file(argv[1]) != input_sha256) {
                std::cerr << "shared Sampulse witness changed during load\n";
                result = 73;
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

                    const double final_beat = player.playPos();
                    player.stop();
                    player.stopRecording();

                    std::ifstream output(argv[2], std::ios::binary | std::ios::ate);
                    std::streamoff output_size = static_cast<std::streamoff>(-1);
                    if (output) {
                        output_size = static_cast<std::streamoff>(output.tellg());
                    }
                    if (!output || output_size <= 44) {
                        std::cerr << "candidate render output missing or empty\n";
                        result = 67;
                    } else {
                        output.close();
                        const std::string output_sha256 = sha256_file(argv[2]);
                        if (output_sha256.size() != 64u) {
                            std::cerr << "candidate render SHA-256 failed\n";
                            result = 71;
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
                            << ",\"renderer_source_sha256\":\""
                            << PHASE6C_RENDER_SOURCE_SHA256 << "\""
                            << ",\"renderer_project_sha256\":\""
                            << PHASE6C_RENDER_PROJECT_SHA256 << "\""
                            << ",\"engine_sequencer_sha256\":\""
                            << PHASE6C_ENGINE_SEQUENCER_SHA256 << "\""
                            << ",\"engine_psy3_loader_sha256\":\""
                            << PHASE6C_ENGINE_PSY3_LOADER_SHA256 << "\""
                            << ",\"engine_xmsampler_sha256\":\""
                            << PHASE6C_ENGINE_XMSAMPLER_SHA256 << "\""
                            << ",\"master_buffer_float_count\":"
                            << master_output.size()
                            << ",\"final_play_beat\":" << final_beat
                            << ",\"input_path\":\""
                            << json_escape(argv[1]) << "\""
                            << ",\"input_size_bytes\":"
                            << static_cast<long long>(input_size)
                            << ",\"input_sha256\":\""
                            << input_sha256 << "\""
                            << ",\"output_path\":\""
                            << json_escape(argv[2]) << "\""
                            << ",\"output_size_bytes\":"
                            << static_cast<long long>(output_size)
                            << ",\"output_sha256\":\""
                            << output_sha256 << "\""
                            << "}" << std::endl;
                        }
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
