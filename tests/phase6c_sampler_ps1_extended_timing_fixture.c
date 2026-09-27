// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Phase 6C PS1 extended timing fixtures.
 *
 * Two deliberately narrow PSY3 songs exercise only Sampler-local E-D3 and E-C3.
 * C-Psycle authors and reloads the modern fixtures; pinned Psycle 1.12.0 is the
 * original-runtime authority. The frozen r12005 candidate consumes a bridge
 * that removes only SMSB and injects the exact decoded PCM into the legacy
 * Instrument before executing its untouched Sampler path.
 */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
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
#include <psyconvert.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define BPM 120.0
#define LPB 4
#define TPB 24
#define EXTRA_TICKS 0
#define MACHINE 0
#define NOTE 60
#define SAMPLE_RATE 44100.0
#define SAMPLE_FRAMES 44100
#define PATTERN_BEATS 1.0
#define SAMPLER_CMD_EXTENDED 0x0E
#define EXT_DELAY_PARAM 0xD3
#define EXT_NOTEOFF_PARAM 0xC3

typedef enum VariantKind {
    VARIANT_DELAY = 0,
    VARIANT_NOTEOFF = 1
} VariantKind;

typedef struct Variant {
    const char* name;
    const char* title;
    VariantKind kind;
} Variant;

static const Variant VARIANTS[] = {
    {
        "delay",
        "PSYCLE-LINUX Phase 6C Sampler PS1 E-D3 timing witness",
        VARIANT_DELAY
    },
    {
        "noteoff",
        "PSYCLE-LINUX Phase 6C Sampler PS1 E-C3 timing witness",
        VARIANT_NOTEOFF
    }
};

static int fail(const char* name, const char* message)
{
    fprintf(stderr, "phase6c-sampler-ps1-extended-timing[%s]: FAIL: %s\n",
        name, message);
    return 1;
}

static int close_enough(double lhs, double rhs)
{
    return fabs(lhs - rhs) <= 0.000001;
}

static psy_audio_SequenceCursor cursor_at(
    psy_audio_Song* song, uintptr_t channel, double offset)
{
    psy_audio_SequenceCursor cursor =
        psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(
        &cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, channel);
    psy_audio_sequencecursor_set_offset(
        &cursor, psy_dsp_beatpos_make_real(offset, psy_dsp_DEFAULT_PPQ));
    return cursor;
}

static int put_event(
    psy_audio_Song* song,
    psy_audio_Pattern* pattern,
    uintptr_t channel,
    double offset,
    uint8_t note,
    uint8_t inst,
    uint8_t cmd,
    uint8_t parameter,
    const Variant* variant)
{
    psy_audio_PatternEvent event;
    psy_audio_SequenceCursor cursor = cursor_at(song, channel, offset);
    psy_audio_patternevent_clear(&event);
    event.note = note;
    event.inst = inst;
    event.mach = MACHINE;
    event.cmd = cmd;
    event.parameter = parameter;
    return psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)
        ? 0 : fail(variant->name, "could not insert timing event");
}

static int install_sampler(
    psy_audio_Song* song,
    psy_audio_MachineFactory* factory,
    const Variant* variant)
{
    psy_audio_Machine* sampler = psy_audio_machinefactory_make_machine_from_path(
        factory, psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
    if (!sampler)
        return fail(variant->name, "could not create built-in PS1 Sampler");
    psy_audio_machines_insert(psy_audio_song_machines(song), MACHINE, sampler);
    psy_audio_machines_connect(
        psy_audio_song_machines(song),
        psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX));
    return 0;
}

