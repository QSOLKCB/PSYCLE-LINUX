/*
** PSYCLE-LINUX Phase 5C LADSPA GVerb preservation regression.
**
** Loads the retained source-built effect through Psycle's native ABI and
** freezes identity, eight-parameter metadata, value descriptions, mono/stereo
** routing, the host block boundary, and live sample-rate reinitialization.
*/

#include <algorithm>
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

constexpr int PARAM_ROOM = 0;
constexpr int PARAM_REVTIME = 1;
constexpr int PARAM_DAMPING = 2;
constexpr int PARAM_BANDWIDTH = 3;
constexpr int PARAM_DRY = 4;
constexpr int PARAM_EARLY = 5;
constexpr int PARAM_TAIL = 6;
constexpr int PARAM_INPUT = 7;
constexpr float IMPULSE = 1000.0f;

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"Room size", "Room size", 1, 300, 144},
    {"Reverb time", "Reverb time", 10, 3000, 1800},
    {"Damping", "Damping", 0, 1000, 1000},
    {"Input bandwidth", "Input bandwidth", 0, 1000, 0},
    {"Dry signal level", "Dry", -70000, 0, -70000},
    {"Early reflection level", "Early", -70000, 0, 0},
    {"Tail level", "Tail level", -70000, 0, -17500},
    {"Input", "Input", 0, 1, 0},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}
    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return 5512; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-gverb: FAIL: %s\n", message);
    return 1;
}

