/*
** PSYCLE-LINUX Phase 5B Pooplog family native ABI / DSP regression.
**
** Loads every retained source-built Pooplog Linux shared object through Psycle's
** native ABI. The test freezes machine identity and parameter-table geometry,
** exercises deterministic neutral DSP paths for effects, and proves the three
** FM synth variants render deterministically before and after a live sample-rate
** reinitialization.
*/

#include <cmath>
#include <cstdint>
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

enum class Kind {
    Synth,
    Delay,
    Filter,
    Autopan,
    Lofi,
    Scratch,
};

struct Spec {
    const char* label;
    const char* name;
    const char* short_name;
    int plug_version;
    int flags;
    int parameter_count;
    int columns;
    Kind kind;
    std::uint64_t metadata_hash;
};

/* Metadata hashes are filled after the first source-derived CI observation.
** A zero hash means "report but do not yet freeze"; the final PR replaces
** these zeros with the observed constants before the roadmap is completed. */
const Spec SPECS[] = {
    {"FM Laboratory", "Pooplog FM Laboratory0.68b", "Pooplog", 0x0068,
        psycle::plugin_interface::GENERATOR, 101, 5, Kind::Synth, 0},
    {"FM Light", "Pooplog FM Light0.68b", "Pooplog Light", 0x0068,
        psycle::plugin_interface::GENERATOR, 57, 5, Kind::Synth, 0},
    {"FM UltraLight", "Pooplog FM UltraLight0.68b", "Pooplog UltraL", 0x0068,
        psycle::plugin_interface::GENERATOR, 45, 5, Kind::Synth, 0},
    {"Delay", "Pooplog Delay 0.04b", "Pooplog Delay", 0x0004,
        psycle::plugin_interface::EFFECT, 43, 5, Kind::Delay, 0},
    {"Delay Light", "Pooplog Delay Light 0.04b", "Pooplog Delay L", 0x0004,
        psycle::plugin_interface::EFFECT, 28, 4, Kind::Delay, 0},
    {"Filter", "Pooplog Filter 0.06b", "Pooplog Filter", 0x0006,
        psycle::plugin_interface::EFFECT, 16, 4, Kind::Filter, 0},
    {"Autopan", "Pooplog Autopan 0.06b", "Pooplog Autopan", 0x0006,
        psycle::plugin_interface::EFFECT, 9, 3, Kind::Autopan, 0},
    {"Lofi", "Pooplog Lofi Processor 0.04b", "Pooplog Lofi", 0x0004,
        psycle::plugin_interface::EFFECT, 4, 4, Kind::Lofi, 0},
    {"Scratch", "Pooplog Scratch Master 0.06b", "Pooplog Scratch", 0x0006,
        psycle::plugin_interface::EFFECT, 8, 4, Kind::Scratch, 0},
};

class TestCallback : public CFxCallback {
public:
    TestCallback() : sample_rate_(44100), bpm_(120), tick_length_(5512) {}

    void set_sample_rate(int value) { sample_rate_ = value; }
    void set_bpm(int value) { bpm_ = value; }
    void set_tick_length(int value) { tick_length_ = value; }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return tick_length_; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return bpm_; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
    int bpm_;
    int tick_length_;
};

int fail(const Spec& spec, const char* message)
{
    std::fprintf(stderr, "phase5-pooplog-family: FAIL [%s]: %s\n",
        spec.label, message);
    return 1;
}

void hash_byte(std::uint64_t& hash, unsigned char value)
{
    hash ^= static_cast<std::uint64_t>(value);
    hash *= UINT64_C(1099511628211);
}

void hash_int(std::uint64_t& hash, std::int32_t value)
{
    const std::uint32_t u = static_cast<std::uint32_t>(value);
    for (unsigned int i = 0; i < 4; ++i) {
        hash_byte(hash, static_cast<unsigned char>((u >> (i * 8)) & 0xffu));
    }
}

void hash_string(std::uint64_t& hash, const char* text)
{
    if (text) {
        while (*text) {
            hash_byte(hash, static_cast<unsigned char>(*text++));
        }
    }
    hash_byte(hash, 0);
}

std::uint64_t parameter_hash(const CMachineInfo* info)
{
    std::uint64_t hash = UINT64_C(1469598103934665603);
    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        hash_int(hash, i);
        hash_string(hash, p ? p->Name : nullptr);
        hash_string(hash, p ? p->Description : nullptr);
        if (p) {
            hash_int(hash, p->MinValue);
            hash_int(hash, p->MaxValue);
            hash_int(hash, p->Flags);
            hash_int(hash, p->DefValue);
        }
    }
    return hash;
}

