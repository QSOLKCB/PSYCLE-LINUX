// SPDX-License-Identifier: GPL-2.0-or-later
/*
** Phase 6C real-world MIDI donor witness.
**
** Loads an external SMF through the retained C-Psycle MidiLoader, freezes a
** digest of the untouched imported event graph, then attaches a deterministic
** project-owned Sampulse substrate and renders a bounded 16-beat execution
** projection through the production Player/FileOutDriver path.
**
** The projection is not a claim about native MIDI instrument selection.
*/
#include <math.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <audioconfig.h>
#include <audiodriverplugin.h>
#include <buffer.h>
#include <fileoutdriver.h>
#include <instrument.h>
#include <instruments.h>
#include <machine.h>
#include <machinefactory.h>
#include <machines.h>
#include <pattern.h>
#include <patternentry.h>
#include <patterns.h>
#include <player.h>
#include <properties.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>
#include <thread.h>
#include <wire.h>

#define PROJECTION_BEATS 16.0
#define PROJECTION_SAMPLE_RATE 44100u
#define PROJECTION_SAMPLE_FRAMES 2048u
#define PROJECTION_MACHINE 0u
#define PROJECTION_INSTRUMENT_GROUP 1u
#define PROJECTION_INSTRUMENT 0u
#define RENDER_TIMEOUT_TICKS 1500u
#define RENDER_WAIT_US 10000u
#define MIN_RENDER_PEAK 50
#define PPQN_DIGEST 480.0

typedef struct RenderStopState {
    atomic_bool stopped;
} RenderStopState;

typedef struct ImportStats {
    uintptr_t sequence_tracks;
    uintptr_t patterns;
    uintptr_t pattern_entries;
    uintptr_t events;
    uintptr_t notes;
    uintptr_t releases;
    uintptr_t midi_cc;
    uintptr_t tempo_commands;
    uint32_t midi_channel_mask;
    uintptr_t machines_before_projection;
    uintptr_t projection_notes;
    uint64_t digest;
} ImportStats;

typedef struct WavStats {
    uint32_t frames;
    int peak;
} WavStats;

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-midi-corpus-probe: FAIL: %s\n", message);
    return 1;
}

static uint16_t read_u16_le(const unsigned char* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const unsigned char* p)
{
    return (uint32_t)p[0] |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

static void fnv_byte(uint64_t* hash, uint8_t value)
{
    *hash ^= (uint64_t)value;
    *hash *= UINT64_C(1099511628211);
}

static void fnv_u64(uint64_t* hash, uint64_t value)
{
    int shift;
    for (shift = 0; shift < 64; shift += 8)
        fnv_byte(hash, (uint8_t)((value >> shift) & 0xffu));
}

static void hash_event(
    uint64_t* hash,
    uintptr_t sequence_track,
    uintptr_t tracker_track,
    psy_dsp_beatpos_t offset,
    uintptr_t event_index,
    const psy_audio_PatternEvent* event)
{
    int64_t tick = (int64_t)llround(
        psy_dsp_beatpos_real(offset) * PPQN_DIGEST);
    fnv_u64(hash, (uint64_t)sequence_track);
    fnv_u64(hash, (uint64_t)tracker_track);
    fnv_u64(hash, (uint64_t)tick);
    fnv_u64(hash, (uint64_t)event_index);
    fnv_byte(hash, event->note);
    fnv_byte(hash, event->inst);
    fnv_byte(hash, event->mach);
    fnv_byte(hash, event->cmd);
    fnv_byte(hash, event->parameter);
}

static psy_audio_Song* load_song(
    psy_audio_Player* player,
    psy_audio_MachineCallback* callback,
    const char* path)
{
    psy_audio_Song* song = psy_audio_song_alloc_init(&player->machinefactory);
    psy_audio_SongReader reader;
    int status;

    if (!song)
        return NULL;
    psy_audio_machinecallback_set_song(callback, song);
    psy_audio_machinecallback_set_player(callback, player);
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        psy_audio_song_deallocate(song);
        return NULL;
    }
    return song;
}

