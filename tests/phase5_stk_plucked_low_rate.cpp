/*
** PSYCLE-LINUX Phase 5C STK Plucked low-note rate regression.
**
** A middle-C probe is not sufficient to prove that stk::Plucked was
** reconstructed after an upward sample-rate change: its constructor sizes the
** maximum delay from the construction rate and the requested 20 Hz floor, and
** note 60 still fits in a delay line created at 44.1 kHz. Note 24 deliberately
** crosses that stale capacity at 88.2/96 kHz, so this oracle fails if the
** wrapper only updates Stk::sampleRate() without rebuilding the instrument.
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
#include <stk/Plucked.h>
#include <stk/Stk.h>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::MAX_BUFFER_LENGTH;

namespace {

constexpr int CONSTRUCTION_RATE = 44100;
constexpr int LIVE_RATE = 88200;
constexpr int STARTUP_RATE = 96000;
constexpr int LOW_NOTE = 24;
constexpr double LOWEST_FREQUENCY = 20.0;
constexpr double PLUCKED_OFFSET = -36.3763165623;
constexpr unsigned int LIVE_SEED = 0x882024u;
constexpr unsigned int STARTUP_SEED = 0x960024u;
constexpr std::size_t RENDER_SAMPLES = 16384;

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
        std::fprintf(stderr, "phase5-stk-plucked-low-rate: dlopen failed: %s\n", dlerror());
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
        std::fprintf(stderr, "phase5-stk-plucked-low-rate: native ABI lookup failed: %s\n",
            error ? error : "missing symbol");
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
    std::vector<float>& right)
{
    std::size_t offset = 0;
    while (offset < left.size()) {
        const int block = static_cast<int>(std::min<std::size_t>(
            left.size() - offset, MAX_BUFFER_LENGTH));
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
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

stk::StkFloat note_frequency(int note)
{
    return static_cast<stk::StkFloat>(
        std::pow(2.0, (static_cast<double>(note) - PLUCKED_OFFSET) / 12.0));
}

bool crosses_stale_capacity(int target_rate)
{
    const double stale_capacity = static_cast<double>(CONSTRUCTION_RATE) / LOWEST_FREQUENCY;
    const double required_delay = static_cast<double>(target_rate) /
        static_cast<double>(note_frequency(LOW_NOTE));
    return required_delay > stale_capacity;
}

void render_direct(int sample_rate, unsigned int seed,
    std::vector<float>& left, std::vector<float>& right)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    stk::Plucked track(LOWEST_FREQUENCY);
    stk::ADSR adsr;
    track.clear();
    track.noteOff(0.0);
    adsr.setAllTimes(
        static_cast<stk::StkFloat>(32.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(32.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(16768.0 * 0.000030517578125),
        static_cast<stk::StkFloat>(328.0 * 0.000030517578125));

    std::srand(seed);
    adsr.keyOn();
    track.noteOn(note_frequency(LOW_NOTE), 1.0);

    left.assign(RENDER_SAMPLES, 0.0f);
    right.assign(RENDER_SAMPLES, 0.0f);
    for (std::size_t i = 0; i < RENDER_SAMPLES; ++i) {
        float value = static_cast<float>(adsr.tick() * track.tick()) * 32767.0f;
        value = std::max(-32767.0f, std::min(32767.0f, value));
        left[i] = value;
        right[i] = value;
    }
}

bool verify_live_rate(Module& module)
{
    if (!crosses_stale_capacity(LIVE_RATE)) {
        std::fprintf(stderr,
            "phase5-stk-plucked-low-rate: note %d does not cross stale 44.1 kHz capacity at 88.2 kHz\n",
            LOW_NOTE);
        return false;
    }

    stk::Stk::setSampleRate(CONSTRUCTION_RATE);
    TestCallback callback(CONSTRUCTION_RATE);
    CMachineInterface* machine = module.create();
    if (!machine || !machine->Vals) {
        if (machine) module.destroy(*machine);
        return false;
    }
    apply_defaults(machine, module.info, callback);

    callback.set_sample_rate(LIVE_RATE);
    machine->SequencerTick();
    if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - LIVE_RATE) > 0.5) {
        module.destroy(*machine);
        return false;
    }

    std::srand(LIVE_SEED);
    machine->SeqTick(0, LOW_NOTE, 0, 0x0c, 255);
    std::vector<float> left(RENDER_SAMPLES, 0.0f), right(RENDER_SAMPLES, 0.0f);
    process_blocks(machine, left, right);

    std::vector<float> expected_l, expected_r;
    render_direct(LIVE_RATE, LIVE_SEED, expected_l, expected_r);
    const bool ok = same_signal(left, right, expected_l, expected_r, 1.0e-3f);
    module.destroy(*machine);
    return ok;
}

bool verify_startup_rate(Module& module)
{
    if (!crosses_stale_capacity(STARTUP_RATE)) {
        std::fprintf(stderr,
            "phase5-stk-plucked-low-rate: note %d does not cross stale 44.1 kHz capacity at 96 kHz\n",
            LOW_NOTE);
        return false;
    }

    stk::Stk::setSampleRate(CONSTRUCTION_RATE);
    TestCallback callback(STARTUP_RATE);
    CMachineInterface* machine = module.create();
    if (!machine || !machine->Vals) {
        if (machine) module.destroy(*machine);
        return false;
    }
    apply_defaults(machine, module.info, callback);

    if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - STARTUP_RATE) > 0.5) {
        module.destroy(*machine);
        return false;
    }

    std::srand(STARTUP_SEED);
    machine->SeqTick(0, LOW_NOTE, 0, 0x0c, 255);
    std::vector<float> left(RENDER_SAMPLES, 0.0f), right(RENDER_SAMPLES, 0.0f);
    process_blocks(machine, left, right);

    std::vector<float> expected_l, expected_r;
    render_direct(STARTUP_RATE, STARTUP_SEED, expected_l, expected_r);
    const bool ok = same_signal(left, right, expected_l, expected_r, 1.0e-3f);
    module.destroy(*machine);
    return ok;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr,
            "usage: %s /path/to/stk-plucked.so\n", argc > 0 ? argv[0] : "phase5-stk-plucked-low-rate");
        return 2;
    }

    Module module{};
    if (!load_module(argv[1], module)) return 1;

    if (!module.info->Name || std::strcmp(module.info->Name, "stk Plucked") != 0) {
        std::fprintf(stderr, "phase5-stk-plucked-low-rate: unexpected module identity\n");
        close_module(module);
        return 1;
    }

    if (!verify_live_rate(module)) {
        std::fprintf(stderr,
            "phase5-stk-plucked-low-rate: FAIL live 44.1->88.2 kHz note=%d diverged from STK reference\n",
            LOW_NOTE);
        close_module(module);
        return 1;
    }
    std::printf(
        "phase5-stk-plucked-low-rate: PASS live=44.1->88.2k note=%d STK-reference\n",
        LOW_NOTE);

    if (!verify_startup_rate(module)) {
        std::fprintf(stderr,
            "phase5-stk-plucked-low-rate: FAIL startup 96 kHz note=%d diverged from STK reference\n",
            LOW_NOTE);
        close_module(module);
        return 1;
    }
    std::printf(
        "phase5-stk-plucked-low-rate: PASS startup=96k note=%d STK-reference\n",
        LOW_NOTE);

    close_module(module);
    std::printf("phase5-stk-plucked-low-rate: PASS\n");
    return 0;
}
