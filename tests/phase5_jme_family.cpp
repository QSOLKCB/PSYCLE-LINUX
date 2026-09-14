/*
** PSYCLE-LINUX Phase 5C JME-family native preservation regression.
**
** Loads the four retained JME generators through Psycle's historical native
** ABI and freezes identity/version/parameter geometry plus representative
** deterministic note and tracker-volume behavior.  The 1.2/1.3 identities
** remain separate from the later 1.6 builds.
*/

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

struct Spec {
    const char* label;
    const char* name;
    const char* short_name;
    int version;
    int parameters;
    int columns;
    std::uint64_t metadata_hash;
};

/* Hashes are filled after the first source-built observation and then frozen.
** A zero value is deliberately observation-only while bringing up the gate;
** the smoke script refuses to mark the family complete until all are nonzero. */
const Spec SPECS[] = {
    {"Blitz 1.2.1", "Blitz 1.2.1", "Blitz", 0x0121, 112, 7, 0},
    {"Blitz 1.6", "Blitz 1.6", "Blitz", 0x0160, 112, 7, 0},
    {"GameFX 1.3.1", "GameFX ver. 1.3.1", "GameFX", 0x0131, 128, 8, 0},
    {"GameFX 1.6", "GameFX ver. 1.6", "GameFX", 0x0160, 128, 8, 0},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}

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

int fail(const Spec& spec, const char* message)
{
    std::fprintf(stderr, "phase5-jme-family: FAIL [%s]: %s\n", spec.label, message);
    return 1;
}

void hash_byte(std::uint64_t& hash, unsigned char byte)
{
    hash ^= static_cast<std::uint64_t>(byte);
    hash *= UINT64_C(1099511628211);
}

void hash_string(std::uint64_t& hash, const char* text)
{
    if (!text) {
        hash_byte(hash, 0xff);
        return;
    }
    while (*text) hash_byte(hash, static_cast<unsigned char>(*text++));
    hash_byte(hash, 0);
}

void hash_int(std::uint64_t& hash, int value)
{
    const std::uint32_t bits = static_cast<std::uint32_t>(value);
    for (unsigned shift = 0; shift < 32; shift += 8) {
        hash_byte(hash, static_cast<unsigned char>((bits >> shift) & 0xff));
    }
}

std::uint64_t metadata_hash(const CMachineInfo* info)
{
    std::uint64_t hash = UINT64_C(1469598103934665603);
    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        hash_string(hash, p ? p->Name : nullptr);
        hash_string(hash, p ? p->Description : nullptr);
        hash_int(hash, p ? p->MinValue : 0);
        hash_int(hash, p ? p->MaxValue : 0);
        hash_int(hash, p ? p->Flags : 0);
        hash_int(hash, p ? p->DefValue : 0);
    }
    return hash;
}

int verify_metadata(const Spec& spec, const CMachineInfo* info)
{
    if (!info) return fail(spec, "GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != spec.version ||
            info->Flags != psycle::plugin_interface::GENERATOR ||
            info->numParameters != spec.parameters ||
            info->numCols != spec.columns || !info->Parameters) {
        return fail(spec, "ABI/version/type/parameter geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, spec.name) != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, spec.short_name) != 0 ||
            !info->Author || std::strcmp(info->Author, "jme") != 0) {
        return fail(spec, "identity metadata changed");
    }
    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        if (!p || !p->Name || !p->Description || p->MinValue > p->MaxValue ||
                p->DefValue < p->MinValue || p->DefValue > p->MaxValue) {
            std::fprintf(stderr,
                "phase5-jme-family: FAIL [%s]: malformed parameter metadata at %d\n",
                spec.label, i);
            return 1;
        }
    }
    const std::uint64_t actual_hash = metadata_hash(info);
    std::printf("phase5-jme-family: metadata OBSERVE [%s] parameters=%d version=0x%04x hash=0x%016llx\n",
        spec.label, info->numParameters, info->PlugVersion,
        static_cast<unsigned long long>(actual_hash));
    if (spec.metadata_hash != 0 && actual_hash != spec.metadata_hash) {
        return fail(spec, "complete parameter metadata hash changed");
    }
    return 0;
}

