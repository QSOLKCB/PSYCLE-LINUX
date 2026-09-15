/*
** PSYCLE-LINUX Phase 5C ThunderPalace SoftSat preservation regression.
**
** Loads the retained source-built effect through Psycle's native ABI and
** freezes identity, two-parameter metadata, deterministic default-state
** waveshaping, historical setter-order behaviour and host block boundaries.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr int PARAM_THRESHOLD = 0;
constexpr int PARAM_HARDNESS = 1;
constexpr int DEFAULT_THRESHOLD = 32768;
constexpr int DEFAULT_HARDNESS = 1024;

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"Threshold", "Threshold level", 16, 32768, 32768},
    {"Hardness", "Hardness", 1, 2048, 1024},
};

class TestCallback : public CFxCallback {
public:
    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return 5512; }
    int GetSamplingRate() const override { return 44100; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-softsat: FAIL: %s\n", message);
    return 1;
}

bool close_enough(float actual, double expected, double tolerance = 2e-2)
{
    return std::fabs(static_cast<double>(actual) - expected) <= tolerance;
}

double gradation_from_raw(int hardness)
{
    return static_cast<double>(hardness) / 2049.0;
}

double range_from_state(int threshold, double gradation)
{
    return static_cast<double>(threshold) / ((gradation + 1.0) / 2.0);
}

double process_expected(double sample, double gradation, double range)
{
    const double sign = sample < 0.0 ? -1.0 : 1.0;
    double normalized = std::fabs(sample) / range;
    if (normalized > gradation) {
        const double delta = normalized - gradation;
        const double width = 1.0 - gradation;
        normalized = gradation + delta /
            (1.0 + (delta / width) * (delta / width));
    }
    if (normalized > 1.0)
        normalized = (gradation + 1.0) / 2.0;
    return sign * normalized * range;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0100 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 3)
        return fail("ABI/version/type/column geometry changed");
    if (!info->Name || std::strcmp(info->Name, "ThunderPalace SoftSat") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "SoftSat") != 0 ||
            !info->Author || std::strcmp(info->Author, "Catatonic Porpoise") != 0)
        return fail("historical identity changed");

    const int count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != count || !info->Parameters)
        return fail("two-parameter geometry changed");
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
                "phase5-softsat: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }

    std::printf("phase5-softsat: metadata PASS version=0x0100 parameters=2 identity=ThunderPalace-SoftSat\n");
    return 0;
}

void initialize_host_order(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

void reset_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

int verify_descriptions(CMachineInterface* machine)
{
    char text[128] = {};
    if (machine->DescribeValue(text, PARAM_THRESHOLD, DEFAULT_THRESHOLD))
        return fail("Threshold unexpectedly acquired a custom value description");

    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_HARDNESS, DEFAULT_HARDNESS) ||
            std::strcmp(text, "0.500") != 0)
        return fail("default Hardness description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_HARDNESS, 2048) ||
            std::strcmp(text, "1.000") != 0)
        return fail("maximum Hardness description changed");

    std::printf("phase5-softsat: describe PASS threshold=host-integer hardness=0.500/1.000\n");
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

    std::printf("phase5-softsat: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

int verify_default_dsp(CMachineInterface* machine, const CMachineInfo* info)
{
    reset_defaults(machine, info);
    const double gradation = gradation_from_raw(DEFAULT_HARDNESS);
    const double range = range_from_state(DEFAULT_THRESHOLD, gradation);

    float left[] = {10000.0f, 32768.0f, -32768.0f, 50000.0f};
    float right[] = {-10000.0f, -32768.0f, 32768.0f, -50000.0f};
    const float left_in[] = {10000.0f, 32768.0f, -32768.0f, 50000.0f};
    const float right_in[] = {-10000.0f, -32768.0f, 32768.0f, -50000.0f};
    machine->Work(left, right, 4, 1);

    for (int i = 0; i < 4; ++i) {
        if (!close_enough(left[i], process_expected(left_in[i], gradation, range)) ||
                !close_enough(right[i], process_expected(right_in[i], gradation, range)))
            return fail("default host-order waveshaping changed");
        if (!close_enough(left[i], -right[i]))
            return fail("SoftSat lost odd-symmetric stereo shaping");
    }

    std::printf("phase5-softsat: default PASS threshold=32768 hardness=1024 deterministic-host-order odd-symmetric=yes\n");
    return 0;
}

int verify_setter_order(CMachineInterface* machine, const CMachineInfo* info)
{
    reset_defaults(machine, info);

    const double default_gradation = gradation_from_raw(DEFAULT_HARDNESS);
    machine->ParameterTweak(PARAM_THRESHOLD, 10000);
    const double retained_range = range_from_state(10000, default_gradation);

    machine->ParameterTweak(PARAM_HARDNESS, 2048);
    const double hard_gradation = gradation_from_raw(2048);
    float before_left[] = {15000.0f};
    float before_right[] = {-15000.0f};
    machine->Work(before_left, before_right, 1, 1);
    const double expected_before = process_expected(15000.0,
        hard_gradation, retained_range);
    if (!close_enough(before_left[0], expected_before) ||
            !close_enough(before_right[0], -expected_before))
        return fail("Hardness-only tweak no longer preserves the existing Threshold range");

    machine->ParameterTweak(PARAM_THRESHOLD, 10000);
    const double recomputed_range = range_from_state(10000, hard_gradation);
    float after_left[] = {15000.0f};
    float after_right[] = {-15000.0f};
    machine->Work(after_left, after_right, 1, 1);
    const double expected_after = process_expected(15000.0,
        hard_gradation, recomputed_range);
    if (!close_enough(after_left[0], expected_after) ||
            !close_enough(after_right[0], -expected_after))
        return fail("Threshold retweak no longer recomputes range from current Hardness");
    if (std::fabs(static_cast<double>(before_left[0]) - after_left[0]) < 1000.0)
        return fail("setter-order oracle no longer distinguishes retained and recomputed range");

    std::printf("phase5-softsat: setter-order PASS hardness-alone=retains-range threshold-retweak=recomputes-range\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_SOFTSAT_SO\n", argv[0]);
        return 2;
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::fprintf(stderr, "phase5-softsat: FAIL: dlopen: %s\n", dlerror());
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

    CMachineInterface* machine = create_machine();
    if (!machine) {
        dlclose(handle);
        return fail("CreateMachine returned null");
    }
    TestCallback callback;
    initialize_host_order(machine, info, &callback);

    const int rc = verify_descriptions(machine) ||
        verify_nonpositive(machine) ||
        verify_default_dsp(machine, info) ||
        verify_setter_order(machine, info);

    delete_machine(machine);
    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-softsat: PASS\n");
    return 0;
}
