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

void configure_timing(CMachineInterface* machine)
{
    /* Bandwidth=0 keeps the input-damper/tap path silent after the impulse, so
    ** the first later non-zero samples are the source-derived early-reflection
    ** allpass delays. Input mode last flushes both engines. */
    machine->ParameterTweak(PARAM_BANDWIDTH, 0);
    machine->ParameterTweak(PARAM_DRY, -70000);
    machine->ParameterTweak(PARAM_EARLY, 0);
    machine->ParameterTweak(PARAM_TAIL, -70000);
    machine->ParameterTweak(PARAM_INPUT, 0);
}

struct FirstDelays {
    int left;
    int right;
};

FirstDelays expected_first_delays(int sample_rate)
{
    /* Diffuser sizes are fixed in the gverb constructor from its initial
    ** roomsize=50 m. The first later output on each side comes from diffuser
    ** stage 1. Match the retained float-to-int truncation explicitly. */
    const double largest_delay = static_cast<double>(sample_rate) * 50.0 / 340.0;
    const int fdn_len3 = static_cast<int>(std::floor(0.632450 * largest_delay));
    const double diffscale = static_cast<double>(fdn_len3) / 1341.0;
    const int left_cc = 159 + static_cast<int>(15.0 * 0.125541);
    const int right_cc = 159 + static_cast<int>(15.0 * -0.568366);
    return {
        static_cast<int>(diffscale * static_cast<double>(left_cc)),
        static_cast<int>(diffscale * static_cast<double>(right_cc))
    };
}

void render_impulse(CMachineInterface* machine, std::size_t frames,
    std::vector<float>& left, std::vector<float>& right)
{
    left.assign(frames, 0.0f);
    right.assign(frames, 0.0f);
    left[0] = IMPULSE;
    for (std::size_t offset = 0; offset < frames;) {
        const int block = static_cast<int>(std::min<std::size_t>(256,
            frames - offset));
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
        offset += static_cast<std::size_t>(block);
    }
}

int first_nonzero_after_zero(const std::vector<float>& data)
{
    for (std::size_t i = 1; i < data.size(); ++i) {
        if (std::fabs(data[i]) > 1e-5f)
            return static_cast<int>(i);
    }
    return -1;
}

int verify_sample_rate_transition(CMachineInterface* machine,
    const CMachineInfo* info)
{
    TestCallback callback(44100);
    initialize(machine, info, &callback);
    configure_timing(machine);

    std::vector<float> left;
    std::vector<float> right;
    render_impulse(machine, 1600, left, right);
    const FirstDelays expected_44 = expected_first_delays(44100);
    const int left_44 = first_nonzero_after_zero(left);
    const int right_44 = first_nonzero_after_zero(right);
    if (left_44 != expected_44.left || right_44 != expected_44.right) {
        std::fprintf(stderr,
            "phase5-gverb: FAIL: 44.1 kHz first delays expected L%d/R%d got L%d/R%d\n",
            expected_44.left, expected_44.right, left_44, right_44);
        return 1;
    }

    callback.set_sample_rate(88200);
    machine->SequencerTick();
    render_impulse(machine, 2600, left, right);
    const FirstDelays expected_88 = expected_first_delays(88200);
    const int left_88 = first_nonzero_after_zero(left);
    const int right_88 = first_nonzero_after_zero(right);
    if (left_88 != expected_88.left || right_88 != expected_88.right) {
        std::fprintf(stderr,
            "phase5-gverb: FAIL: 88.2 kHz first delays expected L%d/R%d got L%d/R%d\n",
            expected_88.left, expected_88.right, left_88, right_88);
        return 1;
    }
    if (left_88 == left_44 || right_88 == right_44)
        return fail("sample-rate transition retained stale diffuser timing");

    std::printf("phase5-gverb: samplerate PASS first-delay-44100=L489/R461 first-delay-88200=L978/R923\n");
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
        CMachineInterface* timing = create_machine();
        if (!timing) {
            dlclose(handle);
            return fail("sample-rate timing machine creation failed");
        }
        rc = verify_sample_rate_transition(timing, info);
        delete_machine(timing);
    }

    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-gverb: PASS\n");
    return 0;
}
