/*
** PSYCLE-LINUX Phase 5C production native callback timing regression.
**
** The retained native-machine ABI asks CFxCallback for an integer number of
** samples per tracker tick. Gate the real PluginFxCallback adapter used by the
** Linux host so family-level timing tests cannot pass with synthetic timing
** that production plugins never receive.
*/

#include <cstdio>

#include <machine.h>
#include <plugin_interface.h>

namespace {

struct TimingCallback {
    psy_audio_MachineCallback base;
    psy_audio_MachineCallbackVtable vtable;
    double sample_rate;
    double bpm;
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

void timing_callback_init(TimingCallback& self)
{
    psy_audio_machinecallback_init(&self.base);
    self.vtable = *self.base.vtable;
    self.vtable.samplerate = callback_samplerate;
    self.vtable.bpm = callback_bpm;
    self.vtable.beatspertick = callback_beatspertick;
    self.vtable.beatspersample = callback_beatspersample;
    self.base.vtable = &self.vtable;
    self.sample_rate = 44100.0;
    self.bpm = 120.0;
    self.tpb = 4.0;
}

int expect_tick(CMachineInterface& machine, TimingCallback& callback,
    double sample_rate, double bpm, double tpb, int expected)
{
    callback.sample_rate = sample_rate;
    callback.bpm = bpm;
    callback.tpb = tpb;
    const int actual = machine.pCB->GetTickLength();
    if (actual != expected) {
        std::fprintf(stderr,
            "phase5-druttis-production-callback: FAIL sr=%.0f bpm=%.0f tpb=%.0f expected=%d got=%d\n",
            sample_rate, bpm, tpb, expected, actual);
        return 1;
    }
    std::printf(
        "phase5-druttis-production-callback: tick PASS sr=%.0f bpm=%.0f tpb=%.0f samples=%d\n",
        sample_rate, bpm, tpb, actual);
    return 0;
}

class DummyMachine : public CMachineInterface {};

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
    if ((rc = expect_tick(machine, callback, 44100.0, 120.0, 4.0, 5512)) == 0 &&
            (rc = expect_tick(machine, callback, 88200.0, 120.0, 4.0, 11025)) == 0 &&
            (rc = expect_tick(machine, callback, 88200.0, 137.0, 4.0, 9656)) == 0 &&
            (rc = expect_tick(machine, callback, 88200.0, 120.0, 8.0, 5512)) == 0) {
        std::printf("phase5-druttis-production-callback: PASS host-derived native tick timing\n");
    }

    mi_dispose(&machine);
    machine.pCB = nullptr;
    return rc;
}
