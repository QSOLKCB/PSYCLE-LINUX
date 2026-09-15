/*
** PSYCLE-LINUX Phase 5C Audacity Compressor preservation regression.
**
** Loads the retained Psycle wrapper through the native ABI and freezes the
** historical identity/parameter surface plus representative deterministic
** compressor behavior and live sample-rate reinitialization.
*/

#include <algorithm>
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
    {"Threshold", "Threshold", -36, -1, psycle::plugin_interface::MPF_STATE, -12},
    {"Ratio", "Ratio", 100, 2000, psycle::plugin_interface::MPF_STATE, 200},
    {"Attack", "Attack", 10, 10000, psycle::plugin_interface::MPF_STATE, 200},
    {"Decay", "Decay", 100, 10000, psycle::plugin_interface::MPF_STATE, 1000},
    {"Old Gain method", "Old Gain method", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Noise floor", "Noise floor", -61, -21, psycle::plugin_interface::MPF_STATE, -40},
    {"Compressor method", "Compressor method", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
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
    std::fprintf(stderr, "phase5-audacity-compressor: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-4)
{
    return std::fabs(static_cast<double>(actual - expected)) <= tolerance;
}

void configure_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->Vals[i] = info->Parameters[i]->DefValue;
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0120 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1) {
        return fail("ABI/version/type/column metadata changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Audacity Compressor") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "ACompressor") != 0 ||
            !info->Author ||
            std::strcmp(info->Author, "Dominic Mazzoni/Sartorius/JosepMa") != 0) {
        return fail("historical identity metadata changed");
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
                "phase5-audacity-compressor: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue ||
                actual->DefValue > actual->MaxValue) {
            return fail("default parameter outside public range");
        }
    }
    std::printf("phase5-audacity-compressor: metadata PASS version=0x0120 parameters=7 identity=Audacity-Compressor\n");
    return 0;
}

int verify_descriptions(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    char text[256];

    if (!machine->DescribeValue(text, 0, -12) ||
            std::strcmp(text, "-12 dB (-12.0, -12.0)") != 0) {
        return fail("Threshold value description changed");
    }
    if (!machine->DescribeValue(text, 1, 200) || std::strcmp(text, "2.0:1") != 0)
        return fail("Ratio value description changed");
    if (!machine->DescribeValue(text, 2, 200) || std::strcmp(text, "0.20 s") != 0)
        return fail("Attack value description changed");
    if (!machine->DescribeValue(text, 3, 1000) || std::strcmp(text, "1.00 s") != 0)
        return fail("Decay value description changed");
    if (!machine->DescribeValue(text, 4, 0) || std::strcmp(text, "off") != 0 ||
            !machine->DescribeValue(text, 4, 1) || std::strcmp(text, "on") != 0)
        return fail("Old Gain method description changed");
    if (!machine->DescribeValue(text, 5, -40) || std::strcmp(text, "-40 dB") != 0)
        return fail("Noise floor description changed");
    if (!machine->DescribeValue(text, 6, 0) ||
            std::strcmp(text, "downward (to threshold)") != 0 ||
            !machine->DescribeValue(text, 6, 1) ||
            std::strcmp(text, "Upward (to full scale)") != 0) {
        return fail("Compressor method description changed");
    }

    std::printf("phase5-audacity-compressor: describe PASS ratio=2.0:1 attack=0.20s decay=1.00s methods=downward+upward\n");
    return 0;
}

int verify_default_downward_behavior(CMachineInterface* machine,
    const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);

    float quiet_left[16];
    float quiet_right[16];
    for (int i = 0; i < 16; ++i) {
        quiet_left[i] = 1000.0f;
        quiet_right[i] = -1000.0f;
    }
    machine->Work(quiet_left, quiet_right, 16, 1);
    for (int i = 0; i < 16; ++i) {
        if (!near(quiet_left[i], 1000.0f) || !near(quiet_right[i], -1000.0f))
            return fail("sub-threshold default path is no longer unity");
    }

    /* Use a fresh machine state for the loud-channel isolation check. */
    configure_defaults(machine, info, &callback);
    std::vector<float> left(256, 30000.0f);
    std::vector<float> right(256, 1000.0f);
    machine->Work(left.data(), right.data(), 256, 1);

    bool left_changed = false;
    for (int i = 0; i < 256; ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("default compressor produced non-finite output");
        if (!near(right[i], 1000.0f))
            return fail("left compression leaked into independent right channel");
        if (!near(left[i], 30000.0f, 0.1)) left_changed = true;
    }
    if (!left_changed) return fail("loud default signal was not compressed");

    std::printf("phase5-audacity-compressor: downward PASS quiet=unity loud=compressed stereo=independent\n");
    return 0;
}

