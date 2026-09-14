/*
** PSYCLE-LINUX Phase 5C production native callback timing regression.
**
** The retained native-machine ABI asks CFxCallback for an integer number of
** samples per tracker line. Gate the real PluginFxCallback adapter used by the
** Linux host so family-level timing tests cannot pass with synthetic timing
** that production plugins never receive.
*/

#include <cmath>
#include <cstdio>

#include <audioconfig.h>
#include <machine.h>
#include <player.h>
#include <plugin_interface.h>
#include <properties.h>
#include <song.h>

namespace {

struct TimingCallback {
    psy_audio_MachineCallback base;
    psy_audio_MachineCallbackVtable vtable;
    double sample_rate;
    double bpm;
    double lpb;
    double tpb;
};

double callback_samplerate(void* context)
{
    return static_cast<TimingCallback*>(context)->sample_rate;
}

double callback_bpm(void* context)
{
    return static_cast<TimingCallback*>(context)->bpm;
}

double callback_beatspertick(void* context)
{
    const TimingCallback* self = static_cast<TimingCallback*>(context);
    return 1.0 / self->tpb;
}

double callback_beatspersample(void* context)
{
    const TimingCallback* self = static_cast<TimingCallback*>(context);
    return self->bpm / (60.0 * self->sample_rate);
}

double callback_currbeatsperline(void* context)
{
    const TimingCallback* self = static_cast<TimingCallback*>(context);
    return 1.0 / self->lpb;
}

void timing_callback_init(TimingCallback& self)
{
    psy_audio_machinecallback_init(&self.base);
    self.vtable = *self.base.vtable;
    self.vtable.samplerate = callback_samplerate;
    self.vtable.bpm = callback_bpm;
    self.vtable.beatspertick = callback_beatspertick;
    self.vtable.beatspersample = callback_beatspersample;
    self.vtable.currbeatsperline = callback_currbeatsperline;
    self.base.vtable = &self.vtable;
    self.sample_rate = 44100.0;
    self.bpm = 120.0;
    self.lpb = 4.0;
    self.tpb = 24.0;
}

int expect_line(CMachineInterface& machine, TimingCallback& callback,
    double sample_rate, double bpm, double lpb, double tpb, int expected)
{
    callback.sample_rate = sample_rate;
    callback.bpm = bpm;
    callback.lpb = lpb;
    callback.tpb = tpb;
    const int actual = machine.pCB->GetTickLength();
    if (actual != expected) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL sr=%.0f bpm=%.0f lpb=%.0f tpb=%.0f expected=%d got=%d\n",
            sample_rate, bpm, lpb, tpb, expected, actual);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: line PASS sr=%.0f bpm=%.0f lpb=%.0f tpb=%.0f samples=%d\n",
        sample_rate, bpm, lpb, tpb, actual);
    return 0;
}

class DummyMachine : public CMachineInterface {};

int expect_real_player_callback()
{
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_MachineCallback callback;
    psy_audio_Player player;
    psy_audio_Song* song;
    DummyMachine machine;
    int rc = 0;

    psy_audio_init();
    config = psy_property_allocinit_key(NULL);
    if (!config) {
        psy_audio_dispose();
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL could not allocate player configuration\n");
        return 1;
    }

    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_machinecallback_init(&callback);
    psy_audio_player_init(&player, &callback, NULL,
        psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = psy_audio_song_alloc_init(&player.machinefactory);
    if (!song) {
        psy_audio_player_dispose(&player);
        psy_audio_audioconfig_dispose(&audioconfig);
        psy_property_deallocate(config);
        psy_audio_dispose();
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL could not allocate production song\n");
        return 1;
    }

    psy_audio_song_set_bpm(song, 120.0);
    psy_audio_song_set_lpb(song, 4);
    psy_audio_song_set_tpb(song, 24);
    psy_audio_player_set_song(&player, song);

    mi_resetcallback(&machine);
    mi_setcallback(&machine, &callback);
    if (!machine.pCB) {
        rc = 1;
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL production adapter was not installed for player callback\n");
    } else {
        const double sample_rate = callback.vtable->samplerate(&callback);
        const double bpm = callback.vtable->bpm(&callback);
        const double beats_per_tick = callback.vtable->beatspertick(&callback);
        const double beats_per_sample = callback.vtable->beatspersample(&callback);
        const double beats_per_line = callback.vtable->currbeatsperline(&callback);
        const int transport_tick_samples = (int)(beats_per_tick / beats_per_sample);
        const int line_samples = (int)(beats_per_line / beats_per_sample);
        const int actual = machine.pCB->GetTickLength();

        if (std::fabs(sample_rate - 44100.0) > 0.5 ||
                std::fabs(bpm - 120.0) > 0.0001 ||
                psy_audio_song_lpb(song) != 4 || psy_audio_song_tpb(song) != 24 ||
                transport_tick_samples != 918 || line_samples != 5512 || actual != 5512) {
            rc = 1;
            std::fprintf(stderr,
                "phase5-druttis-production-callback: FAIL actual player callback sr=%.0f bpm=%.3f lpb=%lu tpb=%lu transport-tick=%d line=%d adapter=%d\n",
                sample_rate, bpm,
                (unsigned long)psy_audio_song_lpb(song),
                (unsigned long)psy_audio_song_tpb(song),
                transport_tick_samples, line_samples, actual);
        } else {
            std::printf(
                "phase5-druttis-production-callback: actual-player PASS sr=44100 bpm=120 lpb=4 tpb=24 transport-tick=918 line=5512\n");
        }
    }

    if (machine.pCB) {
        mi_dispose(&machine);
        machine.pCB = nullptr;
    }
    psy_audio_player_set_empty_song(&player);
    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    psy_audio_dispose();
    return rc;
}

} // namespace

int main()
{
    TimingCallback callback;
    DummyMachine machine;
    timing_callback_init(callback);
    mi_resetcallback(&machine);
    mi_setcallback(&machine, &callback.base);

    if (!machine.pCB) {
        std::fprintf(stderr, "phase5-druttis-production-callback: FAIL adapter was not installed\n");
        return 1;
    }

    int rc = 0;
    if ((rc = expect_line(machine, callback, 44100.0, 120.0, 4.0, 24.0, 5512)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 4.0, 24.0, 11025)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 137.0, 4.0, 24.0, 9656)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 8.0, 24.0, 5512)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 4.0, 48.0, 11025)) == 0 &&
            (rc = expect_real_player_callback()) == 0) {
        std::printf("phase5-druttis-production-callback: PASS tracker-line-derived native timing\n");
    }

    mi_dispose(&machine);
    machine.pCB = nullptr;
    return rc;
}
