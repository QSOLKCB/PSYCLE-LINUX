/*
** PSYCLE-LINUX Phase 5B Pooplog family native ABI / DSP preservation.
**
** Covers the nine retained source-built Pooplog Linux binaries. Parameter
** metadata is frozen with complete hashes, effects use deterministic neutral
** signal invariants, and all three FM synth variants exercise both fresh-machine
** determinism and a live 44.1 -> 88.2 kHz SequencerTick transition.
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

enum class Kind { Synth, Delay, Filter, Autopan, Lofi, Scratch };

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

const Spec SPECS[] = {
    {"FM Laboratory", "Pooplog FM Laboratory0.68b", "Pooplog", 0x0068,
        psycle::plugin_interface::GENERATOR, 101, 5, Kind::Synth,
        UINT64_C(0xd2db9e08a2e098a4)},
    {"FM Light", "Pooplog FM Light0.68b", "Pooplog Light", 0x0068,
        psycle::plugin_interface::GENERATOR, 57, 5, Kind::Synth,
        UINT64_C(0xc955c42530dc6506)},
    {"FM UltraLight", "Pooplog FM UltraLight0.68b", "Pooplog UltraL", 0x0068,
        psycle::plugin_interface::GENERATOR, 45, 5, Kind::Synth,
        UINT64_C(0xf9faacf032cc58f3)},
    {"Delay", "Pooplog Delay 0.04b", "Pooplog Delay", 0x0004,
        psycle::plugin_interface::EFFECT, 43, 5, Kind::Delay,
        UINT64_C(0xd794adcf1c57390e)},
    {"Delay Light", "Pooplog Delay Light 0.04b", "Pooplog Delay L", 0x0004,
        psycle::plugin_interface::EFFECT, 28, 4, Kind::Delay,
        UINT64_C(0xdd879bae9f506e07)},
    {"Filter", "Pooplog Filter 0.06b", "Pooplog Filter", 0x0006,
        psycle::plugin_interface::EFFECT, 16, 4, Kind::Filter,
        UINT64_C(0xec1dd1dbd487381c)},
    {"Autopan", "Pooplog Autopan 0.06b", "Pooplog Autopan", 0x0006,
        psycle::plugin_interface::EFFECT, 9, 3, Kind::Autopan,
        UINT64_C(0xc100243d6fa938a5)},
    {"Lofi", "Pooplog Lofi Processor 0.04b", "Pooplog Lofi", 0x0004,
        psycle::plugin_interface::EFFECT, 4, 4, Kind::Lofi,
        UINT64_C(0x87c4d37392e2128a)},
    {"Scratch", "Pooplog Scratch Master 0.06b", "Pooplog Scratch", 0x0006,
        psycle::plugin_interface::EFFECT, 8, 4, Kind::Scratch,
        UINT64_C(0xd4eb1c2f76e2e4c9)},
};

class TestCallback : public CFxCallback {
public:
    TestCallback() : sample_rate_(44100), bpm_(120), tick_length_(5512) {}
    void set_sample_rate(int v) { sample_rate_ = v; }
    void set_bpm(int v) { bpm_ = v; }
    void set_tick_length(int v) { tick_length_ = v; }
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
    for (unsigned int i = 0; i < 4; ++i)
        hash_byte(hash, static_cast<unsigned char>((u >> (i * 8)) & 0xffu));
}

void hash_string(std::uint64_t& hash, const char* text)
{
    if (text) while (*text) hash_byte(hash, static_cast<unsigned char>(*text++));
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

    const std::uint64_t actual = parameter_hash(info);
    if (actual != spec.metadata_hash) {
        std::fprintf(stderr,
            "phase5-pooplog-family: FAIL [%s]: metadata hash expected 0x%016llx got 0x%016llx\n",
            spec.label,
            static_cast<unsigned long long>(spec.metadata_hash),
            static_cast<unsigned long long>(actual));
        return 1;
    }
    std::printf("pooplog-metadata-hash[%s]=0x%016llx\n", spec.label,
        static_cast<unsigned long long>(actual));
    return 0;
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
        std::fprintf(stderr, "phase5-pooplog-family: FAIL [%s]: parameter '%s' missing\n",
            spec.label, name);
        return 1;
    }
    const CMachineParameter* p = info->Parameters[index];
    if (value < p->MinValue || value > p->MaxValue)
        return fail(spec, "test attempted an out-of-range parameter value");
    machine->ParameterTweak(index, value);
    return 0;
}

bool same_signal(const float* a, const float* b, int count,
    float tolerance = 1.0e-5f)
{
    for (int i = 0; i < count; ++i) {
        if (!std::isfinite(a[i]) || !std::isfinite(b[i])) return false;
        if (std::fabs(a[i] - b[i]) > tolerance) return false;
    }
    return true;
}

int positive_zero_crossings(const std::vector<float>& signal)
{
    int crossings = 0;
    for (std::size_t i = 1; i < signal.size(); ++i) {
        if (signal[i - 1] <= 0.0f && signal[i] > 0.0f) ++crossings;
    }
    return crossings;
}

int configure_effect_neutral(const Spec& spec, CMachineInterface* machine,
    const CMachineInfo* info)
{
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
        return 0;
    case Kind::Autopan:
        return set_parameter(spec, machine, info, "Panning", 256) ||
            set_parameter(spec, machine, info, "Pan LFO Depth", 0) ||
            set_parameter(spec, machine, info, "Tweak Inertia", 0) ||
            set_parameter(spec, machine, info, "Input Gain", 256) ||
            set_parameter(spec, machine, info, "Mix", 256);
    case Kind::Lofi:
        return set_parameter(spec, machine, info, "Resample Frequency", 0) ||
            set_parameter(spec, machine, info, "Resample Bits", 0) ||
            set_parameter(spec, machine, info, "Frequency Unbalance", 256) ||
            set_parameter(spec, machine, info, "Input Gain", 256);
    case Kind::Synth:
        return fail(spec, "internal effect dispatch error");
    }
    return 1;
}

int verify_effect_unity(const Spec& spec, CMachineInterface* machine,
    const CMachineInfo* info)
{
    const float input_l[] = {-1234.5f, -64.0f, 0.0f, 77.25f, 2048.0f, 0.5f};
    const float input_r[] = {4321.0f, 17.0f, 0.0f, -99.5f, -1024.0f, -0.25f};
    float left[6];
    float right[6];
    TestCallback callback;

    apply_defaults(machine, info, callback);
    if (configure_effect_neutral(spec, machine, info) != 0) return 1;
    std::memcpy(left, input_l, sizeof(left));
    std::memcpy(right, input_r, sizeof(right));
    machine->Work(left, right, 6, 1);
    if (!same_signal(left, input_l, 6) || !same_signal(right, input_r, 6))
        return fail(spec, "neutral DSP path is not finite exact unity");

    callback.set_sample_rate(88200);
    callback.set_bpm(137);
    callback.set_tick_length(8044);
    machine->SequencerTick();
    std::memcpy(left, input_l, sizeof(left));
    std::memcpy(right, input_r, sizeof(right));
    machine->Work(left, right, 6, 1);
    if (!same_signal(left, input_l, 6) || !same_signal(right, input_r, 6))
        return fail(spec, "neutral DSP changed after live host-timing transition");
    return 0;
}

int render_active_note(const Spec& spec, CMachineInterface* machine,
    std::vector<float>& left, std::vector<float>& right)
{
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
    return nonzero ? 0 : fail(spec, "generator note render remained silent");
}

int verify_synth(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    std::vector<float> left_a, right_a, left_b, right_b, left_live, right_live;
    TestCallback callback_a;
    TestCallback callback_b;

    CMachineInterface* a = create_machine();
    CMachineInterface* b = create_machine();
    if (!a || !a->Vals || !b || !b->Vals) {
        if (a) delete_machine(*a);
        if (b) delete_machine(*b);
        return fail(spec, "CreateMachine failed for deterministic synth render");
    }
    apply_defaults(a, info, callback_a);
    apply_defaults(b, info, callback_b);

    float silent_l[64] = {};
    float silent_r[64] = {};
    a->Work(silent_l, silent_r, 64, 1);
    for (int i = 0; i < 64; ++i) {
        if (silent_l[i] != 0.0f || silent_r[i] != 0.0f) {
            delete_machine(*b); delete_machine(*a);
            return fail(spec, "generator emitted audio without an active note");
        }
    }

    int rc = render_active_note(spec, a, left_a, right_a);
    if (rc == 0) rc = render_active_note(spec, b, left_b, right_b);
    if (rc == 0 && (!same_signal(left_a.data(), left_b.data(), 1024) ||
            !same_signal(right_a.data(), right_b.data(), 1024)))
        rc = fail(spec, "fresh-machine note rendering is no longer deterministic");
    delete_machine(*b);
    if (rc != 0) { delete_machine(*a); return rc; }

    /* Exercise the actual live sample-rate transition on the already initialized
    ** and already rendered instance. The same musical note should retain roughly
    ** the same physical frequency after the sample rate doubles: over an equal
    ** sample count the 88.2 kHz render therefore has about half as many cycles.
    ** A no-op SequencerTick leaves the old phase increment in place and makes the
    ** physical frequency estimate roughly double, which this oracle rejects. */
    a->Stop();
    callback_a.set_sample_rate(88200);
    callback_a.set_tick_length(11025);
    callback_a.set_bpm(137);
    a->SequencerTick();
    rc = render_active_note(spec, a, left_live, right_live);
    if (rc == 0) {
        const int crossings_44 = positive_zero_crossings(left_a);
        const int crossings_88 = positive_zero_crossings(left_live);
        if (crossings_44 < 4 || crossings_88 < 2) {
            rc = fail(spec, "insufficient zero crossings for sample-rate pitch oracle");
        } else {
            const double hz_44 = crossings_44 * 44100.0 / left_a.size();
            const double hz_88 = crossings_88 * 88200.0 / left_live.size();
            if (hz_88 < hz_44 * 0.75 || hz_88 > hz_44 * 1.25) {
                std::fprintf(stderr,
                    "phase5-pooplog-family: FAIL [%s]: live sample-rate pitch estimate "
                    "changed from %.2f Hz to %.2f Hz (%d -> %d crossings)\n",
                    spec.label, hz_44, hz_88, crossings_44, crossings_88);
                rc = 1;
            } else {
                std::printf(
                    "phase5-pooplog-family: live pitch %.2f->%.2f Hz PASS [%s]\n",
                    hz_44, hz_88, spec.label);
            }
        }
    }
    delete_machine(*a);
    if (rc != 0) return rc;

    std::printf("phase5-pooplog-family: live 44.1->88.2 kHz PASS [%s]\n", spec.label);
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
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-pooplog-family: FAIL [%s]: ABI exports missing: %s\n",
            spec.label, error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(spec, info);
    if (rc == 0) {
        if (spec.kind == Kind::Synth) {
            rc = verify_synth(spec, info, create_machine, delete_machine);
        } else {
            CMachineInterface* machine = create_machine();
            if (!machine || !machine->Vals)
                rc = fail(spec, "CreateMachine did not provide a usable effect instance");
            else {
                rc = verify_effect_unity(spec, machine, info);
                delete_machine(*machine);
            }
        }
    }
    if (dlclose(library) != 0 && rc == 0) rc = fail(spec, "dlclose failed");
    if (rc == 0) std::printf("phase5-pooplog-family: PASS [%s]\n", spec.label);
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
    for (int i = 0; i < count; ++i)
        if (test_plugin(SPECS[i], argv[i + 1]) != 0) return 1;
    std::printf("phase5-pooplog-family: PASS all %d retained source-built targets\n", count);
    return 0;
}
