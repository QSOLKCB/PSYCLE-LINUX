/*
** PSYCLE-LINUX Phase 5C Dalay Delay preservation regression.
**
** Loads the retained ayeternal Dalay Delay through Psycle's native ABI and
** freezes its identity, seven-parameter surface, snap semantics, stereo
** line-delay timing and live host timing reconfiguration.
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

constexpr int PARAM_DRY = 0;
constexpr int PARAM_WET = 1;
constexpr int PARAM_DELAY_LEFT = 2;
constexpr int PARAM_FEEDBACK_LEFT = 3;
constexpr int PARAM_DELAY_RIGHT = 4;
constexpr int PARAM_FEEDBACK_RIGHT = 5;
constexpr int PARAM_SNAP = 6;
constexpr int INPUT_MAX = 65535;
constexpr float IMPULSE = 1024.0f;

struct ExpectedParameter {
    const char* name;
    int min_value;
    int max_value;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"dry", 0, 65535, 65535},
    {"wet", 0, 65535, 32767},
    {"delay left", 0, 65535, 0},
    {"feedback left", 0, 65535, 32767},
    {"delay right", 0, 65535, 0},
    {"feedback right", 0, 65535, 32767},
    {"snap to", 0, 839, 3},
};

class TestCallback : public CFxCallback {
public:
    TestCallback(int sample_rate, int bpm, int tpb)
        : sample_rate_(sample_rate), bpm_(bpm), tpb_(tpb) {}

    void set_timing(int sample_rate, int bpm, int tpb)
    {
        sample_rate_ = sample_rate;
        bpm_ = bpm;
        tpb_ = tpb;
    }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override
    {
        return static_cast<int>(samples_per_tick());
    }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return bpm_; }
    int GetTPB() const override { return tpb_; }
    bool FileBox(bool, char[], char[]) override { return false; }

    double samples_per_tick() const
    {
        return static_cast<double>(sample_rate_) * 60.0 /
            static_cast<double>(bpm_ * tpb_);
    }

private:
    int sample_rate_;
    int bpm_;
    int tpb_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-dalay-delay: FAIL: %s\n", message);
    return 1;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0110 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 4) {
        return fail("ABI/version/type/column geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, "ayeternal Dalay Delay") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Dalay Delay") != 0 ||
            !info->Author || std::strcmp(info->Author, "bohan") != 0) {
        return fail("historical identity changed");
    }
    const int count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != count || !info->Parameters)
        return fail("seven-parameter geometry changed");
    for (int i = 0; i < count; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        const ExpectedParameter& e = EXPECTED_PARAMETERS[i];
        if (!p || !p->Name || !p->Description ||
                std::strcmp(p->Name, e.name) != 0 ||
                std::strcmp(p->Description, e.name) != 0 ||
                p->MinValue != e.min_value || p->MaxValue != e.max_value ||
                p->Flags != psycle::plugin_interface::MPF_STATE ||
                p->DefValue != e.default_value) {
            std::fprintf(stderr,
                "phase5-dalay-delay: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-dalay-delay: metadata PASS version=0x0110 parameters=7 identity=ayeternal-Dalay-Delay\n");
    return 0;
}

void initialize(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

void configure_delay(CMachineInterface* machine)
{
    /* snap=7 means 1/8-line quantisation.  Raw 841 and 421 map to exact
    ** snapped delays of 1.0 and 0.5 tracker lines respectively. */
    machine->ParameterTweak(PARAM_SNAP, 7);
    machine->ParameterTweak(PARAM_DRY, 32767);       // historical nearest-to-zero: -1/65535
    machine->ParameterTweak(PARAM_WET, 65535);       // +1
    machine->ParameterTweak(PARAM_FEEDBACK_LEFT, 32767);
    machine->ParameterTweak(PARAM_FEEDBACK_RIGHT, 32767);
    machine->ParameterTweak(PARAM_DELAY_LEFT, 841);  // 1 line after snap
    machine->ParameterTweak(PARAM_DELAY_RIGHT, 421); // 1/2 line after snap
}

