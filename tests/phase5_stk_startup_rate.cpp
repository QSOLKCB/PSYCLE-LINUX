/*
** PSYCLE-LINUX Phase 5C STK startup-rate and rate-change regressions.
**
** This companion oracle covers behavior that cannot be proven by a
** 44.1 -> 88.2 kHz transition alone: construction when the host starts at a
** non-default rate, and the rule that rebuilding a one-shot Shakers voice on
** a later rate change must not replay an old percussion hit.
*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>
#include <stk/ADSR.h>
#include <stk/NRev.h>
#include <stk/Plucked.h>
#include <stk/Shakers.h>
#include <stk/Stk.h>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::MAX_BUFFER_LENGTH;

namespace {

constexpr int INITIAL_RATE = 96000;
constexpr double PLUCKED_OFFSET = -36.3763165623;
constexpr unsigned int PLUCKED_SEED = 0x960051a7u;
constexpr unsigned int SHAKER_SEED = 0x96005a17u;
constexpr unsigned int STALE_SHAKER_SEED = 0x88205a17u;

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}
    void set_sample_rate(int value) { sample_rate_ = value; }
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

struct Module {
    void* library;
    const CMachineInfo* info;
    CMachineInterface* (*create)();
    void (*destroy)(CMachineInterface&);
};

bool load_module(const char* path, Module& module)
{
    module = {};
    module.library = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!module.library) {
        std::fprintf(stderr, "phase5-stk-startup-rate: dlopen failed for %s: %s\n",
            path, dlerror());
        return false;
    }
    using GetInfoFn = const CMachineInfo* (*)();
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(module.library, "GetInfo"));
    module.create = reinterpret_cast<CMachineInterface* (*)()>(
        dlsym(module.library, "CreateMachine"));
    module.destroy = reinterpret_cast<void (*)(CMachineInterface&)>(
        dlsym(module.library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !module.create || !module.destroy) {
        std::fprintf(stderr, "phase5-stk-startup-rate: native ABI lookup failed for %s: %s\n",
            path, error ? error : "missing symbol");
        dlclose(module.library);
        module.library = nullptr;
        return false;
    }
    module.info = get_info();
    return module.info != nullptr;
}

void close_module(Module& module)
{
    if (module.library) dlclose(module.library);
    module = {};
}

void apply_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback& callback)
{
    machine->pCB = &callback;
    for (int i = 0; i < info->numParameters; ++i)
        machine->Vals[i] = info->Parameters[i]->DefValue;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        if (info->Parameters[i]->Flags == psycle::plugin_interface::MPF_STATE)
            machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }
}

void process_blocks(CMachineInterface* machine, std::vector<float>& left,
    std::vector<float>& right, int tracks = 1)
{
    std::size_t offset = 0;
    while (offset < left.size()) {
        const int block = static_cast<int>(std::min<std::size_t>(
            left.size() - offset, MAX_BUFFER_LENGTH));
        machine->Work(left.data() + offset, right.data() + offset, block, tracks);
        offset += static_cast<std::size_t>(block);
    }
}

bool finite_signal(const std::vector<float>& left, const std::vector<float>& right)
{
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i])) return false;
    }
    return true;
}

bool same_signal(const std::vector<float>& actual_l, const std::vector<float>& actual_r,
    const std::vector<float>& expected_l, const std::vector<float>& expected_r,
    float tolerance)
{
    if (actual_l.size() != expected_l.size() || actual_r.size() != expected_r.size() ||
            !finite_signal(actual_l, actual_r) || !finite_signal(expected_l, expected_r))
        return false;
    for (std::size_t i = 0; i < actual_l.size(); ++i) {
        if (std::fabs(actual_l[i] - expected_l[i]) > tolerance ||
                std::fabs(actual_r[i] - expected_r[i]) > tolerance)
            return false;
    }
    return true;
}

bool silent_signal(const std::vector<float>& left, const std::vector<float>& right,
    float tolerance = 0.0f)
{
    if (!finite_signal(left, right)) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::fabs(left[i]) > tolerance || std::fabs(right[i]) > tolerance)
            return false;
    }
    return true;
}

void render_direct_plucked(int sample_rate, int note, std::size_t samples,
    unsigned int seed, std::vector<float>& left, std::vector<float>& right)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    stk::Plucked track(20.0);
    stk::ADSR adsr;
    track.clear();
    track.noteOff(0.0);
    adsr.setAllTimes(
        static_cast<stk::StkFloat>(32.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(32.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(16768.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(328.0 * 0.000030517578125));
    const stk::StkFloat frequency = static_cast<stk::StkFloat>(
        std::pow(2.0, (static_cast<double>(note) - PLUCKED_OFFSET) / 12.0));
    std::srand(seed);
    adsr.keyOn();
    track.noteOn(frequency, 1.0);
    left.assign(samples, 0.0f);
    right.assign(samples, 0.0f);
    for (std::size_t i = 0; i < samples; ++i) {
        float value = static_cast<float>(adsr.tick() * track.tick()) * 32767.0f;
        value = std::max(-32767.0f, std::min(32767.0f, value));
        left[i] = value;
        right[i] = value;
    }
}

std::vector<float> render_direct_nrev(int sample_rate, std::size_t samples)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    stk::NRev reverb;
    reverb.setT60(static_cast<stk::StkFloat>(80.0 * 0.03125));
    reverb.setEffectMix(1.0);
    reverb.clear();
    std::vector<float> output(samples, 0.0f);
    for (std::size_t i = 0; i < samples; ++i) {
        const stk::StkFloat input = i == 0 ? 1.0 : 0.0;
        output[i] = static_cast<float>(reverb.tick(input, 0));
    }
    return output;
}

void render_direct_shaker(int sample_rate, std::size_t samples, unsigned int seed,
    std::vector<float>& left, std::vector<float>& right)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    stk::Shakers shaker;
    shaker.controlChange(2, 64.0);
    shaker.controlChange(4, 64.0);
    shaker.controlChange(11, 10.0);
    shaker.controlChange(1, 64.0);
    shaker.controlChange(128, 64.0);
    const stk::StkFloat frequency = static_cast<stk::StkFloat>(
        220.0 * std::pow(2.0, 7.0 / 12.0));
    std::srand(seed);
    shaker.noteOn(frequency, 10.0);
    left.assign(samples, 0.0f);
    right.assign(samples, 0.0f);
    for (std::size_t i = 0; i < samples; ++i) {
        float value = static_cast<float>(shaker.tick()) * 32767.0f;
        value = std::max(-32767.0f, std::min(32767.0f, value));
        left[i] = value;
        right[i] = value;
    }
}

int verify_plucked(Module& module)
{
    stk::Stk::setSampleRate(44100.0);
    TestCallback callback(INITIAL_RATE);
    CMachineInterface* machine = module.create();
    if (!machine || !machine->Vals) return 1;
    apply_defaults(machine, module.info, callback);
    if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - INITIAL_RATE) > 0.5) {
        module.destroy(*machine);
        return 1;
    }
    std::srand(PLUCKED_SEED);
    machine->SeqTick(0, 60, 0, 0x0c, 255);
    std::vector<float> left(8192, 0.0f), right(8192, 0.0f);
    process_blocks(machine, left, right);
    std::vector<float> ref_l, ref_r;
    render_direct_plucked(INITIAL_RATE, 60, left.size(), PLUCKED_SEED, ref_l, ref_r);
    const bool ok = same_signal(left, right, ref_l, ref_r, 1.0e-3f);
    module.destroy(*machine);
    if (!ok) return 1;
    std::printf("phase5-stk-startup-rate: Plucked PASS initial=96k-STK-reference\n");
    return 0;
}

int verify_reverbs(Module& module)
{
    stk::Stk::setSampleRate(44100.0);
    TestCallback callback(INITIAL_RATE);
    CMachineInterface* machine = module.create();
    if (!machine || !machine->Vals) return 1;
    apply_defaults(machine, module.info, callback);
    machine->ParameterTweak(0, 1);
    machine->ParameterTweak(1, 80);
    machine->ParameterTweak(2, 100);
    machine->ParameterTweak(3, 0);
    std::vector<float> left(65536, 0.0f), right(65536, 0.0f);
    left[0] = 1.0f;
    process_blocks(machine, left, right);
    const std::vector<float> expected = render_direct_nrev(INITIAL_RATE, left.size());
    bool ok = right.size() == left.size();
    for (std::size_t i = 0; ok && i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]) ||
                std::fabs(left[i] - expected[i]) > 1.0e-6f ||
                std::fabs(right[i]) > 1.0e-7f)
            ok = false;
    }
    module.destroy(*machine);
    if (!ok) return 1;
    std::printf("phase5-stk-startup-rate: Reverbs PASS initial=96k-STK-reference\n");
    return 0;
}

int verify_shakers(Module& module)
{
    stk::Stk::setSampleRate(44100.0);
    TestCallback callback(INITIAL_RATE);
    CMachineInterface* machine = module.create();
    if (!machine || !machine->Vals) return 1;
    apply_defaults(machine, module.info, callback);
    std::srand(SHAKER_SEED);
    machine->SeqTick(0, 48, 0, 0x0c, 255);
    std::vector<float> left(8192, 0.0f), right(8192, 0.0f);
    process_blocks(machine, left, right);
    std::vector<float> ref_l, ref_r;
    render_direct_shaker(INITIAL_RATE, left.size(), SHAKER_SEED, ref_l, ref_r);
    const bool initial_ok = same_signal(left, right, ref_l, ref_r, 1.0e-3f);
    module.destroy(*machine);
    if (!initial_ok) return 1;
    std::printf("phase5-stk-startup-rate: Shakers PASS initial=96k-STK-reference\n");

    stk::Stk::setSampleRate(44100.0);
    TestCallback stale_callback(44100);
    CMachineInterface* stale = module.create();
    if (!stale || !stale->Vals) return 1;
    apply_defaults(stale, module.info, stale_callback);
    std::srand(STALE_SHAKER_SEED);
    stale->SeqTick(0, 48, 0, 0x0c, 255);
    left.assign(262144, 0.0f);
    right.assign(262144, 0.0f);
    process_blocks(stale, left, right);

    stale_callback.set_sample_rate(88200);
    stale->SequencerTick();
    left.assign(8192, 0.0f);
    right.assign(8192, 0.0f);
    process_blocks(stale, left, right);
    const bool stale_ok = silent_signal(left, right);
    module.destroy(*stale);
    if (!stale_ok) return 1;
    std::printf("phase5-stk-startup-rate: Shakers PASS stale-rate-change=no-retrigger\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::fprintf(stderr,
            "usage: %s STK_PLUCKED_SO STK_REVERBS_SO STK_SHAKERS_SO\n", argv[0]);
        return 2;
    }

    Module plucked{}, reverbs{}, shakers{};
    if (!load_module(argv[1], plucked) || !load_module(argv[2], reverbs) ||
            !load_module(argv[3], shakers)) {
        close_module(plucked);
        close_module(reverbs);
        close_module(shakers);
        return 1;
    }

    int rc = verify_plucked(plucked);
    if (rc == 0) rc = verify_reverbs(reverbs);
    if (rc == 0) rc = verify_shakers(shakers);

    close_module(plucked);
    close_module(reverbs);
    close_module(shakers);

    if (rc != 0) {
        std::fprintf(stderr, "phase5-stk-startup-rate: FAIL\n");
        return rc;
    }
    std::printf("phase5-stk-startup-rate: PASS machines=3\n");
    return 0;
}