void apply_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    for (int i = 0; i < info->numParameters; ++i) {
        machine->Vals[i] = info->Parameters[i]->DefValue;
    }
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }
}

std::vector<float> render_note(CMachineInterface* machine, int samples,
    int command = 0, int value = 0)
{
    std::vector<float> signal;
    signal.reserve(static_cast<std::size_t>(samples));
    machine->SeqTick(0, 48, 0, command, value);
    int remaining = samples;
    while (remaining > 0) {
        const int block = remaining > psycle::plugin_interface::MAX_BUFFER_LENGTH
            ? psycle::plugin_interface::MAX_BUFFER_LENGTH : remaining;
        std::vector<float> left(static_cast<std::size_t>(block), 0.0f);
        std::vector<float> right(static_cast<std::size_t>(block), 0.0f);
        machine->Work(left.data(), right.data(), block, 1);
        for (int i = 0; i < block; ++i) {
            if (!std::isfinite(left[static_cast<std::size_t>(i)]) ||
                    !std::isfinite(right[static_cast<std::size_t>(i)])) {
                return {};
            }
            /* These retained generators are allowed to use stereo synthesis;
            ** freeze the left stream for deterministic comparisons. */
            signal.push_back(left[static_cast<std::size_t>(i)]);
        }
        remaining -= block;
    }
    return signal;
}

double rms(const std::vector<float>& signal)
{
    double sum = 0.0;
    for (float sample : signal) {
        const double v = static_cast<double>(sample);
        sum += v * v;
    }
    return signal.empty() ? 0.0 : std::sqrt(sum / signal.size());
}

bool same_signal(const std::vector<float>& a, const std::vector<float>& b,
    double tolerance = 1.0e-4)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::fabs(static_cast<double>(a[i] - b[i])) > tolerance) return false;
    }
    return true;
}

struct LoadedPlugin {
    void* library = nullptr;
    const CMachineInfo* info = nullptr;
    CMachineInterface* (*create)() = nullptr;
    void (*destroy)(CMachineInterface&) = nullptr;
};

int load_plugin(const Spec& spec, const char* path, LoadedPlugin& out)
{
    using GetInfoFn = const CMachineInfo* (*)();
    out.library = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!out.library) return fail(spec, dlerror());
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(out.library, "GetInfo"));
    out.create = reinterpret_cast<CMachineInterface* (*)()>(dlsym(out.library, "CreateMachine"));
    out.destroy = reinterpret_cast<void (*)(CMachineInterface&)>(dlsym(out.library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !out.create || !out.destroy) {
        return fail(spec, error ? error : "native ABI export missing");
    }
    out.info = get_info();
    return verify_metadata(spec, out.info);
}

CMachineInterface* make_machine(const Spec& spec, const LoadedPlugin& plugin,
    TestCallback& callback)
{
    /* Both Blitz and GameFX contain optional random waveforms.  Seed the
    ** process PRNG before construction so two fresh comparison instances get
    ** identical historical wavetable contents without changing production. */
    std::srand(0x4a4d45);
    CMachineInterface* machine = plugin.create();
    if (!machine || !machine->Vals) {
        fail(spec, "CreateMachine returned an unusable instance");
        return nullptr;
    }
    machine->pCB = &callback;
    apply_defaults(machine, plugin.info);
    return machine;
}

