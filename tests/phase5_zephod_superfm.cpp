/*
** PSYCLE-LINUX Phase 5C Zephod SuperFM preservation regression.
**
** Loads the retained Zephod/Arguru SuperFM generator through Psycle's native
** ABI and freezes its historical metadata plus representative synthesis,
** tracker-volume, release and live sample-rate behavior.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"VCA Attack", "VCA Attack in smp", 16, 65535, psycle::plugin_interface::MPF_STATE, 29},
    {"VCA Decay", "VCA Decay in smp", 16, 65535, psycle::plugin_interface::MPF_STATE, 46974},
    {"VCA Sustain Length", "VCA Sustain lenght in smp", 0, 65535, psycle::plugin_interface::MPF_STATE, 944},
    {"VCA Sustain Level", "VCA Sustain level", 1, 255, psycle::plugin_interface::MPF_STATE, 13},
    {"VCA Release", "VCA Release in smp", 1, 65535, psycle::plugin_interface::MPF_STATE, 2414},
    {"MOD Env Attack", "MOD Envelope attack in smp", 16, 65535, psycle::plugin_interface::MPF_STATE, 16},
    {"MOD Env Decay", "MOD Envelope decay in smp", 16, 65535, psycle::plugin_interface::MPF_STATE, 15717},
    {"MOD Env Sustain Len", "MOD Envelope sustain lenght in smp", 0, 65535, psycle::plugin_interface::MPF_STATE, 10967},
    {"MOD Env Sustain Level", "MOD Envelope sustain level", 1, 255, psycle::plugin_interface::MPF_STATE, 8},
    {"MOD Env Release", "MOD Envelope release in smp", 1, 65535, psycle::plugin_interface::MPF_STATE, 3714},
    {"MOD1 Amount Offset", "Modulator 1 depth", 0, 1000, psycle::plugin_interface::MPF_STATE, 150},
    {"MOD2 Amount Offset", "Modulator 2 depth", 0, 1000, psycle::plugin_interface::MPF_STATE, 0},
    {"MOD3 Amount Offset", "Modulator 3 depth", 0, 1000, psycle::plugin_interface::MPF_STATE, 51},
    {"MOD1 Env Amount", "Modulator 1 envelope amount", -256, 256, psycle::plugin_interface::MPF_STATE, 256},
    {"MOD2 Env Amount", "Modulator 2 envelope amount", -256, 256, psycle::plugin_interface::MPF_STATE, 94},
    {"MOD3 Env Amount", "Modulator 3 envelope amount", -256, 256, psycle::plugin_interface::MPF_STATE, 110},
    {"Osci Wave", "Oscillator waveform", 0, 3, psycle::plugin_interface::MPF_STATE, 0},
    {"Mod Wave", "Modulator waveform", 0, 3, psycle::plugin_interface::MPF_STATE, 0},
    {"FM Route Mode", "Modulator Routing Mode", 0, 3, psycle::plugin_interface::MPF_STATE, 0},
    {"Finetune", "Pitch finetune", -128, 256, psycle::plugin_interface::MPF_STATE, 128},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}
    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sample_rate_ / 8; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-zephod-superfm: FAIL: %s\n", message);
    return 1;
}

bool near(float a, float b, double tolerance = 1.0e-4)
{
    return std::fabs(static_cast<double>(a - b)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0110 ||
            info->Flags != psycle::plugin_interface::GENERATOR ||
            info->numCols != 2) {
        return fail("SuperFM ABI/version/type/column metadata changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Zephod SuperFM (Arguru Remix)") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "SuperFM") != 0 ||
            !info->Author || std::strcmp(info->Author, "Zephod / Arguru") != 0) {
        return fail("SuperFM identity metadata changed");
    }
    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters) {
        return fail("SuperFM parameter count/table changed");
    }
    for (int i = 0; i < expected_count; ++i) {
        const CMachineParameter* actual = info->Parameters[i];
        const ExpectedParameter& expected = EXPECTED_PARAMETERS[i];
        if (!actual || !actual->Name || !actual->Description ||
                std::strcmp(actual->Name, expected.name) != 0 ||
                std::strcmp(actual->Description, expected.description) != 0 ||
                actual->MinValue != expected.min_value ||
                actual->MaxValue != expected.max_value ||
                actual->Flags != expected.flags ||
                actual->DefValue != expected.default_value) {
            std::fprintf(stderr,
                "phase5-zephod-superfm: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-zephod-superfm: metadata PASS parameters=20 version=0x0110\n");
    return 0;
}

void apply_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    for (int i = 0; i < info->numParameters; ++i) {
        machine->Vals[i] = info->Parameters[i]->DefValue;
    }
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }
}

std::vector<float> render_note(CMachineInterface* machine, int samples,
    int command = 0, int value = 0, int note = 69)
{
    machine->SeqTick(0, note, 0, command, value);
    std::vector<float> signal;
    signal.reserve(static_cast<std::size_t>(samples));
    int remaining = samples;
    while (remaining > 0) {
        const int block = remaining > psycle::plugin_interface::MAX_BUFFER_LENGTH
            ? psycle::plugin_interface::MAX_BUFFER_LENGTH : remaining;
        std::vector<float> left(static_cast<std::size_t>(block), 0.0f);
        std::vector<float> right(static_cast<std::size_t>(block), 0.0f);
        machine->Work(left.data(), right.data(), block, 1);
        for (int i = 0; i < block; ++i) {
            const float l = left[static_cast<std::size_t>(i)];
            const float r = right[static_cast<std::size_t>(i)];
            if (!std::isfinite(l) || !std::isfinite(r) || !near(l, r, 1.0e-5)) {
                return {};
            }
            signal.push_back(l);
        }
        remaining -= block;
    }
    return signal;
}

std::vector<float> render_continuation(CMachineInterface* machine, int samples)
{
    std::vector<float> signal;
    signal.reserve(static_cast<std::size_t>(samples));
    int remaining = samples;
    while (remaining > 0) {
        const int block = remaining > psycle::plugin_interface::MAX_BUFFER_LENGTH
            ? psycle::plugin_interface::MAX_BUFFER_LENGTH : remaining;
        std::vector<float> left(static_cast<std::size_t>(block), 0.0f);
        std::vector<float> right(static_cast<std::size_t>(block), 0.0f);
        machine->Work(left.data(), right.data(), block, 1);
        for (int i = 0; i < block; ++i) {
            const float l = left[static_cast<std::size_t>(i)];
            const float r = right[static_cast<std::size_t>(i)];
            if (!std::isfinite(l) || !std::isfinite(r) || !near(l, r, 1.0e-5)) {
                return {};
            }
            signal.push_back(l);
        }
        remaining -= block;
    }
    return signal;
}

double rms(const std::vector<float>& signal)
{
    double sum = 0.0;
    for (float sample : signal) {
        const double v = static_cast<double>(sample);
        sum += v * v;
    }
    return signal.empty() ? 0.0 : std::sqrt(sum / signal.size());
}

double tail_rms(const std::vector<float>& signal, std::size_t count)
{
    if (signal.empty() || count == 0) return 0.0;
    if (count > signal.size()) count = signal.size();
    double sum = 0.0;
    const std::size_t start = signal.size() - count;
    for (std::size_t i = start; i < signal.size(); ++i) {
        const double v = static_cast<double>(signal[i]);
        sum += v * v;
    }
    return std::sqrt(sum / static_cast<double>(count));
}

bool same_signal(const std::vector<float>& a, const std::vector<float>& b,
    double tolerance = 2.0e-4)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!near(a[i], b[i], tolerance)) return false;
    }
    return true;
}

bool same_magnitude_signal(const std::vector<float>& a, const std::vector<float>& b,
    double tolerance)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const double delta = std::fabs(std::fabs(static_cast<double>(a[i])) -
            std::fabs(static_cast<double>(b[i])));
        if (delta > tolerance) return false;
    }
    return true;
}

int sign_changes(const std::vector<float>& signal)
{
    int changes = 0;
    int previous = 0;
    for (float sample : signal) {
        int current = 0;
        if (sample > 1.0e-4f) current = 1;
        else if (sample < -1.0e-4f) current = -1;
        if (current != 0) {
            if (previous != 0 && current != previous) ++changes;
            previous = current;
        }
    }
    return changes;
}

int verify_default_render(CMachineInterface* a, CMachineInterface* b,
    const CMachineInfo* info)
{
    TestCallback cb_a(44100);
    TestCallback cb_b(44100);
    a->pCB = &cb_a;
    b->pCB = &cb_b;
    apply_defaults(a, info);
    apply_defaults(b, info);
    const std::vector<float> first = render_note(a, 2048);
    const std::vector<float> second = render_note(b, 2048);
    const double level = rms(first);
    if (first.empty() || second.empty() || level <= 0.01 ||
            !same_signal(first, second)) {
        return fail("fresh default SuperFM instances are not deterministic and active");
    }
    std::printf("phase5-zephod-superfm: deterministic PASS rms=%.6f\n", level);
    return 0;
}

int verify_volume_command(CMachineInterface* full, CMachineInterface* half,
    const CMachineInfo* info)
{
    TestCallback cb_full(44100);
    TestCallback cb_half(44100);
    full->pCB = &cb_full;
    half->pCB = &cb_half;
    apply_defaults(full, info);
    apply_defaults(half, info);
    const std::vector<float> full_signal = render_note(full, 1024);
    const std::vector<float> half_signal = render_note(half, 1024, 0x0C, 128);
    const double full_rms = rms(full_signal);
    const double half_rms = rms(half_signal);
    if (full_signal.empty() || half_signal.empty() || full_rms <= 0.01) {
        return fail("SuperFM 0C80 render failed");
    }
    const double ratio = half_rms / full_rms;
    const double expected = 126.0 / 254.0;
    if (std::fabs(ratio - expected) > 0.002) {
        std::fprintf(stderr,
            "phase5-zephod-superfm: FAIL: 0C80 ratio %.6f expected %.6f\n",
            ratio, expected);
        return 1;
    }
    std::printf("phase5-zephod-superfm: volume-command PASS command=0C80 ratio=%.6f\n",
        ratio);
    return 0;
}

int verify_noteoff_release(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    machine->pCB = &callback;
    apply_defaults(machine, info);
    const std::vector<float> lead = render_note(machine, 256);
    if (lead.empty() || rms(lead) <= 0.01) return fail("SuperFM note-on failed before release test");
    machine->SeqTick(0, psycle::plugin_interface::NOTE_NOTEOFF, 0, 0, 0);
    const std::vector<float> release = render_continuation(machine, 3000);
    if (release.empty()) return fail("SuperFM release render failed");

    bool saw_active = false;
    std::size_t last_active = 0;
    for (std::size_t i = 0; i < release.size(); ++i) {
        if (std::fabs(release[i]) > 1.0e-3f) {
            saw_active = true;
            last_active = i;
        }
    }
    if (!saw_active || last_active + 1 < 2300 || last_active + 1 > 2450) {
        return fail("SuperFM retained 2414-sample VCA release ended at the wrong duration");
    }
    for (std::size_t i = release.size() - 32; i < release.size(); ++i) {
        if (!near(release[i], 0.0f, 1.0e-5)) {
            return fail("SuperFM retained VCA release no longer reaches silence");
        }
    }
    std::printf(
        "phase5-zephod-superfm: noteoff PASS release-default=2414 active-through=%zu tail=silent\n",
        last_active + 1);
    return 0;
}

void configure_rate_transition(CMachineInterface* machine)
{
    /* Keep both envelopes in a long attack when the rate changes, then let them
    ** reach the historical until-noteoff sustain. Pulse/no-FM output makes the
    ** absolute sample magnitude a direct envelope oracle while sign changes
    ** retain a simple pitch oracle. */
    machine->ParameterTweak(0, 4410);
    machine->ParameterTweak(1, 16);
    machine->ParameterTweak(2, 1);
    machine->ParameterTweak(3, 128);
    machine->ParameterTweak(4, 4410);
    machine->ParameterTweak(5, 4410);
    machine->ParameterTweak(6, 16);
    machine->ParameterTweak(7, 1);
    machine->ParameterTweak(8, 128);
    machine->ParameterTweak(9, 4410);
    machine->ParameterTweak(10, 0);
    machine->ParameterTweak(11, 0);
    machine->ParameterTweak(12, 0);
    machine->ParameterTweak(13, 0);
    machine->ParameterTweak(14, 0);
    machine->ParameterTweak(15, 0);
    machine->ParameterTweak(16, 1);
    machine->ParameterTweak(17, 0);
    machine->ParameterTweak(18, 0);
    machine->ParameterTweak(19, 128);
}

