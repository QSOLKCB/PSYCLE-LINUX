/*
** PSYCLE-LINUX Phase 5C LegaSynth TB303 native preservation regression.
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

struct ExpectedSampleMarker {
    std::size_t index;
    float left;
    float right;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"Coarse", "Coarse", -24, 24, psycle::plugin_interface::MPF_STATE, 0},
    {"Fine", "Fine", -100, 100, psycle::plugin_interface::MPF_STATE, 0},
    {"Pitch bend", "Pitch bend", 0, 0x4000, psycle::plugin_interface::MPF_STATE, 0x2000},
    {"Pitch bend depth", "Pitch bend depth", 0, 12, psycle::plugin_interface::MPF_STATE, 12},
    {"Main Volume", "Main Volume", 0, 127, psycle::plugin_interface::MPF_STATE, 127},
    {"Distortion", "Distortion", 0, 100, psycle::plugin_interface::MPF_STATE, 0},
    {"Envelope&Filter", "Envelope&Filter", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"Cutoff", "Cutoff", 0, 0xFFFF, psycle::plugin_interface::MPF_STATE, 0x7FFF},
    {"Cutoff sweep", "Cutoff sweep", -127, 127, psycle::plugin_interface::MPF_STATE, 0},
    {"Cutoff LFO speed", "Cutoff", 0, 0xFF, psycle::plugin_interface::MPF_STATE, 0},
    {"Cutoff LFO depth", "Cutoff LFO depth", 0, 0x2000, psycle::plugin_interface::MPF_STATE, 0},
    {"Resonance", "Resonance", 0, 0xFFFF, psycle::plugin_interface::MPF_STATE, 0},
    {"Modulation", "Modulation", 0, 0xFFFF, psycle::plugin_interface::MPF_STATE, 0},
    {"Decay", "Modulation", 0, 0xFFFF, psycle::plugin_interface::MPF_STATE, 0},
    {"Vibrato&Sweep", "Vibrato&Sweep", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"Vibrato Speed", "Vibrato Speed", 0, 0xFF, psycle::plugin_interface::MPF_STATE, 0},
    {"Vibrato Depth", "Vibrato Depth", 0, 0xFF, psycle::plugin_interface::MPF_STATE, 0},
    {"LFO Type", "LFO Type", 0, 2, psycle::plugin_interface::MPF_STATE, 0},
    {"Vibrato attack", "Vibrato attack", 0, 0xFF, psycle::plugin_interface::MPF_STATE, 0},
    {"sweep delay", "sweep delay", 0, 0xFF, psycle::plugin_interface::MPF_STATE, 0},
    {"sweep amount", "sweep amount", -127, 127, psycle::plugin_interface::MPF_STATE, 0},
    {"Chorus", "Chorus", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"On/Off", "On/Off", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Delay", "Delay", 0, 127, psycle::plugin_interface::MPF_STATE, 3},
    {"LFO Speed", "LFO Speed", 0, 127, psycle::plugin_interface::MPF_STATE, 16},
    {"LFO Depth", "LFO Depth", 0, 127, psycle::plugin_interface::MPF_STATE, 3},
    {"Amount", "Amoun t", 1, 100, psycle::plugin_interface::MPF_STATE, 64},
    {"Stereo Width", "Stereo Width", -127, 127, psycle::plugin_interface::MPF_STATE, -10},
};

/* Frozen from the retained default note-48 source path at 44.1 kHz after
** deterministic initialization was restored. Spread-out markers cover the
** oscillator/filter/envelope trajectory rather than only liveness. */
const ExpectedSampleMarker DEFAULT_NOTE_MARKERS[] = {
    {64, 2234.22974f, 2269.69360f},
    {127, 5407.29053f, 5493.12061f},
    {255, -4679.22314f, -4753.49658f},
    {511, 5020.45459f, 5100.14404f},
    {1023, -175.271317f, -178.053391f},
    {1535, -1979.30615f, -2010.72363f},
    {2047, 421.357361f, 428.045593f},
};
const double DEFAULT_NOTE_RMS = 4252.98775;

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
    std::fprintf(stderr, "phase5-legasynth: FAIL: %s\n", message);
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
            info->PlugVersion != 0x0020 ||
            info->Flags != psycle::plugin_interface::GENERATOR ||
            info->numCols != 2 || info->numParameters != 28) {
        return fail("LegaSynth ABI/version/type/geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, "LegaSynth TB303") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "TB303") != 0 ||
            !info->Author || std::strcmp(info->Author,
                "Juan Linietsky, ported by Sartorius") != 0) {
        return fail("LegaSynth identity metadata changed");
    }
    int state_count = 0;
    int label_count = 0;
    for (int i = 0; i < 28; ++i) {
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
                "phase5-legasynth: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->Flags == psycle::plugin_interface::MPF_STATE) ++state_count;
        if (actual->Flags == psycle::plugin_interface::MPF_LABEL) ++label_count;
    }
    if (state_count != 25 || label_count != 3)
        return fail("LegaSynth state/label partition changed");
    std::printf("phase5-legasynth: metadata PASS version=0x0020 slots=28 state=25 labels=3 identity=TB303\n");
    return 0;
}

