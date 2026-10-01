// SPDX-License-Identifier: GPL-2.0-or-later
/* Runtime regressions for C-Psycle uncompressed 8-bit IT sample decoding. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <audioconfig.h>
#include <player.h>
#include <properties.h>
#include <sample.h>
#include <samples.h>
#include <song.h>
#include <songio.h>

typedef struct Variant {
    const char* name;
    uintptr_t channels;
    uintptr_t frames;
    float left[3];
    float right[3];
} Variant;

static const Variant VARIANTS[] = {
    {
        "signed", 1, 3,
        { -32768.0f, 0.0f, 32512.0f },
        { 0.0f, 0.0f, 0.0f }
    },
    {
        "unsigned", 1, 3,
        { -32768.0f, 0.0f, 32512.0f },
        { 0.0f, 0.0f, 0.0f }
    },
    {
        "signed-delta-wrap", 1, 3,
        { 32512.0f, -32512.0f, 256.0f },
        { 0.0f, 0.0f, 0.0f }
    },
    {
        "unsigned-delta-wrap", 1, 3,
        { 32512.0f, -32512.0f, 256.0f },
        { 0.0f, 0.0f, 0.0f }
    },
    {
        "stereo-signed-delta-reset", 2, 3,
        { 2560.0f, 2816.0f, 3072.0f },
        { 256.0f, 512.0f, 768.0f }
    }
};

static int fail(const char* variant, const char* message)
{
    fprintf(stderr, "phase6c-legacy-it-decoder[%s]: FAIL: %s\n", variant, message);
    return 1;
}

static const Variant* find_variant(const char* name)
{
    size_t i;
    for (i = 0; i < sizeof(VARIANTS) / sizeof(VARIANTS[0]); ++i) {
        if (strcmp(VARIANTS[i].name, name) == 0)
            return &VARIANTS[i];
    }
    return NULL;
}

int main(int argc, char** argv)
{
    const Variant* variant;
    psy_audio_MachineCallback callback;
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_Player player;
    psy_audio_Song* song;
    psy_audio_SongReader reader;
    psy_audio_Sample* sample;
    uintptr_t frame;
    int status;

    if (argc != 3) {
        fprintf(stderr, "usage: %s VARIANT FIXTURE.it\n", argv[0]);
        return 64;
    }
    variant = find_variant(argv[1]);
    if (!variant)
        return fail(argv[1], "unknown decoder variant");

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    config = psy_property_allocinit_key(NULL);
    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_player_init(
        &player, &callback, NULL, psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = psy_audio_song_alloc_init(&player.machinefactory);
    if (!song)
        return fail(variant->name, "song allocation failed");
    psy_audio_machinecallback_set_song(&callback, song);
    psy_audio_machinecallback_set_player(&callback, &player);
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, argv[2]);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK)
        return fail(variant->name, "SongReader rejected decoder fixture");

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    if (!sample)
        return fail(variant->name, "decoder fixture sample missing");
    if (psy_audio_sample_num_frames(sample) != variant->frames)
        return fail(variant->name, "decoded frame count changed");
    if (psy_audio_sample_num_channels(sample) != variant->channels)
        return fail(variant->name, "decoded channel count changed");

    for (frame = 0; frame < variant->frames; ++frame) {
        if (sample->channels.samples[0][frame] != variant->left[frame])
            return fail(variant->name, "left-channel decoded PCM changed");
    }
    if (variant->channels == 2) {
        for (frame = 0; frame < variant->frames; ++frame) {
            if (sample->channels.samples[1][frame] != variant->right[frame])
                return fail(
                    variant->name,
                    "right-channel decoded PCM changed or delta state was not reset");
        }
    }

    printf(
        "phase6c-legacy-it-decoder[%s]: PASS channels=%lu frames=%lu\n",
        variant->name,
        (unsigned long)variant->channels,
        (unsigned long)variant->frames);

    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    psy_audio_dispose();
    return 0;
}
