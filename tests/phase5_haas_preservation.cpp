/*
** PSYCLE-LINUX Phase 5C Haas preservation regression.
**
** Loads the retained source-built effect through Psycle's native ABI and
** freezes its historical 17-slot parameter surface, spatial routing,
** signed Haas delay direction, channel-mix modes, sample-rate reconstruction,
** and non-positive host-block behaviour.
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr int RAW_MAX = 65535;

enum Parameter {
    OVERALL_GAIN = 0,
    OVERALL_DRY_WET = 1,
    SEPARATOR_DIRECT = 2,
    DIRECT_GAIN = 3,
    DIRECT_PAN = 4,
    DIRECT_DELAY_STEREO_DELTA = 5,
    SEPARATOR_EARLY = 6,
    EARLY_GAIN = 7,
    EARLY_PAN = 8,
    EARLY_DELAY = 9,
    EARLY_DELAY_STEREO_DELTA = 10,
    SEPARATOR_LATE = 11,
    LATE_GAIN = 12,
    LATE_PAN = 13,
    LATE_DELAY = 14,
    SEPARATOR_NULL = 15,
    CHANNEL_MIX = 16,
    PARAMETER_COUNT = 17
};

enum ChannelMix { NORMAL = 0, SWAPPED = 1, MONO = 2 };

struct ExpectedParameter {
    const char* name;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[PARAMETER_COUNT] = {
    {"gain", 0, 65535, psycle::plugin_interface::MPF_STATE, 46810},
    {"dry / wet", 0, 65535, psycle::plugin_interface::MPF_STATE, 65535},
    {"direct", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"gain", 0, 65535, psycle::plugin_interface::MPF_STATE, 42113},
    {"pan", 0, 65535, psycle::plugin_interface::MPF_STATE, 32767},
    {"delay stereo delta", 0, 65535, psycle::plugin_interface::MPF_STATE, 32767},
    {"early reflection", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"gain", 0, 65535, psycle::plugin_interface::MPF_STATE, 0},
    {"pan", 0, 65535, psycle::plugin_interface::MPF_STATE, 32767},
    {"delay", 0, 65535, psycle::plugin_interface::MPF_STATE, 43629},
    {"delay stereo delta", 0, 65535, psycle::plugin_interface::MPF_STATE, 32767},
    {"late reflection", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"gain", 0, 65535, psycle::plugin_interface::MPF_STATE, 0},
    {"pan", 0, 65535, psycle::plugin_interface::MPF_STATE, 32767},
    {"delay", 0, 65535, psycle::plugin_interface::MPF_STATE, 33882},
    {"", 0, 0, psycle::plugin_interface::MPF_NULL, 0},
    {"channel mix", 0, 2, psycle::plugin_interface::MPF_STATE, 0},
};

class TestCallback : public CFxCallback {
public:
    int sampling_rate = 44100;
    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sampling_rate / 8; }
    int GetSamplingRate() const override { return sampling_rate; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-haas: FAIL: %s\n", message);
    return 1;
}

bool near_zero(float value, double tolerance = 1e-5)
{
    return std::fabs(static_cast<double>(value)) <= tolerance;
}

bool nonzero(float value, double threshold = 1e-4)
{
    return std::fabs(static_cast<double>(value)) > threshold;
}

double exponential_apply(int raw, double minimum, double maximum)
{
    const double lo = std::log(minimum);
    const double ratio = (std::log(maximum) - lo) / RAW_MAX;
    return std::exp(lo + raw * ratio);
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0100 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1)
        return fail("ABI/version/type/column geometry changed");
    if (!info->Name || std::strcmp(info->Name,
            "Haas stereo time delay spatial localization") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Haas") != 0 ||
            !info->Author || std::strcmp(info->Author,
            "bohan/dilvie collaboration") != 0)
        return fail("historical identity changed");
    if (info->numParameters != PARAMETER_COUNT || !info->Parameters)
        return fail("17-slot parameter geometry changed");

    int state_count = 0;
    int label_count = 0;
    int null_count = 0;
    for (int i = 0; i < PARAMETER_COUNT; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        const ExpectedParameter& e = EXPECTED_PARAMETERS[i];
        if (!p || !p->Name || !p->Description ||
                std::strcmp(p->Name, e.name) != 0 ||
                std::strcmp(p->Description, e.name) != 0 ||
                p->MinValue != e.min_value || p->MaxValue != e.max_value ||
                p->Flags != e.flags || p->DefValue != e.default_value) {
            std::fprintf(stderr,
                "phase5-haas: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (p->Flags == psycle::plugin_interface::MPF_STATE) ++state_count;
        else if (p->Flags == psycle::plugin_interface::MPF_LABEL) ++label_count;
        else if (p->Flags == psycle::plugin_interface::MPF_NULL) ++null_count;
    }
    if (state_count != 13 || label_count != 3 || null_count != 1)
        return fail("state/separator partition changed");

    std::printf("phase5-haas: metadata PASS version=0x0100 slots=17 state=13 labels=3 null=1 identity=Haas\n");
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

int verify_descriptions(CMachineInterface* machine)
{
    char text[128];

    machine->ParameterTweak(DIRECT_DELAY_STEREO_DELTA, 32767);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, DIRECT_DELAY_STEREO_DELTA, 32767) ||
            std::strcmp(text, "0") != 0)
        return fail("near-zero stereo-delay description changed");

    machine->ParameterTweak(EARLY_DELAY, 65535);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, EARLY_DELAY, 65535) ||
            std::strstr(text, "45") == nullptr || std::strstr(text, "ms") == nullptr)
        return fail("early-delay millisecond description changed");

    machine->ParameterTweak(DIRECT_PAN, 32767);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, DIRECT_PAN, 32767) ||
            std::strcmp(text, "0") != 0)
        return fail("near-zero pan description changed");

    machine->ParameterTweak(OVERALL_GAIN, 46810);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, OVERALL_GAIN, 46810) ||
            std::strstr(text, "dB") == nullptr)
        return fail("overall-gain dB description changed");

    const char* mix_names[] = {"normal", "swapped", "mono"};
    for (int mix = 0; mix <= 2; ++mix) {
        machine->ParameterTweak(CHANNEL_MIX, mix);
        std::memset(text, 0, sizeof(text));
        if (!machine->DescribeValue(text, CHANNEL_MIX, mix) ||
                std::strcmp(text, mix_names[mix]) != 0)
            return fail("channel-mix description changed");
    }

    std::printf("phase5-haas: describe PASS delta=0 delay=ms pan=0 gain=dB mix=normal/swapped/mono\n");
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
    std::printf("phase5-haas: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

void configure_routing(CMachineInterface* machine, int pan_raw,
    int delta_raw, int mix)
{
    machine->ParameterTweak(OVERALL_GAIN, 46810);      // approximately unity
    machine->ParameterTweak(OVERALL_DRY_WET, 65535);  // wet only
    machine->ParameterTweak(DIRECT_GAIN, 46810);       // approximately unity
    machine->ParameterTweak(DIRECT_PAN, pan_raw);
    machine->ParameterTweak(DIRECT_DELAY_STEREO_DELTA, delta_raw);
    machine->ParameterTweak(EARLY_GAIN, 0);            // historical exponential minimum
    machine->ParameterTweak(EARLY_PAN, 32767);
    machine->ParameterTweak(EARLY_DELAY, 65535);       // 45 ms, outside direct-delay oracle
    machine->ParameterTweak(EARLY_DELAY_STEREO_DELTA, 32767);
    machine->ParameterTweak(LATE_GAIN, 0);
    machine->ParameterTweak(LATE_PAN, 32767);
    machine->ParameterTweak(LATE_DELAY, 65535);        // 100 ms
    machine->ParameterTweak(CHANNEL_MIX, mix);
}

CMachineInterface* make_machine(CMachineInterface* (*create_machine)(),
    const CMachineInfo* info, TestCallback* callback)
{
    CMachineInterface* machine = create_machine();
    if (machine) initialize(machine, info, callback);
    return machine;
}

int verify_channel_mix(CMachineInterface* (*create_machine)(),
    void (*delete_machine)(CMachineInterface*), const CMachineInfo* info)
{
    const double gain = exponential_apply(46810, std::pow(10.0, -3.0),
        std::pow(10.0, 24.0 / 20.0));
    const double expected = 3000.0 * gain * gain;

    for (int mix = NORMAL; mix <= MONO; ++mix) {
        TestCallback callback;
        CMachineInterface* machine = make_machine(create_machine, info, &callback);
        if (!machine) return fail("CreateMachine failed in channel-mix test");
        configure_routing(machine, 65535, 32767, mix); // pan hard right, zero-sample delta
        float left = 1000.0f;
        float right = 2000.0f;
        machine->Work(&left, &right, 1, 1);

        bool ok = false;
        if (mix == NORMAL)
            ok = near_zero(left) && std::fabs(right - expected) < 0.5;
        else if (mix == SWAPPED)
            ok = std::fabs(left - expected) < 0.5 && near_zero(right);
        else
            ok = std::fabs(left - expected) < 0.5 &&
                std::fabs(right - expected) < 0.5;
        delete_machine(machine);
        if (!ok) return fail("normal/swapped/mono direct routing changed");
    }

    std::printf("phase5-haas: channel-mix PASS normal=right swapped=left mono=dual mono-input=sum(L+R)\n");
    return 0;
}

int verify_signed_delta(CMachineInterface* (*create_machine)(),
    void (*delete_machine)(CMachineInterface*), const CMachineInfo* info)
{
    constexpr int DELAY_44100 = 264; // trunc(0.006 * 44100)
    for (int sign = 0; sign < 2; ++sign) {
        TestCallback callback;
        CMachineInterface* machine = make_machine(create_machine, info, &callback);
        if (!machine) return fail("CreateMachine failed in signed-delay test");
        configure_routing(machine, 32767, sign == 0 ? 65535 : 0, NORMAL);
        std::vector<float> left(270, 0.0f);
        std::vector<float> right(270, 0.0f);
        left[0] = 1.0f;
        machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

        bool ok;
        if (sign == 0) {
            ok = near_zero(left[0]) && nonzero(right[0]) &&
                nonzero(left[DELAY_44100]) && near_zero(right[DELAY_44100]);
        } else {
            ok = nonzero(left[0]) && near_zero(right[0]) &&
                near_zero(left[DELAY_44100]) && nonzero(right[DELAY_44100]);
        }
        delete_machine(machine);
        if (!ok) return fail("signed direct Haas-delay direction/index changed");
    }

    std::printf("phase5-haas: signed-delay PASS +6ms=R0/L264 -6ms=L0/R264 rate=44100\n");
    return 0;
}

int verify_sample_rate(CMachineInterface* (*create_machine)(),
    void (*delete_machine)(CMachineInterface*), const CMachineInfo* info)
{
    constexpr int DELAY_88200 = 529; // trunc(0.006 * 88200)
    TestCallback callback;
    CMachineInterface* machine = make_machine(create_machine, info, &callback);
    if (!machine) return fail("CreateMachine failed in sample-rate test");
    configure_routing(machine, 32767, 65535, NORMAL);

    callback.sampling_rate = 88200;
    machine->SequencerTick();

    std::vector<float> left(535, 0.0f);
    std::vector<float> right(535, 0.0f);
    left[0] = 1.0f;
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    const bool ok = near_zero(left[0]) && nonzero(right[0]) &&
        near_zero(left[264]) && nonzero(left[DELAY_88200]) &&
        near_zero(right[DELAY_88200]);
    delete_machine(machine);
    if (!ok) return fail("live sample-rate delay reconstruction changed");

    std::printf("phase5-haas: samplerate PASS direct-delta +6ms 44100=264 88200=529 stale264=zero\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_HAAS_SO\n", argv[0]);
        return 2;
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::fprintf(stderr, "phase5-haas: FAIL: dlopen: %s\n", dlerror());
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

    TestCallback callback;
    CMachineInterface* machine = make_machine(create_machine, info, &callback);
    if (!machine) {
        dlclose(handle);
        return fail("CreateMachine returned null");
    }
    const int basic_rc = verify_descriptions(machine) || verify_nonpositive(machine);
    delete_machine(machine);
    if (basic_rc != 0) {
        dlclose(handle);
        return 1;
    }

    const int rc = verify_channel_mix(create_machine, delete_machine, info) ||
        verify_signed_delta(create_machine, delete_machine, info) ||
        verify_sample_rate(create_machine, delete_machine, info);
    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-haas: PASS\n");
    return 0;
}