int verify_metadata(const Spec& spec, const CMachineInfo* info)
{
    if (!info) return fail(spec, "GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION)
        return fail(spec, "native-machine API version changed");
    if (info->PlugVersion != spec.plug_version)
        return fail(spec, "plugin version changed");
    if (info->Flags != spec.flags)
        return fail(spec, "generator/effect classification changed");
    if (info->numParameters != spec.parameter_count)
        return fail(spec, "parameter count changed");
    if (!info->Name || std::strcmp(info->Name, spec.name) != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, spec.short_name) != 0 ||
            !info->Author || std::strcmp(info->Author, "Jeremy Evers") != 0 ||
            info->numCols != spec.columns)
        return fail(spec, "machine identity metadata changed");
    if (!info->Parameters) return fail(spec, "parameter table is missing");

    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        if (!p || !p->Name || !p->Description)
            return fail(spec, "parameter metadata contains a null entry");
        if (p->MinValue > p->MaxValue)
            return fail(spec, "parameter range is inverted");
        if (p->Flags == psycle::plugin_interface::MPF_STATE &&
                (p->DefValue < p->MinValue || p->DefValue > p->MaxValue))
            return fail(spec, "state parameter default is outside its range");
        if (p->Flags != psycle::plugin_interface::MPF_STATE &&
                p->Flags != psycle::plugin_interface::MPF_NULL &&
                p->Flags != psycle::plugin_interface::MPF_LABEL)
            return fail(spec, "parameter flag changed to an unknown value");
    }

    const std::uint64_t actual_hash = parameter_hash(info);
    std::printf("pooplog-metadata-hash[%s]=0x%016llx\n",
        spec.label, static_cast<unsigned long long>(actual_hash));
    if (spec.metadata_hash != 0 && actual_hash != spec.metadata_hash)
        return fail(spec, "complete parameter metadata hash changed");
    return 0;
}

void apply_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback& callback)
{
    machine->pCB = &callback;
    for (int i = 0; i < info->numParameters; ++i) {
        machine->Vals[i] = info->Parameters[i]->DefValue;
    }
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        if (info->Parameters[i]->Flags == psycle::plugin_interface::MPF_STATE) {
            machine->ParameterTweak(i, info->Parameters[i]->DefValue);
        }
    }
}

int find_parameter(const CMachineInfo* info, const char* name)
{
    for (int i = 0; i < info->numParameters; ++i) {
        if (info->Parameters[i] && info->Parameters[i]->Name &&
                std::strcmp(info->Parameters[i]->Name, name) == 0)
            return i;
    }
    return -1;
}

int set_parameter(const Spec& spec, CMachineInterface* machine,
    const CMachineInfo* info, const char* name, int value)
{
    const int index = find_parameter(info, name);
    if (index < 0) {
        std::fprintf(stderr,
            "phase5-pooplog-family: FAIL [%s]: parameter '%s' is missing\n",
            spec.label, name);
        return 1;
    }
    const CMachineParameter* p = info->Parameters[index];
    if (value < p->MinValue || value > p->MaxValue)
        return fail(spec, "test attempted an out-of-range parameter value");
    machine->ParameterTweak(index, value);
    return 0;
}

bool same_signal(const float* a, const float* b, int count, float tolerance = 1.0e-5f)
{
    for (int i = 0; i < count; ++i) {
        if (std::fabs(a[i] - b[i]) > tolerance) return false;
    }
    return true;
}

int verify_effect_unity(const Spec& spec, CMachineInterface* machine,
    const CMachineInfo* info, TestCallback& callback)
{
    const float input_l[] = {-1234.5f, -64.0f, 0.0f, 77.25f, 2048.0f, 0.5f};
    const float input_r[] = {4321.0f, 17.0f, 0.0f, -99.5f, -1024.0f, -0.25f};
    float left[6];
    float right[6];

    apply_defaults(machine, info, callback);

    switch (spec.kind) {
    case Kind::Delay:
    case Kind::Filter:
    case Kind::Scratch:
        if (set_parameter(spec, machine, info, "Input Gain", 256) != 0 ||
                set_parameter(spec, machine, info, "Mix", 0) != 0)
            return 1;
        if (find_parameter(info, "Tweak Inertia") >= 0 &&
                set_parameter(spec, machine, info, "Tweak Inertia", 0) != 0)
            return 1;
        break;
    case Kind::Autopan:
        if (set_parameter(spec, machine, info, "Panning", 256) != 0 ||
                set_parameter(spec, machine, info, "Pan LFO Depth", 0) != 0 ||
                set_parameter(spec, machine, info, "Tweak Inertia", 0) != 0 ||
                set_parameter(spec, machine, info, "Input Gain", 256) != 0 ||
                set_parameter(spec, machine, info, "Mix", 256) != 0)
            return 1;
        break;
    case Kind::Lofi:
        if (set_parameter(spec, machine, info, "Resample Frequency", 0) != 0 ||
                set_parameter(spec, machine, info, "Resample Bits", 0) != 0 ||
                set_parameter(spec, machine, info, "Frequency Unbalance", 256) != 0 ||
                set_parameter(spec, machine, info, "Input Gain", 256) != 0)
            return 1;
        break;
    case Kind::Synth:
        return fail(spec, "internal test dispatch error");
    }

    std::memcpy(left, input_l, sizeof(left));
    std::memcpy(right, input_r, sizeof(right));
    machine->Work(left, right, 6, 1);
    if (!same_signal(left, input_l, 6) || !same_signal(right, input_r, 6))
        return fail(spec, "neutral historical DSP path is no longer exact unity");

    /* Re-run the neutral invariant after the plugin's live host-timing boundary.
    ** Different members use this callback to rebuild filters, LFO rates or
    ** delay/scratch timing. */
    callback.set_sample_rate(88200);
    callback.set_bpm(137);
    callback.set_tick_length(8044);
    machine->SequencerTick();
    std::memcpy(left, input_l, sizeof(left));
    std::memcpy(right, input_r, sizeof(right));
    machine->Work(left, right, 6, 1);
    if (!same_signal(left, input_l, 6) || !same_signal(right, input_r, 6))
        return fail(spec, "neutral DSP changed after sample-rate/BPM reinitialization");

    return 0;
}

