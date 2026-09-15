/*
** PSYCLE-LINUX Phase 5C Audacity WahWah preservation regression.
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
    {"LFO Freq", "LFOFreq", 1, 100, psycle::plugin_interface::MPF_STATE, 15},
    {"LFO start phase", "LFOStartPhase", 0, 359, psycle::plugin_interface::MPF_STATE, 0},
    {"Depth", "Depth", 0, 100, psycle::plugin_interface::MPF_STATE, 70},
    {"Resonance", "Resonance", 1, 100, psycle::plugin_interface::MPF_STATE, 25},
    {"Wah freq offset", "WahFreqOff", 0, 100, psycle::plugin_interface::MPF_STATE, 30},
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
    std::fprintf(stderr, "phase5-audacity-wahwah: FAIL: %s\n", message);
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
    if (!info->Name || std::strcmp(info->Name, "Audacity WahWah") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "WahWah") != 0 ||
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
                "phase5-audacity-wahwah: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }

    std::printf("phase5-audacity-wahwah: metadata PASS version=0x0120 parameters=5 identity=Audacity-WahWah\n");
    return 0;
}

int verify_descriptions(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    char text[256];

    if (!machine->DescribeValue(text, 0, 15) || std::strcmp(text, "1.5 Hz") != 0)
        return fail("LFO frequency description changed");
    if (!machine->DescribeValue(text, 1, 90) || std::strcmp(text, "90°") != 0)
        return fail("LFO phase description changed");
    if (!machine->DescribeValue(text, 2, 70) || std::strcmp(text, "70%") != 0)
        return fail("Depth description changed");
    if (!machine->DescribeValue(text, 3, 25) || std::strcmp(text, "2.5") != 0)
        return fail("Resonance description changed");
    if (!machine->DescribeValue(text, 4, 30) || std::strcmp(text, "331 Hz") != 0)
        return fail("Wah offset description changed");

    std::printf("phase5-audacity-wahwah: describe PASS lfo=1.5Hz phase=90deg depth=70 resonance=2.5 offset=331Hz\n");
    return 0;
}

int verify_zero_block(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    float left = 1234.5f;
    float right = -987.25f;
    machine->Work(&left, &right, 0, 1);
    if (!near(left, 1234.5f) || !near(right, -987.25f))
        return fail("zero-length callback touched host buffers");
    std::printf("phase5-audacity-wahwah: zero-block PASS strict-noop\n");
    return 0;
}

int verify_stereo_lfo(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);

    std::vector<float> left(512, 0.0f);
    std::vector<float> right(512, 0.0f);
    left[0] = right[0] = 12000.0f;
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    double max_difference = 0.0;
    double energy = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("default WahWah produced non-finite output");
        max_difference = std::max(max_difference,
            std::fabs(static_cast<double>(left[i] - right[i])));
        energy += static_cast<double>(left[i]) * left[i] +
            static_cast<double>(right[i]) * right[i];
    }
    if (energy <= 0.0) return fail("default WahWah impulse produced no output");
    if (max_difference < 1.0e-3)
        return fail("opposed stereo Wah modulation collapsed to mono");

    std::printf("phase5-audacity-wahwah: stereo PASS opposed-modulation=yes finite=yes\n");
    return 0;
}

int verify_depth_zero_symmetry(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    machine->Vals[2] = 0;
    machine->ParameterTweak(2, 0);

    std::vector<float> left(256, 0.0f);
    std::vector<float> right(256, 0.0f);
    left[0] = right[0] = 8000.0f;
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("Depth=0 produced non-finite output");
        if (!near(left[i], right[i], 1.0e-5))
            return fail("Depth=0 no longer produces symmetric stereo filtering");
    }

    std::printf("phase5-audacity-wahwah: depth-zero PASS stereo-symmetric=yes\n");
    return 0;
}

int verify_max_offset_guard(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    machine->Vals[4] = 100;
    machine->ParameterTweak(4, 100);

    std::vector<float> left(1024, 0.0f);
    std::vector<float> right(1024, 0.0f);
    left[0] = right[0] = 16000.0f;
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("maximum Wah offset lost its finite cutoff guard");
    }
    std::printf("phase5-audacity-wahwah: max-offset PASS value=100 finite=yes\n");
    return 0;
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

    transition_callback.set_sample_rate(88200);
    transitioned->SequencerTick();

    std::vector<float> trans_l(512, 0.0f), trans_r(512, 0.0f);
    std::vector<float> ref88_l(512, 0.0f), ref88_r(512, 0.0f);
    std::vector<float> ref44_l(512, 0.0f), ref44_r(512, 0.0f);
    trans_l[0] = trans_r[0] = 10000.0f;
    ref88_l[0] = ref88_r[0] = 10000.0f;
    ref44_l[0] = ref44_r[0] = 10000.0f;

    transitioned->Work(trans_l.data(), trans_r.data(), 512, 1);
    fresh88->Work(ref88_l.data(), ref88_r.data(), 512, 1);
    fresh44->Work(ref44_l.data(), ref44_r.data(), 512, 1);

    double max_transition_error = 0.0;
    double max_rate_difference = 0.0;
    for (int i = 0; i < 512; ++i) {
        if (!std::isfinite(trans_l[i]) || !std::isfinite(ref88_l[i]) ||
                !std::isfinite(ref44_l[i]))
            return fail("sample-rate gate produced non-finite output");
        max_transition_error = std::max(max_transition_error,
            std::fabs(static_cast<double>(trans_l[i] - ref88_l[i])));
        max_rate_difference = std::max(max_rate_difference,
            std::fabs(static_cast<double>(ref44_l[i] - ref88_l[i])));
    }
    if (max_transition_error > 1.0e-3)
        return fail("live 44.1->88.2 kHz state does not match fresh 88.2 kHz response");
    if (max_rate_difference < 1.0e-2)
        return fail("rate oracle is not sensitive to WahWah sample-rate scaling");

    std::printf("phase5-audacity-wahwah: live-rate PASS 44.1->88.2k fresh-reference=yes rate-sensitive=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_WAHWAH_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-audacity-wahwah: FAIL: dlopen: %s\n", dlerror());
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
    auto make = [&]() -> CMachineInterface* {
        CMachineInterface* machine = create_machine();
        if (machine) machines.push_back(machine);
        return machine;
    };

    CMachineInterface* describe = nullptr;
    CMachineInterface* zero = nullptr;
    CMachineInterface* stereo = nullptr;
    CMachineInterface* depth0 = nullptr;
    CMachineInterface* maxoff = nullptr;
    CMachineInterface* transitioned = nullptr;
    CMachineInterface* fresh88 = nullptr;
    CMachineInterface* fresh44 = nullptr;

    if (rc == 0) {
        describe = make();
        if (!describe || !describe->Vals) rc = fail("CreateMachine returned unusable description instance");
        else rc = verify_descriptions(describe, info);
    }
    if (rc == 0) {
        zero = make();
        if (!zero || !zero->Vals) rc = fail("CreateMachine returned unusable zero-block instance");
        else rc = verify_zero_block(zero, info);
    }
    if (rc == 0) {
        stereo = make();
        if (!stereo || !stereo->Vals) rc = fail("CreateMachine returned unusable stereo instance");
        else rc = verify_stereo_lfo(stereo, info);
    }
    if (rc == 0) {
        depth0 = make();
        if (!depth0 || !depth0->Vals) rc = fail("CreateMachine returned unusable Depth=0 instance");
        else rc = verify_depth_zero_symmetry(depth0, info);
    }
    if (rc == 0) {
        maxoff = make();
        if (!maxoff || !maxoff->Vals) rc = fail("CreateMachine returned unusable max-offset instance");
        else rc = verify_max_offset_guard(maxoff, info);
    }
    if (rc == 0) {
        transitioned = make();
        fresh88 = make();
        fresh44 = make();
        if (!transitioned || !fresh88 || !fresh44 || !transitioned->Vals ||
                !fresh88->Vals || !fresh44->Vals)
            rc = fail("CreateMachine returned unusable rate-gate instance");
        else rc = verify_live_rate(transitioned, fresh88, fresh44, info);
    }

    for (CMachineInterface* machine : machines) delete_machine(*machine);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");

    if (rc == 0) std::printf("phase5-audacity-wahwah: PASS\n");
    return rc;
}
