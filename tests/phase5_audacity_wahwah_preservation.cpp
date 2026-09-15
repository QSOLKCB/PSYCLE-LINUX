/*
** PSYCLE-LINUX Phase 5C Audacity WahWah preservation regression.
*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr float PI = 3.14159265359f;
constexpr unsigned int LFO_SKIP_SAMPLES = 30;

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

class WahReference {
public:
    explicit WahReference(int sample_rate) : sample_rate_(sample_rate)
    {
        init();
        set_parameter(0, 15);
        set_parameter(1, 0);
        set_parameter(2, 70);
        set_parameter(3, 25);
        set_parameter(4, 30);
    }

    void set_parameter(int par, int value)
    {
        switch (par) {
        case 0:
            freq_ = static_cast<float>(value) * 0.1f;
            lfoskip_ = freq_ * 2.0f * PI / static_cast<float>(sample_rate_);
            break;
        case 1:
            phase_ = static_cast<float>(value) * (PI / 180.0f);
            break;
        case 2:
            depth_ = static_cast<float>(value) * 0.01f;
            break;
        case 3:
            res_ = 1.0f / (static_cast<float>(value) * 0.2f);
            break;
        case 4:
            freqofs_ = value == 100 ? 0.9999f : static_cast<float>(value) * 0.01f;
            break;
        default:
            break;
        }
    }

    void set_raw_offset(float value) { freqofs_ = value; }

    void process(std::vector<float>& left, std::vector<float>& right)
    {
        if (left.size() != right.size() || left.empty()) return;
        unsigned int numsamples = static_cast<unsigned int>(left.size());
        const float depth_mul_1_minus_freqofs = depth_ * (1.0f - freqofs_) * 0.5f;

        if (skipcount_ == 0) recalc_filter(depth_mul_1_minus_freqofs);

        std::size_t cursor = 0;
        do {
            const float recip_l = a0_l_ / (a0_l_ * a0_r_);
            const float recip_r = a0_r_ / (a0_l_ * a0_r_);
            unsigned int cont = std::min(
                LFO_SKIP_SAMPLES - (skipcount_ % LFO_SKIP_SAMPLES), numsamples);
            skipcount_ += cont;
            numsamples -= cont;

            while (cont--) {
                const float in_l = left[cursor];
                const float in_r = right[cursor];

                const float out_l =
                    (b0_l_ * in_l + b1_l_ * xn1_l_ + b2_l_ * xn2_l_ -
                     a1_l_ * yn1_l_ - a2_l_ * yn2_l_) * recip_r;
                xn2_l_ = xn1_l_;
                xn1_l_ = in_l;
                yn2_l_ = yn1_l_;
                yn1_l_ = out_l;

                const float out_r =
                    (b0_r_ * in_r + b1_r_ * xn1_r_ + b2_r_ * xn2_r_ -
                     a1_r_ * yn1_r_ - a2_r_ * yn2_r_) * recip_l;
                xn2_r_ = xn1_r_;
                xn1_r_ = in_r;
                yn2_r_ = yn1_r_;
                yn1_r_ = out_r;

                left[cursor] = out_l;
                right[cursor] = out_r;
                ++cursor;
            }
            recalc_filter(depth_mul_1_minus_freqofs);
        } while (numsamples);
    }

private:
    void init()
    {
        freq_ = 1.5f;
        phase_ = 0.0f;
        depth_ = 0.7f;
        freqofs_ = 0.3f;
        res_ = 2.5f;
        lfoskip_ = freq_ * 2.0f * PI / static_cast<float>(sample_rate_);
        skipcount_ = 0;
        xn1_l_ = xn2_l_ = yn1_l_ = yn2_l_ = 0.0f;
        xn1_r_ = xn2_r_ = yn1_r_ = yn2_r_ = 0.0f;
        b0_l_ = b1_l_ = b2_l_ = a0_l_ = a1_l_ = a2_l_ = 0.0f;
        b0_r_ = b1_r_ = b2_r_ = a0_r_ = a1_r_ = a2_r_ = 0.0f;
        sample_rate_factor_ = 44100.0f / static_cast<float>(sample_rate_);
    }

    void recalc_filter(float depth_mul_1_minus_freqofs)
    {
        float calc_1_time = static_cast<float>(skipcount_) * lfoskip_ + phase_;
        if (calc_1_time > 4.0f * PI) {
            skipcount_ -= static_cast<unsigned int>(
                static_cast<int>(static_cast<float>(sample_rate_) / freq_));
            calc_1_time = static_cast<float>(skipcount_) * lfoskip_ + phase_;
        }

        const float sintime = std::sin(calc_1_time);
        const float costime = std::cos(calc_1_time);

        float frequency = 1.0f + costime;
        frequency = frequency * depth_mul_1_minus_freqofs + freqofs_;
        frequency = std::exp((frequency - 1.0f) * 6.0f);
        float omega = PI * frequency * sample_rate_factor_;
        float sn = std::sin(omega);
        float cs = std::cos(omega);
        float alpha = sn * res_;
        b1_l_ = 1.0f - cs;
        b2_l_ = b0_l_ = b1_l_ * 0.5f;
        a0_l_ = 1.0f + alpha;
        a1_l_ = -2.0f * cs;
        a2_l_ = 1.0f - alpha;

        frequency = 1.0f - sintime;
        frequency = frequency * depth_mul_1_minus_freqofs + freqofs_;
        frequency = std::exp((frequency - 1.0f) * 6.0f);
        omega = PI * frequency * sample_rate_factor_;
        sn = std::sin(omega);
        cs = std::cos(omega);
        alpha = sn * res_;
        b1_r_ = 1.0f - cs;
        b2_r_ = b0_r_ = b1_r_ * 0.5f;
        a0_r_ = 1.0f + alpha;
        a1_r_ = -2.0f * cs;
        a2_r_ = 1.0f - alpha;
    }

    int sample_rate_;
    float phase_;
    float lfoskip_;
    unsigned int skipcount_;
    float xn1_l_, xn2_l_, yn1_l_, yn2_l_;
    float xn1_r_, xn2_r_, yn1_r_, yn2_r_;
    float b0_l_, b1_l_, b2_l_, a0_l_, a1_l_, a2_l_;
    float b0_r_, b1_r_, b2_r_, a0_r_, a1_r_, a2_r_;
    float freq_;
    float depth_, freqofs_, res_;
    float sample_rate_factor_;
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

int verify_nonpositive_blocks(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);

    float left = 1234.5f;
    float right = -987.25f;
    machine->Work(&left, &right, 0, 1);
    if (!near(left, 1234.5f) || !near(right, -987.25f))
        return fail("zero-length callback touched host buffers");

    const pid_t child = fork();
    if (child < 0) return fail("fork failed for negative-count guard probe");
    if (child == 0) {
        float child_left = 4321.25f;
        float child_right = -2468.5f;
        alarm(2);
        machine->Work(&child_left, &child_right, -1, 1);
        const bool unchanged = near(child_left, 4321.25f) && near(child_right, -2468.5f);
        _exit(unchanged ? 0 : 3);
    }

    int status = 0;
    if (waitpid(child, &status, 0) != child)
        return fail("waitpid failed for negative-count guard probe");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return fail("negative callback count escaped the signed host-boundary guard");

    std::printf("phase5-audacity-wahwah: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

int verify_stereo_lfo(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);

    std::vector<float> left(512, 0.0f), right(512, 0.0f);
    std::vector<float> ref_left(512, 0.0f), ref_right(512, 0.0f);
    left[0] = right[0] = ref_left[0] = ref_right[0] = 12000.0f;

    WahReference reference(44100);
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);
    reference.process(ref_left, ref_right);

    double max_left_error = 0.0;
    double max_right_error = 0.0;
    double reference_channel_difference = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]) ||
                !std::isfinite(ref_left[i]) || !std::isfinite(ref_right[i]))
            return fail("default WahWah/reference produced non-finite output");
        max_left_error = std::max(max_left_error,
            std::fabs(static_cast<double>(left[i] - ref_left[i])));
        max_right_error = std::max(max_right_error,
            std::fabs(static_cast<double>(right[i] - ref_right[i])));
        reference_channel_difference = std::max(reference_channel_difference,
            std::fabs(static_cast<double>(ref_left[i] - ref_right[i])));
    }
    if (max_left_error > 0.1)
        return fail("left Wah modulation no longer matches the source-derived reference");
    if (max_right_error > 0.1)
        return fail("right Wah modulation no longer matches the source-derived reference");
    if (reference_channel_difference < 1.0e-3)
        return fail("source-derived opposed stereo reference unexpectedly collapsed to mono");

    std::printf("phase5-audacity-wahwah: stereo PASS left=reference right=reference opposed=yes\n");
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

    std::vector<float> left(1024, 0.0f), right(1024, 0.0f);
    std::vector<float> guarded_left(1024, 0.0f), guarded_right(1024, 0.0f);
    std::vector<float> raw_left(1024, 0.0f), raw_right(1024, 0.0f);
    left[0] = right[0] = guarded_left[0] = guarded_right[0] =
        raw_left[0] = raw_right[0] = 16000.0f;

    WahReference guarded(44100);
    guarded.set_parameter(4, 100);
    WahReference unguarded(44100);
    unguarded.set_raw_offset(1.0f);

    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);
    guarded.process(guarded_left, guarded_right);
    unguarded.process(raw_left, raw_right);

    double guarded_sse = 0.0;
    double raw_sse = 0.0;
    double guarded_vs_raw = 0.0;
    double actual_tail_peak = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail("maximum Wah offset produced non-finite output");

        const double gl = static_cast<double>(left[i] - guarded_left[i]);
        const double gr = static_cast<double>(right[i] - guarded_right[i]);
        const double rl = static_cast<double>(left[i] - raw_left[i]);
        const double rr = static_cast<double>(right[i] - raw_right[i]);
        guarded_sse += gl * gl + gr * gr;
        raw_sse += rl * rl + rr * rr;
        guarded_vs_raw = std::max(guarded_vs_raw,
            std::fabs(static_cast<double>(guarded_left[i] - raw_left[i])));
        guarded_vs_raw = std::max(guarded_vs_raw,
            std::fabs(static_cast<double>(guarded_right[i] - raw_right[i])));
        if (i > 0) {
            actual_tail_peak = std::max(actual_tail_peak,
                std::fabs(static_cast<double>(left[i])));
            actual_tail_peak = std::max(actual_tail_peak,
                std::fabs(static_cast<double>(right[i])));
        }
    }
    if (guarded_vs_raw < 1.0)
        return fail("max-offset oracle cannot distinguish the 0.9999 guard from raw 1.0");
    if (actual_tail_peak < 0.5)
        return fail("maximum-offset response lost the guarded impulse tail");
    if (!(guarded_sse < raw_sse)) {
        std::fprintf(stderr,
            "phase5-audacity-wahwah: max-offset guard_sse=%g raw_sse=%g tail=%g\n",
            guarded_sse, raw_sse, actual_tail_peak);
        return fail("maximum-offset response is closer to raw 1.0 than guarded 0.9999 behavior");
    }

    std::printf("phase5-audacity-wahwah: max-offset PASS guarded-reference=yes differs-from-1.0=yes\n");
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

    double transition_error_left = 0.0;
    double transition_error_right = 0.0;
    double rate_difference_left = 0.0;
    double rate_difference_right = 0.0;
    for (int i = 0; i < 512; ++i) {
        if (!std::isfinite(trans_l[i]) || !std::isfinite(trans_r[i]) ||
                !std::isfinite(ref88_l[i]) || !std::isfinite(ref88_r[i]) ||
                !std::isfinite(ref44_l[i]) || !std::isfinite(ref44_r[i]))
            return fail("sample-rate gate produced non-finite stereo output");

        transition_error_left = std::max(transition_error_left,
            std::fabs(static_cast<double>(trans_l[i] - ref88_l[i])));
        transition_error_right = std::max(transition_error_right,
            std::fabs(static_cast<double>(trans_r[i] - ref88_r[i])));
        rate_difference_left = std::max(rate_difference_left,
            std::fabs(static_cast<double>(ref44_l[i] - ref88_l[i])));
        rate_difference_right = std::max(rate_difference_right,
            std::fabs(static_cast<double>(ref44_r[i] - ref88_r[i])));
    }
    if (transition_error_left > 1.0e-3 || transition_error_right > 1.0e-3)
        return fail("live 44.1->88.2 kHz stereo state does not match fresh 88.2 kHz response");
    if (rate_difference_left < 1.0e-2 || rate_difference_right < 1.0e-2)
        return fail("stereo rate oracle is not sensitive on both WahWah channels");

    std::printf("phase5-audacity-wahwah: live-rate PASS left=fresh88 right=fresh88 both-rate-sensitive=yes\n");
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
    CMachineInterface* nonpositive = nullptr;
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
        nonpositive = make();
        if (!nonpositive || !nonpositive->Vals) rc = fail("CreateMachine returned unusable nonpositive-block instance");
        else rc = verify_nonpositive_blocks(nonpositive, info);
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
