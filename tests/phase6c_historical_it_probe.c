// SPDX-License-Identifier: GPL-2.0-or-later
/* Public-safe C-Psycle observation probe for the external SickMaate IT witness. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <audioconfig.h>
#include <buffer.h>
#include <buffercontext.h>
#include <instrument.h>
#include <instruments.h>
#include <machine.h>
#include <machines.h>
#include <pattern.h>
#include <patternevent.h>
#include <patterns.h>
#include <player.h>
#include <properties.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

#define EXPECTED_TITLE "SickMaate"
#define EXPECTED_BPM 140.0
#define EXPECTED_LPB 6
#define SAMPLE_SLOTS 48
#define PATTERN_SLOTS 19
#define ORDER_SCAN 80
#define ROW_SCAN 128
#define CHANNEL_SCAN 32
#define RENDER_FRAMES 4096u

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-historical-it-probe: FAIL: %s\n", message);
    return 1;
}

static psy_audio_PatternEvent event_at(
    psy_audio_Song* song,
    uintptr_t order,
    uintptr_t row,
    uintptr_t channel)
{
    psy_audio_Pattern* pattern;
    psy_audio_SequenceCursor cursor =
        psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(
        &cursor, psy_audio_orderindex_make(0, order));
    psy_audio_sequencecursor_set_channel(&cursor, channel);
    psy_audio_sequencecursor_set_offset(
        &cursor,
        psy_dsp_beatpos_make_real(
            (double)row / psy_audio_song_lpb(song),
            psy_dsp_DEFAULT_PPQ));
    pattern = psy_audio_patterns_at(
        psy_audio_song_patterns(song),
        psy_audio_sequence_patternindex(psy_audio_song_sequence(song), cursor.order_index));
    if (!pattern) {
        psy_audio_PatternEvent empty;
        psy_audio_patternevent_clear(&empty);
        return empty;
    }
    return psy_audio_pattern_event_at_cursor(pattern, cursor);
}

int main(int argc, char** argv)
{
    psy_audio_MachineCallback callback;
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_Player player;
    psy_audio_Song* song;
    psy_audio_SongReader reader;
    psy_audio_Machine* sampler;
    psy_audio_Machine* virtual_generator;
    psy_audio_PatternEvent first_note;
    psy_audio_Buffer buffer;
    psy_audio_BufferContext bc;
    float* left;
    float* right;
    uintptr_t order;
    uintptr_t row;
    uintptr_t channel;
    uintptr_t frame;
    uintptr_t first_order = 0;
    uintptr_t first_row = 0;
    uintptr_t first_channel = 0;
    uintptr_t samples_with_pcm = 0;
    uintptr_t looped_samples = 0;
    uintptr_t sample_mode_instruments = 0;
    uintptr_t present_patterns = 0;
    uintptr_t nonzero_frames = 0;
    double peak = 0.0;
    int found_note = 0;
    int status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s HISTORICAL.it\n", argv[0]);
        return 64;
    }

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    config = psy_property_allocinit_key(NULL);
    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_player_init(
        &player, &callback, NULL, psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = psy_audio_song_alloc_init(&player.machinefactory);
    if (!song) return fail("song allocation failed");
    psy_audio_machinecallback_set_song(&callback, song);
    psy_audio_machinecallback_set_player(&callback, &player);

    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, argv[1]);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) return fail("C-Psycle SongReader rejected historical IT witness");

    if (strcmp(psy_audio_song_title(song), EXPECTED_TITLE) != 0)
        return fail("historical title changed during import");
    if (fabs(psy_audio_song_bpm(song) - EXPECTED_BPM) > 0.000001)
        return fail("historical BPM changed during import");
    if (psy_audio_song_lpb(song) != EXPECTED_LPB)
        return fail("historical speed 4 did not map to LPB 6");

    for (frame = 0; frame < SAMPLE_SLOTS; ++frame) {
        psy_audio_Sample* sample = psy_audio_samples_at(
            psy_audio_song_samples(song), psy_audio_sampleindex_make(frame, 0));
        psy_audio_Instrument* instrument = psy_audio_instruments_at(
            psy_audio_song_instruments(song),
            psy_audio_instrumentindex_make(1, frame));
        if (sample && psy_audio_sample_num_frames(sample) > 0) {
            ++samples_with_pcm;
            if (sample->loop.type != psy_audio_SAMPLE_LOOP_DO_NOT)
                ++looped_samples;
        }
        if (instrument && psy_audio_instrument_entries(instrument))
            ++sample_mode_instruments;
    }
    for (frame = 0; frame < PATTERN_SLOTS; ++frame) {
        if (psy_audio_patterns_at(psy_audio_song_patterns(song), frame))
            ++present_patterns;
    }

    psy_audio_patternevent_clear(&first_note);
    for (order = 0; order < ORDER_SCAN && !found_note; ++order) {
        for (row = 0; row < ROW_SCAN && !found_note; ++row) {
            for (channel = 0; channel < CHANNEL_SCAN; ++channel) {
                psy_audio_PatternEvent event = event_at(song, order, row, channel);
                if (event.note <= psy_audio_NOTECOMMANDS_B9 &&
                    event.mach != 0 && event.mach != 255) {
                    first_note = event;
                    first_order = order;
                    first_row = row;
                    first_channel = channel;
                    found_note = 1;
                    break;
                }
            }
        }
    }
    if (!found_note)
        return fail("no playable virtual-generator note found in scanned historical orders");

    sampler = psy_audio_machines_at(psy_audio_song_machines(song), 0);
    virtual_generator =
        psy_audio_machines_at(psy_audio_song_machines(song), first_note.mach);
    if (!sampler || psy_audio_machine_type(sampler) != psy_audio_XMSAMPLER)
        return fail("historical import has no Sampulse machine");
    if (!virtual_generator ||
        psy_audio_machine_type(virtual_generator) != psy_audio_VIRTUALGENERATOR)
        return fail("historical first note has no virtual-generator route");

    psy_audio_buffer_init(&buffer, 2);
    psy_audio_buffer_allocsamples(&buffer, RENDER_FRAMES);
    left = psy_audio_buffer_at(&buffer, 0);
    right = psy_audio_buffer_at(&buffer, 1);
    if (!left || !right) {
        psy_audio_buffer_dispose(&buffer);
        return fail("render buffer allocation failed");
    }
    psy_audio_buffer_clearsamples(&buffer, RENDER_FRAMES);
    psy_audio_buffercontext_init(
        &bc, NULL, &buffer, &buffer, RENDER_FRAMES, 1);

    psy_audio_machine_seq_tick(virtual_generator, first_channel, &first_note);
    psy_audio_machine_work(sampler, &bc);

    for (frame = 0; frame < RENDER_FRAMES; ++frame) {
        double a = fabs((double)left[frame]);
        double b = fabs((double)right[frame]);
        if (a > peak) peak = a;
        if (b > peak) peak = b;
        if (left[frame] != 0.0f || right[frame] != 0.0f)
            ++nonzero_frames;
    }
    psy_audio_buffercontext_dispose(&bc);
    psy_audio_buffer_dispose(&buffer);

    if (samples_with_pcm != 25)
        return fail("historical PCM sample count differs from forensic manifest");
    if (looped_samples != 11)
        return fail("historical looped sample count differs from forensic manifest");
    if (present_patterns != 19)
        return fail("historical pattern count differs from forensic manifest");
    if (nonzero_frames == 0 || peak <= 0.0)
        return fail("historical donor playback witness is silent");

    printf(
        "{"
        "\"schema_version\":1,"
        "\"contract\":\"legacy-impulse-tracker-import-reference\","
        "\"evidence_role\":\"cpsycle-donor-observation\","
        "\"title\":\"%s\","
        "\"bpm\":%.0f,"
        "\"lpb\":%lu,"
        "\"samples_with_pcm\":%lu,"
        "\"looped_samples\":%lu,"
        "\"sample_mode_instruments\":%lu,"
        "\"patterns\":%lu,"
        "\"first_playable_order\":%lu,"
        "\"first_playable_row\":%lu,"
        "\"first_playable_channel\":%lu,"
        "\"first_virtual_generator\":%u,"
        "\"render_frames\":%u,"
        "\"nonzero_frames\":%lu,"
        "\"peak\":%.9g,"
        "\"load_result\":\"accepted\","
        "\"non_silent_playback\":true,"
        "\"parity_status\":\"UNKNOWN\""
        "}\n",
        psy_audio_song_title(song),
        psy_audio_song_bpm(song),
        (unsigned long)psy_audio_song_lpb(song),
        (unsigned long)samples_with_pcm,
        (unsigned long)looped_samples,
        (unsigned long)sample_mode_instruments,
        (unsigned long)present_patterns,
        (unsigned long)first_order,
        (unsigned long)first_row,
        (unsigned long)first_channel,
        (unsigned)first_note.mach,
        (unsigned)RENDER_FRAMES,
        (unsigned long)nonzero_frames,
        peak);

    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    psy_audio_dispose();
    return 0;
}