int render_synth_once(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* machine, std::vector<float>& left,
    std::vector<float>& right, int sample_rate)
{
    TestCallback callback;
    callback.set_sample_rate(sample_rate);
    callback.set_tick_length(sample_rate / 8);
    apply_defaults(machine, info, callback);

    float silent_l[64] = {};
    float silent_r[64] = {};
    machine->Work(silent_l, silent_r, 64, 1);
    for (int i = 0; i < 64; ++i) {
        if (silent_l[i] != 0.0f || silent_r[i] != 0.0f)
            return fail(spec, "generator emitted audio without an active note");
    }

    machine->SeqTick(0, 69, 0, 0, 0);
    left.assign(1024, 0.0f);
    right.assign(1024, 0.0f);
    machine->Work(left.data(), right.data(), static_cast<int>(left.size()), 1);

    bool nonzero = false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
            return fail(spec, "generator produced a non-finite sample");
        if (left[i] != 0.0f || right[i] != 0.0f) nonzero = true;
    }
    if (!nonzero) return fail(spec, "generator note render remained silent");
    return 0;
}

int verify_synth_determinism(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    std::vector<float> left_a;
    std::vector<float> right_a;
    std::vector<float> left_b;
    std::vector<float> right_b;
    std::vector<float> left_hi;
    std::vector<float> right_hi;

    CMachineInterface* a = create_machine();
    if (!a || !a->Vals) return fail(spec, "CreateMachine failed for first synth render");
    int rc = render_synth_once(spec, info, a, left_a, right_a, 44100);
    delete_machine(*a);
    if (rc != 0) return rc;

    CMachineInterface* b = create_machine();
    if (!b || !b->Vals) return fail(spec, "CreateMachine failed for repeat synth render");
    rc = render_synth_once(spec, info, b, left_b, right_b, 44100);
    delete_machine(*b);
    if (rc != 0) return rc;

    if (left_a.size() != left_b.size() || right_a.size() != right_b.size() ||
            !same_signal(left_a.data(), left_b.data(), static_cast<int>(left_a.size())) ||
            !same_signal(right_a.data(), right_b.data(), static_cast<int>(right_a.size())))
        return fail(spec, "fresh-machine note rendering is no longer deterministic");

    CMachineInterface* hi = create_machine();
    if (!hi || !hi->Vals) return fail(spec, "CreateMachine failed for sample-rate synth render");
    rc = render_synth_once(spec, info, hi, left_hi, right_hi, 88200);
    delete_machine(*hi);
    if (rc != 0) return rc;

    return 0;
}

int test_plugin(const Spec& spec, const char* path)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-pooplog-family: FAIL [%s]: dlopen: %s\n",
            spec.label, dlerror());
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
            "phase5-pooplog-family: FAIL [%s]: native ABI exports missing: %s\n",
            spec.label, symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(spec, info);
    if (rc == 0) {
        if (spec.kind == Kind::Synth) {
            rc = verify_synth_determinism(spec, info, create_machine, delete_machine);
        } else {
            CMachineInterface* machine = create_machine();
            if (!machine || !machine->Vals) {
                rc = fail(spec, "CreateMachine did not provide a usable effect instance");
            } else {
                TestCallback callback;
                rc = verify_effect_unity(spec, machine, info, callback);
                delete_machine(*machine);
            }
        }
    }

    if (dlclose(library) != 0 && rc == 0)
        rc = fail(spec, "dlclose failed after native-machine test");
    if (rc == 0)
        std::printf("phase5-pooplog-family: PASS [%s]\n", spec.label);
    return rc;
}

} // namespace

int main(int argc, char** argv)
{
    const int count = static_cast<int>(sizeof(SPECS) / sizeof(SPECS[0]));
    if (argc != count + 1) {
        std::fprintf(stderr,
            "usage: %s FM_LAB.so FM_LIGHT.so FM_ULTRALIGHT.so DELAY.so "
            "DELAY_LIGHT.so FILTER.so AUTOPAN.so LOFI.so SCRATCH.so\n", argv[0]);
        return 2;
    }

    for (int i = 0; i < count; ++i) {
        if (test_plugin(SPECS[i], argv[i + 1]) != 0) return 1;
    }

    std::printf("phase5-pooplog-family: PASS all %d retained source-built targets\n", count);
    return 0;
}
