/*
** PSYCLE-LINUX Phase 5C BexPhase preservation regression.
**
** Loads the retained BexPhase machine through the native Psycle ABI and freezes
** its public identity/metadata plus deterministic behavior that remains active in
** the r12005 source.  The historical FFT/LFO analysis block is compiled out in
** this baseline, so this gate deliberately preserves the observable processing
** that is present rather than inventing a new phaser algorithm.
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
    {"Phase Shift", "Phase Shift", 1, 512, psycle::plugin_interface::MPF_STATE, 256},
    {"Dry / Wet", "Dry / Wet", 0, 512, psycle::plugin_interface::MPF_STATE, 390},
    {"Lfo/Freq Speed", "Lfo/Freq Speed", 1, 32, psycle::plugin_interface::MPF_STATE, 8},
    {"Mode", "Mode", 1, 6, psycle::plugin_interface::MPF_STATE, 1},
    {"LFO / Freq", "LFO / Freq", 0, 512, psycle::plugin_interface::MPF_STATE, 384},
    {"Differentiator", "Differentiator", 0, 512, psycle::plugin_interface::MPF_STATE, 0},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int tick_length = 5512) : tick_length_(tick_length) {}

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return tick_length_; }
    int GetSamplingRate() const override { return 44100; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 24; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int tick_length_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-bexphase: FAIL: %s\n", message);
    return 1;
}

bool exact(float actual, float expected)
{
    return actual == expected;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0120 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 3) {
        return fail("ABI/version/type/column geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, "DocBexter'S PhaZaR") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "BexPhase!") != 0 ||
            !info->Author || std::strcmp(info->Author, "Simon Bucher") != 0) {
        return fail("historical identity changed");
    }

    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters) {
        return fail("parameter table geometry changed");
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
                "phase5-bexphase: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }

    std::printf("phase5-bexphase: metadata PASS version=0x0120 parameters=6 identity=DocBexter-PhaZaR\n");
    return 0;
}

void configure_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }
}

int verify_descriptions(CMachineInterface* machine)
{
    char text[128];
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 0, 256) || std::strcmp(text, "90.00 deg") != 0)
        return fail("Phase Shift description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 1, 390) || std::strcmp(text, "100% Effect") != 0)
        return fail("Dry/Wet description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 2, 8) || std::strcmp(text, "Tick x8") != 0)
        return fail("Refresh description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 3, 1) || std::strcmp(text, "Single Mode") != 0)
        return fail("Mode description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 4, 384) || std::strcmp(text, "0.25 / 0.75") != 0)
        return fail("LFO/Freq description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 5, 0) || std::strcmp(text, "0 %") != 0)
        return fail("Differentiator description changed");

    std::printf("phase5-bexphase: describe PASS phase=90.00 refresh=Tick-x8 mode=Single\n");
    return 0;
}

int verify_zero_block(CMachineInterface* machine)
{
    float left[3] = {11.0f, 22.0f, 33.0f};
    float right[3] = {-11.0f, -22.0f, -33.0f};
    machine->Work(&left[1], &right[1], 0, 1);
    if (!exact(left[0], 11.0f) || !exact(left[1], 22.0f) || !exact(left[2], 33.0f) ||
            !exact(right[0], -11.0f) || !exact(right[1], -22.0f) || !exact(right[2], -33.0f)) {
        return fail("zero-length Work modified host buffers");
    }
    std::printf("phase5-bexphase: zero-block PASS strict-noop\n");
    return 0;
}

int verify_default_unity(CMachineInterface* machine)
{
    float left[] = {-4.0f, -1.0f, 0.0f, 0.5f, 3.0f, 9.0f};
    float right[] = {2.0f, -3.0f, 0.25f, 0.0f, -7.0f, 1.0f};
    const float expected_left[] = {-4.0f, -1.0f, 0.0f, 0.5f, 3.0f, 9.0f};
    const float expected_right[] = {2.0f, -3.0f, 0.25f, 0.0f, -7.0f, 1.0f};
    machine->Work(left, right, 6, 1);
    for (int i = 0; i < 6; ++i) {
        if (!exact(left[i], expected_left[i]) || !exact(right[i], expected_right[i]))
            return fail("default retained processing is no longer exact unity");
    }
    std::printf("phase5-bexphase: default-unity PASS samples=6\n");
    return 0;
}

int verify_differentiator(CMachineInterface* machine)
{
    /* With the dormant FFT/LFO path in this retained source, the delay tap is the
    ** current ring sample.  Full Differentiator therefore subtracts that exact
    ** current path and yields deterministic zero. */
    machine->ParameterTweak(5, 512);
    float left[] = {1.0f, -2.0f, 3.0f, -4.0f};
    float right[] = {-1.0f, 2.0f, -3.0f, 4.0f};
    machine->Work(left, right, 4, 1);
    for (int i = 0; i < 4; ++i) {
        if (!exact(left[i], 0.0f) || !exact(right[i], 0.0f))
            return fail("full Differentiator deterministic cancellation changed");
    }

    machine->ParameterTweak(5, 256);
    float half_left[] = {2.0f, -4.0f};
    float half_right[] = {-6.0f, 8.0f};
    machine->Work(half_left, half_right, 2, 1);
    if (!exact(half_left[0], 1.0f) || !exact(half_left[1], -2.0f) ||
            !exact(half_right[0], -3.0f) || !exact(half_right[1], 4.0f)) {
        return fail("half Differentiator deterministic scaling changed");
    }

    std::printf("phase5-bexphase: differentiator PASS full=zero half=0.5x\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_BEXPHASE_SO\n", argv[0]);
        return 2;
    }

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-bexphase: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    if (!get_info || !create_machine || !delete_machine) {
        dlclose(library);
        return fail("native ABI exports missing");
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(info);
    CMachineInterface* machine = nullptr;
    if (rc == 0) {
        machine = create_machine();
        if (!machine || !machine->Vals) rc = fail("CreateMachine returned unusable instance");
    }

    TestCallback callback;
    if (rc == 0) configure_defaults(machine, info, &callback);
    if (rc == 0) rc = verify_descriptions(machine);
    if (rc == 0) rc = verify_zero_block(machine);
    if (rc == 0) rc = verify_default_unity(machine);
    if (rc == 0) rc = verify_differentiator(machine);

    if (machine) delete_machine(*machine);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-bexphase: PASS\n");
    return 0;
}
