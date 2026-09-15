/*
** PSYCLE-LINUX Phase 5C Audacity Phaser preservation regression.
**
** Loads the retained Psycle wrapper through the native ABI and freezes its
** historical identity/parameter surface plus representative deterministic DSP,
** stereo LFO behavior and live sample-rate handling.
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
    {"LFO Freq", "LFOFreq", 1, 40, psycle::plugin_interface::MPF_STATE, 4},
    {"LFO start phase", "LFOStartPhase", 0, 359, psycle::plugin_interface::MPF_STATE, 0},
    {"Depth", "Depth", 0, 100, psycle::plugin_interface::MPF_STATE, 70},
    {"Stages", "Stages", 2, 24, psycle::plugin_interface::MPF_STATE, 2},
    {"Dry/Wet", "Dry/Wet", 0, 100, psycle::plugin_interface::MPF_STATE, 50},
    {"Feedback", "Feedback", -100, 100, psycle::plugin_interface::MPF_STATE, 0},
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
    std::fprintf(stderr, "phase5-audacity-phaser: FAIL: %s\n", message);
    return 1;
}

bool near(float a, float b, double tolerance = 1.0e-4)
{
    return std::fabs(static_cast<double>(a - b)) <= tolerance;
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

void set_parameter(CMachineInterface* machine, int index, int value)
{
    machine->Vals[index] = value;
    machine->ParameterTweak(index, value);
}

void process_chunked(CMachineInterface* machine, std::vector<float>& left,
    std::vector<float>& right)
{
    const int total = static_cast<int>(left.size());
    for (int offset = 0; offset < total; offset += psycle::plugin_interface::MAX_BUFFER_LENGTH) {
        const int count = std::min(psycle::plugin_interface::MAX_BUFFER_LENGTH, total - offset);
        machine->Work(left.data() + offset, right.data() + offset, count, 1);
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
    if (!info->Name || std::strcmp(info->Name, "Audacity Phaser") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "APhaser") != 0 ||
            !info->Author ||
            std::strcmp(info->Author, "Nasca Octavian Paul/Sartorius") != 0) {
        return fail("historical identity metadata changed");
    }
    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters)
        return fail("parameter table geometry changed");

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
                "phase5-audacity-phaser: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-audacity-phaser: metadata PASS version=0x0120 parameters=6 identity=Audacity-Phaser\n");
    return 0;
}

int verify_descriptions(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    char text[128];

    if (!machine->DescribeValue(text, 0, 4) || std::strcmp(text, "0.4 Hz") != 0)
        return fail("LFO frequency description changed");
    if (!machine->DescribeValue(text, 1, 90) || std::strcmp(text, "90°") != 0)
        return fail("LFO phase description changed");
    if (!machine->DescribeValue(text, 2, 70) || std::strcmp(text, "70%") != 0)
        return fail("Depth description changed");
    if (!machine->DescribeValue(text, 3, 3) || std::strcmp(text, "2") != 0)
        return fail("historical odd-stage display coercion changed");
    if (!machine->DescribeValue(text, 4, 50) || std::strcmp(text, "50%:50%") != 0)
        return fail("Dry/Wet description changed");
    if (!machine->DescribeValue(text, 5, -25) || std::strcmp(text, "-0.25") != 0)
        return fail("Feedback description changed");

    std::printf("phase5-audacity-phaser: describe PASS lfo=0.4Hz phase=90deg odd-stage=2 mix=50:50 feedback=-0.25\n");
    return 0;
}

int verify_zero_block(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    float left[2] = {123.25f, 777.0f};
    float right[2] = {-456.5f, -888.0f};
    machine->Work(left, right, 0, 1);
    if (left[0] != 123.25f || left[1] != 777.0f ||
            right[0] != -456.5f || right[1] != -888.0f) {
        return fail("zero-length host block modified audio memory");
    }
    std::printf("phase5-audacity-phaser: zero-block PASS strict-noop\n");
    return 0;
}

int verify_dry_unity(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    set_parameter(machine, 4, 0);

    std::vector<float> left(512);
    std::vector<float> right(512);
    std::vector<float> expected_left(512);
    std::vector<float> expected_right(512);
    for (int i = 0; i < 512; ++i) {
        left[i] = static_cast<float>((i % 37) * 211 - 3000);
        right[i] = static_cast<float>(2500 - (i % 29) * 173);
        expected_left[i] = left[i];
        expected_right[i] = right[i];
    }
    process_chunked(machine, left, right);
    for (int i = 0; i < 512; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i])
            return fail("Dry/Wet=0 no longer preserves exact stereo unity");
    }
    std::printf("phase5-audacity-phaser: dry PASS mix=0 exact-unity=yes\n");
    return 0;
}

int verify_stage_coercion(CMachineInterface* even_machine,
    CMachineInterface* odd_machine, const CMachineInfo* info)
{
    TestCallback even_callback(44100);
    TestCallback odd_callback(44100);
    configure_defaults(even_machine, info, &even_callback);
    configure_defaults(odd_machine, info, &odd_callback);
    set_parameter(even_machine, 3, 2);
    set_parameter(odd_machine, 3, 3);
    set_parameter(even_machine, 4, 100);
    set_parameter(odd_machine, 4, 100);

    std::vector<float> even_left(512, 0.0f);
    std::vector<float> even_right(512, 0.0f);
    std::vector<float> odd_left(512, 0.0f);
    std::vector<float> odd_right(512, 0.0f);
    even_left[0] = odd_left[0] = 12000.0f;
    even_right[0] = odd_right[0] = -9000.0f;
    process_chunked(even_machine, even_left, even_right);
    process_chunked(odd_machine, odd_left, odd_right);
    for (int i = 0; i < 512; ++i) {
        if (!near(even_left[i], odd_left[i], 1.0e-5) ||
                !near(even_right[i], odd_right[i], 1.0e-5)) {
            return fail("odd Stages value no longer coerces to the previous even count");
        }
    }
    std::printf("phase5-audacity-phaser: stages PASS requested=3 effective=2 waveform=reference\n");
    return 0;
}

int verify_stereo_lfo(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    std::vector<float> left(2048, 12000.0f);
    std::vector<float> right(2048, 12000.0f);
    process_chunked(machine, left, right);

    double max_difference = 0.0;
    for (int i = 0; i < 2048; ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("default phaser produced non-finite output");
        max_difference = std::max(max_difference,
            std::fabs(static_cast<double>(left[i] - right[i])));
    }
    if (max_difference < 1.0)
        return fail("opposed stereo LFO phases no longer produce channel separation");

    std::printf("phase5-audacity-phaser: stereo PASS opposed-lfo=yes finite=yes\n");
    return 0;
}

void configure_rate_probe(CMachineInterface* machine)
{
    set_parameter(machine, 0, 40);
    set_parameter(machine, 1, 0);
    set_parameter(machine, 2, 100);
    set_parameter(machine, 3, 8);
    set_parameter(machine, 4, 100);
    set_parameter(machine, 5, 0);
}

int verify_live_rate(CMachineInterface* transitioned, CMachineInterface* fresh88,
    CMachineInterface* fresh44, const CMachineInfo* info)
{
    TestCallback transition_callback(44100);
    TestCallback callback88(88200);
    TestCallback callback44(44100);
    configure_defaults(transitioned, info, &transition_callback);
    configure_defaults(fresh88, info, &callback88);
    configure_defaults(fresh44, info, &callback44);
    configure_rate_probe(transitioned);
    configure_rate_probe(fresh88);
    configure_rate_probe(fresh44);

    transition_callback.set_sample_rate(88200);
    transitioned->SequencerTick();

    std::vector<float> trans_left(4096, 12000.0f);
    std::vector<float> trans_right(4096, 12000.0f);
    std::vector<float> ref88_left(4096, 12000.0f);
    std::vector<float> ref88_right(4096, 12000.0f);
    std::vector<float> ref44_left(4096, 12000.0f);
    std::vector<float> ref44_right(4096, 12000.0f);
    process_chunked(transitioned, trans_left, trans_right);
    process_chunked(fresh88, ref88_left, ref88_right);
    process_chunked(fresh44, ref44_left, ref44_right);

    double max_transition_error = 0.0;
    double max_rate_difference = 0.0;
    for (int i = 0; i < 4096; ++i) {
        if (!std::isfinite(trans_left[i]) || !std::isfinite(trans_right[i]) ||
                !std::isfinite(ref88_left[i]) || !std::isfinite(ref88_right[i]) ||
                !std::isfinite(ref44_left[i]) || !std::isfinite(ref44_right[i])) {
            return fail("sample-rate gate produced non-finite output");
        }
        max_transition_error = std::max(max_transition_error,
            std::max(std::fabs(static_cast<double>(trans_left[i] - ref88_left[i])),
                     std::fabs(static_cast<double>(trans_right[i] - ref88_right[i]))));
        max_rate_difference = std::max(max_rate_difference,
            std::max(std::fabs(static_cast<double>(ref44_left[i] - ref88_left[i])),
                     std::fabs(static_cast<double>(ref44_right[i] - ref88_right[i]))));
    }
    if (max_transition_error > 1.0e-3)
        return fail("live 44.1->88.2 kHz LFO state diverged from a fresh 88.2 kHz instance");
    if (max_rate_difference < 1.0)
        return fail("sample-rate oracle is not sensitive to 44.1/88.2 kHz LFO scaling");

    std::printf("phase5-audacity-phaser: live-rate PASS 44.1->88.2k fresh-reference=yes rate-sensitive=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_AUDACITY_PHASER_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-audacity-phaser: FAIL: dlopen: %s\n", dlerror());
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
    std::vector<CMachineInterface*> machines;
    auto fresh = [&]() -> CMachineInterface* {
        CMachineInterface* machine = create_machine();
        if (machine) machines.push_back(machine);
        return machine;
    };

    CMachineInterface* describe = nullptr;
    CMachineInterface* zero = nullptr;
    CMachineInterface* dry = nullptr;
    CMachineInterface* even_stage = nullptr;
    CMachineInterface* odd_stage = nullptr;
    CMachineInterface* stereo = nullptr;
    CMachineInterface* transitioned = nullptr;
    CMachineInterface* fresh88 = nullptr;
    CMachineInterface* fresh44 = nullptr;

    if (rc == 0) {
        describe = fresh();
        if (!describe || !describe->Vals) rc = fail("CreateMachine returned unusable description instance");
        else rc = verify_descriptions(describe, info);
    }
    if (rc == 0) {
        zero = fresh();
        if (!zero || !zero->Vals) rc = fail("CreateMachine returned unusable zero-block instance");
        else rc = verify_zero_block(zero, info);
    }
    if (rc == 0) {
        dry = fresh();
        if (!dry || !dry->Vals) rc = fail("CreateMachine returned unusable dry instance");
        else rc = verify_dry_unity(dry, info);
    }
    if (rc == 0) {
        even_stage = fresh();
        odd_stage = fresh();
        if (!even_stage || !odd_stage || !even_stage->Vals || !odd_stage->Vals)
            rc = fail("CreateMachine returned unusable stage instances");
        else rc = verify_stage_coercion(even_stage, odd_stage, info);
    }
    if (rc == 0) {
        stereo = fresh();
        if (!stereo || !stereo->Vals) rc = fail("CreateMachine returned unusable stereo instance");
        else rc = verify_stereo_lfo(stereo, info);
    }
    if (rc == 0) {
        transitioned = fresh();
        fresh88 = fresh();
        fresh44 = fresh();
        if (!transitioned || !fresh88 || !fresh44 ||
                !transitioned->Vals || !fresh88->Vals || !fresh44->Vals) {
            rc = fail("CreateMachine returned unusable sample-rate instances");
        } else {
            rc = verify_live_rate(transitioned, fresh88, fresh44, info);
        }
    }

    for (CMachineInterface* machine : machines) {
        if (machine) delete_machine(*machine);
    }
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");

    if (rc == 0) std::printf("phase5-audacity-phaser: PASS\n");
    return rc;
}