static int collect_import_stats(psy_audio_Song* song, ImportStats* stats)
{
    psy_audio_Sequence* sequence = psy_audio_song_sequence(song);
    uintptr_t sequence_track;

    memset(stats, 0, sizeof(*stats));
    stats->digest = UINT64_C(1469598103934665603);
    stats->sequence_tracks = psy_audio_sequence_num_tracks(sequence);

    for (sequence_track = 0;
            sequence_track < stats->sequence_tracks;
            ++sequence_track) {
        psy_audio_OrderIndex order = psy_audio_orderindex_make(sequence_track, 0);
        uintptr_t pattern_index;
        psy_audio_Pattern* pattern;
        psy_audio_PatternNode* node;

        if (psy_audio_sequence_track_size(sequence, sequence_track) == 0)
            continue;
        pattern_index = psy_audio_sequence_patternindex(sequence, order);
        if (pattern_index == psy_INDEX_INVALID)
            continue;
        pattern = psy_audio_patterns_at(
            psy_audio_song_patterns(song), pattern_index);
        if (!pattern)
            return fail("sequence references a missing MIDI-import pattern");
        ++stats->patterns;

        for (node = psy_audio_pattern_begin(pattern);
                node != NULL;
                psy_audio_patternnode_next(&node)) {
            psy_audio_PatternEntry* entry =
                psy_audio_patternnode_entry(node);
            psy_audio_PatternEventNode* event_node;
            uintptr_t event_index = 0;

            ++stats->pattern_entries;
            for (event_node = psy_audio_patternentry_begin(entry);
                    event_node != NULL;
                    event_node = event_node->next, ++event_index) {
                psy_audio_PatternEvent* event =
                    (psy_audio_PatternEvent*)event_node->entry;
                ++stats->events;
                hash_event(
                    &stats->digest,
                    sequence_track,
                    psy_audio_patternentry_track(entry),
                    psy_audio_patternentry_offset(entry),
                    event_index,
                    event);

                if (event->note <= psy_audio_NOTECOMMANDS_B9) {
                    ++stats->notes;
                    if (event->mach < 16u)
                        stats->midi_channel_mask |= UINT32_C(1) << event->mach;
                } else if (event->note == psy_audio_NOTECOMMANDS_RELEASE) {
                    ++stats->releases;
                    if (event->mach < 16u)
                        stats->midi_channel_mask |= UINT32_C(1) << event->mach;
                } else if (event->note == psy_audio_NOTECOMMANDS_MIDICC) {
                    ++stats->midi_cc;
                    if (event->mach < 16u)
                        stats->midi_channel_mask |= UINT32_C(1) << event->mach;
                }
                if (event->cmd == psy_audio_PATTERNCMD_SET_TEMPO)
                    ++stats->tempo_commands;
            }
        }
    }

    if (stats->notes == 0)
        return fail("MIDI import produced no playable note events");
    return 0;
}

