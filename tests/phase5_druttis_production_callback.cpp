/*
** PSYCLE-LINUX Phase 5C production native callback timing regression.
**
** The retained native-machine ABI asks CFxCallback for an integer number of
** samples per tracker line. Gate the real PluginFxCallback adapter used by the
** Linux host so family-level timing tests cannot pass with synthetic timing
** that production plugins never receive.
*/

#include <cstdio>
#include <limits>

#include <machine.h>
#include <plugin_interface.h>

extern "C" {
void* phase5_druttis_player_callback_fixture_create(void);
void phase5_druttis_player_callback_fixture_destroy(void*);
psy_audio_MachineCallback* phase5_druttis_player_callback_fixture_callback(void*);
double phase5_druttis_player_callback_fixture_samplerate(void*);
double phase5_druttis_player_callback_fixture_bpm(void*);
unsigned long phase5_druttis_player_callback_fixture_lpb(void*);
unsigned long phase5_druttis_player_callback_fixture_tpb(void*);
int phase5_druttis_player_callback_fixture_transport_tick_samples(void*);
int phase5_druttis_player_callback_fixture_line_samples(void*);
}

namespace {

struct TimingCallback {
    psy_audio_MachineCallback base;
    psy_audio_MachineCallbackVtable vtable;
    double sample_rate;
    double bpm;
    double lpb;
    double tpb;
    double line_beats_override;
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
    return self->line_beats_override > 0.0
        ? self->line_beats_override
        : 1.0 / self->lpb;
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
    /* Model an attached/live host without pulling the legacy Player definition
    ** into this C++ translation unit. The synthetic vtable above owns every
    ** timing read; production only uses player presence to distinguish it from
    ** a freshly initialized headless callback. */
    self.base.player = (struct psy_audio_Player*)1;
    self.sample_rate = 44100.0;
    self.bpm = 120.0;
    self.lpb = 4.0;
    self.tpb = 24.0;
    self.line_beats_override = 0.0;
}

int expect_line(CMachineInterface& machine, TimingCallback& callback,
    double sample_rate, double bpm, double lpb, double tpb, int expected)
{
    callback.sample_rate = sample_rate;
    callback.bpm = bpm;
    callback.lpb = lpb;
    callback.tpb = tpb;
    callback.line_beats_override = 0.0;
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

int expect_precision_guards(CMachineInterface& machine, TimingCallback& callback)
{
    /* Reproduce callback arithmetic that lands more than one ULP below an
    ** exact integral line: 263.9999999999999 must still expose 264 samples. */
    callback.sample_rate = 8000.0;
    callback.bpm = 32.0;
    callback.lpb = 11.0;
    callback.tpb = 24.0;
    const double bps = callback_beatspersample(&callback);
    callback.line_beats_override = 263.9999999999999 * bps;
    const double raw_integral = callback_currbeatsperline(&callback) / bps;
    const int integral = machine.pCB->GetTickLength();
    if (!(raw_integral < 264.0) || integral != 264) {
        callback.line_beats_override = 0.0;
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL integral snap raw=%.17g expected=264 got=%d\n",
            raw_integral, integral);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: integral-snap PASS samples=264\n");

    /* Positive sub-sample line durations must never collapse the native ABI to
    ** zero: retained consumers use the value as a divisor and delay quantum. */
    callback.sample_rate = 8000.0;
    callback.bpm = 999.0;
    callback.lpb = 1.0;
    callback.tpb = 24.0;
    const double fast_bps = callback_beatspersample(&callback);
    callback.line_beats_override = fast_bps * 0.5;
    const double raw_subsample = callback_currbeatsperline(&callback) / fast_bps;
    const int subsample = machine.pCB->GetTickLength();
    callback.line_beats_override = 0.0;
    if (!(raw_subsample > 0.0 && raw_subsample < 1.0) || subsample != 1) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL sub-sample floor raw=%.17g expected=1 got=%d\n",
            raw_subsample, subsample);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: subsample-floor PASS samples=1\n");
    return 0;
}

int expect_extreme_line_guards(CMachineInterface& machine, TimingCallback& callback)
{
    callback.sample_rate = 96000.0;
    callback.bpm = 32.0;
    callback.lpb = 4.0;
    callback.tpb = 24.0;
    callback.line_beats_override = 25000.0;

    const double raw_samples = callback_currbeatsperline(&callback) /
        callback_beatspersample(&callback);
    const int max_int = std::numeric_limits<int>::max();
    const int legacy_tick_limit = max_int / 256;
    const int clamped = machine.pCB->GetTickLength();
    const long long crossdelay_product = (long long)clamped * 8LL * 2LL;
    const long long bexphase_product = (long long)clamped * 32LL;
    const long long sublime_product = (long long)clamped * 256LL;

    if (raw_samples <= (double)legacy_tick_limit || clamped != legacy_tick_limit ||
            crossdelay_product > max_int || bexphase_product > max_int ||
            sublime_product > max_int || crossdelay_product < 0 ||
            bexphase_product < 0 || sublime_product < 0) {
        callback.line_beats_override = 0.0;
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL legacy cap raw=%.0f cap=%d got=%d crossdelay=%lld bexphase=%lld sublime=%lld int-max=%d\n",
            raw_samples, legacy_tick_limit, clamped,
            crossdelay_product, bexphase_product, sublime_product, max_int);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: legacy-cap PASS raw-samples=%.0f cap=%d crossdelay=%lld bexphase=%lld sublime=%lld\n",
        raw_samples, clamped,
        crossdelay_product, bexphase_product, sublime_product);

