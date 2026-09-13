/*
** PSYCLE-LINUX Phase 5 Arguru Distortion preservation regression.
**
** Loads the retained Linux native-machine shared object through Psycle's
** exported ABI, freezes its public metadata/parameter contract, and exercises
** deterministic clip, phase-inversion, and saturate behaviour without changing
** the historical DSP implementation.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include <psycle/plugin_interface.hpp>

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
    {"Threshold", "Threshold level", 1, 0x8000,
        psycle::plugin_interface::MPF_STATE, 0x200},
    {"Output Gain", "Output Gain", 1, 2048,
        psycle::plugin_interface::MPF_STATE, 1024},
    {"Phase inversor", "Stereo phase inversor", 0, 1,
        psycle::plugin_interface::MPF_STATE, 0},
    {"Mode", "Operational mode", 0, 1,
        psycle::plugin_interface::MPF_STATE, 0},
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-arguru-distortion: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-3)
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
        return fail("Arguru Distortion plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::EFFECT) {
        return fail("Arguru Distortion is no longer classified as an effect");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru Distortion parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru Distortion") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Distortion") != 0 ||
            !info->Author || std::strcmp(info->Author, "J. Arguelles") != 0 ||
            info->numCols != 2) {
        return fail("Arguru Distortion machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru Distortion parameter table is missing");
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
                "phase5-arguru-distortion: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue || actual->DefValue > actual->MaxValue) {
            return fail("Arguru Distortion default parameter value is outside its range");
        }
    }
    return 0;
}

void set_parameters(CMachineInterface* machine, int threshold, int gain,
    int invert, int mode)
{
    machine->ParameterTweak(0, threshold);
    machine->ParameterTweak(1, gain);
    machine->ParameterTweak(2, invert);
    machine->ParameterTweak(3, mode);
}

int verify_clip_and_phase_inversion(CMachineInterface* machine)
{
    float left[] = {-200.0f, -100.0f, -99.0f, 0.0f, 99.0f, 100.0f, 200.0f};
    float right[] = {200.0f, 100.0f, 99.0f, 0.0f, -99.0f, -100.0f, -200.0f};
    const float expected_left[] = {-100.0f, -100.0f, -99.0f, 0.0f, 99.0f, 100.0f, 100.0f};
    const float expected_right[] = {100.0f, 100.0f, 99.0f, 0.0f, -99.0f, -100.0f, -100.0f};

    set_parameters(machine, 100, 256, 0, 0);
    machine->Work(left, right, 7, 1);
    for (int i = 0; i < 7; ++i) {
        if (!near(left[i], expected_left[i]) || !near(right[i], expected_right[i])) {
            return fail("hard-clip transfer function changed");
        }
    }

    float inv_left[] = {50.0f, -150.0f};
    float inv_right[] = {50.0f, -150.0f};
    set_parameters(machine, 100, 256, 1, 0);
    machine->Work(inv_left, inv_right, 2, 1);
    if (!near(inv_left[0], 50.0f) || !near(inv_left[1], -100.0f) ||
            !near(inv_right[0], -50.0f) || !near(inv_right[1], 100.0f)) {
        return fail("retained stereo phase-inversion behaviour changed");
    }
    return 0;
}

int verify_saturate_state(CMachineInterface* machine)
{
    float left[] = {200.0f, 200.0f};
    float right[] = {50.0f, 50.0f};

    set_parameters(machine, 100, 256, 0, 1);
    machine->Work(left, right, 2, 1);

    /* Freeze the retained stateful saturator's sample-to-sample evolution.
    ** This is intentionally a behavioural regression, not a DSP rewrite. */
    if (!near(left[0], 200.0f) || !near(right[0], 47.5f) ||
            !near(left[1], 190.1f) || !near(right[1], 45.2725f)) {
        return fail("stateful saturate-mode response changed");
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
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_DISTORTION_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-distortion: FAIL: dlopen: %s\n", dlerror());
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
            "phase5-arguru-distortion: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* clip_machine = nullptr;
    CMachineInterface* saturate_machine = nullptr;
    if (rc == 0) {
        clip_machine = create_machine();
        if (!clip_machine || !clip_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable clip-mode instance");
        } else {
            rc = verify_clip_and_phase_inversion(clip_machine);
        }
    }
    if (rc == 0) {
        saturate_machine = create_machine();
        if (!saturate_machine || !saturate_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable saturate-mode instance");
        } else {
            rc = verify_saturate_state(saturate_machine);
        }
    }
    if (saturate_machine) {
        delete_machine(*saturate_machine);
    }
    if (clip_machine) {
        delete_machine(*clip_machine);
    }
    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-distortion: PASS\n");
    std::printf("machine: Arguru Distortion\n");
    std::printf("parameters: 4\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: clip + phase inversion + stateful saturate response\n");
    return 0;
}