static int install_sample_and_instrument(
    psy_audio_Song* song, const Variant* variant)
{
    psy_audio_Sample* sample = psy_audio_sample_alloc_init(1);
    psy_audio_Instrument* instrument;
    psy_audio_InstrumentEntry entry;
    psy_audio_LegacyInstrument legacy;
    psy_audio_SampleIndex sample_index = psy_audio_sampleindex_make(0, 0);
    psy_audio_InstrumentIndex instrument_index =
        psy_audio_instrumentindex_make(0, 0);
    uintptr_t frame;

    if (!sample)
        return fail(variant->name, "could not allocate deterministic sample");
    sample->numframes = SAMPLE_FRAMES;
    psy_audio_sample_set_name(sample, "Phase 6C PS1 extended timing sample");
    psy_audio_sample_set_sample_rate(sample, SAMPLE_RATE);
    psy_audio_sample_set_global_volume(sample, 1.0);
    psy_audio_sample_set_volume(sample, 0x80);
    psy_audio_sample_alloc_wave_data(sample);
    if (!sample->channels.samples || !sample->channels.samples[0]) {
        psy_audio_sample_deallocate(sample);
        return fail(variant->name, "could not allocate sample data");
    }
    for (frame = 0; frame < SAMPLE_FRAMES; ++frame) {
        sample->channels.samples[0][frame] =
            10000.0f + (float)(frame % 97u);
    }
    psy_audio_samples_insert(
        psy_audio_song_samples(song), sample, sample_index);

    instrument = psy_audio_instrument_allocinit();
    if (!instrument)
        return fail(variant->name, "could not allocate instrument");
    psy_audio_instrument_set_name(
        instrument, "Phase 6C PS1 extended timing instrument");

    /*
     * Make note-off observable at the command boundary rather than smearing the
     * timing result across a long default release. These are ordinary legacy
     * instrument fields serialized by the fixture, not a runtime override.
     */
    psy_audio_legacyinstrument_init(&legacy);
    legacy.ENV_AT = 1;
    legacy.ENV_DT = 1;
    legacy.ENV_SL = 100;
    legacy.ENV_RT = 1;
    legacy.ENV_F_AT = 1;
    legacy.ENV_F_DT = 1;
    legacy.ENV_F_SL = 128;
    legacy.ENV_F_RT = 1;
    psy_audio_convert_legacy_to_instrument(instrument, legacy);

    psy_audio_instrumententry_init(&entry);
    entry.sampleindex = sample_index;
    psy_audio_instrument_add_entry(instrument, &entry);
    psy_audio_instruments_insert(
        psy_audio_song_instruments(song), instrument, instrument_index);
    return 0;
}

static int install_events(
    psy_audio_Song* song,
    psy_audio_Pattern* pattern,
    const Variant* variant)
{
    if (variant->kind == VARIANT_DELAY) {
        return put_event(
            song, pattern, 0, 0.0, NOTE, 0,
            SAMPLER_CMD_EXTENDED, EXT_DELAY_PARAM, variant);
    }

    if (put_event(song, pattern, 0, 0.0, NOTE, 0, 0, 0, variant) != 0)
        return 1;
    return put_event(
        song, pattern, 0, 1.0 / LPB,
        psy_audio_NOTECOMMANDS_EMPTY,
        psy_audio_NOTECOMMANDS_INST_EMPTY,
        SAMPLER_CMD_EXTENDED,
        EXT_NOTEOFF_PARAM,
        variant);
}

static int verify_event(
    psy_audio_Song* song,
    psy_audio_Pattern* pattern,
    double offset,
    uint8_t note,
    uint8_t cmd,
    uint8_t parameter,
    const Variant* variant)
{
    psy_audio_SequenceCursor cursor = cursor_at(song, 0, offset);
    psy_audio_PatternEvent event =
        psy_audio_pattern_event_at_cursor(pattern, cursor);
    if (event.note != note || event.mach != MACHINE ||
        event.cmd != cmd || event.parameter != parameter)
        return fail(variant->name, "timing event changed");
    return 0;
}

static int verify_song(psy_audio_Song* song, const Variant* variant)
{
    psy_audio_Machine* sampler;
    psy_audio_Sample* sample;
    psy_audio_Instrument* instrument;
    psy_audio_Pattern* pattern;

    if (strcmp(psy_audio_song_title(song), variant->title) != 0)
        return fail(variant->name, "title changed");
    if (!close_enough(psy_audio_song_bpm(song), BPM) ||
        psy_audio_song_lpb(song) != LPB ||
        psy_audio_song_tpb(song) != TPB ||
        psy_audio_song_extra_ticks_per_beat(song) != EXTRA_TICKS)
        return fail(variant->name, "timing metadata changed");

    sampler = psy_audio_machines_at(
        psy_audio_song_machines(song), MACHINE);
    if (!sampler || psy_audio_machine_type(sampler) != psy_audio_SAMPLER)
        return fail(variant->name, "built-in PS1 Sampler missing");
    if (!psy_audio_machines_connected(
            psy_audio_song_machines(song),
            psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX)))
        return fail(variant->name, "Sampler-to-Master wire missing");

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    if (!sample ||
        psy_audio_sample_num_frames(sample) != SAMPLE_FRAMES ||
        !close_enough(psy_audio_sample_sample_rate(sample), SAMPLE_RATE))
        return fail(variant->name, "44.1-kHz sample identity changed");

    instrument = psy_audio_instruments_at(
        psy_audio_song_instruments(song), psy_audio_instrumentindex_make(0, 0));
    if (!instrument || !psy_audio_instrument_entries(instrument))
        return fail(variant->name, "classic Sampler instrument state missing");

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern ||
        !close_enough(
            psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)),
            PATTERN_BEATS))
        return fail(variant->name, "pattern duration changed");

    if (variant->kind == VARIANT_DELAY) {
        return verify_event(
            song, pattern, 0.0, NOTE,
            SAMPLER_CMD_EXTENDED, EXT_DELAY_PARAM, variant);
    }
    if (verify_event(song, pattern, 0.0, NOTE, 0, 0, variant) != 0)
        return 1;
    return verify_event(
        song, pattern, 1.0 / LPB,
        psy_audio_NOTECOMMANDS_EMPTY,
        SAMPLER_CMD_EXTENDED, EXT_NOTEOFF_PARAM, variant);
}