static int install_projection_substrate(
    psy_audio_Song* song,
    psy_audio_Player* player,
    ImportStats* stats)
{
    psy_audio_Machines* machines = psy_audio_song_machines(song);
    psy_audio_Machine* sampler;
    psy_audio_Sample* sample;
    psy_audio_Instrument* instrument;
    psy_audio_InstrumentEntry entry;
    uintptr_t slot;
    uintptr_t frame;
    psy_audio_Sequence* sequence = psy_audio_song_sequence(song);
    uintptr_t sequence_track;

    for (slot = 0; slot < 16u; ++slot) {
        if (psy_audio_machines_at(machines, slot))
            ++stats->machines_before_projection;
    }
    if (stats->machines_before_projection != 0)
        return fail("MIDI importer unexpectedly created sound-generating machines");

    sampler = psy_audio_machinefactory_make_machine_from_path(
        &player->machinefactory,
        psy_audio_XMSAMPLER,
        NULL,
        0,
        psy_INDEX_INVALID);
    if (!sampler)
        return fail("could not create deterministic Sampulse projection machine");
    psy_audio_machines_insert(machines, PROJECTION_MACHINE, sampler);
    psy_audio_machines_connect(
        machines,
        psy_audio_wire_make(PROJECTION_MACHINE, psy_audio_MASTER_INDEX));

    sample = psy_audio_sample_alloc_init(1);
    if (!sample)
        return fail("could not allocate deterministic MIDI projection sample");
    sample->numframes = PROJECTION_SAMPLE_FRAMES;
    psy_audio_sample_set_name(sample, "Phase 6C MIDI corpus projection tone");
    psy_audio_sample_set_sample_rate(sample, PROJECTION_SAMPLE_RATE);
    psy_audio_sample_set_volume(sample, 0x80);
    sample->loop.type = psy_audio_SAMPLE_LOOP_NORMAL;
    sample->loop.start = 0;
    sample->loop.end = PROJECTION_SAMPLE_FRAMES;
    psy_audio_sample_alloc_wave_data(sample);
    if (!sample->channels.samples || !sample->channels.samples[0]) {
        psy_audio_sample_deallocate(sample);
        return fail("could not allocate deterministic MIDI projection PCM");
    }
    for (frame = 0; frame < PROJECTION_SAMPLE_FRAMES; ++frame) {
        double phase = 2.0 * M_PI * (double)(frame % 128u) / 128.0;
        sample->channels.samples[0][frame] = (float)(sin(phase) * 8000.0);
    }
    psy_audio_samples_insert(
        psy_audio_song_samples(song),
        sample,
        psy_audio_sampleindex_make(0, 0));

    instrument = psy_audio_instrument_allocinit();
    if (!instrument)
        return fail("could not allocate deterministic MIDI projection instrument");
    psy_audio_instrument_set_name(
        instrument, "Phase 6C MIDI corpus projection instrument");
    psy_audio_instrumententry_init(&entry);
    entry.sampleindex = psy_audio_sampleindex_make(0, 0);
    psy_audio_instrument_add_entry(instrument, &entry);
    psy_audio_instruments_insert(
        psy_audio_song_instruments(song),
        instrument,
        psy_audio_instrumentindex_make(
            PROJECTION_INSTRUMENT_GROUP, PROJECTION_INSTRUMENT));

    for (sequence_track = 0;
            sequence_track < psy_audio_sequence_num_tracks(sequence);
            ++sequence_track) {
        psy_audio_OrderIndex order = psy_audio_orderindex_make(sequence_track, 0);
        uintptr_t pattern_index;
        psy_audio_Pattern* pattern;
        psy_audio_PatternNode* node;
        double pattern_beats;

        if (psy_audio_sequence_track_size(sequence, sequence_track) == 0)
            continue;
        pattern_index = psy_audio_sequence_patternindex(sequence, order);
        if (pattern_index == psy_INDEX_INVALID)
            continue;
        pattern = psy_audio_patterns_at(
            psy_audio_song_patterns(song), pattern_index);
        if (!pattern)
            continue;

        pattern_beats = psy_dsp_beatpos_real(psy_audio_pattern_length(pattern));
        if (pattern_beats > PROJECTION_BEATS) {
            psy_audio_pattern_set_length(
                pattern,
                psy_dsp_beatpos_make_real(
                    PROJECTION_BEATS, psy_dsp_DEFAULT_PPQ));
        }

        for (node = psy_audio_pattern_begin(pattern);
                node != NULL;
                psy_audio_patternnode_next(&node)) {
            psy_audio_PatternEntry* pattern_entry =
                psy_audio_patternnode_entry(node);
            psy_audio_PatternEventNode* event_node;
            double offset = psy_dsp_beatpos_real(
                psy_audio_patternentry_offset(pattern_entry));

            if (offset >= PROJECTION_BEATS)
                continue;
            for (event_node = psy_audio_patternentry_begin(pattern_entry);
                    event_node != NULL;
                    event_node = event_node->next) {
                psy_audio_PatternEvent* event =
                    (psy_audio_PatternEvent*)event_node->entry;
                if (event->note <= psy_audio_NOTECOMMANDS_B9) {
                    event->mach = PROJECTION_MACHINE;
                    event->inst = PROJECTION_INSTRUMENT;
                    ++stats->projection_notes;
                } else if (
                    event->note == psy_audio_NOTECOMMANDS_RELEASE ||
                    event->note == psy_audio_NOTECOMMANDS_MIDICC) {
                    event->mach = PROJECTION_MACHINE;
                }
            }
        }
    }

    if (stats->projection_notes == 0)
        return fail("16-beat MIDI projection contains no note events");
    return 0;
}

