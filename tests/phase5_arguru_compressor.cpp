/*
** PSYCLE-LINUX Phase 5 Arguru Compressor preservation regression.
**
** Loads the real Linux native-machine shared object through Psycle's retained
** plugin ABI, freezes its public metadata/parameter contract, instantiates it,
** and exercises deterministic DSP that does not require a host callback.
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
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"Input Gain", 0, 128, psycle::plugin_interface::MPF_STATE, 0},
    {"Threshold", 0, 128, psycle::plugin_interface::MPF_STATE, 64},
    {"Ratio", 0, 16, psycle::plugin_interface::MPF_STATE, 16},
    {"Attack", 0, 128, psycle::plugin_interface::MPF_STATE, 6},
    {"Release", 0, 128, psycle::plugin_interface::MPF_STATE, 0x2c},
    {"Soft clip", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-arguru-compressor: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected)
{
    return std::fabs(static_cast<double>(actual - expected)) <= 1.0e-4;
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
        return fail("Arguru Compressor plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::EFFECT) {
        return fail("Arguru Compressor is no longer classified as an effect");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru Compressor parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru Compressor") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Compressor") != 0 ||
            !info->Author || std::strcmp(info->Author,
                "J. Arguelles & psycledelics") != 0 ||
            info->numCols != 1) {
        return fail("Arguru Compressor machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru Compressor parameter table is missing");
    }

    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* actual = info->Parameters[i];
        const ExpectedParameter& expected = EXPECTED_PARAMETERS[i];
        if (!actual || !actual->Name || std::strcmp(actual->Name, expected.name) != 0 ||
                actual->MinValue != expected.min_value ||
                actual->MaxValue != expected.max_value ||
                actual->Flags != expected.flags ||
                actual->DefValue != expected.default_value) {
            std::fprintf(stderr,
                "phase5-arguru-compressor: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue || actual->DefValue > actual->MaxValue) {
            return fail("Arguru Compressor default parameter value is outside its range");
        }
    }
    return 0;
}

int verify_audio(CMachineInterface* machine)
{
    float left[] = {1.0f, -2.0f, 64.0f, -128.0f};
    float right[] = {-3.0f, 4.0f, -32.0f, 256.0f};
    const float original_left[] = {1.0f, -2.0f, 64.0f, -128.0f};
    const float original_right[] = {-3.0f, 4.0f, -32.0f, 256.0f};

    if (!machine || !machine->Vals) {
        return fail("CreateMachine did not provide a usable machine instance");
    }

    /* Ratio=0 is the retained compressor bypass path. Gain=0 and Clip=0 make
    ** the full Work() path an exact unity operation after Psycle's historical
    ** internal 32768 scaling. */
    machine->ParameterTweak(0, 0);
    machine->ParameterTweak(1, 64);
    machine->ParameterTweak(2, 0);
    machine->ParameterTweak(3, 6);
    machine->ParameterTweak(4, 0x2c);
    machine->ParameterTweak(5, 0);
    machine->Work(left, right, 4, 1);

    for (int i = 0; i < 4; ++i) {
        if (!near(left[i], original_left[i]) || !near(right[i], original_right[i])) {
            return fail("ratio-bypass unity DSP changed");
        }
    }

    /* Input Gain=64 is exactly 2x in the retained equation while Ratio=0
    ** continues to bypass gain reduction. */
    machine->ParameterTweak(0, 64);
    machine->Work(left, right, 4, 1);
    for (int i = 0; i < 4; ++i) {
        if (!near(left[i], original_left[i] * 2.0f) ||
                !near(right[i], original_right[i] * 2.0f)) {
            return fail("deterministic 2x input-gain DSP changed");
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
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_COMPRESSOR_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-compressor: FAIL: dlopen: %s\n", dlerror());
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
            "phase5-arguru-compressor: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* machine = nullptr;
    if (rc == 0) {
        machine = create_machine();
        rc = verify_audio(machine);
    }
    if (machine) {
        delete_machine(*machine);
    }
    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-compressor: PASS\n");
    std::printf("machine: Arguru Compressor\n");
    std::printf("parameters: 6\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: ratio-bypass unity + deterministic 2x input gain\n");
    return 0;
}
