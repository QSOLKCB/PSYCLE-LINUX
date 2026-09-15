/*
** PSYCLE-LINUX Phase 5C ayeternal Dist! Distortion preservation regression.
**
** Loads the retained source-built effect through Psycle's native ABI and
** freezes its historical identity, seven-parameter surface, asymmetric and
** symmetric clipping behaviour, value descriptions, and host block boundary.
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

constexpr int PARAM_INPUT_GAIN = 0;
constexpr int PARAM_OUTPUT_GAIN = 1;
constexpr int PARAM_POSITIVE_THRESHOLD = 2;
constexpr int PARAM_POSITIVE_CLAMP = 3;
constexpr int PARAM_NEGATIVE_THRESHOLD = 4;
constexpr int PARAM_NEGATIVE_CLAMP = 5;
constexpr int PARAM_SYMMETRIC = 6;
constexpr int RAW_MAX = 65535;
constexpr double AMPLITUDE = 32767.0;

struct ExpectedParameter {
    const char* name;
    int min_value;
    int max_value;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"input gain", 0, 65535, 32767},
    {"output gain", 0, 65535, 32767},
    {"positive threshold", 0, 65535, 65535},
    {"positive clamp", 0, 65535, 65535},
    {"negative threshold", 0, 65535, 65535},
    {"negative clamp", 0, 65535, 65535},
    {"symmetric", 0, 1, 0},
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
    std::fprintf(stderr, "phase5-distortion: FAIL: %s\n", message);
    return 1;
}

double gain_from_raw(int raw)
{
    return std::exp((static_cast<double>(raw) - 32767.5) * 8.0 /
        static_cast<double>(RAW_MAX));
}

double amplitude_from_raw(int raw)
{
    return static_cast<double>(raw) * AMPLITUDE /
        static_cast<double>(RAW_MAX);
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
            info->numCols != 4)
        return fail("ABI/version/type/column geometry changed");
    if (!info->Name || std::strcmp(info->Name, "ayeternal Dist! Distortion") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Dist!ortion") != 0 ||
            !info->Author || std::strcmp(info->Author, "bohan") != 0)
        return fail("historical identity changed");

    const int count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != count || !info->Parameters)
        return fail("seven-parameter geometry changed");
    for (int i = 0; i < count; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        const ExpectedParameter& e = EXPECTED_PARAMETERS[i];
        if (!p || !p->Name || !p->Description ||
                std::strcmp(p->Name, e.name) != 0 ||
                std::strcmp(p->Description, e.name) != 0 ||
                p->MinValue != e.min_value || p->MaxValue != e.max_value ||
                p->Flags != psycle::plugin_interface::MPF_STATE ||
                p->DefValue != e.default_value) {
            std::fprintf(stderr,
                "phase5-distortion: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }

    std::printf("phase5-distortion: metadata PASS version=0x0110 parameters=7 identity=ayeternal-Dist!-Distortion\n");
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
    char* end = nullptr;

    machine->ParameterTweak(PARAM_SYMMETRIC, 0);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_SYMMETRIC, 0) ||
            std::strcmp(text, "no") != 0)
        return fail("asymmetric description changed");

    machine->ParameterTweak(PARAM_SYMMETRIC, 1);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_SYMMETRIC, 1) ||
            std::strcmp(text, "yes (use positive)") != 0)
        return fail("symmetric description changed");

    machine->ParameterTweak(PARAM_POSITIVE_THRESHOLD, 32768);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_POSITIVE_THRESHOLD, 32768))
        return fail("positive threshold description missing");
    end = nullptr;
    const double positive = std::strtod(text, &end);
    if (end == text || std::fabs(positive - 32768.0 / 65535.0) > 1e-5)
        return fail("positive threshold description scaling changed");

    machine->ParameterTweak(PARAM_NEGATIVE_THRESHOLD, 32768);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_NEGATIVE_THRESHOLD, 32768))
        return fail("negative threshold description missing");
    end = nullptr;
    const double negative = std::strtod(text, &end);
    if (end == text || std::fabs(negative + 32768.0 / 65535.0) > 1e-5)
        return fail("negative threshold description sign/scaling changed");

    machine->ParameterTweak(PARAM_INPUT_GAIN, 65535);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_INPUT_GAIN, 65535) ||
            std::strstr(text, "dB") == nullptr)
        return fail("gain description lost dB representation");

    std::printf("phase5-distortion: describe PASS symmetric=no/yes thresholds=+/-normalized gain=dB\n");
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
    std::printf("phase5-distortion: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

void configure_clipping(CMachineInterface* machine, int symmetric)
{
    /* Adjacent exponential raw values straddle unity, so their gain product is
    ** exactly exp(0) in the retained scale while clipping still exposes the
    ** historical output-gain multiplication after clamping. */
    machine->ParameterTweak(PARAM_INPUT_GAIN, 32767);
    machine->ParameterTweak(PARAM_OUTPUT_GAIN, 32768);
    machine->ParameterTweak(PARAM_POSITIVE_THRESHOLD, 20000);
    machine->ParameterTweak(PARAM_POSITIVE_CLAMP, 8000);
    machine->ParameterTweak(PARAM_NEGATIVE_THRESHOLD, 30000);
    machine->ParameterTweak(PARAM_NEGATIVE_CLAMP, 6000);
    machine->ParameterTweak(PARAM_SYMMETRIC, symmetric);
}

int verify_asymmetric(CMachineInterface* machine)
{
    configure_clipping(machine, 0);
    float left[] = {15000.0f, -20000.0f, 5000.0f};
    float right[] = {12000.0f, -16000.0f, -5000.0f};
    machine->Work(left, right, 3, 1);

    const double output_gain = gain_from_raw(32768);
    const double positive_clamp = amplitude_from_raw(8000) * output_gain;
    const double negative_clamp = -amplitude_from_raw(6000) * output_gain;
    const double gain_product = gain_from_raw(32767) * output_gain;

    if (!close_enough(left[0], positive_clamp) ||
            !close_enough(left[1], negative_clamp) ||
            !close_enough(left[2], 5000.0 * gain_product) ||
            !close_enough(right[0], positive_clamp) ||
            !close_enough(right[1], negative_clamp) ||
            !close_enough(right[2], -5000.0 * gain_product))
        return fail("asymmetric threshold/clamp DSP changed");

    std::printf("phase5-distortion: asymmetric PASS distinct-positive-negative-clamps stereo=yes\n");
    return 0;
}

int verify_symmetric(CMachineInterface* machine)
{
    configure_clipping(machine, 1);
    float left[] = {15000.0f, -15000.0f};
    float right[] = {-20000.0f, 20000.0f};
    machine->Work(left, right, 2, 1);

    const double output_gain = gain_from_raw(32768);
    const double clamp = amplitude_from_raw(8000) * output_gain;
    if (!close_enough(left[0], clamp) || !close_enough(left[1], -clamp) ||
            !close_enough(right[0], -clamp) || !close_enough(right[1], clamp))
        return fail("symmetric mode no longer mirrors positive threshold/clamp");

    std::printf("phase5-distortion: symmetric PASS positive-controls-both-polarities\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_DISTORTION_SO\n", argv[0]);
        return 2;
    }

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        std::fprintf(stderr, "phase5-distortion: FAIL: dlopen: %s\n", dlerror());
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
        verify_asymmetric(machine) ||
        verify_symmetric(machine);

    delete_machine(machine);
    dlclose(handle);
    if (rc != 0) return 1;

    std::printf("phase5-distortion: PASS\n");
    return 0;
}