int verify_constructor_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    if (!machine || !machine->Vals) return fail("machine/default value storage missing");
    for (int i = 0; i < info->numParameters; ++i) {
        if (machine->Vals[i] != info->Parameters[i]->DefValue) {
            std::fprintf(stderr,
                "phase5-legasynth: FAIL: constructor slot %d expected %d got %d\n",
                i, info->Parameters[i]->DefValue, machine->Vals[i]);
            return 1;
        }
    }
    std::printf("phase5-legasynth: defaults PASS constructor=published-28-slot-state\n");
    return 0;
}

void apply_host_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

int expect_description(CMachineInterface* machine, int param, int value,
    const char* expected)
{
    char text[256] = {};
    if (!machine->DescribeValue(text, param, value) ||
            std::strcmp(text, expected) != 0) {
        std::fprintf(stderr,
            "phase5-legasynth: FAIL: description param=%d value=%d expected '%s' got '%s'\n",
            param, value, expected, text);
        return 1;
    }
    return 0;
}

int verify_descriptions(CMachineInterface* machine)
{
    if (expect_description(machine, 0, 12, "12 notes") ||
            expect_description(machine, 1, -25, "-25 cents") ||
            expect_description(machine, 17, 0, "Sine") ||
            expect_description(machine, 17, 1, "Sawtooth") ||
            expect_description(machine, 17, 2, "Square") ||
            expect_description(machine, 22, 0, "Off") ||
            expect_description(machine, 22, 1, "On") ||
            expect_description(machine, 23, 3, "3 ms") ||
            expect_description(machine, 24, 16, "L2.00 Hz, R2.50 Hz") ||
            expect_description(machine, 25, 3, "0.3 ms") ||
            expect_description(machine, 26, 64, "64 %")) {
        return 1;
    }
    std::printf("phase5-legasynth: describe PASS notes/cents lfo=sine/saw/square chorus=off/on delay/lfo/depth/amount\n");
    return 0;
}

struct StereoSignal {
    std::vector<float> left;
    std::vector<float> right;
};

StereoSignal render_note(CMachineInterface* machine, int samples,
    int command = 0, int value = 0)
{
    StereoSignal signal{std::vector<float>(samples, 0.0f),
        std::vector<float>(samples, 0.0f)};
    machine->SeqTick(0, 48, 0, command, value);
    machine->Work(signal.left.data(), signal.right.data(), samples, 1);
    for (int i = 0; i < samples; ++i) {
        if (!std::isfinite(signal.left[i]) || !std::isfinite(signal.right[i])) {
            signal.left.clear();
            signal.right.clear();
            break;
        }
    }
    return signal;
}

void advance_chorus(CMachineInterface* machine, int samples)
{
    std::vector<float> left(samples, 0.0f);
    std::vector<float> right(samples, 0.0f);
    machine->Work(left.data(), right.data(), samples, 0);
}

bool same_signal(const StereoSignal& a, const StereoSignal& b,
    double tolerance = 1.0e-4)
{
    if (a.left.size() != b.left.size() || a.right.size() != b.right.size()) return false;
    for (std::size_t i = 0; i < a.left.size(); ++i) {
        if (!near(a.left[i], b.left[i], tolerance) ||
                !near(a.right[i], b.right[i], tolerance)) return false;
    }
    return true;
}

double rms(const StereoSignal& signal)
{
    double sum = 0.0;
    std::size_t n = signal.left.size() + signal.right.size();
    for (float sample : signal.left) sum += static_cast<double>(sample) * sample;
    for (float sample : signal.right) sum += static_cast<double>(sample) * sample;
    return n ? std::sqrt(sum / n) : 0.0;
}

int verify_deterministic_default(CMachineInterface* a, CMachineInterface* b,
    const CMachineInfo* info)
{
    TestCallback cb_a(44100);
    TestCallback cb_b(44100);
    apply_host_defaults(a, info, &cb_a);
    apply_host_defaults(b, info, &cb_b);
    StereoSignal first = render_note(a, 2048);
    StereoSignal second = render_note(b, 2048);
    const double observed_rms = rms(first);
    if (first.left.empty() || second.left.empty() || observed_rms < 1.0 ||
            !same_signal(first, second, 2.0e-4)) {
        return fail("fresh default LegaSynth instances are not deterministic/active");
    }
    for (const ExpectedSampleMarker& marker : DEFAULT_NOTE_MARKERS) {
        if (!near(first.left[marker.index], marker.left, 5.0e-2) ||
                !near(first.right[marker.index], marker.right, 5.0e-2)) {
            std::fprintf(stderr,
                "phase5-legasynth: FAIL: historical sample marker %zu changed: got %.9g/%.9g expected %.9g/%.9g\n",
                marker.index, first.left[marker.index], first.right[marker.index],
                marker.left, marker.right);
            return 1;
        }
    }
    if (std::fabs(observed_rms - DEFAULT_NOTE_RMS) > 5.0e-2)
        return fail("historical default-note RMS changed");
    std::printf("phase5-legasynth: historical-oracle PASS note=48 rate=44100 markers=7 rms=4252.98775\n");
    std::printf("phase5-legasynth: deterministic PASS note=48 rate=44100 rms=%.6f\n", observed_rms);
    return 0;
}