bool close_enough(float actual, double expected, double tolerance = 1e-3)
{
    return std::fabs(static_cast<double>(actual) - expected) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0110 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1)
        return fail("ABI/version/type/column geometry changed");
    if (!info->Name || std::strcmp(info->Name, "LADSPA GVerb") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "GVerb") != 0 ||
            !info->Author ||
            std::strcmp(info->Author, "Juhana Sadeharju/Steve Harris/Sartorius") != 0)
        return fail("historical identity changed");

    const int count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != count || !info->Parameters)
        return fail("eight-parameter geometry changed");
    for (int i = 0; i < count; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        const ExpectedParameter& e = EXPECTED_PARAMETERS[i];
        if (!p || !p->Name || !p->Description ||
                std::strcmp(p->Name, e.name) != 0 ||
                std::strcmp(p->Description, e.description) != 0 ||
                p->MinValue != e.min_value || p->MaxValue != e.max_value ||
                p->Flags != psycle::plugin_interface::MPF_STATE ||
                p->DefValue != e.default_value) {
            std::fprintf(stderr,
                "phase5-gverb: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }

    std::printf("phase5-gverb: metadata PASS version=0x0110 parameters=8 identity=LADSPA-GVerb\n");
    return 0;
}

void initialize(CMachineInterface* machine, const CMachineInfo* info,
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
    char text[128] = {};
    if (!machine->DescribeValue(text, param, value) ||
            std::strcmp(text, expected) != 0) {
        std::fprintf(stderr,
            "phase5-gverb: FAIL: description param=%d value=%d expected='%s' got='%s'\n",
            param, value, expected, text);
        return 1;
    }
    return 0;
}

int verify_descriptions(CMachineInterface* machine)
{
    if (expect_description(machine, PARAM_ROOM, 144, "144 m") ||
            expect_description(machine, PARAM_REVTIME, 1800, "18.00 s") ||
            expect_description(machine, PARAM_DAMPING, 1000, "1.0") ||
            expect_description(machine, PARAM_BANDWIDTH, 0, "0.0") ||
            expect_description(machine, PARAM_DRY, -70000, "-70 dB") ||
            expect_description(machine, PARAM_EARLY, 0, "0 dB") ||
            expect_description(machine, PARAM_TAIL, -17500, "-17 dB") ||
            expect_description(machine, PARAM_INPUT, 0, "mono") ||
            expect_description(machine, PARAM_INPUT, 1, "stereo"))
        return 1;

    std::printf("phase5-gverb: describe PASS room=m revtime=s damping/bandwidth=fraction levels=integer-dB input=mono/stereo\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine)
{
    float left[] = {11.0f, 22.0f, 33.0f};
    float right[] = {-11.0f, -22.0f, -33.0f};
    machine->Work(&left[1], &right[1], 0, 1);
    machine->Work(&left[1], &right[1], -7, 1);
    if (left[0] != 11.0f || left[1] != 22.0f || left[2] != 33.0f ||
            right[0] != -11.0f || right[1] != -22.0f || right[2] != -33.0f)
        return fail("non-positive callbacks modified guarded host buffers");

    std::printf("phase5-gverb: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

void configure_immediate(CMachineInterface* machine, int input_mode)
{
    machine->ParameterTweak(PARAM_ROOM, 144);
    machine->ParameterTweak(PARAM_REVTIME, 1800);
    machine->ParameterTweak(PARAM_DAMPING, 1000);
    machine->ParameterTweak(PARAM_BANDWIDTH, 0);
    machine->ParameterTweak(PARAM_DRY, -70000);
    machine->ParameterTweak(PARAM_EARLY, 0);
    machine->ParameterTweak(PARAM_TAIL, -70000);
    /* Input-mode tweaks flush both engines, giving a deterministic fresh-state
    ** first-sample oracle for mono/stereo routing. */
    machine->ParameterTweak(PARAM_INPUT, input_mode);
}

int verify_mono_stereo_routing(CMachineInterface* machine)
{
    const double dry = std::pow(10.0, -70.0 * 0.05);
    const double diffuser = 0.75 * 0.625 * 0.625;

    configure_immediate(machine, 0);
    float mono_left[] = {IMPULSE};
    float mono_right[] = {0.0f};
    machine->Work(mono_left, mono_right, 1, 1);
    const double mono_wet = 0.5 * IMPULSE * diffuser;
    if (!close_enough(mono_left[0], IMPULSE * dry + mono_wet) ||
            !close_enough(mono_right[0], mono_wet))
        return fail("mono first-sample averaging/routing changed");

    configure_immediate(machine, 1);
    float stereo_left[] = {IMPULSE};
    float stereo_right[] = {0.0f};
    machine->Work(stereo_left, stereo_right, 1, 1);
    const double stereo_wet = IMPULSE * diffuser;
    if (!close_enough(stereo_left[0], IMPULSE * dry + stereo_wet) ||
            !close_enough(stereo_right[0], stereo_wet))
        return fail("stereo dual-engine first-sample routing changed");

    if (!close_enough(stereo_right[0], 2.0 * mono_right[0], 2e-3))
        return fail("stereo wet contribution no longer doubles mono-average case");

    std::printf("phase5-gverb: routing PASS mono=averaged-input stereo=dual-engine immediate-diffuser=0.29296875\n");
    return 0;
}

void configure_transition(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_ROOM, 100);
    machine->ParameterTweak(PARAM_REVTIME, 1200);
    machine->ParameterTweak(PARAM_DAMPING, 500);
    machine->ParameterTweak(PARAM_BANDWIDTH, 1000);
    machine->ParameterTweak(PARAM_DRY, -70000);
    machine->ParameterTweak(PARAM_EARLY, 0);
    machine->ParameterTweak(PARAM_TAIL, 0);
    machine->ParameterTweak(PARAM_INPUT, 0);
}

void render_impulse(CMachineInterface* machine,
    std::vector<float>& left, std::vector<float>& right)
{
    std::fill(left.begin(), left.end(), 0.0f);
    std::fill(right.begin(), right.end(), 0.0f);
    left[0] = IMPULSE;
    for (std::size_t offset = 0; offset < left.size();) {
        const int block = static_cast<int>(std::min<std::size_t>(256,
            left.size() - offset));
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
        offset += static_cast<std::size_t>(block);
    }
}

double max_difference(const std::vector<float>& a,
    const std::vector<float>& b)
{
    double result = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i)
        result = std::max(result,
            std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
    return result;
}

int verify_sample_rate_transition(CMachineInterface* live,
    CMachineInterface* fresh, CMachineInterface* stale,
    const CMachineInfo* info)
{
    TestCallback live_callback(44100);
    TestCallback fresh_callback(88200);
    TestCallback stale_callback(44100);

    initialize(live, info, &live_callback);
    configure_transition(live);
    live_callback.set_sample_rate(88200);
    live->SequencerTick();

    initialize(fresh, info, &fresh_callback);
    configure_transition(fresh);
    initialize(stale, info, &stale_callback);
    configure_transition(stale);

    constexpr std::size_t frames = 20000;
    std::vector<float> live_l(frames), live_r(frames);
    std::vector<float> fresh_l(frames), fresh_r(frames);
    std::vector<float> stale_l(frames), stale_r(frames);
    render_impulse(live, live_l, live_r);
    render_impulse(fresh, fresh_l, fresh_r);
    render_impulse(stale, stale_l, stale_r);

    const double target_diff = std::max(max_difference(live_l, fresh_l),
        max_difference(live_r, fresh_r));
    const double stale_diff = std::max(max_difference(live_l, stale_l),
        max_difference(live_r, stale_r));
    if (target_diff > 1e-5)
        return fail("live 88.2 kHz reinit diverged from a fresh target instance");
    if (stale_diff < 1e-3)
        return fail("live sample-rate transition retained stale 44.1 kHz timing");

    std::printf("phase5-gverb: samplerate PASS live-44100->88200 matches-fresh-88200 rejects-stale-44100 frames=20000\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_GVERB_SO\n", argv[0]);
        return 2;
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::fprintf(stderr, "phase5-gverb: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    using GetInfoFn = CMachineInfo* (*)();
    using CreateFn = CMachineInterface* (*)();
    using DeleteFn = void (*)(CMachineInterface*);
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(handle, "GetInfo"));
    auto create_machine = reinterpret_cast<CreateFn>(dlsym(handle, "CreateMachine"));
    auto delete_machine = reinterpret_cast<DeleteFn>(dlsym(handle, "DeleteMachine"));
    if (!get_info || !create_machine || !delete_machine) {
        dlclose(handle);
        return fail("native ABI exports are incomplete");
    }

    const CMachineInfo* info = get_info();
    if (verify_metadata(info) != 0) {
        dlclose(handle);
        return 1;
    }

    TestCallback callback(44100);
    CMachineInterface* machine = create_machine();
    if (!machine) {
        dlclose(handle);
        return fail("CreateMachine returned null");
    }
    initialize(machine, info, &callback);
    int rc = verify_descriptions(machine) ||
        verify_nonpositive(machine) ||
        verify_mono_stereo_routing(machine);
    delete_machine(machine);

    if (rc == 0) {
        CMachineInterface* live = create_machine();
        CMachineInterface* fresh = create_machine();
        CMachineInterface* stale = create_machine();
        if (!live || !fresh || !stale) {
            if (live) delete_machine(live);
            if (fresh) delete_machine(fresh);
            if (stale) delete_machine(stale);
            dlclose(handle);
            return fail("sample-rate comparison machine creation failed");
        }
        rc = verify_sample_rate_transition(live, fresh, stale, info);
        delete_machine(live);
        delete_machine(fresh);
        delete_machine(stale);
    }

    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-gverb: PASS\n");
    return 0;
}