int verify_rate_transition(CMachineInterface* live, CMachineInterface* target,
    const CMachineInfo* info)
{
    TestCallback live_cb(44100);
    TestCallback target_cb(88200);
    live->pCB = &live_cb;
    target->pCB = &target_cb;
    apply_defaults(live, info);
    apply_defaults(target, info);
    configure_rate_transition(live);
    configure_rate_transition(target);

    live->SeqTick(0, 69, 0, 0, 0);
    target->SeqTick(0, 69, 0, 0, 0);

    /* Advance both machines by the same 10 ms wall-clock interval while the
    ** VCA/MOD envelopes are still in attack. */
    const std::vector<float> live_before = render_continuation(live, 441);
    const std::vector<float> target_before = render_continuation(target, 882);
    if (live_before.empty() || target_before.empty() ||
            rms(live_before) <= 0.01 || rms(target_before) <= 0.01 ||
            std::fabs(std::fabs(static_cast<double>(live_before.back())) -
                std::fabs(static_cast<double>(target_before.back()))) > 2.0) {
        return fail("SuperFM in-flight rate-transition setup did not align at equal wall-clock time");
    }

    live_cb.set_sample_rate(88200);
    live->SequencerTick();

    /* Do not Stop()/retrigger here: production driver reconfiguration leaves
    ** machines alive. This continuation therefore checks the active attack
    ** slope, oscillator pitch, and later until-noteoff sustain in one path. */
    const std::vector<float> live_after = render_continuation(live, 10000);
    const std::vector<float> target_after = render_continuation(target, 10000);
    if (live_after.empty() || target_after.empty() ||
            !same_magnitude_signal(live_after, target_after, 2.0)) {
        return fail("SuperFM in-flight envelope timing changed across 44.1 -> 88.2 kHz transition");
    }
    const int live_changes = sign_changes(live_after);
    const int target_changes = sign_changes(target_after);
    if (std::abs(live_changes - target_changes) > 1) {
        return fail("SuperFM active-note pitch changed across 44.1 -> 88.2 kHz transition");
    }
    if (tail_rms(live_after, 256) <= 0.01 || tail_rms(target_after, 256) <= 0.01) {
        return fail("SuperFM sustain<16 no longer remains active until noteoff after rate transition");
    }
    std::printf(
        "phase5-zephod-superfm: rate-transition PASS sr=44100->88200 in-flight=attack pitch=stable sustain<16=until-noteoff\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ZEPHOD_SUPERFM_SO\n", argv[0]);
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-zephod-superfm: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-zephod-superfm: FAIL: native ABI exports missing: %s\n",
            error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(info);
    std::vector<CMachineInterface*> machines;
    for (int i = 0; rc == 0 && i < 7; ++i) {
        CMachineInterface* machine = create_machine();
        if (!machine || !machine->Vals) {
            rc = fail("CreateMachine returned an unusable SuperFM instance");
            break;
        }
        machines.push_back(machine);
    }
    if (rc == 0) rc = verify_default_render(machines[0], machines[1], info);
    if (rc == 0) rc = verify_volume_command(machines[2], machines[3], info);
    if (rc == 0) rc = verify_noteoff_release(machines[4], info);
    if (rc == 0) rc = verify_rate_transition(machines[5], machines[6], info);

    for (CMachineInterface* machine : machines) delete_machine(*machine);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-zephod-superfm: PASS\n");
    std::printf("machine: Zephod SuperFM (Arguru Remix)\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: deterministic FM + 0Cxx volume + timed Note Off release + in-flight sample-rate transition\n");
    return 0;
}