int verify_velocity_command(CMachineInterface* full, CMachineInterface* reduced,
    const CMachineInfo* info)
{
    TestCallback cb_full(44100);
    TestCallback cb_reduced(44100);
    apply_host_defaults(full, info, &cb_full);
    apply_host_defaults(reduced, info, &cb_reduced);
    StereoSignal a = render_note(full, 1024);
    StereoSignal b = render_note(reduced, 1024, 0x0C, 128);
    if (a.left.empty() || b.left.empty()) return fail("velocity-command render failed");
    const float ratio = 64.0f / 127.0f;
    for (std::size_t i = 0; i < a.left.size(); ++i) {
        if (!near(b.left[i], a.left[i] * ratio, 4.0e-4) ||
                !near(b.right[i], a.right[i] * ratio, 4.0e-4)) {
            return fail("0C80 no longer applies retained velocity=64/127 scaling");
        }
    }
    std::printf("phase5-legasynth: velocity PASS command=0C80 scale=64/127\n");
    return 0;
}

int verify_nonpositive_chorus(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    apply_host_defaults(machine, info, &callback);
    machine->ParameterTweak(22, 1);
    float left[7] = {11, 12, 13, 14, 15, 16, 17};
    float right[7] = {-11, -12, -13, -14, -15, -16, -17};
    const float expected_left[7] = {11, 12, 13, 14, 15, 16, 17};
    const float expected_right[7] = {-11, -12, -13, -14, -15, -16, -17};
    machine->Work(left + 2, right + 2, 0, 0);
    machine->Work(left + 2, right + 2, -7, 0);
    for (int i = 0; i < 7; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i])
            return fail("chorus-enabled non-positive callback modified canary buffers");
    }
    std::printf("phase5-legasynth: nonpositive PASS chorus=on zero+negative strict-noop\n");
    return 0;
}

int verify_rate_transition(CMachineInterface* live, CMachineInterface* target,
    CMachineInterface* stale, const CMachineInfo* info)
{
    TestCallback live_cb(44100);
    TestCallback target_cb(88200);
    TestCallback stale_cb(44100);
    apply_host_defaults(live, info, &live_cb);
    apply_host_defaults(target, info, &target_cb);
    apply_host_defaults(stale, info, &stale_cb);
    live->ParameterTweak(22, 1);
    target->ParameterTweak(22, 1);
    stale->ParameterTweak(22, 1);

    /* Advance only the chorus for 250 ms at each native rate. The zero input
    ** keeps the delay rings empty while making the accumulated LFO phase
    ** nonzero. A correct rate transition must preserve that physical phase. */
    advance_chorus(live, 11025);
    advance_chorus(target, 22050);
    advance_chorus(stale, 11025);

    live_cb.set_sample_rate(88200);
    live->SequencerTick();
    StereoSignal live_signal = render_note(live, 2048);
    StereoSignal target_signal = render_note(target, 2048);
    StereoSignal stale_signal = render_note(stale, 2048);
    if (live_signal.left.empty() || target_signal.left.empty() || stale_signal.left.empty() ||
            !same_signal(live_signal, target_signal, 5.0e-4) ||
            same_signal(live_signal, stale_signal, 5.0e-4)) {
        return fail("pre-rolled chorus/voice rate transition no longer matches fresh 88.2 kHz timebase");
    }
    std::printf("phase5-legasynth: samplerate PASS chorus=on preroll=250ms live=44100->88200 matches=fresh-88200 differs=44100\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_LEGASYNTH_SO\n", argv[0]);
        return 2;
    }
    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) return fail(dlerror());
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateFn = CMachineInterface* (*)();
    using DeleteFn = void (*)(CMachineInterface*);
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(handle, "GetInfo"));
    auto create = reinterpret_cast<CreateFn>(dlsym(handle, "CreateMachine"));
    auto destroy = reinterpret_cast<DeleteFn>(dlsym(handle, "DeleteMachine"));
    if (!get_info || !create || !destroy) return fail("native ABI exports missing");
    const CMachineInfo* info = get_info();
    if (verify_metadata(info)) return 1;

    std::vector<CMachineInterface*> machines;
    for (int i = 0; i < 9; ++i) {
        CMachineInterface* machine = create();
        if (!machine) return fail("CreateMachine returned null");
        machines.push_back(machine);
    }

    int rc = 0;
    if (verify_constructor_defaults(machines[0], info) ||
            verify_descriptions(machines[0]) ||
            verify_deterministic_default(machines[1], machines[2], info) ||
            verify_velocity_command(machines[3], machines[4], info) ||
            verify_nonpositive_chorus(machines[5], info) ||
            verify_rate_transition(machines[6], machines[7], machines[8], info)) {
        rc = 1;
    }

    for (CMachineInterface* machine : machines) destroy(machine);
    dlclose(handle);
    if (rc == 0) std::printf("phase5-legasynth: PASS\n");
    return rc;
}