int verify_peak_mode(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    machine->Vals[6] = 1;
    machine->ParameterTweak(6, 1);

    std::vector<float> left(256, 10000.0f);
    std::vector<float> right(256, 10000.0f);
    machine->Work(left.data(), right.data(), 256, 1);

    double input_energy = 256.0 * 10000.0 * 10000.0;
    double output_energy = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("upward compressor produced non-finite output");
        output_energy += static_cast<double>(left[i]) * left[i];
        if (!near(left[i], right[i], 1.0e-3))
            return fail("equal stereo inputs diverged in upward mode");
    }
    if (!(output_energy > input_energy))
        return fail("upward compressor no longer raises sub-full-scale energy");

    std::printf("phase5-audacity-compressor: upward PASS energy-raised=yes stereo=symmetric\n");
    return 0;
}

int verify_live_rate_reinitialization(CMachineInterface* transitioned,
    CMachineInterface* fresh_88200, CMachineInterface* fresh_44100,
    const CMachineInfo* info)
{
    TestCallback transition_callback(44100);
    TestCallback callback_88200(88200);
    TestCallback callback_44100(44100);
    configure_defaults(transitioned, info, &transition_callback);
    configure_defaults(fresh_88200, info, &callback_88200);
    configure_defaults(fresh_44100, info, &callback_44100);

    std::vector<float> pre_left(256, 30000.0f);
    std::vector<float> pre_right(256, 30000.0f);
    transitioned->Work(pre_left.data(), pre_right.data(), 256, 1);
    transition_callback.set_sample_rate(88200);
    transitioned->SequencerTick();

    std::vector<float> trans_left(256, 30000.0f);
    std::vector<float> trans_right(256, 30000.0f);
    std::vector<float> ref88_left(256, 30000.0f);
    std::vector<float> ref88_right(256, 30000.0f);
    std::vector<float> ref44_left(256, 30000.0f);
    std::vector<float> ref44_right(256, 30000.0f);

    transitioned->Work(trans_left.data(), trans_right.data(), 256, 1);
    fresh_88200->Work(ref88_left.data(), ref88_right.data(), 256, 1);
    fresh_44100->Work(ref44_left.data(), ref44_right.data(), 256, 1);

    double max_transition_error = 0.0;
    double max_rate_difference = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (!std::isfinite(trans_left[i]) || !std::isfinite(ref88_left[i]) ||
                !std::isfinite(ref44_left[i])) {
            return fail("rate gate produced non-finite output");
        }
        max_transition_error = std::max(max_transition_error,
            std::fabs(static_cast<double>(trans_left[i] - ref88_left[i])));
        max_rate_difference = std::max(max_rate_difference,
            std::fabs(static_cast<double>(ref44_left[i] - ref88_left[i])));
    }
    if (max_transition_error > 1.0e-3)
        return fail("live 44.1->88.2 kHz instance does not match fresh 88.2 kHz state");
    if (max_rate_difference < 1.0)
        return fail("rate oracle is not sensitive to the 44.1/88.2 kHz envelope window");

    std::printf(
        "phase5-audacity-compressor: live-rate PASS 44.1->88.2k fresh-reference=yes rate-sensitive=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_AUDACITY_COMPRESSOR_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-audacity-compressor: FAIL: dlopen: %s\n", dlerror());
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
    CMachineInterface* describe = nullptr;
    CMachineInterface* downward = nullptr;
    CMachineInterface* upward = nullptr;
    CMachineInterface* transitioned = nullptr;
    CMachineInterface* fresh_88200 = nullptr;
    CMachineInterface* fresh_44100 = nullptr;

    if (rc == 0) {
        describe = create_machine();
        if (!describe || !describe->Vals) rc = fail("CreateMachine returned unusable description instance");
        else rc = verify_descriptions(describe, info);
    }
    if (rc == 0) {
        downward = create_machine();
        if (!downward || !downward->Vals) rc = fail("CreateMachine returned unusable downward instance");
        else rc = verify_default_downward_behavior(downward, info);
    }
    if (rc == 0) {
        upward = create_machine();
        if (!upward || !upward->Vals) rc = fail("CreateMachine returned unusable upward instance");
        else rc = verify_peak_mode(upward, info);
    }
    if (rc == 0) {
        transitioned = create_machine();
        fresh_88200 = create_machine();
        fresh_44100 = create_machine();
        if (!transitioned || !fresh_88200 || !fresh_44100 ||
                !transitioned->Vals || !fresh_88200->Vals || !fresh_44100->Vals) {
            rc = fail("CreateMachine returned unusable rate-gate instance");
        } else {
            rc = verify_live_rate_reinitialization(transitioned, fresh_88200,
                fresh_44100, info);
        }
    }

    if (fresh_44100) delete_machine(*fresh_44100);
    if (fresh_88200) delete_machine(*fresh_88200);
    if (transitioned) delete_machine(*transitioned);
    if (upward) delete_machine(*upward);
    if (downward) delete_machine(*downward);
    if (describe) delete_machine(*describe);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");

    if (rc == 0) std::printf("phase5-audacity-compressor: PASS\n");
    return rc;
}
