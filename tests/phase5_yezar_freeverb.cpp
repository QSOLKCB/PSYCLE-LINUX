/*
** PSYCLE-LINUX Phase 5C Yezar/Jezar Freeverb preservation regression.
**
** Loads the retained Jezar Freeverb Psycle port through the historical native
** ABI and freezes its identity, complete five-parameter surface, dry path,
** wet impulse timing and live sample-rate reinitialization behavior.
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
    {"Absortion", "Damp", 1, 640, psycle::plugin_interface::MPF_STATE, 320},
    {"Stereo Width", "Width", 1, 640, psycle::plugin_interface::MPF_STATE, 126},
    {"Room size", "Room size", 1, 640, psycle::plugin_interface::MPF_STATE, 175},
    {"Dry Amount", "Dry", 0, 640, psycle::plugin_interface::MPF_STATE, 256},
    {"Wet Amount", "Wet", 0, 640, psycle::plugin_interface::MPF_STATE, 128},
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
    std::fprintf(stderr, "phase5-yezar-freeverb: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-5)
{
    return std::fabs(static_cast<double>(actual - expected)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0110 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 2) {
        return fail("Freeverb ABI/version/type/column metadata changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Jezar Freeverb") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Freeverb") != 0 ||
            !info->Author || std::strcmp(info->Author, "Jezar") != 0) {
        return fail("Freeverb historical identity metadata changed");
    }
    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters) {
        return fail("Freeverb parameter table changed");
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
                "phase5-yezar-freeverb: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue ||
                actual->DefValue > actual->MaxValue) {
            return fail("Freeverb default parameter is outside its public range");
        }
    }
    std::printf("phase5-yezar-freeverb: metadata PASS parameters=5 version=0x0110 identity=Jezar-Freeverb\n");
    return 0;
}

void configure(CMachineInterface* machine, const CMachineInfo* info,
    const int values[5], TestCallback* callback)
{
    machine->pCB = callback;
    /* ParameterTweak recalculates the model from all five Vals entries on every
    ** call. Prime the complete public state first so no tweak can observe an
    ** uninitialized neighboring value. */
    for (int i = 0; i < info->numParameters; ++i) {
        machine->Vals[i] = values[i];
    }
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->ParameterTweak(i, values[i]);
    }
}

int verify_dry_unity(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    const int values[5] = {320, 126, 175, 320, 0};
    float left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    float right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};
    const float expected_left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    const float expected_right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};

    configure(machine, info, values, &callback);
    machine->Work(left, right, 5, 1);
    for (int i = 0; i < 5; ++i) {
        if (!near(left[i], expected_left[i]) ||
                !near(right[i], expected_right[i])) {
            return fail("Dry=320/Wet=0 unity path changed");
        }
    }
    std::printf("phase5-yezar-freeverb: dry-unity PASS dry=320 wet=0\n");
    return 0;
}

int verify_wet_impulse_44100(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    const int values[5] = {320, 640, 175, 0, 640};
    std::vector<float> left(1200, 0.0f);
    std::vector<float> right(1200, 0.0f);
    left[0] = 1.0f;
    right[0] = 1.0f;

    configure(machine, info, values, &callback);
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    for (int i = 0; i < 1116; ++i) {
        if (!near(left[static_cast<std::size_t>(i)], 0.0f)) {
            return fail("44.1 kHz left wet path arrived before comb tuning 1116");
        }
    }
    if (!near(left[1116], 0.09f, 2.0e-5)) {
        return fail("44.1 kHz first left Freeverb impulse changed");
    }
    for (int i = 0; i < 1139; ++i) {
        if (!near(right[static_cast<std::size_t>(i)], 0.0f)) {
            return fail("44.1 kHz right wet path arrived before comb tuning 1139");
        }
    }
    if (!near(right[1139], 0.09f, 2.0e-5)) {
        return fail("44.1 kHz first right Freeverb impulse changed");
    }
    std::printf("phase5-yezar-freeverb: wet-timing PASS sr=44100 first-left=1116 first-right=1139 amplitude=0.09\n");
    return 0;
}

int verify_rate_reinitialization(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    const int values[5] = {320, 640, 175, 0, 640};
    std::vector<float> left(2300, 0.0f);
    std::vector<float> right(2300, 0.0f);
    left[0] = 1.0f;
    right[0] = 1.0f;

    configure(machine, info, values, &callback);
    callback.set_sample_rate(88200);
    machine->SequencerTick();
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    for (int i = 0; i < 2232; ++i) {
        if (!near(left[static_cast<std::size_t>(i)], 0.0f)) {
            return fail("88.2 kHz left wet path arrived before scaled comb tuning 2232");
        }
    }
    if (!near(left[2232], 0.09f, 2.0e-5)) {
        return fail("88.2 kHz first left Freeverb impulse changed");
    }
    for (int i = 0; i < 2278; ++i) {
        if (!near(right[static_cast<std::size_t>(i)], 0.0f)) {
            return fail("88.2 kHz right wet path arrived before scaled comb tuning 2278");
        }
    }
    if (!near(right[2278], 0.09f, 2.0e-5)) {
        return fail("88.2 kHz first right Freeverb impulse changed");
    }
    std::printf("phase5-yezar-freeverb: rate-transition PASS sr=44100->88200 first-left=2232 first-right=2278 network=reinitialized\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_FREEVERB_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-yezar-freeverb: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-yezar-freeverb: FAIL: native ABI exports missing: %s\n",
            error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(info);
    CMachineInterface* dry = nullptr;
    CMachineInterface* wet = nullptr;
    CMachineInterface* rate = nullptr;

    if (rc == 0) {
        dry = create_machine();
        if (!dry || !dry->Vals) rc = fail("CreateMachine returned unusable dry-path instance");
        else rc = verify_dry_unity(dry, info);
    }
    if (rc == 0) {
        wet = create_machine();
        if (!wet || !wet->Vals) rc = fail("CreateMachine returned unusable wet-path instance");
        else rc = verify_wet_impulse_44100(wet, info);
    }
    if (rc == 0) {
        rate = create_machine();
        if (!rate || !rate->Vals) rc = fail("CreateMachine returned unusable rate-transition instance");
        else rc = verify_rate_reinitialization(rate, info);
    }

    if (rate) delete_machine(*rate);
    if (wet) delete_machine(*wet);
    if (dry) delete_machine(*dry);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-yezar-freeverb: PASS\n");
    std::printf("machine: Jezar Freeverb\n");
    std::printf("module: arguru-freeverb.so\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: dry unity + retained stereo comb timing + 88.2 kHz network reinitialization\n");
    return 0;
}
