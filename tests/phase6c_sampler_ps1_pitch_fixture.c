// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Project-authored Phase 6C Sampler PS1 non-44.1-kHz pitch witness.
 *
 * The modern C-Psycle save is the authoritative authored fixture for pinned
 * Psycle 1.12.0. For the frozen r12005 candidate, a fail-closed bridge removes
 * only the unsupported modern SMSB chunk and extracts its exact decoded PCM16
 * bytes to a hash-bound sidecar. The candidate harness installs those bytes into
 * the already-loaded legacy Instrument before executing the untouched Sampler.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <instrument.h>
#include <instruments.h>
#include <machine.h>
#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <patternevent.h>
#include <player.h>
#include <plugincatcher.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

#define BPM 120.0
#define LPB 4.0
#define TPB 24
#define EXTRA_TICKS 0
#define MACHINE 0
#define NOTE 60
#define SAMPLE_RATE 22050.0
#define SAMPLE_FRAMES 11025
#define PATTERN_BEATS 2.0

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-sampler-ps1-pitch: FAIL: %s\n", message);
    return 1;
}

static int close_enough(double lhs, double rhs)
{
    return fabs(lhs - rhs) <= 0.000001;
}

static psy_audio_SequenceCursor cursor_at(psy_audio_Song* song, double offset)
{
    psy_audio_SequenceCursor cursor =
        psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(
        &cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, 0);
    psy_audio_sequencecursor_set_offset(
        &cursor, psy_dsp_beatpos_make_real(offset, psy_dsp_DEFAULT_PPQ));
    return cursor;
}

static int install_sampler(
    psy_audio_Song* song, psy_audio_MachineFactory* factory)
{
    psy_audio_Machine* sampler = psy_audio_machinefactory_make_machine_from_path(
        factory, psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
    if (!sampler)
        return fail("could not create built-in PS1 Sampler");
    psy_audio_machines_insert(psy_audio_song_machines(song), MACHINE, sampler);
    psy_audio_machines_connect(
        psy_audio_song_machines(song),
        psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX));
    return 0;
}

static int install_sample_and_instrument(psy_audio_Song* song)
{
    psy_audio_Sample* sample = psy_audio_sample_alloc_init(1);
    psy_audio_Instrument* instrument;
    psy_audio_InstrumentEntry entry;
    psy_audio_SampleIndex sample_index = psy_audio_sampleindex_make(0, 0);
    psy_audio_InstrumentIndex instrument_index =
        psy_audio_instrumentindex_make(0, 0);
    uintptr_t frame;

    if (!sample)
        return fail("could not allocate deterministic sample");
    sample->numframes = SAMPLE_FRAMES;
    psy_audio_sample_set_name(sample, "Phase 6C PS1 22.05 kHz witness");
    psy_audio_sample_set_sample_rate(sample, SAMPLE_RATE);
    psy_audio_sample_set_global_volume(sample, 1.0);
    psy_audio_sample_set_volume(sample, 0x80);
    psy_audio_sample_alloc_wave_data(sample);
    if (!sample->channels.samples || !sample->channels.samples[0]) {
        psy_audio_sample_deallocate(sample);
        return fail("could not allocate deterministic sample data");
    }
    for (frame = 0; frame < SAMPLE_FRAMES; ++frame) {
        /*
         * Deliberately never zero. The last active frame therefore measures
         * playback duration without requiring frequency-domain inference.
         */
        sample->channels.samples[0][frame] =
            10000.0f + (float)(frame % 97u);
    }
    psy_audio_samples_insert(
        psy_audio_song_samples(song), sample, sample_index);

    instrument = psy_audio_instrument_allocinit();
    if (!instrument)
        return fail("could not allocate deterministic instrument");
    psy_audio_instrument_set_name(
        instrument, "Phase 6C PS1 pitch instrument");
    psy_audio_instrumententry_init(&entry);
    entry.sampleindex = sample_index;
    psy_audio_instrument_add_entry(instrument, &entry);
    psy_audio_instruments_insert(
        psy_audio_song_instruments(song), instrument, instrument_index);
    return 0;
}

static int install_event(psy_audio_Song* song, psy_audio_Pattern* pattern)
{
    psy_audio_PatternEvent event;
    psy_audio_SequenceCursor cursor = cursor_at(song, 0.0);

    psy_audio_patternevent_clear(&event);
    event.note = NOTE;
    event.inst = 0;
    event.mach = MACHINE;
    return psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)
        ? 0 : fail("could not insert witness note");
}