static void on_render_stopped(RenderStopState* state, psy_AudioDriver* sender)
{
    (void)sender;
    atomic_store_explicit(&state->stopped, true, memory_order_release);
}

static int inspect_wav(const char* path, WavStats* stats)
{
    FILE* file;
    unsigned char header[44];
    uint32_t data_bytes;
    uint16_t channels;
    uint16_t bits;
    uint16_t block_align;
    uint32_t i;
    int peak = 0;

    memset(stats, 0, sizeof(*stats));
    file = fopen(path, "rb");
    if (!file)
        return fail("FileOutDriver did not create the projection WAV");
    if (fread(header, 1, sizeof(header), file) != sizeof(header)) {
        fclose(file);
        return fail("projection WAV is shorter than the canonical PCM header");
    }
    if (memcmp(header, "RIFF", 4) != 0 ||
            memcmp(header + 8, "WAVE", 4) != 0 ||
            memcmp(header + 12, "fmt ", 4) != 0 ||
            memcmp(header + 36, "data", 4) != 0) {
        fclose(file);
        return fail("projection output is not canonical RIFF/WAVE PCM");
    }
    if (read_u16_le(header + 20) != 1u) {
        fclose(file);
        return fail("projection WAV is not PCM");
    }
    channels = read_u16_le(header + 22);
    bits = read_u16_le(header + 34);
    block_align = read_u16_le(header + 32);
    data_bytes = read_u32_le(header + 40);
    if (channels == 0 || bits != 16u ||
            block_align != channels * 2u ||
            data_bytes == 0 ||
            data_bytes % block_align != 0) {
        fclose(file);
        return fail("projection WAV geometry is invalid");
    }
    stats->frames = data_bytes / block_align;
    for (i = 0; i < data_bytes / 2u; ++i) {
        unsigned char bytes[2];
        int16_t sample;
        int magnitude;
        if (fread(bytes, 1, 2, file) != 2) {
            fclose(file);
            return fail("projection WAV PCM is truncated");
        }
        sample = (int16_t)read_u16_le(bytes);
        magnitude = sample < 0 ? -(int)sample : (int)sample;
        if (magnitude > peak)
            peak = magnitude;
    }
    fclose(file);
    stats->peak = peak;
    if (peak < MIN_RENDER_PEAK)
        return fail("deterministic MIDI execution projection is silent");
    return 0;
}

static int render_projection(
    psy_audio_Player* player,
    psy_audio_Song* song,
    const char* output,
    WavStats* stats)
{
    psy_AudioDriver* fileout;
    psy_AudioDriver* original_driver;
    RenderStopState stop_state;
    uintptr_t tick;
    int rc = 0;

    fileout = psy_audio_create_fileout_driver();
    if (!fileout)
        return fail("could not create FileOutDriver");
    psy_property_set_str(
        (psy_Property*)psy_audiodriver_configuration(fileout),
        "outputpath",
        output);
    psy_audiodriver_configure(fileout, NULL);

    original_driver = player->audiodrivers.driver_plugin.client;
    player->audiodrivers.driver_plugin.client = fileout;
    psy_audio_audiodriverplugin_connect(
        &player->audiodrivers.driver_plugin,
        player->audiodrivers.systemhandle,
        player->audiodrivers.context,
        (AUDIODRIVERWORKFN)player->audiodrivers.fp);

    psy_audio_player_set_song(player, song);
    psy_audio_sequencer_stop_loop(&player->sequencer);
    psy_audio_player_set_position(player, 0.0);
    psy_audio_player_start(player);
    atomic_init(&stop_state.stopped, false);
    psy_signal_connect(
        &fileout->signal_stop, &stop_state, on_render_stopped);

    if (psy_audiodriver_open(fileout) != 0) {
        rc = fail("FileOutDriver failed to open");
    } else {
        for (tick = 0;
                tick < RENDER_TIMEOUT_TICKS &&
                !atomic_load_explicit(
                    &stop_state.stopped, memory_order_acquire);
                ++tick) {
            psy_sleep_for(RENDER_WAIT_US);
        }
        if (!atomic_load_explicit(
                &stop_state.stopped, memory_order_acquire)) {
            rc = fail("MIDI projection render exceeded timeout");
        }
    }

    psy_audio_player_stop(player);
    psy_audiodriver_close(fileout);
    player->audiodrivers.driver_plugin.client = original_driver;
    psy_audio_audiodriverplugin_connect(
        &player->audiodrivers.driver_plugin,
        player->audiodrivers.systemhandle,
        player->audiodrivers.context,
        (AUDIODRIVERWORKFN)player->audiodrivers.fp);
    psy_audiodriver_deallocate(fileout);

    if (rc == 0)
        rc = inspect_wav(output, stats);
    return rc;
}

