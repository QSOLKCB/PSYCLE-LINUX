/*
** PSYCLE-LINUX Phase 4 historical PSY2 compatibility regression.
**
** The fixture is project-authored at test time from the documented legacy
** Psycle 1.66 / PSY2SONG layout. No upstream demo-song content is copied.
*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <constants.h>
#include <machine.h>
#include <machinefactory.h>
#include <machines.h>
#include <pattern.h>
#include <plugincatcher.h>
#include <psyconvert.h>
#include <song.h>
#include <songio.h>

#define LEGACY_TITLE "QSOL PSY2 compatibility fixture"
#define LEGACY_AUTHOR "QSOL-IMC"
#define LEGACY_COMMENT "Project-authored PSY2SONG fixture; no upstream demo content."
#define LEGACY_BPM 125
#define LEGACY_SAMPLES_PER_TICK 5292
#define LEGACY_LPB 4
#define LEGACY_LINES 4
#define LEGACY_TRACKS 4
#define PSY2_CONNECTIONS 12

static int fail(const char* message)
{
    fprintf(stderr, "phase4-historical-psy2: FAIL: %s\n", message);
    return 1;
}

static int put(FILE* fp, const void* data, size_t size)
{
    return fwrite(data, 1, size, fp) == size ? 0 : -1;
}

static int put_zero(FILE* fp, size_t size)
{
    unsigned char zero[4096] = {0};
    while (size) {
        size_t chunk = size < sizeof(zero) ? size : sizeof(zero);
        if (put(fp, zero, chunk) != 0) return -1;
        size -= chunk;
    }
    return 0;
}

static int put_i32(FILE* fp, int32_t value)
{
    return put(fp, &value, sizeof(value));
}

static int put_fixed(FILE* fp, const char* text, size_t size)
{
    char* buf = (char*)calloc(1, size);
    size_t len;
    int rc;
    if (!buf) return -1;
    len = strlen(text);
    if (len > size) len = size;
    memcpy(buf, text, len);
    rc = put(fp, buf, size);
    free(buf);
    return rc;
}

static int write_legacy_wires(FILE* fp)
{
    int i;
    for (i = 0; i < PSY2_CONNECTIONS; ++i) if (put_i32(fp, -1)) return -1;
    for (i = 0; i < PSY2_CONNECTIONS; ++i) if (put_i32(fp, -1)) return -1;
    for (i = 0; i < PSY2_CONNECTIONS; ++i) {
        float volume = 1.0f;
        if (put(fp, &volume, sizeof(volume))) return -1;
    }
    if (put_zero(fp, PSY2_CONNECTIONS)) return -1; /* outgoing active */
    if (put_zero(fp, PSY2_CONNECTIONS)) return -1; /* incoming active */
    return 0;
}