int verify_descriptions(CMachineInterface* machine)
{
    configure_delay(machine);
    char text[128];

    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_DRY, 12345) || std::strcmp(text, "0") != 0)
        return fail("near-zero dry description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_DELAY_LEFT, 0) ||
            std::strcmp(text, "1 ticks (lines)") != 0)
        return fail("left delay description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_DELAY_RIGHT, 0) ||
            std::strcmp(text, "0.5 ticks (lines)") != 0)
        return fail("right delay description changed");
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_SNAP, 0) ||
            std::strcmp(text, "1 / 8 ticks (lines)") != 0)
        return fail("snap description changed");

    machine->ParameterTweak(PARAM_SNAP, 839);
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, PARAM_SNAP, 0) ||
            std::strcmp(text, "off 1 / 840 ticks (lines)") != 0)
        return fail("snap-off description changed");

    std::printf("phase5-dalay-delay: describe PASS left=1-line right=0.5-line snap=1/8 off=1/840\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine)
{
    float left[] = {11.0f, 22.0f, 33.0f};
    float right[] = {-11.0f, -22.0f, -33.0f};
    machine->Work(&left[1], &right[1], 0, 1);
    machine->Work(&left[1], &right[1], -7, 1);
    if (left[0] != 11.0f || left[1] != 22.0f || left[2] != 33.0f ||
            right[0] != -11.0f || right[1] != -22.0f || right[2] != -33.0f)
        return fail("non-positive callbacks modified host buffers");
    std::printf("phase5-dalay-delay: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

void process_in_host_blocks(CMachineInterface* machine,
    std::vector<float>& left, std::vector<float>& right)
{
    const int total = static_cast<int>(left.size());
    for (int offset = 0; offset < total;) {
        const int block = std::min(256, total - offset);
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
        offset += block;
    }
}

int first_large_after_zero(const std::vector<float>& data)
{
    for (std::size_t i = 1; i < data.size(); ++i) {
        if (std::fabs(data[i]) > 1.0f) return static_cast<int>(i);
    }
    return -1;
}

int expected_ring_length(double delay_lines, const TestCallback& callback)
{
    return 1 + static_cast<int>(delay_lines * callback.samples_per_tick());
}

int verify_stereo_timing(CMachineInterface* machine, const TestCallback& callback)
{
    configure_delay(machine);
    const int expected_left = expected_ring_length(1.0, callback);
    const int expected_right = expected_ring_length(0.5, callback);
    const int frames = expected_left + 300;
    std::vector<float> left(frames, 0.0f);
    std::vector<float> right(frames, 0.0f);
    left[0] = IMPULSE;
    right[0] = IMPULSE;
    process_in_host_blocks(machine, left, right);

    if (first_large_after_zero(left) != expected_left ||
            first_large_after_zero(right) != expected_right)
        return fail("stereo snapped delay timing changed");

    const float epsilon = 1.0f / static_cast<float>(INPUT_MAX);
    if (std::fabs(left[0] + IMPULSE * epsilon) > 1e-5f ||
            std::fabs(right[0] + IMPULSE * epsilon) > 1e-5f ||
            std::fabs(left[expected_left] - IMPULSE) > 1e-4f ||
            std::fabs(right[expected_right] - IMPULSE) > 1e-4f)
        return fail("dry/wet source-derived impulse amplitudes changed");

    std::printf("phase5-dalay-delay: stereo PASS left=%d right=%d dry-near-zero=-1/65535 wet=1\n",
        expected_left, expected_right);
    return 0;
}

int render_target(CMachineInterface* machine, const TestCallback& callback,
    std::vector<float>& left, std::vector<float>& right)
{
    const int expected_left = expected_ring_length(1.0, callback);
    const int expected_right = expected_ring_length(0.5, callback);
    const int frames = expected_left + 300;
    left.assign(frames, 0.0f);
    right.assign(frames, 0.0f);
    left[0] = IMPULSE;
    right[0] = IMPULSE;
    process_in_host_blocks(machine, left, right);
    if (first_large_after_zero(left) != expected_left ||
            first_large_after_zero(right) != expected_right)
        return fail("target timing render did not hit source-derived ring lengths");
    return 0;
}

int verify_live_timing(CMachineInterface* live, const CMachineInfo* info,
    TestCallback& live_callback, CMachineInterface* fresh)
{
    configure_delay(live);
    live_callback.set_timing(88200, 150, 8);
    live->SequencerTick();

    TestCallback fresh_callback(88200, 150, 8);
    initialize(fresh, info, &fresh_callback);
    configure_delay(fresh);

    std::vector<float> live_left;
    std::vector<float> live_right;
    std::vector<float> fresh_left;
    std::vector<float> fresh_right;
    if (render_target(live, live_callback, live_left, live_right) != 0) return 1;
    if (render_target(fresh, fresh_callback, fresh_left, fresh_right) != 0) return 1;
    if (live_left != fresh_left || live_right != fresh_right)
        return fail("live timing transition no longer matches a fresh target-timing instance");

    const int target_left = expected_ring_length(1.0, fresh_callback);
    const int target_right = expected_ring_length(0.5, fresh_callback);
    TestCallback stale_callback(44100, 120, 4);
    const int stale_left = expected_ring_length(1.0, stale_callback);
    const int stale_right = expected_ring_length(0.5, stale_callback);
    if (target_left == stale_left || target_right == stale_right)
        return fail("live timing oracle is not rate/timing-sensitive");

    std::printf("phase5-dalay-delay: live-timing PASS 44.1k/120/4->88.2k/150/8 left=%d right=%d stale-left=%d stale-right=%d\n",
        target_left, target_right, stale_left, stale_right);
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_DELAY_SO\n", argv[0]);
        return 2;
    }

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-dalay-delay: FAIL: dlopen: %s\n", dlerror());
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
    CMachineInterface* descriptions = nullptr;
    CMachineInterface* timing = nullptr;
    CMachineInterface* live = nullptr;
    CMachineInterface* fresh = nullptr;
    TestCallback standard_callback(44100, 120, 4);
    TestCallback live_callback(44100, 120, 4);

    if (rc == 0) {
        descriptions = create_machine();
        timing = create_machine();
        live = create_machine();
        fresh = create_machine();
        if (!descriptions || !timing || !live || !fresh)
            rc = fail("CreateMachine returned null");
    }
    if (rc == 0) {
        initialize(descriptions, info, &standard_callback);
        initialize(timing, info, &standard_callback);
        initialize(live, info, &live_callback);
        rc = verify_descriptions(descriptions);
    }
    if (rc == 0) rc = verify_nonpositive(descriptions);
    if (rc == 0) rc = verify_stereo_timing(timing, standard_callback);
    if (rc == 0) rc = verify_live_timing(live, info, live_callback, fresh);

    if (descriptions) delete_machine(*descriptions);
    if (timing) delete_machine(*timing);
    if (live) delete_machine(*live);
    if (fresh) delete_machine(*fresh);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-dalay-delay: PASS\n");
    return 0;
}
