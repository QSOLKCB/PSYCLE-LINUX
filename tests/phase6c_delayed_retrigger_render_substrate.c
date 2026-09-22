/* SPDX-License-Identifier: GPL-2.0-or-later
** Project-authored Phase 6C original-render substrate isolation fixtures.
**
** These fixtures narrow the shared sampled-fixture/original-render failure
** established by PR #72. Each rung adds exactly one substrate layer:
**
**   master-only   -> Master + four-beat song only
**   sampler-empty -> add built-in Sampler wired to Master
**   sample-state  -> add deterministic sample + instrument state
**   ordinary-note -> add one ordinary note that exercises Sampler playback
**
** They are diagnostic only and do not classify delayed/retrigger parity.
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
#include <plugincatcher.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define BPM 137.0
#define LPB 8
#define TPB 24
#define EXTRA_TICKS 0
#define MACHINE 0
#define NOTE 60
#define PATTERN_BEATS 4.0
#define CLICK_FRAMES 512
#define CLICK_IMPULSE_FRAME 256

typedef struct SubstrateVariant {
    const char* name;
    const char* title;
    int sampler;
    int sample_state;
    int ordinary_note;
} SubstrateVariant;

static const SubstrateVariant VARIANTS[] = {
    {
        "master-only",
        "PSYCLE-LINUX Phase 6C render substrate master-only",
        0, 0, 0
    },
    {
        "sampler-empty",
        "PSYCLE-LINUX Phase 6C render substrate sampler-empty",
        1, 0, 0
    },
    {
        "sample-state",
        "PSYCLE-LINUX Phase 6C render substrate sample-state",
        1, 1, 0
    },
    {
        "ordinary-note",
        "PSYCLE-LINUX Phase 6C render substrate ordinary-note",
        1, 1, 1
    }
};

static int fail(const char* variant, const char* message)
{
    fprintf(stderr,
        "phase6c-delayed-retrigger-render-substrate[%s]: FAIL: %s\n",
        variant, message);
    return 1;
}

static int close_enough(double lhs, double rhs)
{
    return fabs(lhs - rhs) <= 0.000001;
}

static psy_audio_SequenceCursor cursor_at(
    psy_audio_Song* song, double offset)
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
    psy_audio_Song* song,
    psy_audio_MachineFactory* factory,
    const SubstrateVariant* variant)
{
    psy_audio_Machine* sampler = psy_audio_machinefactory_make_machine_from_path(
        factory, psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
    if (!sampler)
        return fail(variant->name, "could not create built-in sampler");
    psy_audio_machines_insert(
        psy_audio_song_machines(song), MACHINE, sampler);
    psy_audio_machines_connect(
        psy_audio_song_machines(song),
        psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX));
    return 0;
}

static int install_sample_state(
    psy_audio_Song* song, const SubstrateVariant* variant)
{
    psy_audio_Sample* sample = psy_audio_sample_alloc_init(1);
    psy_audio_Instrument* instrument;
    psy_audio_InstrumentEntry entry;
    psy_audio_SampleIndex sample_index = psy_audio_sampleindex_make(0, 0);
    psy_audio_InstrumentIndex instrument_index =
        psy_audio_instrumentindex_make(0, 0);
    uintptr_t frame;

    if (!sample)
        return fail(variant->name, "could not allocate deterministic sample");
    sample->numframes = CLICK_FRAMES;
    psy_audio_sample_set_name(sample, "Phase 6C deterministic impulse");
    psy_audio_sample_set_sample_rate(sample, 44100.0);
    psy_audio_sample_set_volume(sample, 0x80);
    psy_audio_sample_alloc_wave_data(sample);
    if (!sample->channels.samples || !sample->channels.samples[0]) {
        psy_audio_sample_deallocate(sample);
        return fail(variant->name, "could not allocate deterministic sample data");
    }
    for (frame = 0; frame < CLICK_FRAMES; ++frame)
        sample->channels.samples[0][frame] = 0.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 0] = 16000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 1] = -16000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 2] = 8000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 3] = -8000.0f;
    psy_audio_samples_insert(
        psy_audio_song_samples(song), sample, sample_index);

    instrument = psy_audio_instrument_allocinit();
    if (!instrument)
        return fail(variant->name, "could not allocate deterministic instrument");
    psy_audio_instrument_set_name(
        instrument, "Phase 6C deterministic instrument");
    psy_audio_instrumententry_init(&entry);
    entry.sampleindex = sample_index;
    psy_audio_instrument_add_entry(instrument, &entry);
    psy_audio_instruments_insert(
        psy_audio_song_instruments(song), instrument, instrument_index);
    return 0;
}

static int install_note(
    psy_audio_Song* song,
    psy_audio_Pattern* pattern,
    const SubstrateVariant* variant)
{
    psy_audio_PatternEvent event;
    psy_audio_SequenceCursor cursor = cursor_at(song, 0.0);
    psy_audio_patternevent_clear(&event);
    event.note = NOTE;
    event.inst = 0;
    event.mach = MACHINE;
    event.cmd = 0;
    event.parameter = 0;
    return psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)
        ? 0 : fail(variant->name, "could not insert ordinary note");
}

static int verify_song(
    psy_audio_Song* song, const SubstrateVariant* variant)
{
    psy_audio_Machine* master;
    psy_audio_Machine* sampler;
    psy_audio_Pattern* pattern;
    psy_audio_Sample* sample;
    psy_audio_Instrument* instrument;

    if (strcmp(psy_audio_song_title(song), variant->title) != 0)
        return fail(variant->name, "title changed");
    if (!close_enough(psy_audio_song_bpm(song), BPM) ||
        psy_audio_song_lpb(song) != LPB ||
        psy_audio_song_tpb(song) != TPB ||
        psy_audio_song_extra_ticks_per_beat(song) != EXTRA_TICKS)
        return fail(variant->name, "timing metadata changed");

    master = psy_audio_machines_at(
        psy_audio_song_machines(song), psy_audio_MASTER_INDEX);
    if (!master || psy_audio_machine_type(master) != psy_audio_MASTER)
        return fail(variant->name, "Master machine missing");

    sampler = psy_audio_machines_at(
        psy_audio_song_machines(song), MACHINE);
    if (variant->sampler) {
        if (!sampler || psy_audio_machine_type(sampler) != psy_audio_SAMPLER)
            return fail(variant->name, "built-in sampler missing");
        if (!psy_audio_machines_connected(
                psy_audio_song_machines(song),
                psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX)))
            return fail(variant->name, "sampler-to-master wire missing");
    } else if (sampler) {
        return fail(variant->name, "master-only fixture gained a sampler");
    }

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    instrument = psy_audio_instruments_at(
        psy_audio_song_instruments(song), psy_audio_instrumentindex_make(0, 0));
    if (variant->sample_state) {
        if (!sample || psy_audio_sample_num_frames(sample) != CLICK_FRAMES)
            return fail(variant->name, "deterministic sample missing");
        if (!instrument || !psy_audio_instrument_entries(instrument))
            return fail(variant->name, "deterministic instrument missing");
    } else if (sample || instrument) {
        return fail(variant->name, "fixture gained sample/instrument state");
    }

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern)
        return fail(variant->name, "pattern 0 missing");
    if (!close_enough(
            psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)),
            PATTERN_BEATS))
        return fail(variant->name, "pattern length changed");

    if (variant->ordinary_note) {
        psy_audio_SequenceCursor cursor = cursor_at(song, 0.0);
        psy_audio_PatternEvent event =
            psy_audio_pattern_event_at_cursor(pattern, cursor);
        if (event.note != NOTE || event.inst != 0 ||
            event.mach != MACHINE || event.cmd != 0 || event.parameter != 0)
            return fail(variant->name, "ordinary note changed");
    }
    return 0;
}

static int save_song(
    psy_audio_Song* song,
    const SubstrateVariant* variant,
    const char* path)
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

static int build_variant(
    const SubstrateVariant* variant, const char* output_directory)
{
    char path[4096];
    psy_audio_MachineCallback callback, loaded_callback;
    psy_audio_PluginCatcher catcher, loaded_catcher;
    psy_audio_MachineFactory factory, loaded_factory;
    psy_audio_Song* song;
    psy_audio_Song* loaded;
    psy_audio_Pattern* pattern;

    if (snprintf(path, sizeof(path),
            "%s/phase6c-delayed-retrigger-substrate-%s.psy",
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

    if (variant->sampler && install_sampler(song, &factory, variant) != 0)
        return 1;
    if (variant->sample_state && install_sample_state(song, variant) != 0)
        return 1;

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern)
        return fail(variant->name, "default pattern missing");
    psy_audio_pattern_set_name(pattern, "Render Substrate Isolation");
    psy_audio_pattern_set_length(
        pattern, psy_dsp_beatpos_make_real(PATTERN_BEATS, psy_dsp_DEFAULT_PPQ));
    if (variant->ordinary_note && install_note(song, pattern, variant) != 0)
        return 1;

    if (verify_song(song, variant) != 0 ||
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
            "fresh C-Psycle load did not preserve substrate fixture");

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
        return 2;
    }

    psy_audio_init();
    for (index = 0; index < sizeof(VARIANTS) / sizeof(VARIANTS[0]); ++index) {
        if (build_variant(&VARIANTS[index], argv[1]) != 0) {
            psy_audio_dispose();
            return 1;
        }
    }
    psy_audio_dispose();

    puts("phase6c-delayed-retrigger-render-substrate: PASS");
    return 0;
}
