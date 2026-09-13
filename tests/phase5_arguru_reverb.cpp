/*
** PSYCLE-LINUX Phase 5 Arguru Reverb preservation regression.
**
** Loads the retained Linux native-machine shared object through Psycle's
** exported ABI, freezes its metadata/parameter contract, and exercises
** deterministic dry and delayed-wet behaviour without changing the historical
** reverb implementation.
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

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"Pre Delay", "Pre Delay time", 1, 32768,
        psycle::plugin_interface::MPF_STATE, 929},
    {"Comb Spread", "Comb Filter separation", 16, 512,
        psycle::plugin_interface::MPF_STATE, 126},
    {"Room size", "Room size", 1, 640,
        psycle::plugin_interface::MPF_STATE, 175},
    {"Feedback", "Feedback", 1, 1024,
        psycle::plugin_interface::MPF_STATE, 1001},
    {"Absortion", "Absortion", 1, 22050,
        psycle::plugin_interface::MPF_STATE, 7059},
    {"Dry", "Dry", 0, 256,
        psycle::plugin_interface::MPF_STATE, 256},
    {"Wet", "Wet", 0, 256,
        psycle::plugin_interface::MPF_STATE, 128},
    {"Filters", "Number of allpass filters", 0, 12,
        psycle::plugin_interface::MPF_STATE, 12},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate)
        : sample_rate_(sample_rate) {}

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
    std::fprintf(stderr, "phase5-arguru-reverb: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-5)
{
    return std::fabs(static_cast<double>(actual - expected)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) {
        return fail("GetInfo returned null");
    }
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION) {
        return fail("native-machine API version changed");
    }
    if (info->PlugVersion != 0x0120) {
        return fail("Arguru Reverb plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::EFFECT) {
        return fail("Arguru Reverb is no longer classified as an effect");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru Reverb parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru Reverb") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Reverb") != 0 ||
            !info->Author || std::strcmp(info->Author, "J. Arguelles") != 0 ||
            info->numCols != 2) {
        return fail("Arguru Reverb machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru Reverb parameter table is missing");
    }

    for (int i = 0; i < info->numParameters; ++i) {
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
                "phase5-arguru-reverb: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue ||
                actual->DefValue > actual->MaxValue) {
            return fail("Arguru Reverb default parameter is outside its range");
        }
    }
    return 0;
}

void apply_values(CMachineInterface* machine, const int values[8])
{
    /* ParameterTweak for the comb delay reads both parameters 0 and 1. Prime
    ** the retained Vals array first, then exercise every historical tweak path,
    ** matching the final state the host establishes after Init(). */
    for (int i = 0; i < 8; ++i) {
        machine->Vals[i] = values[i];
    }
    for (int i = 0; i < 8; ++i) {
        machine->ParameterTweak(i, values[i]);
    }
}

int verify_dry_unity(CMachineInterface* machine)
{
    TestCallback callback(44100);
    const int values[8] = {4, 16, 1, 1, 22050, 256, 0, 0};
    float left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    float right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};
    const float expected_left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    const float expected_right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);
    machine->Work(left, right, 5, 1);

    for (int i = 0; i < 5; ++i) {
        if (!near(left[i], expected_left[i]) ||
                !near(right[i], expected_right[i])) {
            return fail("Dry=100%, Wet=0% unity behaviour changed");
        }
    }
    return 0;
}

int verify_wet_predelay_44100(CMachineInterface* machine)
{
    TestCallback callback(44100);
    const int values[8] = {4, 16, 1, 1, 22050, 0, 256, 0};
    float left[24] = {};
    float right[24] = {};
    left[0] = 1.0f;
    right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);
    machine->Work(left, right, 24, 1);

    for (int i = 0; i < 24; ++i) {
        const float expected_left = (i == 3) ? 1.0f : 0.0f;
        const float expected_right = (i == 19) ? 1.0f : 0.0f;
        if (!near(left[i], expected_left) || !near(right[i], expected_right)) {
            return fail("44.1 kHz wet-only stereo pre-delay response changed");
        }
    }
    return 0;
}

int verify_sample_rate_reinitialization(CMachineInterface* machine)
{
    TestCallback callback(44100);
    const int values[8] = {4, 16, 1, 1, 22050, 0, 256, 0};
    float left[40] = {};
    float right[40] = {};
    left[0] = 1.0f;
    right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);

    /* SequencerTick is the retained sample-rate-change boundary. At 88.2 kHz
    ** the comb offsets double (4/16 -> 8/32 samples), and the low-pass cutoff
    ** coefficient halves, so the first delayed impulse sample is 0.5. */
    callback.set_sample_rate(88200);
    machine->SequencerTick();
    machine->Work(left, right, 40, 1);

    for (int i = 0; i < 7; ++i) {
        if (!near(left[i], 0.0f)) {
            return fail("88.2 kHz left pre-delay scaling changed");
        }
    }
    if (!near(left[7], 0.5f)) {
        return fail("88.2 kHz left delayed impulse/cutoff response changed");
    }
    for (int i = 0; i < 39; ++i) {
        if (!near(right[i], 0.0f)) {
            return fail("88.2 kHz right pre-delay scaling changed");
        }
    }
    if (!near(right[39], 0.5f)) {
        return fail("88.2 kHz right delayed impulse/cutoff response changed");
    }
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_REVERB_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-reverb: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine =
        reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine =
        reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* symbol_error = dlerror();
    if (symbol_error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr,
            "phase5-arguru-reverb: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* dry_machine = nullptr;
    CMachineInterface* wet_machine = nullptr;
    CMachineInterface* rate_machine = nullptr;

    if (rc == 0) {
        dry_machine = create_machine();
        if (!dry_machine || !dry_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable dry-path instance");
        } else {
            rc = verify_dry_unity(dry_machine);
        }
    }
    if (rc == 0) {
        wet_machine = create_machine();
        if (!wet_machine || !wet_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable wet-path instance");
        } else {
            rc = verify_wet_predelay_44100(wet_machine);
        }
    }
    if (rc == 0) {
        rate_machine = create_machine();
        if (!rate_machine || !rate_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable sample-rate instance");
        } else {
            rc = verify_sample_rate_reinitialization(rate_machine);
        }
    }

    if (rate_machine) {
        delete_machine(*rate_machine);
    }
    if (wet_machine) {
        delete_machine(*wet_machine);
    }
    if (dry_machine) {
        delete_machine(*dry_machine);
    }
    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-reverb: PASS\n");
    std::printf("machine: Arguru Reverb\n");
    std::printf("parameters: 8\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: dry unity + stereo pre-delay + 88.2 kHz reinitialization\n");
    return 0;
}
