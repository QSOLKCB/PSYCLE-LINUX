/*
** PSYCLE-LINUX Phase 5 JM Drum preservation regression.
**
** Loads JAZ's retained JM Drum generator through Psycle's native ABI and
** freezes its historical metadata plus representative deterministic synthesis,
** volume-command, note-release and live sample-rate behavior.
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
    {"Start Freq", "Start Frequency (Hz)", 80, 1200, psycle::plugin_interface::MPF_STATE, 225},
    {"End Freq", "End Frequency (Hz)", 20, 400, psycle::plugin_interface::MPF_STATE, 40},
    {"Freq Decay", "Frequency Down Speed (in mscs)", 10, 300, psycle::plugin_interface::MPF_STATE, 49},
    {"Start Amp", "Starting Amplitude", 0, 32767, psycle::plugin_interface::MPF_STATE, 32767},
    {"End Amp", "Ending Amplitude", 0, 32767, psycle::plugin_interface::MPF_STATE, 21844},
    {"Length", "Duration of the note (ms)", 10, 500, psycle::plugin_interface::MPF_STATE, 220},
    {"Volume", "Volume (0-32767)", 0, 32767, psycle::plugin_interface::MPF_STATE, 32767},
    {"Dec Mode", "Decrement Mode", 0, 3, psycle::plugin_interface::MPF_STATE, 2},
    {"Compatibility", "Compatible With version", 0, 1, psycle::plugin_interface::MPF_STATE, 1},
    {"NNA Command", "NNA Command when New Note", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Attack up to", "Attack from 0/100 to x/100", 0, 99, psycle::plugin_interface::MPF_STATE, 2},
    {"Decay up to", "Decay from Attack to x/100", 1, 100, psycle::plugin_interface::MPF_STATE, 51},
    {"Sustain Volume", "Sustain Volume at Decay-End Pos", 0, 100, psycle::plugin_interface::MPF_STATE, 77},
    {"Drum&Thump Mix", "Mix of Drum and Thump Signals", -100, 100, psycle::plugin_interface::MPF_STATE, -53},
    {"Thump Length", "Thump Length", 1, 120, psycle::plugin_interface::MPF_STATE, 100},
    {"Thump Freq", "Thump Frequency", 220, 6000, psycle::plugin_interface::MPF_STATE, 2000},
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
    std::fprintf(stderr, "phase5-jm-drum: FAIL: %s\n", message);
    return 1;
}

bool near(float a, float b, double tolerance = 1.0e-5)
{
    return std::fabs(static_cast<double>(a - b)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0250 ||
            info->Flags != psycle::plugin_interface::GENERATOR ||
            info->numCols != 4) {
        return fail("JM Drum ABI/version/type/column metadata changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Drum Synth v.2.5") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Drum2.5") != 0 ||
            !info->Author || std::strncmp(info->Author, "[JAZ] on ", 9) != 0) {
        return fail("JM Drum identity metadata changed");
    }
    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters) {
        return fail("JM Drum parameter count/table changed");
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
                "phase5-jm-drum: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-jm-drum: metadata PASS parameters=16 version=0x0250\n");
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
    int note = 48, int command = 0, int value = 0)
{
    std::vector<float> left(samples, 0.0f);
    std::vector<float> right(samples, 0.0f);
    machine->SeqTick(0, note, 0, command, value);
    machine->Work(left.data(), right.data(), samples, 1);
    for (int i = 0; i < samples; ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]) ||
                !near(left[i], right[i], 1.0e-6)) {
            return {};
        }
    }
    return left;
}

bool same_signal(const std::vector<float>& a, const std::vector<float>& b,
    double tolerance = 1.0e-5)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!near(a[i], b[i], tolerance)) return false;
    }
    return true;
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

int verify_deterministic_default(CMachineInterface* a, CMachineInterface* b,
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
    if (first.empty() || second.empty() || rms(first) < 1.0 ||
            !same_signal(first, second)) {
        return fail("fresh default JM Drum instances are not deterministic/active");
    }
    std::printf("phase5-jm-drum: deterministic PASS rms=%.6f\n", rms(first));
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
    const std::vector<float> half_signal = render_note(half, 1024, 48, 0x0C, 128);
    if (full_signal.empty() || half_signal.empty()) {
        return fail("JM Drum volume-command render failed");
    }
    for (std::size_t i = 0; i < full_signal.size(); ++i) {
        if (!near(half_signal[i], full_signal[i] * 0.5f, 2.0e-4)) {
            return fail("JM Drum 0C80 command no longer applies retained half-volume scaling");
        }
    }
    std::printf("phase5-jm-drum: volume-command PASS command=0C80 scale=0.5\n");
    return 0;
}

int verify_noteoff_release(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    machine->pCB = &callback;
    apply_defaults(machine, info);
    std::vector<float> lead = render_note(machine, 64);
    if (lead.empty()) return fail("JM Drum note-on failed before release test");

    machine->SeqTick(0, psycle::plugin_interface::NOTE_NOTEOFF, 0, 0, 0);
    std::vector<float> left(320, 0.0f);
    std::vector<float> right(320, 0.0f);
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);
    for (std::size_t i = left.size() - 32; i < left.size(); ++i) {
        if (!near(left[i], 0.0f, 1.0e-5) || !near(right[i], 0.0f, 1.0e-5)) {
            return fail("JM Drum Note Off no longer reaches silence after retained 256-sample release");
        }
    }
    std::printf("phase5-jm-drum: noteoff PASS release=256 tail=silent\n");
    return 0;
}

int verify_rate_transition(CMachineInterface* live, CMachineInterface* target,
    CMachineInterface* stale, const CMachineInfo* info)
{
    TestCallback live_cb(44100);
    TestCallback target_cb(88200);
    TestCallback stale_cb(44100);
    live->pCB = &live_cb;
    target->pCB = &target_cb;
    stale->pCB = &stale_cb;
    apply_defaults(live, info);
    apply_defaults(target, info);
    apply_defaults(stale, info);

    const std::vector<float> active = render_note(live, 64);
    if (active.empty()) return fail("JM Drum live transition pre-roll failed");
    live_cb.set_sample_rate(88200);
    live->SequencerTick();

    /* SequencerTick stops active voices on a rate change. Drain the retained
    ** 256-sample release before comparing a fresh target-rate note. */
    std::vector<float> drain_l(320, 0.0f);
    std::vector<float> drain_r(320, 0.0f);
    live->Work(drain_l.data(), drain_r.data(), static_cast<int>(drain_l.size()), 1);

    const std::vector<float> live_signal = render_note(live, 2048);
    const std::vector<float> target_signal = render_note(target, 2048);
    const std::vector<float> stale_signal = render_note(stale, 2048);
    if (live_signal.empty() || target_signal.empty() || stale_signal.empty() ||
            !same_signal(live_signal, target_signal, 3.0e-4) ||
            same_signal(live_signal, stale_signal, 3.0e-4)) {
        return fail("JM Drum live 44.1 -> 88.2 kHz transition diverged from fresh target rate");
    }
    std::printf("phase5-jm-drum: rate-transition PASS sr=44100->88200\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_JMDRUM_SO\n", argv[0]);
        return 2;
    }
    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-jm-drum: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-jm-drum: FAIL: native ABI exports missing: %s\n",
            error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(info);
    std::vector<CMachineInterface*> machines;
    for (int i = 0; rc == 0 && i < 8; ++i) {
        CMachineInterface* machine = create_machine();
        if (!machine || !machine->Vals) {
            rc = fail("CreateMachine returned an unusable JM Drum instance");
            break;
        }
        machines.push_back(machine);
    }
    if (rc == 0) rc = verify_deterministic_default(machines[0], machines[1], info);
    if (rc == 0) rc = verify_volume_command(machines[2], machines[3], info);
    if (rc == 0) rc = verify_noteoff_release(machines[4], info);
    if (rc == 0) rc = verify_rate_transition(machines[5], machines[6], machines[7], info);

    for (CMachineInterface* machine : machines) delete_machine(*machine);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-jm-drum: PASS\n");
    std::printf("machine: Drum Synth v.2.5\n");
    std::printf("parameters: 16\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: deterministic drum synthesis + 0C volume + Note Off + live sample-rate transition\n");
    return 0;
}
