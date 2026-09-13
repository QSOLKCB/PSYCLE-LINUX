/*
** PSYCLE-LINUX Phase 5 Arguru Goaslicer preservation regression.
**
** Loads the retained Linux native-machine shared object through Psycle's
** exported ABI, freezes its metadata/parameter contract, and exercises the
** tick-synchronised gate/fade behaviour with a deterministic host callback.
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
    {"Length", "Length", 1, 8192, psycle::plugin_interface::MPF_STATE, 2048},
    {"Slope", "Slope", 1, 2048, psycle::plugin_interface::MPF_STATE, 512},
};

class TestCallback : public CFxCallback {
public:
    TestCallback(int sample_rate, int tick_length)
        : sample_rate_(sample_rate), tick_length_(tick_length) {}

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return tick_length_; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 125; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
    int tick_length_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-arguru-goaslicer: FAIL: %s\n", message);
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
        return fail("Arguru Goaslicer plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::EFFECT) {
        return fail("Arguru Goaslicer is no longer classified as an effect");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru Goaslicer parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru Goaslicer") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Goaslicer") != 0 ||
            !info->Author || std::strcmp(info->Author, "J. Arguelles") != 0 ||
            info->numCols != 1) {
        return fail("Arguru Goaslicer machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru Goaslicer parameter table is missing");
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
                "phase5-arguru-goaslicer: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    return 0;
}

int verify_44100_gate_and_tick(CMachineInterface* machine)
{
    TestCallback callback(44100, 256);
    float left[15];
    float right[15];
    for (int i = 0; i < 15; ++i) {
        left[i] = right[i] = 1.0f;
    }

    machine->pCB = &callback;
    machine->Init();
    machine->ParameterTweak(0, 10);      // ten samples before the gate closes
    machine->ParameterTweak(1, 2048);    // published maximum: 0.25 step/sample
    machine->Work(left, right, 15, 1);

    for (int i = 0; i <= 10; ++i) {
        if (!near(left[i], 1.0f) || !near(right[i], 1.0f)) {
            return fail("44.1 kHz Goaslicer Length boundary changed");
        }
    }
    const float fade_down[] = {0.75f, 0.5f, 0.25f, 0.0f};
    for (int i = 0; i < 4; ++i) {
        if (!near(left[11 + i], fade_down[i]) ||
                !near(right[11 + i], fade_down[i])) {
            return fail("44.1 kHz gate/fade-down response changed");
        }
    }

    /* SequencerTick starts the next slice and releases a muted gate.  Keep the
    ** work block below Length so the test isolates the retained fade-up path. */
    machine->SequencerTick();
    float tick_left[] = {1, 1, 1, 1, 1};
    float tick_right[] = {1, 1, 1, 1, 1};
    const float tick_expected[] = {0, 0.25f, 0.5f, 0.75f, 1};
    machine->Work(tick_left, tick_right, 5, 1);
    for (int i = 0; i < 5; ++i) {
        if (!near(tick_left[i], tick_expected[i]) ||
                !near(tick_right[i], tick_expected[i])) {
            return fail("SequencerTick gate-release response changed");
        }
    }
    return 0;
}

int verify_sample_rate_scaling(CMachineInterface* machine)
{
    TestCallback callback(88200, 512);
    float left[29];
    float right[29];
    for (int i = 0; i < 29; ++i) {
        left[i] = right[i] = 1.0f;
    }

    machine->pCB = &callback;
    machine->Init();
    /* Use the exact same valid parameter values as the 44.1 kHz test. */
    machine->ParameterTweak(0, 10);      // scales from 10 to 20 samples
    machine->ParameterTweak(1, 2048);    // scales from 0.25 to 0.125/sample
    machine->Work(left, right, 29, 1);

    for (int i = 0; i < 20; ++i) {
        if (!near(left[i], 1.0f) || !near(right[i], 1.0f)) {
            return fail("sample-rate-scaled Goaslicer Length changed");
        }
    }
    const float scaled_fade[] = {
        1.0f, 0.875f, 0.75f, 0.625f, 0.5f,
        0.375f, 0.25f, 0.125f, 0.0f
    };
    for (int i = 0; i < 9; ++i) {
        if (!near(left[20 + i], scaled_fade[i]) ||
                !near(right[20 + i], scaled_fade[i])) {
            return fail("sample-rate-scaled Goaslicer Slope changed");
        }
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
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_GOASLICER_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-goaslicer: FAIL: dlopen: %s\n", dlerror());
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
            "phase5-arguru-goaslicer: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* gate_machine = nullptr;
    CMachineInterface* rate_machine = nullptr;
    if (rc == 0) {
        gate_machine = create_machine();
        if (!gate_machine || !gate_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable gate instance");
        } else {
            rc = verify_44100_gate_and_tick(gate_machine);
        }
    }
    if (rc == 0) {
        rate_machine = create_machine();
        if (!rate_machine || !rate_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable sample-rate instance");
        } else {
            rc = verify_sample_rate_scaling(rate_machine);
        }
    }
    if (rate_machine) {
        delete_machine(*rate_machine);
    }
    if (gate_machine) {
        delete_machine(*gate_machine);
    }
    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-goaslicer: PASS\n");
    std::printf("machine: Arguru Goaslicer\n");
    std::printf("parameters: 2\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("timing: in-range gate fade + SequencerTick release + sample-rate scaling\n");
    return 0;
}
