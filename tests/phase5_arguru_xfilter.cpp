/*
** PSYCLE-LINUX Phase 5 Arguru XFilter / CrossDelay preservation regression.
**
** Loads the retained Linux native-machine shared object through Psycle's
** exported ABI, freezes its historical metadata/parameter contract, and
** exercises deterministic sample-delay and tracker-line timing behaviour.
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
    {"Delay time", "Delay time", 0, 88200,
        psycle::plugin_interface::MPF_STATE, 11025},
    {"Feedback", "Feedback", 0, 256,
        psycle::plugin_interface::MPF_STATE, 128},
    {"Dry", "Dry", 0, 256,
        psycle::plugin_interface::MPF_STATE, 256},
    {"Wet", "Wet", 0, 256,
        psycle::plugin_interface::MPF_STATE, 128},
    {"Lines mode", "Lines mode", 0, 1,
        psycle::plugin_interface::MPF_STATE, 0},
    {"Lines", "Lines", 0, 8,
        psycle::plugin_interface::MPF_STATE, 3},
};

class TestCallback : public CFxCallback {
public:
    TestCallback(int sample_rate, int tick_length)
        : sample_rate_(sample_rate), tick_length_(tick_length) {}

    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }
    void set_tick_length(int tick_length) { tick_length_ = tick_length; }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return tick_length_; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
    int tick_length_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-arguru-xfilter: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-6)
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
        return fail("Arguru CrossDelay plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::EFFECT) {
        return fail("Arguru CrossDelay is no longer classified as an effect");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru CrossDelay parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru CrossDelay") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "CrossDelay") != 0 ||
            !info->Author || std::strcmp(info->Author, "J. Arguelles") != 0 ||
            info->numCols != 1) {
        return fail("Arguru CrossDelay machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru CrossDelay parameter table is missing");
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
                "phase5-arguru-xfilter: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue ||
                actual->DefValue > actual->MaxValue) {
            return fail("Arguru CrossDelay default parameter is outside its range");
        }
    }
    return 0;
}

void apply_values(CMachineInterface* machine, const int values[6])
{
    /* Delay updates depend on the mode/line values, so prime the retained Vals
    ** array before exercising the historical ParameterTweak paths. */
    for (int i = 0; i < 6; ++i) {
        machine->Vals[i] = values[i];
    }
    for (int i = 0; i < 6; ++i) {
        machine->ParameterTweak(i, values[i]);
    }
}

int verify_dry_unity(CMachineInterface* machine)
{
    TestCallback callback(44100, 5);
    const int values[6] = {4, 0, 256, 0, 0, 3};
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

int verify_sample_delay_44100(CMachineInterface* machine)
{
    TestCallback callback(44100, 5);
    const int values[6] = {4, 0, 0, 256, 0, 3};
    float left[6] = {};
    float right[6] = {};
    left[0] = 1.0f;
    right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);
    machine->Work(left, right, 6, 1);

    for (int i = 0; i < 6; ++i) {
        const float expected_left = (i == 4) ? 1.0f : 0.0f;
        const float expected_right = (i == 2) ? 1.0f : 0.0f;
        if (!near(left[i], expected_left) || !near(right[i], expected_right)) {
            return fail("44.1 kHz sample-delay stereo offset changed");
        }
    }
    return 0;
}

int verify_sample_rate_scaling(CMachineInterface* machine)
{
    TestCallback callback(44100, 5);
    const int values[6] = {4, 0, 0, 256, 0, 3};
    float left[10] = {};
    float right[10] = {};
    left[0] = 1.0f;
    right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);

    callback.set_sample_rate(88200);
    machine->SequencerTick();
    machine->Work(left, right, 10, 1);

    for (int i = 0; i < 10; ++i) {
        const float expected_left = (i == 8) ? 1.0f : 0.0f;
        const float expected_right = (i == 4) ? 1.0f : 0.0f;
        if (!near(left[i], expected_left) || !near(right[i], expected_right)) {
            return fail("88.2 kHz sample-delay scaling changed");
        }
    }
    return 0;
}

int verify_lines_mode_tick_scaling(CMachineInterface* machine)
{
    TestCallback callback(44100, 5);
    const int values[6] = {4, 0, 0, 256, 1, 1};
    float first_left[12] = {};
    float first_right[12] = {};
    float second_left[16] = {};
    float second_right[16] = {};
    first_left[0] = first_right[0] = 1.0f;
    second_left[0] = second_right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);
    machine->Work(first_left, first_right, 12, 1);

    for (int i = 0; i < 12; ++i) {
        const float expected_left = (i == 10) ? 1.0f : 0.0f;
        const float expected_right = (i == 5) ? 1.0f : 0.0f;
        if (!near(first_left[i], expected_left) ||
                !near(first_right[i], expected_right)) {
            return fail("Lines mode initial tracker-tick delay changed");
        }
    }

    /* One line uses ticks * GetTickLength() * 2 samples.  Changing host tick
    ** length from 5 to 7 must therefore move the wet impulses from 10/5 to
    ** 14/7 samples, with SequencerTick as the retained reconfiguration edge. */
    callback.set_tick_length(7);
    machine->SequencerTick();
    machine->Work(second_left, second_right, 16, 1);

    for (int i = 0; i < 16; ++i) {
        const float expected_left = (i == 14) ? 1.0f : 0.0f;
        const float expected_right = (i == 7) ? 1.0f : 0.0f;
        if (!near(second_left[i], expected_left) ||
                !near(second_right[i], expected_right)) {
            return fail("Lines mode tick-length reconfiguration changed");
        }
    }
    return 0;
}

