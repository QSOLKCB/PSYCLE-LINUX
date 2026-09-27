// SPDX-License-Identifier: GPL-2.0-or-later
// Fixed-frame frozen-candidate render for the Phase 6C PS1 pitch witness.
#include <psycle/core/detail/project.private.hpp>
#include <psycle/core/song.h>
#include <psycle/core/instrument.h>
#include <psycle/core/machinefactory.h>
#include <psycle/core/player.h>
#include <psycle/core/sequencer.h>
#include <psycle/core/internal_machines.h>
#include <psycle/audiodrivers/audiodriver.h>
#include <universalis/os/loggers.hpp>
#include <universalis/os/thread_name.hpp>
#include "phase6c-ps1-pitch-provenance.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
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
        std::uint32_t a=h[0],b=h[1],c=h[2],d=h[3];
        std::uint32_t e=h[4],f=h[5],g=h[6],hh=h[7];
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 =
                rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
            const std::uint32_t ch=(e&f)^((~e)&g);
            const std::uint32_t t1=hh+s1+ch+k[i]+w[i];
            const std::uint32_t s0 =
                rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
            const std::uint32_t maj=(a&b)^(a&c)^(b&c);
            const std::uint32_t t2=s0+maj;
            hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=d;
        h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;
    }
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (int i=0;i<8;++i) hex << std::setw(8) << h[i];
    return hex.str();
}

static std::string json_escape(const std::string& value) {
    std::ostringstream out;
    for (std::string::const_iterator it=value.begin(); it!=value.end(); ++it) {
        const unsigned char ch=static_cast<unsigned char>(*it);
        if (ch=='\\' || ch=='"') out << '\\' << static_cast<char>(ch);
        else if (ch=='\n') out << "\\n";
        else if (ch=='\r') out << "\\r";
        else if (ch=='\t') out << "\\t";
        else out << static_cast<char>(ch);
    }
    return out.str();
}

static std::uint16_t le16(const unsigned char* data) {
    return static_cast<std::uint16_t>(data[0])
        | (static_cast<std::uint16_t>(data[1]) << 8);
}