int verify_default_render(const Spec& spec, const LoadedPlugin& plugin)
{
    TestCallback callback_a(44100);
    TestCallback callback_b(44100);
    CMachineInterface* a = make_machine(spec, plugin, callback_a);
    CMachineInterface* b = make_machine(spec, plugin, callback_b);
    if (!a || !b) {
        if (a) plugin.destroy(*a);
        if (b) plugin.destroy(*b);
        return 1;
    }
    const std::vector<float> first = render_note(a, 2048);
    const std::vector<float> second = render_note(b, 2048);
    const double level = rms(first);
    const bool ok = !first.empty() && !second.empty() && level > 0.01 &&
        same_signal(first, second);
    plugin.destroy(*a);
    plugin.destroy(*b);
    if (!ok) return fail(spec, "fresh default instances are not deterministic and active");
    std::printf("phase5-jme-family: deterministic PASS [%s] rms=%.6f\n",
        spec.label, level);
    return 0;
}

int verify_volume_command(const Spec& spec, const LoadedPlugin& plugin)
{
    TestCallback callback_full(44100);
    TestCallback callback_half(44100);
    CMachineInterface* full = make_machine(spec, plugin, callback_full);
    CMachineInterface* half = make_machine(spec, plugin, callback_half);
    if (!full || !half) {
        if (full) plugin.destroy(*full);
        if (half) plugin.destroy(*half);
        return 1;
    }
    const std::vector<float> full_signal = render_note(full, 1024);
    const std::vector<float> half_signal = render_note(half, 1024, 0x0C, 128);
    const double full_rms = rms(full_signal);
    const double half_rms = rms(half_signal);
    plugin.destroy(*full);
    plugin.destroy(*half);
    if (full_signal.empty() || half_signal.empty() || full_rms <= 0.01 ||
            half_rms <= 0.0) {
        return fail(spec, "0Cxx volume-command render failed");
    }
    const double ratio = half_rms / full_rms;
    /* Historical JME code scales 0Cxx on a 0..255 tracker byte. */
    if (ratio < 0.47 || ratio > 0.54) {
        std::fprintf(stderr,
            "phase5-jme-family: FAIL [%s]: 0C80 RMS ratio %.6f outside retained half-volume envelope\n",
            spec.label, ratio);
        return 1;
    }
    std::printf("phase5-jme-family: volume-command PASS [%s] command=0C80 ratio=%.6f\n",
        spec.label, ratio);
    return 0;
}

int verify_target_rate_render(const Spec& spec, const LoadedPlugin& plugin)
{
    TestCallback callback(88200);
    CMachineInterface* machine = make_machine(spec, plugin, callback);
    if (!machine) return 1;
    machine->SequencerTick();
    const std::vector<float> signal = render_note(machine, 2048);
    const double level = rms(signal);
    plugin.destroy(*machine);
    if (signal.empty() || level <= 0.01) {
        return fail(spec, "88.2 kHz target-rate render is silent or non-finite");
    }
    std::printf("phase5-jme-family: target-rate PASS [%s] sr=88200 rms=%.6f\n",
        spec.label, level);
    return 0;
}

int exercise_plugin(const Spec& spec, const char* path)
{
    LoadedPlugin plugin;
    int rc = load_plugin(spec, path, plugin);
    if (rc == 0) rc = verify_default_render(spec, plugin);
    if (rc == 0) rc = verify_volume_command(spec, plugin);
    if (rc == 0) rc = verify_target_rate_render(spec, plugin);
    if (plugin.library && dlclose(plugin.library) != 0 && rc == 0) {
        rc = fail(spec, "dlclose failed");
    }
    return rc;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5) {
        std::fprintf(stderr,
            "usage: %s BLITZ12_SO BLITZN_SO GAMEFX13_SO GAMEFXN_SO\n", argv[0]);
        return 2;
    }
    for (std::size_t i = 0; i < sizeof(SPECS) / sizeof(SPECS[0]); ++i) {
        if (exercise_plugin(SPECS[i], argv[i + 1]) != 0) return 1;
    }
    std::printf("phase5-jme-family: PASS\n");
    std::printf("machines: Blitz 1.2.1 + Blitz 1.6 + GameFX 1.3.1 + GameFX 1.6\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: deterministic default note + 0Cxx volume + 88.2 kHz target-rate render\n");
    return 0;
}