static int write_psy2_fixture(const char* path)
{
    FILE* fp;
    unsigned char bus_machine[64];
    unsigned char play_order[128];
    unsigned char machine_active[128];
    unsigned char bus_effect[64];
    unsigned char event_data[LEGACY_LINES * OLD_MAX_TRACKS * PSY2_EVENT_SIZE];
    int i;
    int row;
    int track;

    fp = fopen(path, "wb");
    if (!fp) return -1;
    memset(bus_machine, 255, sizeof(bus_machine));
    bus_machine[0] = 0;
    memset(play_order, 255, sizeof(play_order));
    play_order[0] = 0;

    if (put(fp, "PSY2SONG", 8) ||
            put_fixed(fp, LEGACY_TITLE, 32) ||
            put_fixed(fp, LEGACY_AUTHOR, 32) ||
            put_fixed(fp, LEGACY_COMMENT, 128) ||
            put_i32(fp, LEGACY_BPM) ||
            put_i32(fp, LEGACY_SAMPLES_PER_TICK)) goto error;
    {
        uint8_t octave = 4;
        if (put(fp, &octave, sizeof(octave)) ||
                put(fp, bus_machine, sizeof(bus_machine)) ||
                put(fp, play_order, sizeof(play_order)) ||
                put_i32(fp, 1) || put_i32(fp, LEGACY_TRACKS)) goto error;
    }

    /* One four-line legacy pattern, with a C-4 event in track zero. */
    if (put_i32(fp, 1) || put_i32(fp, LEGACY_LINES) ||
            put_fixed(fp, "Legacy Pattern", 32)) goto error;
    for (row = 0; row < LEGACY_LINES; ++row) {
        for (track = 0; track < OLD_MAX_TRACKS; ++track) {
            size_t off = (size_t)(row * OLD_MAX_TRACKS + track) * PSY2_EVENT_SIZE;
            event_data[off + 0] = PSY2_NOTECOMMANDS_EMPTY;
            event_data[off + 1] = 255;
            event_data[off + 2] = 255;
            event_data[off + 3] = 0;
            event_data[off + 4] = 0;
        }
    }
    event_data[0] = 48;
    event_data[1] = 0;
    if (put(fp, event_data, sizeof(event_data))) goto error;

    /* Empty legacy instrument bank, preserving the exact field layout. */
    if (put_i32(fp, 0) || put_zero(fp, OLD_MAX_INSTRUMENTS * 32u)) goto error;
    if (put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(uint8_t))) goto error; /* NNA */
    for (i = 0; i < 12; ++i) {
        if (put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(int32_t))) goto error;
    }
    if (put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(int32_t))) goto error; /* pans */
    if (put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(uint8_t)) ||
            put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(uint8_t)) ||
            put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(uint8_t))) goto error;
    if (put_i32(fp, 0)) goto error;
    if (put_zero(fp, OLD_MAX_INSTRUMENTS * OLD_MAX_WAVES * sizeof(int32_t))) goto error;

    /* No VST table entries. */
    if (put_zero(fp, OLD_MAX_PLUGINS * sizeof(uint8_t))) goto error;

    /* One legacy Master machine at old slot zero. */
    memset(machine_active, 0, sizeof(machine_active));
    machine_active[0] = 1;
    if (put(fp, machine_active, sizeof(machine_active)) ||
            put_i32(fp, 64) || put_i32(fp, 48) || put_i32(fp, psy_audio_MASTER) ||
            put_fixed(fp, "Legacy Master", 16) || write_legacy_wires(fp) ||
            put_zero(fp, PSY2_CONNECTIONS * 2u * sizeof(int32_t)) ||
            put_i32(fp, 0) || put_i32(fp, 0) || put_i32(fp, 64) ||
            put_zero(fp, 40) || put_i32(fp, 1024) || put_zero(fp, 65)) goto error;

    /* Patch-0 instrument loop/line metadata. */
    if (put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(uint8_t)) ||
            put_zero(fp, OLD_MAX_INSTRUMENTS * sizeof(int32_t))) goto error;

    memset(bus_effect, 255, sizeof(bus_effect));
    if (put(fp, bus_effect, sizeof(bus_effect))) goto error;

    if (fclose(fp) != 0) return -1;
    return 0;
error:
    fclose(fp);
    return -1;
}

static psy_audio_SequenceCursor cursor_at(psy_audio_Song* song, double beat)
{
    psy_audio_SequenceCursor cursor = psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(&cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, 0);
    psy_audio_sequencecursor_set_offset(&cursor,
        psy_dsp_beatpos_make_real(beat, psy_dsp_DEFAULT_PPQ));
    return cursor;
}

int main(int argc, char** argv)
{
    char path[4096];
    psy_audio_MachineCallback callback;
    psy_audio_PluginCatcher catcher;
    psy_audio_MachineFactory factory;
    psy_audio_Song* song;
    psy_audio_SongReader reader;
    psy_audio_Pattern* pattern;
    psy_audio_PatternEvent event;
    int status;

    if (argc != 2) return 2;
    if (snprintf(path, sizeof(path), "%s/phase4-historical-psy2.psy", argv[1]) >= (int)sizeof(path))
        return fail("output path too long");
    if (write_psy2_fixture(path) != 0) return fail("could not write project-authored PSY2 fixture");

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    psy_audio_plugincatcher_init(&catcher, NULL);
    psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
    song = psy_audio_song_alloc_init(&factory);
    if (!song) return fail("could not allocate song");

    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        fprintf(stderr, "phase4-historical-psy2: loader status=%d\n", status);
        return fail("PSY2 SongReader load failed");
    }

    if (strcmp(psy_audio_song_title(song), LEGACY_TITLE) != 0 ||
            strcmp(psy_audio_song_credits(song), LEGACY_AUTHOR) != 0 ||
            fabs(psy_audio_song_bpm(song) - LEGACY_BPM) > 0.001 ||
            psy_audio_song_lpb(song) != LEGACY_LPB) {
        return fail("legacy song metadata conversion mismatch");
    }
    if (!psy_audio_machines_at(psy_audio_song_machines(song), psy_audio_MASTER_INDEX) ||
            psy_audio_machine_type(psy_audio_machines_at(
                psy_audio_song_machines(song), psy_audio_MASTER_INDEX)) != psy_audio_MASTER) {
        return fail("legacy Master machine was not remapped to the modern Master slot");
    }
    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern) return fail("legacy pattern zero missing");
    event = psy_audio_pattern_event_at_cursor(pattern, cursor_at(song, 0.0));
    if (event.note != 48) return fail("legacy pattern event did not convert");
    if (!psy_audio_sequence_entry(psy_audio_song_sequence(song),
            psy_audio_orderindex_make(0, 0))) {
        return fail("legacy play order did not convert to sequence entry");
    }

    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    psy_audio_dispose();
    printf("phase4-historical-psy2: PASS\n");
    printf("generated: %s\n", path);
    return 0;
}