static std::uint32_t le32(const unsigned char* data) {
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

static bool wav_pcm16_mono_frames(
    const char* path,
    std::uint32_t& frame_count
) {
    std::ifstream input(path, std::ios::binary);
    unsigned char header[12];
    input.read(reinterpret_cast<char*>(header), sizeof(header));
    if (
        !input
        || std::memcmp(header, "RIFF", 4) != 0
        || std::memcmp(header + 8, "WAVE", 4) != 0
    ) {
        return false;
    }

    bool have_fmt = false;
    bool have_data = false;
    std::uint16_t audio_format = 0;
    std::uint16_t channels = 0;
    std::uint16_t block_align = 0;
    std::uint16_t bits_per_sample = 0;
    std::uint32_t sample_rate = 0;
    std::uint32_t data_size = 0;

    while (input && (!have_fmt || !have_data)) {
        unsigned char chunk_header[8];
        input.read(reinterpret_cast<char*>(chunk_header), sizeof(chunk_header));
        if (!input) return false;
        const std::uint32_t chunk_size = le32(chunk_header + 4);

        if (std::memcmp(chunk_header, "fmt ", 4) == 0) {
            if (chunk_size < 16u) return false;
            unsigned char format[16];
            input.read(reinterpret_cast<char*>(format), sizeof(format));
            if (!input) return false;
            audio_format = le16(format);
            channels = le16(format + 2);
            sample_rate = le32(format + 4);
            block_align = le16(format + 12);
            bits_per_sample = le16(format + 14);
            if (chunk_size > 16u) {
                input.seekg(
                    static_cast<std::streamoff>(chunk_size - 16u),
                    std::ios::cur);
            }
            have_fmt = true;
        } else if (std::memcmp(chunk_header, "data", 4) == 0) {
            data_size = chunk_size;
            input.seekg(static_cast<std::streamoff>(chunk_size), std::ios::cur);
            have_data = true;
        } else {
            input.seekg(static_cast<std::streamoff>(chunk_size), std::ios::cur);
        }
        if (!input) return false;
        if ((chunk_size & 1u) != 0u) input.seekg(1, std::ios::cur);
        if (!input) return false;
    }

    if (
        !have_fmt
        || !have_data
        || audio_format != 1u
        || channels != 1u
        || sample_rate != 44100u
        || block_align != 2u
        || bits_per_sample != 16u
        || (data_size % block_align) != 0u
    ) {
        return false;
    }
    frame_count = data_size / block_align;
    return true;
}

static void print_compiled_provenance() {
    std::cout
        << "{\"schema_version\":1"
        << ",\"renderer_source_sha256\":\"" << PHASE6C_RENDER_SOURCE_SHA256 << "\""
        << ",\"renderer_project_sha256\":\"" << PHASE6C_RENDER_PROJECT_SHA256 << "\""
        << ",\"engine_sequencer_sha256\":\"" << PHASE6C_ENGINE_SEQUENCER_SHA256 << "\""
        << ",\"engine_song_sha256\":\"" << PHASE6C_ENGINE_SONG_SHA256 << "\""
        << ",\"engine_sequence_sha256\":\"" << PHASE6C_ENGINE_SEQUENCE_SHA256 << "\""
        << ",\"engine_pattern_sha256\":\"" << PHASE6C_ENGINE_PATTERN_SHA256 << "\""
        << ",\"engine_machinefactory_sha256\":\"" << PHASE6C_ENGINE_MACHINEFACTORY_SHA256 << "\""
        << ",\"engine_sampler_sha256\":\"" << PHASE6C_ENGINE_SAMPLER_SHA256 << "\""
        << ",\"engine_instrument_sha256\":\"" << PHASE6C_ENGINE_INSTRUMENT_SHA256 << "\""
        << ",\"compat_script_sha256\":\"" << PHASE6C_COMPAT_SCRIPT_SHA256 << "\""
        << ",\"fixture_generator_sha256\":\"" << PHASE6C_FIXTURE_GENERATOR_SHA256 << "\""
        << "}" << std::endl;
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--phase6c-provenance") {
        print_compiled_provenance();
        return 0;
    }
    if (argc != 4) {
        std::cerr << "usage: " << argv[0]
                  << " FIXTURE PCM16LE OUTPUT_WAV\n";
        return 64;
    }

    try {
        using namespace universalis::os::loggers;
        static stream_logger diagnostics(std::cerr);
        multiplex_logger::singleton().add(diagnostics);
        universalis::os::thread_name thread_name("phase6c-ps1-pitch");

        const char* requested_threads = std::getenv("PSYCLE_THREADS");
        if (requested_threads == 0 || std::string(requested_threads) != "1") {
            std::cerr << "candidate render requires PSYCLE_THREADS=1\n";
            return 69;
        }

        const std::string renderer_executable_sha256 = sha256_file(argv[0]);
        if (renderer_executable_sha256.size() != 64u) {
            std::cerr << "candidate renderer executable SHA-256 failed\n";
            return 76;
        }

        const std::string input_sha256 = sha256_file(argv[1]);
        if (input_sha256.size() != 64u) {
            std::cerr << "candidate fixture SHA-256 failed\n";
            return 72;
        }

        std::ifstream pcm_input(argv[2], std::ios::binary | std::ios::ate);
        const std::streamoff pcm_size =
            pcm_input ? static_cast<std::streamoff>(pcm_input.tellg()) : -1;
        if (!pcm_input || pcm_size != 11025 * 2) {
            std::cerr << "candidate PCM sidecar size changed\n";
            return 74;
        }
        pcm_input.seekg(0, std::ios::beg);
        std::vector<unsigned char> pcm(
            static_cast<std::size_t>(pcm_size), 0u);
        pcm_input.read(
            reinterpret_cast<char*>(pcm.data()),
            static_cast<std::streamsize>(pcm.size()));
        if (!pcm_input) {
            std::cerr << "candidate PCM sidecar read failed\n";
            return 74;
        }
        pcm_input.close();
        const std::string pcm_sha256 = sha256_file(argv[2]);
        if (pcm_sha256.size() != 64u) {
            std::cerr << "candidate PCM sidecar SHA-256 failed\n";
            return 74;
        }

        psycle::core::Player& player = psycle::core::Player::singleton();
        psycle::core::MachineFactory& factory =
            psycle::core::MachineFactory::getInstance();
        factory.Initialize(&player);

        int result = 0;
        {
            psycle::core::CoreSong song;
            player.song(song);

            psycle::core::Machine* master =
                factory.CreateMachine(psycle::core::InternalKeys::master, psycle::core::MASTER_INDEX);
            psycle::core::Machine* sampler =
                factory.CreateMachine(psycle::core::InternalKeys::sampler, 0);
            if (master == 0 || sampler == 0) {
                std::cerr << "minimal candidate machine construction failed\n";
                result = 65;
            } else {
                song.AddMachine(master, psycle::core::MASTER_INDEX);
                song.AddMachine(sampler, 0);
                if (song.InsertConnection(*sampler, *master) < 0) {
                    std::cerr << "minimal candidate routing construction failed\n";
                    result = 66;
                }
            }

            if (result == 0) {
                song.tracks(1);
                song.bpm(120.0f);
                song.tick_speed(4, false);

                psycle::core::Pattern* pattern = new psycle::core::Pattern();
                pattern->setID(0);
                pattern->setName("Phase 6C PS1 pitch witness");
                psycle::core::PatternEvent event;
                event.setNote(60);
                event.setInstrument(0);
                event.setMachine(0);
                pattern->insert(0.0, 0, event);
                song.sequence().Add(*pattern);
                psycle::core::SequenceLine& line = song.sequence().createNewLine();
                line.createEntry(*pattern, 0.0);
                song.is_ready(true);

                if (sha256_file(argv[1]) != input_sha256) {
                    std::cerr << "PS1 pitch witness fixture changed during setup\n";
                    result = 73;
                } else if (
                    song._pInstrument[0] == 0
                    || song._pInstrument[0]->waveLength != 0
                    || song._pInstrument[0]->waveDataL != 0
                ) {
                    std::cerr << "candidate pre-injection Instrument is not wave-empty\n";
                    result = 68;
                }
            }

            if (result == 0) {
                psycle::core::Instrument* instrument = song._pInstrument[0];
                instrument->waveLength = 11025;
                instrument->waveVolume = 100;
                instrument->waveLoopStart = 0;
                instrument->waveLoopEnd = 0;
                instrument->waveTune = 0;
                instrument->waveFinetune = 0;
                instrument->waveLoopType = false;
                instrument->waveStereo = false;
                instrument->waveDataL = new std::int16_t[11025];
                instrument->waveDataR = 0;
                for (std::size_t frame = 0; frame < 11025u; ++frame) {
                    const std::uint16_t bits =
                        static_cast<std::uint16_t>(pcm[frame * 2u])
                        | (static_cast<std::uint16_t>(pcm[frame * 2u + 1u]) << 8);
                    instrument->waveDataL[frame] =
                        static_cast<std::int16_t>(bits);
                }

                psycle::audiodrivers::AudioDriverSettings settings(
                    player.driver().playbackSettings());
                settings.setSamplesPerSec(44100);
                settings.setBitDepth(16);
                settings.setChannelMode(0);
                settings.setBlockFrames(2048);
                player.driver().setPlaybackSettings(settings);

                player.setFileName(argv[3]);
                player.startRecording();
                if (!player.recording()) {
                    std::cerr << "candidate recording did not start\n";
                    result = 66;
                } else {
                    player.start(0.0);
                    const int target_frames = 44100;
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

                    std::ifstream output(argv[3], std::ios::binary | std::ios::ate);
                    const std::streamoff output_size =
                        output ? static_cast<std::streamoff>(output.tellg()) : -1;
                    if (!output || output_size <= 44) {
                        std::cerr << "candidate render output missing or empty\n";
                        result = 67;
                    } else {
                        output.close();
                        std::uint32_t output_frames = 0u;
                        if (
                            !wav_pcm16_mono_frames(argv[3], output_frames)
                            || output_frames != static_cast<std::uint32_t>(target_frames)
                        ) {
                            std::cerr
                                << "candidate render WAV is not an exact 44100-frame "
                                << "PCM16 mono render\n";
                            result = 75;
                        } else {
                            const std::string output_sha256=sha256_file(argv[3]);
                            if (output_sha256.size()!=64u) {
                                std::cerr << "candidate render SHA-256 failed\n";
                                result=71;
                            } else {
                                std::cout
                                    << "{\"schema_version\":1"
                                    << ",\"fixed_frame_render\":true"
                                    << ",\"renderer_executable_sha256\":\""
                                    << renderer_executable_sha256 << "\""
                                << ",\"sample_rate\":44100"
                                << ",\"channels\":1"
                                << ",\"bits_per_sample\":16"
                                << ",\"target_frames\":" << target_frames
                                << ",\"threads\":1"
                                << ",\"sequencer_work_calls\":1"
                                << ",\"final_play_beat\":" << final_beat
                                << ",\"input_path\":\"" << json_escape(argv[1]) << "\""
                                << ",\"input_sha256\":\"" << input_sha256 << "\""
                                << ",\"candidate_fixture_loader_bypassed\":true"
                                << ",\"harness_song_topology\":\"sampler-0-to-master-one-note\""
                                << ",\"harness_bpm\":120"
                                << ",\"harness_lpb\":4"
                                << ",\"harness_note\":60"
                                << ",\"harness_track\":0"
                                << ",\"harness_machine\":0"
                                << ",\"harness_instrument\":0"
                                << ",\"pcm_path\":\"" << json_escape(argv[2]) << "\""
                                << ",\"pcm_size_bytes\":" << pcm_size
                                << ",\"pcm_sha256\":\"" << pcm_sha256 << "\""
                                << ",\"harness_sample_injection\":true"
                                << ",\"pre_injection_wave_length\":0"
                                << ",\"injected_wave_length\":11025"
                                << ",\"injected_wave_volume\":100"
                                << ",\"injected_wave_tune\":0"
                                << ",\"injected_wave_finetune\":0"
                                << ",\"output_path\":\"" << json_escape(argv[3]) << "\""
                                    << ",\"output_frame_count\":" << output_frames
                                    << ",\"output_size_bytes\":" << static_cast<long long>(output_size)
                                    << ",\"output_sha256\":\"" << output_sha256 << "\""
                                    << "}" << std::endl;
                            }
                        }
                    }
                }
            }
        }
        factory.Finalize();
        return result;
    } catch (const std::exception& error) {
        std::cerr << "candidate PS1 pitch render exception: "
                  << error.what() << std::endl;
        return 70;
    } catch (...) {
        std::cerr << "candidate PS1 pitch render unknown exception\n";
        return 70;
    }
}
