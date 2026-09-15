/*
** PSYCLE-LINUX Phase 5C Ninereeds Fractal 7900s preservation regression.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr int PARAM_EFFECT = 0;
constexpr int PARAM_DEPTH = 1;

class TestCallback : public CFxCallback {
public:
    mutable std::string message;
    mutable std::string caption;
    int sample_rate = 44100;

    void MessBox(const char* text, const char* title, unsigned int) const override
    {
        message = text ? text : "";
        caption = title ? title : "";
    }
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sample_rate / 8; }
    int GetSamplingRate() const override { return sample_rate; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-ninereeds-fractal: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, float tolerance = 0.08f)
{
    return std::fabs(actual - expected) <= tolerance;
}

void initialize(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0002 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1 || info->numParameters != 2 || !info->Parameters)
        return fail("ABI/version/type/parameter geometry changed");
    if (!info->Name || std::strcmp(info->Name, "Ninereeds Fractal 7900s Port") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Fractal Dist") != 0 ||
            !info->Author || std::strcmp(info->Author, "Ninereeds & 7900") != 0)
        return fail("historical identity changed");

    const CMachineParameter* effect = info->Parameters[PARAM_EFFECT];
    const CMachineParameter* depth = info->Parameters[PARAM_DEPTH];
    if (!effect || !depth ||
            std::strcmp(effect->Name, "effect") != 0 ||
            std::strcmp(effect->Description, "Fractal Effect") != 0 ||
            effect->MinValue != 0 || effect->MaxValue != 65534 ||
            effect->Flags != psycle::plugin_interface::MPF_STATE || effect->DefValue != 128 ||
            std::strcmp(depth->Name, "depth") != 0 ||
            std::strcmp(depth->Description, "Fractal Depth") != 0 ||
            depth->MinValue != 0 || depth->MaxValue != 32 ||
            depth->Flags != psycle::plugin_interface::MPF_STATE || depth->DefValue != 1)
        return fail("historical two-parameter metadata changed");

    std::printf("phase5-ninereeds-fractal: metadata PASS version=0x0002 parameters=2 identity=Fractal-Dist\n");
    return 0;
}

int verify_descriptions_and_about(CMachineInterface* machine, TestCallback* callback)
{
    char text[64] = {};
    if (!machine->DescribeValue(text, PARAM_EFFECT, 65534) || std::strcmp(text, "65534") != 0)
        return fail("effect raw-value description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_DEPTH, 32) || std::strcmp(text, "32") != 0)
        return fail("depth raw-value description changed");

    machine->Command();
    if (callback->caption != "Fractal Dist" ||
            callback->message != "Code: Steve Horne aka Ninereeds\nPsyVsn: Jochem vd. Lubbe aka 7900\nAdvice: Not samplerate aware!")
        return fail("historical About text changed");

    std::printf("phase5-ninereeds-fractal: describe-about PASS raw=decimal samplerate-warning=retained\n");
    return 0;
}

int verify_depth_zero_identity(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_EFFECT, 65534);
    machine->ParameterTweak(PARAM_DEPTH, 0);
    float left[258];
    float right[258];
    left[0] = 123456.0f; right[0] = -654321.0f;
    left[257] = -111111.0f; right[257] = 222222.0f;
    for (int i = 0; i < 256; ++i) {
        left[i + 1] = static_cast<float>(i * 193 - 24000);
        right[i + 1] = static_cast<float>(18000 - i * 137);
    }
    float expected_left[256];
    float expected_right[256];
    std::memcpy(expected_left, &left[1], sizeof(expected_left));
    std::memcpy(expected_right, &right[1], sizeof(expected_right));

    machine->Work(&left[1], &right[1], 256, 1);
    if (left[0] != 123456.0f || right[0] != -654321.0f ||
            left[257] != -111111.0f || right[257] != 222222.0f)
        return fail("256-sample work crossed canary boundaries");
    for (int i = 0; i < 256; ++i) {
        if (left[i + 1] != expected_left[i] || right[i + 1] != expected_right[i])
            return fail("depth zero no longer preserves in-range samples exactly");
    }
    std::printf("phase5-ninereeds-fractal: depth0 PASS identity=yes max-block=256 bounded=yes\n");
    return 0;
}

int verify_historical_oracles(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_EFFECT, 128);
    machine->ParameterTweak(PARAM_DEPTH, 1);
    float left[] = {-30000.0f, -1000.0f, 1000.0f, 30000.0f};
    float right[] = {30000.0f, 1000.0f, -1000.0f, -30000.0f};
    const float expected_left[] = {-32384.4785f, -1490.7539f, 1490.7539f, 32384.4883f};
    const float expected_right[] = {32384.4883f, 1490.7539f, -1490.7539f, -32384.4785f};
    machine->Work(left, right, 4, 1);
    for (int i = 0; i < 4; ++i) {
        if (!near(left[i], expected_left[i]) || !near(right[i], expected_right[i]))
            return fail("default cubic historical marker changed");
    }

    machine->ParameterTweak(PARAM_EFFECT, 65534);
    machine->ParameterTweak(PARAM_DEPTH, 2);
    float iter_left[] = {-30000.0f, -1000.0f, 1000.0f, 30000.0f};
    float iter_right[] = {-16384.0f, 16384.0f, -1000.0f, 1000.0f};
    const float iter_expected_left[] = {27333.1406f, -8888.6250f, 8888.5313f, -27333.0938f};
    const float iter_expected_right[] = {32768.0f, -32768.0f, -8888.6250f, 8888.5313f};
    machine->Work(iter_left, iter_right, 4, 1);
    for (int i = 0; i < 4; ++i) {
        if (!near(iter_left[i], iter_expected_left[i]) ||
                !near(iter_right[i], iter_expected_right[i]))
            return fail("high-effect repeated/clamped historical marker changed");
    }

    std::printf("phase5-ninereeds-fractal: oracle PASS default-markers=8 maxeffect-depth2-markers=8 clipping=yes\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine)
{
    machine->ParameterTweak(PARAM_EFFECT, 65534);
    machine->ParameterTweak(PARAM_DEPTH, 32);
    float left[] = {11.0f, 22.0f, 33.0f, 44.0f, 55.0f};
    float right[] = {-11.0f, -22.0f, -33.0f, -44.0f, -55.0f};
    const float expected_left[] = {11.0f, 22.0f, 33.0f, 44.0f, 55.0f};
    const float expected_right[] = {-11.0f, -22.0f, -33.0f, -44.0f, -55.0f};
    machine->Work(&left[2], &right[2], 0, 1);
    machine->Work(&left[2], &right[2], -7, 1);
    for (int i = 0; i < 5; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i])
            return fail("non-positive work modified guarded buffers");
    }
    std::printf("phase5-ninereeds-fractal: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

int verify_samplerate_invariance(CMachineInterface* machine, TestCallback* callback)
{
    machine->ParameterTweak(PARAM_EFFECT, 128);
    machine->ParameterTweak(PARAM_DEPTH, 1);
    float left44[] = {-30000.0f, -1000.0f, 1000.0f, 30000.0f};
    float right44[] = {30000.0f, 1000.0f, -1000.0f, -30000.0f};
    machine->Work(left44, right44, 4, 1);

    callback->sample_rate = 88200;
    machine->SequencerTick();
    float left88[] = {-30000.0f, -1000.0f, 1000.0f, 30000.0f};
    float right88[] = {30000.0f, 1000.0f, -1000.0f, -30000.0f};
    machine->Work(left88, right88, 4, 1);
    for (int i = 0; i < 4; ++i) {
        if (left44[i] != left88[i] || right44[i] != right88[i])
            return fail("historically sample-rate-invariant recurrence changed");
    }
    std::printf("phase5-ninereeds-fractal: samplerate PASS 44100=88200 historical-not-aware=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_FRACTAL_SO\n", argv[0]);
        return 2;
    }
    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) return fail(dlerror());

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateFn = CMachineInterface* (*)();
    using DeleteFn = psycle::plugin_interface::symbols::delete_machine_function;
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(handle, "GetInfo"));
    auto create = reinterpret_cast<CreateFn>(dlsym(handle, "CreateMachine"));
    auto destroy = reinterpret_cast<DeleteFn>(dlsym(handle, "DeleteMachine"));
    if (!get_info || !create || !destroy) return fail("native ABI exports missing");

    const CMachineInfo* info = get_info();
    if (verify_metadata(info)) return 1;

    CMachineInterface* machine = create();
    if (!machine) return fail("CreateMachine returned null");
    TestCallback callback;
    initialize(machine, info, &callback);

    int rc = verify_descriptions_and_about(machine, &callback) ||
        verify_depth_zero_identity(machine) ||
        verify_historical_oracles(machine) ||
        verify_nonpositive(machine) ||
        verify_samplerate_invariance(machine, &callback);

    destroy(*machine);
    dlclose(handle);
    if (rc == 0) std::printf("phase5-ninereeds-fractal: PASS\n");
    return rc;
}
