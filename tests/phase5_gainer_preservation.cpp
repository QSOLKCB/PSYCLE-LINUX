/*
** PSYCLE-LINUX Phase 5C ayeternal Gainer preservation regression.
**
** Loads the retained source-built effect through Psycle's native ABI and
** freezes its historical identity, one-parameter surface, exponential gain
** mapping, raw-zero hard mute, value description and host block boundary.
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr int PARAM_GAIN = 0;
constexpr int RAW_MAX = 65535;
constexpr int RAW_DEFAULT = 32767;

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
    std::fprintf(stderr, "phase5-gainer: FAIL: %s\n", message);
    return 1;
}

double gain_from_raw(int raw)
{
    return std::exp((static_cast<double>(raw) - 32767.5) * 8.0 /
        static_cast<double>(RAW_MAX));
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
    if (!info->Name || std::strcmp(info->Name, "ayeternal Gainer") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Gainer") != 0 ||
            !info->Author || std::strcmp(info->Author, "bohan") != 0)
        return fail("historical identity changed");
    if (info->numParameters != 1 || !info->Parameters)
        return fail("one-parameter geometry changed");

    const CMachineParameter* p = info->Parameters[0];
    if (!p || !p->Name || !p->Description ||
            std::strcmp(p->Name, "gain") != 0 ||
            std::strcmp(p->Description, "gain") != 0 ||
            p->MinValue != 0 || p->MaxValue != RAW_MAX ||
            p->Flags != psycle::plugin_interface::MPF_STATE ||
            p->DefValue != RAW_DEFAULT)
        return fail("gain parameter metadata changed");

    std::printf("phase5-gainer: metadata PASS version=0x0110 parameters=1 identity=ayeternal-Gainer\n");
    return 0;
}

void initialize(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    machine->ParameterTweak(PARAM_GAIN, info->Parameters[0]->DefValue);
}

int verify_descriptions(CMachineInterface* machine)
{
    char text[128];
    char* end = nullptr;

    machine->ParameterTweak(PARAM_GAIN, 0);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_GAIN, 0))
        return fail("raw-zero gain description missing");
    const double zero = std::strtod(text, &end);
    if (end == text || zero != 0.0 || std::strstr(text, "dB") != nullptr)
        return fail("raw-zero hard-mute description changed");

    machine->ParameterTweak(PARAM_GAIN, RAW_DEFAULT);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_GAIN, RAW_DEFAULT) ||
            std::strstr(text, "dB") == nullptr)
        return fail("default gain description lost dB representation");
    end = nullptr;
    const double default_gain = std::strtod(text, &end);
    if (end == text || std::fabs(default_gain - gain_from_raw(RAW_DEFAULT)) > 1e-3)
        return fail("default gain description scaling changed");

    machine->ParameterTweak(PARAM_GAIN, RAW_MAX);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_GAIN, RAW_MAX) ||
            std::strstr(text, "dB") == nullptr)
        return fail("maximum gain description lost dB representation");

    std::printf("phase5-gainer: describe PASS raw0=hard-zero default=dB max=e^4\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_GAIN, RAW_DEFAULT);
    float left[] = {11.0f, 22.0f, 33.0f};
    float right[] = {-11.0f, -22.0f, -33.0f};
    machine->Work(&left[1], &right[1], 0, 1);
    machine->Work(&left[1], &right[1], -7, 1);
    if (left[0] != 11.0f || left[1] != 22.0f || left[2] != 33.0f ||
            right[0] != -11.0f || right[1] != -22.0f || right[2] != -33.0f)
        return fail("non-positive callbacks modified guarded host buffers");
    std::printf("phase5-gainer: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

int verify_hard_mute(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_GAIN, 0);
    float left[] = {1000.0f, -250.0f, 7.0f};
    float right[] = {-1000.0f, 250.0f, -7.0f};
    machine->Work(left, right, 3, 1);
    for (int i = 0; i < 3; ++i) {
        if (left[i] != 0.0f || right[i] != 0.0f)
            return fail("raw-zero gain no longer hard-mutes stereo samples");
    }
    std::printf("phase5-gainer: mute PASS raw0=hard-silence stereo=yes\n");
    return 0;
}

int verify_exponential_gain(CMachineInterface* machine)
{
    const float input = 123.0f;

    machine->ParameterTweak(PARAM_GAIN, RAW_DEFAULT);
    float left_default[] = {input};
    float right_default[] = {-input};
    machine->Work(left_default, right_default, 1, 1);
    const double expected_default = input * gain_from_raw(RAW_DEFAULT);
    if (!close_enough(left_default[0], expected_default) ||
            !close_enough(right_default[0], -expected_default))
        return fail("default exponential gain mapping changed");

    machine->ParameterTweak(PARAM_GAIN, 32768);
    float left_upper[] = {input};
    float right_upper[] = {-input};
    machine->Work(left_upper, right_upper, 1, 1);
    const double expected_upper = input * gain_from_raw(32768);
    if (!close_enough(left_upper[0], expected_upper) ||
            !close_enough(right_upper[0], -expected_upper))
        return fail("upper midpoint exponential gain mapping changed");

    if (std::fabs(gain_from_raw(RAW_DEFAULT) * gain_from_raw(32768) - 1.0) > 1e-12)
        return fail("source-derived adjacent midpoint reciprocity changed");

    machine->ParameterTweak(PARAM_GAIN, RAW_MAX);
    float left_max[] = {input};
    float right_max[] = {-input};
    machine->Work(left_max, right_max, 1, 1);
    const double expected_max = input * std::exp(4.0);
    if (!close_enough(left_max[0], expected_max, 1e-2) ||
            !close_enough(right_max[0], -expected_max, 1e-2))
        return fail("maximum e^4 gain mapping changed");

    std::printf("phase5-gainer: exponential PASS raw32767=exp(-4/65535) raw32768=exp(+4/65535) max=e^4\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_GAINER_SO\n", argv[0]);
        return 2;
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::fprintf(stderr, "phase5-gainer: FAIL: dlopen: %s\n", dlerror());
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
    initialize(machine, info, &callback);

    const int rc = verify_descriptions(machine) ||
        verify_nonpositive(machine) ||
        verify_hard_mute(machine) ||
        verify_exponential_gain(machine);

    delete_machine(machine);
    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-gainer: PASS\n");
    return 0;
}