int verify_lines_mode_resource_cap(CMachineInterface* machine)
{
    /* Preserve historical Lines timing beyond the sample-mode two-second
    ** parameter envelope. At 44.1 kHz / 60 BPM / LPB4 one tracker line is
    ** 11025 samples; Lines=8 therefore requests 176400 samples and must remain
    ** exactly that long. */
    TestCallback callback(44100, 11025);
    const int values[6] = {4, 0, 0, 256, 1, 8};
    const int slow_delay = 176400;
    std::vector<float> slow_left(slow_delay + 2, 0.0f);
    std::vector<float> slow_right(slow_delay + 2, 0.0f);
    slow_left[0] = 1.0f;
    slow_right[0] = 1.0f;

    machine->pCB = &callback;
    machine->Init();
    apply_values(machine, values);
    machine->Work(slow_left.data(), slow_right.data(),
        static_cast<int>(slow_left.size()), 1);

    for (std::size_t i = 0; i < slow_left.size(); ++i) {
        const float expected_left = (i == static_cast<std::size_t>(slow_delay))
            ? 1.0f : 0.0f;
        const float expected_right =
            (i == static_cast<std::size_t>(slow_delay / 2)) ? 1.0f : 0.0f;
        if (!near(slow_left[i], expected_left) ||
                !near(slow_right[i], expected_right)) {
            return fail("slow-tempo Lines delay was truncated");
        }
    }
    std::printf(
        "phase5-arguru-xfilter: slow-lines PASS requested=176400 preserved=176400\n");

    /* Only genuinely unsafe allocations are capped. CrossDelay permits a
    ** 2^20-sample ring and reserves eight samples for its historical write
    ** cursor, so the largest exposed delay is 1048568 samples. An extreme
    ** 1,000,000-sample tick with eight lines requests 16,000,000 samples but
    ** must stop at that allocation-derived ceiling rather than allocating
    ** hundreds of MiB. */
    callback.set_tick_length(1000000);
    machine->SequencerTick();
    const int max_buffer_samples = 1 << 20;
    const int safe_delay = max_buffer_samples - 8;
    std::vector<float> capped_left(safe_delay + 2, 0.0f);
    std::vector<float> capped_right(safe_delay + 2, 0.0f);
    capped_left[0] = 1.0f;
    capped_right[0] = 1.0f;
    machine->Work(capped_left.data(), capped_right.data(),
        static_cast<int>(capped_left.size()), 1);

    for (std::size_t i = 0; i < capped_left.size(); ++i) {
        const float expected_left = (i == static_cast<std::size_t>(safe_delay))
            ? 1.0f : 0.0f;
        const float expected_right =
            (i == static_cast<std::size_t>(safe_delay / 2)) ? 1.0f : 0.0f;
        if (!near(capped_left[i], expected_left) ||
                !near(capped_right[i], expected_right)) {
            return fail("Lines mode allocation safety ceiling changed");
        }
    }
    std::printf(
        "phase5-arguru-xfilter: resource-cap PASS requested=16000000 bounded=1048568 buffer-limit=1048576\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_XFILTER_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-xfilter: FAIL: dlopen: %s\n", dlerror());
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
            "phase5-arguru-xfilter: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* dry_machine = nullptr;
    CMachineInterface* sample_machine = nullptr;
    CMachineInterface* rate_machine = nullptr;
    CMachineInterface* lines_machine = nullptr;
    CMachineInterface* resource_machine = nullptr;

    if (rc == 0) {
        dry_machine = create_machine();
        if (!dry_machine || !dry_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable dry-path instance");
        } else {
            rc = verify_dry_unity(dry_machine);
        }
    }
    if (rc == 0) {
        sample_machine = create_machine();
        if (!sample_machine || !sample_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable sample-delay instance");
        } else {
            rc = verify_sample_delay_44100(sample_machine);
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
    if (rc == 0) {
        lines_machine = create_machine();
        if (!lines_machine || !lines_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable Lines-mode instance");
        } else {
            rc = verify_lines_mode_tick_scaling(lines_machine);
        }
    }
    if (rc == 0) {
        resource_machine = create_machine();
        if (!resource_machine || !resource_machine->Vals) {
            rc = fail("CreateMachine did not provide a usable resource-cap instance");
        } else {
            rc = verify_lines_mode_resource_cap(resource_machine);
        }
    }

    if (resource_machine) delete_machine(*resource_machine);
    if (lines_machine) delete_machine(*lines_machine);
    if (rate_machine) delete_machine(*rate_machine);
    if (sample_machine) delete_machine(*sample_machine);
    if (dry_machine) delete_machine(*dry_machine);
    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-xfilter: PASS\n");
    std::printf("machine: Arguru CrossDelay\n");
    std::printf("parameters: 6\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: dry unity + sample-delay stereo offset + sample-rate scaling + Lines mode + preserved long Lines timing + bounded extreme Lines resources\n");
    return 0;
}