/* SPDX-License-Identifier: GPL-2.0-or-later
** Project-authored Phase 6C delayed/retrigger Sampulse execution witness.
**
** This fixture is intentionally separate from the frozen comparison fixture.
** It adds a deterministic impulse sample and spaces the command families into
** distinct one-beat windows so pinned original Psycle can expose execution
** through its own offline WAV renderer without changing historical evidence.
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
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define TITLE "PSYCLE-LINUX Phase 6C delayed/retrigger Sampulse execution witness"
#define BPM 137.0
#define LPB 8
#define TPB 24
#define EXTRA_TICKS 0
#define MACHINE 0
#define NOTE 60
#define NOTE_DELAY_PARAM 0x7F
#define RETRIGGER_PARAM 0x3F
#define RETR_CONT_PARAM 0x42
#define EXTENDED_LPB_PARAM 0x04
#define PATTERN_BEATS 4.0
#define CLICK_FRAMES 512
#define CLICK_IMPULSE_FRAME 256

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-delayed-retrigger-sampulse-execution-fixture: FAIL: %s\n", message);
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
    uint8_t cmd,
    uint8_t parameter)
{
    psy_audio_PatternEvent event;
    psy_audio_SequenceCursor cursor = cursor_at(song, channel, offset);
    psy_audio_patternevent_clear(&event);
    event.note = note;
    event.inst = 0;
    event.mach = MACHINE;
    event.cmd = cmd;
    event.parameter = parameter;
    return psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)
        ? 0 : fail("could not insert witness event");
}

static int verify_event(
    psy_audio_Song* song,
    psy_audio_Pattern* pattern,
    uintptr_t channel,
    double offset,
    uint8_t note,
    uint8_t cmd,
    uint8_t parameter)
{
    psy_audio_SequenceCursor cursor = cursor_at(song, channel, offset);
    psy_audio_PatternEvent event =
        psy_audio_pattern_event_at_cursor(pattern, cursor);
    if (event.note != note || event.inst != 0 || event.mach != MACHINE ||
        event.cmd != cmd || event.parameter != parameter) {
        return fail("witness event changed");
    }
    return 0;
}

static int install_click_sample(psy_audio_Song* song)
{
    psy_audio_Sample* sample = psy_audio_sample_alloc_init(1);
    psy_audio_Instrument* instrument;
    psy_audio_InstrumentEntry entry;
    psy_audio_SampleIndex sample_index = psy_audio_sampleindex_make(0, 0);
    psy_audio_InstrumentIndex instrument_index =
        psy_audio_instrumentindex_make(1, 0);
    uintptr_t frame;

    if (!sample) return fail("could not allocate click sample");
    sample->numframes = CLICK_FRAMES;
    psy_audio_sample_set_name(sample, "Phase 6C deterministic impulse");
    psy_audio_sample_set_sample_rate(sample, 44100.0);
    psy_audio_sample_set_volume(sample, 0x80);
    psy_audio_sample_alloc_wave_data(sample);
    if (!sample->channels.samples || !sample->channels.samples[0]) {
        psy_audio_sample_deallocate(sample);
        return fail("could not allocate click sample data");
    }
    for (frame = 0; frame < CLICK_FRAMES; ++frame)
        sample->channels.samples[0][frame] = 0.0f;
    /*
    ** Keep the impulse after the retained Sampler's default ~5 ms attack.
    ** The legacy instrument conversion serializes that attack at roughly
    ** 221 frames at 44.1 kHz, so a frame-0 click can be attenuated below the
    ** execution analyzer threshold in original Psycle.
    */
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 0] = 16000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 1] = -16000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 2] = 8000.0f;
    sample->channels.samples[0][CLICK_IMPULSE_FRAME + 3] = -8000.0f;
    psy_audio_samples_insert(psy_audio_song_samples(song), sample, sample_index);

    instrument = psy_audio_instrument_allocinit();
    if (!instrument) return fail("could not allocate click instrument");
    psy_audio_instrument_set_name(instrument, "Phase 6C click instrument");
    psy_audio_instrumententry_init(&entry);
    entry.sampleindex = sample_index;
    psy_audio_instrument_add_entry(instrument, &entry);
    psy_audio_instruments_insert(
        psy_audio_song_instruments(song), instrument, instrument_index);
    return 0;
}