static int save_song(
    psy_audio_Song* song, const Variant* variant, const char* path)
{
    psy_audio_SongFile songfile;
    int status;
    psy_audio_songfile_init_song(&songfile, song);
    status = psy_audio_songfile_save(&songfile, path);
    psy_audio_songfile_dispose(&songfile);
    return status == PSY_OK
        ? 0 : fail(variant->name, "PSY3 save failed");
}

static psy_audio_Song* load_song(
    psy_audio_MachineFactory* factory, const char* path)
{
    psy_audio_Song* song = psy_audio_song_alloc_init(factory);
    psy_audio_SongReader reader;
    int status;
    if (!song) return NULL;
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        psy_audio_song_deallocate(song);
        return NULL;
    }
    return song;
}

static int build_variant(const Variant* variant, const char* output_directory)
{
    char path[4096];
    psy_audio_MachineCallback callback, loaded_callback;
    psy_audio_PluginCatcher catcher, loaded_catcher;
    psy_audio_MachineFactory factory, loaded_factory;
    psy_audio_Song* song;
    psy_audio_Song* loaded;
    psy_audio_Pattern* pattern;

    if (snprintf(path, sizeof(path),
            "%s/phase6c-sampler-ps1-extended-%s-original.psy",
            output_directory, variant->name) >= (int)sizeof(path))
        return fail(variant->name, "output path too long");

    psy_audio_machinecallback_init(&callback);
    psy_audio_plugincatcher_init(&catcher, NULL);
    psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
    song = psy_audio_song_alloc_init(&factory);
    if (!song)
        return fail(variant->name, "could not allocate song");

    psy_audio_song_set_title(song, variant->title);
    psy_audio_song_set_bpm(song, BPM);
    psy_audio_song_set_lpb(song, LPB);
    psy_audio_song_set_tpb(song, TPB);
    psy_audio_song_set_extra_ticks_per_beat(song, EXTRA_TICKS);

    if (install_sampler(song, &factory, variant) != 0 ||
        install_sample_and_instrument(song, variant) != 0)
        return 1;

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern)
        return fail(variant->name, "default pattern missing");
    psy_audio_pattern_set_name(pattern, "PS1 extended timing");
    psy_audio_pattern_set_length(
        pattern,
        psy_dsp_beatpos_make_real(PATTERN_BEATS, psy_dsp_DEFAULT_PPQ));
    if (install_events(song, pattern, variant) != 0 ||
        verify_song(song, variant) != 0 ||
        save_song(song, variant, path) != 0)
        return 1;

    psy_audio_machinecallback_init(&loaded_callback);
    psy_audio_plugincatcher_init(&loaded_catcher, NULL);
    psy_audio_machinefactory_init(
        &loaded_factory, &loaded_callback, &loaded_catcher, NULL);
    loaded = load_song(&loaded_factory, path);
    if (!loaded || verify_song(loaded, variant) != 0)
        return fail(
            variant->name,
            "fresh C-Psycle load did not preserve timing fixture");

    psy_audio_song_deallocate(loaded);
    psy_audio_machinefactory_dispose(&loaded_factory);
    psy_audio_plugincatcher_dispose(&loaded_catcher);
    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    return 0;
}

int main(int argc, char** argv)
{
    size_t index;
    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 64;
    }

    psy_audio_init();
    for (index = 0; index < sizeof(VARIANTS) / sizeof(VARIANTS[0]); ++index) {
        if (build_variant(&VARIANTS[index], argv[1]) != 0) {
            psy_audio_dispose();
            return 1;
        }
    }
    psy_audio_dispose();

    puts("{\"schema_version\":1,\"phase\":\"6C\","
         "\"contract\":\"sampler-ps1-extended-timing-runtime\","
         "\"commands\":[\"E-D3\",\"E-C3\"],\"bpm\":120,\"lpb\":4}");
    return 0;
}