int main(int argc, char** argv)
{
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_MachineCallback callback;
    psy_audio_Player player;
    psy_audio_Song* song;
    ImportStats import_stats;
    WavStats wav_stats;
    int rc;

    if (argc != 3) {
        fprintf(stderr, "usage: %s INPUT.mid OUTPUT.wav\n", argv[0]);
        return 64;
    }

    psy_audio_init();
    config = psy_property_allocinit_key(NULL);
    if (!config) {
        psy_audio_dispose();
        return fail("could not allocate player configuration");
    }
    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_machinecallback_init(&callback);
    psy_audio_player_init(
        &player,
        &callback,
        NULL,
        psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = load_song(&player, &callback, argv[1]);
    if (!song) {
        psy_audio_player_dispose(&player);
        psy_audio_audioconfig_dispose(&audioconfig);
        psy_property_deallocate(config);
        psy_audio_dispose();
        return fail("C-Psycle SongReader rejected real-world MIDI stem");
    }

    rc = collect_import_stats(song, &import_stats);
    if (rc == 0)
        rc = install_projection_substrate(song, &player, &import_stats);
    if (rc == 0)
        rc = render_projection(&player, song, argv[2], &wav_stats);

    if (rc == 0) {
        printf(
            "{"
            "\"schema_version\":1,"
            "\"phase\":\"6C\","
            "\"contract\":\"legacy-midi-real-world-donor\","
            "\"evidence_role\":\"cpsycle-donor-observation\","
            "\"load_result\":\"accepted\","
            "\"sequence_tracks\":%lu,"
            "\"patterns\":%lu,"
            "\"pattern_entries\":%lu,"
            "\"events\":%lu,"
            "\"imported_notes\":%lu,"
            "\"releases\":%lu,"
            "\"midi_cc\":%lu,"
            "\"tempo_commands\":%lu,"
            "\"midi_channel_mask\":%u,"
            "\"machines_before_projection\":%lu,"
            "\"projection_notes\":%lu,"
            "\"projection_beats\":%.0f,"
            "\"projection_kind\":\"deterministic-sampulse\","
            "\"import_event_digest_fnv64\":\"%016llx\","
            "\"render_frames\":%u,"
            "\"render_peak\":%d,"
            "\"non_silent_projection\":true,"
            "\"parity_status\":\"UNKNOWN\""
            "}\n",
            (unsigned long)import_stats.sequence_tracks,
            (unsigned long)import_stats.patterns,
            (unsigned long)import_stats.pattern_entries,
            (unsigned long)import_stats.events,
            (unsigned long)import_stats.notes,
            (unsigned long)import_stats.releases,
            (unsigned long)import_stats.midi_cc,
            (unsigned long)import_stats.tempo_commands,
            (unsigned)import_stats.midi_channel_mask,
            (unsigned long)import_stats.machines_before_projection,
            (unsigned long)import_stats.projection_notes,
            PROJECTION_BEATS,
            (unsigned long long)import_stats.digest,
            (unsigned)wav_stats.frames,
            wav_stats.peak);
    }

    psy_audio_player_set_empty_song(&player);
    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    psy_audio_dispose();
    return rc;
}