static int verify_song(psy_audio_Song* song)
{
    psy_audio_Machine* sampler;
    psy_audio_Pattern* pattern;
    psy_audio_Sample* sample;
    psy_audio_Instrument* instrument;

    if (strcmp(psy_audio_song_title(song), TITLE) != 0)
        return fail("title changed");
    if (!close_enough(psy_audio_song_bpm(song), BPM))
        return fail("BPM changed");
    if (psy_audio_song_lpb(song) != LPB)
        return fail("LPB changed");
    if (psy_audio_song_tpb(song) != TPB)
        return fail("TPB changed");
    if (psy_audio_song_extra_ticks_per_beat(song) != EXTRA_TICKS)
        return fail("extra ticks changed");

    sampler = psy_audio_machines_at(psy_audio_song_machines(song), MACHINE);
    if (!sampler || psy_audio_machine_type(sampler) != psy_audio_XMSAMPLER)
        return fail("built-in Sampulse missing");
    if (!psy_audio_machines_connected(
            psy_audio_song_machines(song),
            psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX)))
        return fail("sampler-to-master wire missing");

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    if (!sample || psy_audio_sample_num_frames(sample) != CLICK_FRAMES)
        return fail("deterministic click sample missing");
    instrument = psy_audio_instruments_at(
        psy_audio_song_instruments(song), psy_audio_instrumentindex_make(0, 0));
    if (!instrument || !psy_audio_instrument_entries(instrument))
        return fail("click instrument missing");

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern) return fail("pattern 0 missing");
    if (!close_enough(
            psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)),
            PATTERN_BEATS))
        return fail("pattern length changed");

    if (verify_event(song, pattern, 0, 0.0, NOTE,
            psy_audio_PATTERNCMD_NOTE_DELAY, NOTE_DELAY_PARAM) != 0)
        return 1;
    if (verify_event(song, pattern, 0, 1.0, NOTE,
            psy_audio_PATTERNCMD_RETRIGGER, RETRIGGER_PARAM) != 0)
        return 1;
    if (verify_event(song, pattern, 0, 2.0, NOTE,
            psy_audio_PATTERNCMD_RETR_CONT, RETR_CONT_PARAM) != 0)
        return 1;
    if (verify_event(song, pattern, 0, 3.0,
            psy_audio_NOTECOMMANDS_EMPTY,
            psy_audio_PATTERNCMD_EXTENDED, EXTENDED_LPB_PARAM) != 0)
        return 1;
    if (verify_event(song, pattern, 0, 3.0 + (1.0 / LPB), NOTE, 0, 0) != 0)
        return 1;
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

int main(int argc, char** argv)
{
    char path[4096];
    psy_audio_MachineCallback callback, loaded_callback;
    psy_audio_PluginCatcher catcher, loaded_catcher;
    psy_audio_MachineFactory factory, loaded_factory;
    psy_audio_Song* song;
    psy_audio_Song* loaded;
    psy_audio_Machine* sampler;
    psy_audio_Pattern* pattern;

    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    if (snprintf(path, sizeof(path),
            "%s/phase6c-delayed-retrigger-sampulse-execution.psy", argv[1])
            >= (int)sizeof(path))
        return fail("output path too long");

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    psy_audio_plugincatcher_init(&catcher, NULL);
    psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);

    song = psy_audio_song_alloc_init(&factory);
    if (!song) return fail("could not allocate song");
    psy_audio_song_set_title(song, TITLE);
    psy_audio_song_set_bpm(song, BPM);
    psy_audio_song_set_lpb(song, LPB);
    psy_audio_song_set_tpb(song, TPB);
    psy_audio_song_set_extra_ticks_per_beat(song, EXTRA_TICKS);

    sampler = psy_audio_machinefactory_make_machine_from_path(
        &factory, psy_audio_XMSAMPLER, NULL, 0, psy_INDEX_INVALID);
    if (!sampler) return fail("could not create built-in Sampulse");
    psy_audio_machines_insert(
        psy_audio_song_machines(song), MACHINE, sampler);
    psy_audio_machines_connect(
        psy_audio_song_machines(song),
        psy_audio_wire_make(MACHINE, psy_audio_MASTER_INDEX));
    if (install_click_sample(song) != 0) return 1;

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern) return fail("default pattern missing");
    psy_audio_pattern_set_name(pattern, "Execution Witness");
    psy_audio_pattern_set_length(
        pattern, psy_dsp_beatpos_make_real(PATTERN_BEATS, psy_dsp_DEFAULT_PPQ));

    if (put_event(song, pattern, 0, 0.0, NOTE,
            psy_audio_PATTERNCMD_NOTE_DELAY, NOTE_DELAY_PARAM) != 0 ||
        put_event(song, pattern, 0, 1.0, NOTE,
            psy_audio_PATTERNCMD_RETRIGGER, RETRIGGER_PARAM) != 0 ||
        put_event(song, pattern, 0, 2.0, NOTE,
            psy_audio_PATTERNCMD_RETR_CONT, RETR_CONT_PARAM) != 0 ||
        put_event(song, pattern, 0, 3.0, psy_audio_NOTECOMMANDS_EMPTY,
            psy_audio_PATTERNCMD_EXTENDED, EXTENDED_LPB_PARAM) != 0 ||
        put_event(song, pattern, 0, 3.0 + (1.0 / LPB), NOTE, 0, 0) != 0)
        return 1;

    if (verify_song(song) != 0 || save_song(song, path) != 0)
        return 1;

    psy_audio_machinecallback_init(&loaded_callback);
    psy_audio_plugincatcher_init(&loaded_catcher, NULL);
    psy_audio_machinefactory_init(
        &loaded_factory, &loaded_callback, &loaded_catcher, NULL);
    loaded = load_song(&loaded_factory, path);
    if (!loaded || verify_song(loaded) != 0)
        return fail("fresh C-Psycle load did not preserve execution witness");

    psy_audio_song_deallocate(loaded);
    psy_audio_machinefactory_dispose(&loaded_factory);
    psy_audio_plugincatcher_dispose(&loaded_catcher);
    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    psy_audio_dispose();

    puts("phase6c-delayed-retrigger-sampulse-execution-fixture: PASS");
    return 0;
}