    callback.line_beats_override = std::numeric_limits<double>::infinity();
    const int fallback = machine.pCB->GetTickLength();
    callback.line_beats_override = 0.0;
    if (fallback != 45000) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL nonfinite fallback expected=45000 got=%d\n",
            fallback);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: nonfinite-fallback PASS samples=45000\n");
    return 0;
}

int expect_headless_callback_fallback()
{
    psy_audio_MachineCallback callback;
    DummyMachine machine;
    int rc = 0;

    psy_audio_machinecallback_init(&callback);
    if (callback.player != nullptr) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL fresh callback unexpectedly has a player\n");
        return 1;
    }

    const double sentinel_beats_per_line = callback.vtable->currbeatsperline(&callback);
    const double sentinel_beats_per_sample = callback.vtable->beatspersample(&callback);
    if ((int)sentinel_beats_per_line != 4096 || (int)sentinel_beats_per_sample != 512) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL fresh callback sentinels changed line=%.0f sample=%.0f\n",
            sentinel_beats_per_line, sentinel_beats_per_sample);
        return 1;
    }

    mi_resetcallback(&machine);
    mi_setcallback(&machine, &callback);
    if (!machine.pCB) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL adapter was not installed for headless callback\n");
        return 1;
    }

    const int actual = machine.pCB->GetTickLength();
    if (actual != 5292) {
        rc = 1;
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL headless fallback expected=5292 got=%d sentinel-ratio=%d\n",
            actual, (int)(sentinel_beats_per_line / sentinel_beats_per_sample));
    } else {
        std::printf(
            "phase5-druttis-production-callback: headless-fallback PASS sentinel-ratio=8 fallback=5292\n");
    }

    mi_dispose(&machine);
    machine.pCB = nullptr;
    return rc;
}

int expect_real_player_callback()
{
    void* fixture;
    psy_audio_MachineCallback* callback;
    DummyMachine machine;
    int rc = 0;

    fixture = phase5_druttis_player_callback_fixture_create();
    if (!fixture) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL could not create real player callback fixture\n");
        return 1;
    }
    callback = phase5_druttis_player_callback_fixture_callback(fixture);
    if (!callback) {
        phase5_druttis_player_callback_fixture_destroy(fixture);
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL real player callback fixture returned no callback\n");
        return 1;
    }

    mi_resetcallback(&machine);
    mi_setcallback(&machine, callback);
    if (!machine.pCB) {
        rc = 1;
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL production adapter was not installed for player callback\n");
    } else {
        const double sample_rate = phase5_druttis_player_callback_fixture_samplerate(fixture);
        const double bpm = phase5_druttis_player_callback_fixture_bpm(fixture);
        const unsigned long lpb = phase5_druttis_player_callback_fixture_lpb(fixture);
        const unsigned long tpb = phase5_druttis_player_callback_fixture_tpb(fixture);
        const int transport_tick_samples =
            phase5_druttis_player_callback_fixture_transport_tick_samples(fixture);
        const int line_samples = phase5_druttis_player_callback_fixture_line_samples(fixture);
        const int actual = machine.pCB->GetTickLength();

        if ((int)sample_rate != 44100 || (int)bpm != 120 ||
                lpb != 4 || tpb != 24 || transport_tick_samples != 918 ||
                line_samples != 5512 || actual != 5512) {
            rc = 1;
            std::fprintf(stderr,
                "phase5-druttis-production-callback: FAIL actual player callback sr=%.0f bpm=%.3f lpb=%lu tpb=%lu transport-tick=%d line=%d adapter=%d\n",
                sample_rate, bpm, lpb, tpb,
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
    phase5_druttis_player_callback_fixture_destroy(fixture);
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
            (rc = expect_line(machine, callback, 44100.0, 35.0, 3.0, 24.0, 25200)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 4.0, 24.0, 11025)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 137.0, 4.0, 24.0, 9656)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 8.0, 24.0, 5512)) == 0 &&
            (rc = expect_line(machine, callback, 88200.0, 120.0, 4.0, 48.0, 11025)) == 0 &&
            (rc = expect_precision_guards(machine, callback)) == 0 &&
            (rc = expect_extreme_line_guards(machine, callback)) == 0 &&
            (rc = expect_headless_callback_fallback()) == 0 &&
            (rc = expect_real_player_callback()) == 0) {
        std::printf("phase5-druttis-production-callback: PASS tracker-line-derived native timing\n");
    }

    mi_dispose(&machine);
    machine.pCB = nullptr;
    return rc;
}