static int verify_song(psy_audio_Song* song)
{
    psy_audio_Machine* sampler;
    psy_audio_Sample* sample;
    psy_audio_Instrument* instrument;
    psy_audio_Pattern* pattern;
    psy_audio_PatternEvent event;
    psy_audio_SequenceCursor cursor;

    if (strcmp(
            psy_audio_song_title(song),
            "PSYCLE-LINUX Phase 6C Sampler PS1 22.05 kHz pitch witness") != 0)
        return fail("title changed");
    if (!close_enough(psy_audio_song_bpm(song), BPM) ||
        psy_audio_song_lpb(song) != LPB ||
        psy_audio_song_tpb(song) != TPB ||
        psy_audio_song_extra_ticks_per_beat(song) != EXTRA_TICKS)
        return fail("timing metadata changed");

    sampler = psy_audio_machines_at(
        psy_audio_song_machines(song), MACHINE);
    if (!sampler || psy_audio_machine_type(sampler) != psy_audio_SAMPLER)
        return fail("built-in PS1 Sampler missing");
    if (!psy_audio_machines_connected(
            psy_audio_song_machines(song),
            psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX)))
        return fail("Sampler-to-Master wire missing");

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    if (!sample ||
        psy_audio_sample_num_frames(sample) != SAMPLE_FRAMES ||
        !close_enough(psy_audio_sample_sample_rate(sample), SAMPLE_RATE))
        return fail("authored 22.05-kHz sample identity changed");

    instrument = psy_audio_instruments_at(
        psy_audio_song_instruments(song), psy_audio_instrumentindex_make(0, 0));
    if (!instrument || !psy_audio_instrument_entries(instrument))
        return fail("classic Sampler instrument state missing");

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern ||
        !close_enough(
            psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)),
            PATTERN_BEATS))
        return fail("pattern duration changed");
    cursor = cursor_at(song, 0.0);
    event = psy_audio_pattern_event_at_cursor(pattern, cursor);
    if (event.note != NOTE || event.inst != 0 || event.mach != MACHINE)
        return fail("witness event changed");
    return 0;
}

static int save_song(psy_audio_Song* song, const char* path)
{
    psy_audio_SongFile songfile;
    int status;

    psy_audio_songfile_init_song(&songfile, song);
    status = psy_audio_songfile_save(&songfile, path);
    psy_audio_songfile_dispose(&songfile);
    return status == PSY_OK ? 0 : fail("PSY3 save failed");
}

static psy_audio_Song* load_song(
    psy_audio_MachineFactory* factory, const char* path)
{
    psy_audio_Song* song = psy_audio_song_alloc_init(factory);
    psy_audio_SongReader reader;
    int status;

    if (!song)
        return NULL;
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        psy_audio_song_deallocate(song);
        return NULL;
    }
    return song;
}

int main(int argc, char** argv)
{
    psy_audio_MachineCallback callback, loaded_callback;
    psy_audio_PluginCatcher catcher, loaded_catcher;
    psy_audio_MachineFactory factory, loaded_factory;
    psy_audio_Song* song;
    psy_audio_Song* loaded;
    psy_audio_Pattern* pattern;

    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_PSY3\n", argv[0]);
        return 64;
    }

    /* Match the established Phase 6C C-Psycle fixture lifecycle. */
    psy_audio_init();

    psy_audio_machinecallback_init(&callback);
    psy_audio_plugincatcher_init(&catcher, NULL);
    psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
    song = psy_audio_song_alloc_init(&factory);
    if (!song)
        return fail("could not allocate song");

    psy_audio_song_set_title(
        song, "PSYCLE-LINUX Phase 6C Sampler PS1 22.05 kHz pitch witness");
    psy_audio_song_set_bpm(song, BPM);
    psy_audio_song_set_lpb(song, LPB);
    psy_audio_song_set_tpb(song, TPB);
    psy_audio_song_set_extra_ticks_per_beat(song, EXTRA_TICKS);

    if (install_sampler(song, &factory) != 0 ||
        install_sample_and_instrument(song) != 0)
        return 1;

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern)
        return fail("default pattern missing");
    psy_audio_pattern_set_name(pattern, "PS1 22.05 kHz C4 duration witness");
    psy_audio_pattern_set_length(
        pattern,
        psy_dsp_beatpos_make_real(PATTERN_BEATS, psy_dsp_DEFAULT_PPQ));
    if (install_event(song, pattern) != 0 ||
        verify_song(song) != 0 ||
        save_song(song, argv[1]) != 0)
        return 1;

    psy_audio_machinecallback_init(&loaded_callback);
    psy_audio_plugincatcher_init(&loaded_catcher, NULL);
    psy_audio_machinefactory_init(
        &loaded_factory, &loaded_callback, &loaded_catcher, NULL);
    loaded = load_song(&loaded_factory, argv[1]);
    if (!loaded || verify_song(loaded) != 0)
        return fail("fresh C-Psycle load did not preserve pitch fixture");

    printf(
        "{\"schema_version\":1,\"phase\":\"6C\","
        "\"contract\":\"sampler-ps1-pitch-runtime\","
        "\"authored_sample_rate\":22050,"
        "\"authored_sample_frames\":11025,"
        "\"note\":60,\"output_rate\":44100}\n");

    psy_audio_song_deallocate(loaded);
    psy_audio_machinefactory_dispose(&loaded_factory);
    psy_audio_plugincatcher_dispose(&loaded_catcher);
    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    psy_audio_dispose();
    return 0;
}
