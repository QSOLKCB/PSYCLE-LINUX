/*
** PSYCLE-LINUX Phase 5C Druttis family native ABI / DSP preservation.
**
** Covers all seven retained source-built Druttis Linux machines. The first CI
** observation emits complete parameter-table hashes; those hashes are frozen
** before the roadmap item is marked complete.
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

enum class Kind { Effect, Generator, Plucked, Slicit };

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
    {"EQ-3", "EQ-3", "EQ-3", 0x0110, psycle::plugin_interface::EFFECT, 12, 4, Kind::Effect, UINT64_C(0)},
    {"FeedMe", "FeedMe 1.2", "FeedMe", 0x0120, psycle::plugin_interface::GENERATOR, 24, 5, Kind::Generator, UINT64_C(0)},
    {"Koruz", "Koruz", "Koruz", 0x0110, psycle::plugin_interface::EFFECT, 14, 2, Kind::Effect, UINT64_C(0)},
    {"Phantom", "Phantom 1.2", "Phantom", 0x0120, psycle::plugin_interface::GENERATOR, 55, 5, Kind::Generator, UINT64_C(0)},
    {"Plucked String", "Plucked String 1.2", "Plucked String", 0x0120, psycle::plugin_interface::GENERATOR, 7, 1, Kind::Plucked, UINT64_C(0)},
    {"Slicit", "Slicit", "Slicit", 0x0100, psycle::plugin_interface::EFFECT, 68, 4, Kind::Slicit, UINT64_C(0)},
    {"Sublime", "Sublime 1.1", "Sublime", 0x0110, psycle::plugin_interface::GENERATOR, 60, 4, Kind::Generator, UINT64_C(0)},
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
    std::fprintf(stderr, "phase5-druttis-family: FAIL [%s]: %s\n", spec.label, message);
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
    if (text) {
        while (*text) hash_byte(hash, static_cast<unsigned char>(*text++));
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

bool author_is_druttis(const char* author)
{
    static const char prefix[] = "Druttis on ";
    return author && std::strncmp(author, prefix, sizeof(prefix) - 1) == 0;
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
    if (!info->Name || std::strcmp(info->Name, spec.name) != 0 || !info->ShortName ||
            std::strcmp(info->ShortName, spec.short_name) != 0 || !author_is_druttis(info->Author) ||
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
    }

    const std::uint64_t actual = parameter_hash(info);
    std::printf("druttis-metadata-hash[%s]=0x%016llx\n", spec.label,
        static_cast<unsigned long long>(actual));
    if (spec.metadata_hash != 0 && actual != spec.metadata_hash)
        return fail(spec, "frozen parameter metadata hash changed");
    return 0;
}

void apply_defaults(CMachineInterface* machine, const CMachineInfo* info, TestCallback& callback)
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

bool finite_signal(const std::vector<float>& left, const std::vector<float>& right)
{
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i])) return false;
    }
    return true;
}

bool nonzero_signal(const std::vector<float>& left, const std::vector<float>& right)
{
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (std::fabs(left[i]) > 1.0e-8f || std::fabs(right[i]) > 1.0e-8f) return true;
    }
    return false;
}

bool same_signal(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r,
    float tolerance = 1.0e-5f)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size() ||
            !finite_signal(a_l, a_r) || !finite_signal(b_l, b_r))
        return false;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        if (std::fabs(a_l[i] - b_l[i]) > tolerance ||
                std::fabs(a_r[i] - b_r[i]) > tolerance)
            return false;
    }
    return true;
}

bool different_signal(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r,
    float tolerance = 1.0e-4f)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size() ||
            !finite_signal(a_l, a_r) || !finite_signal(b_l, b_r))
        return false;
    bool different = false;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        if (std::fabs(a_l[i] - b_l[i]) > tolerance ||
                std::fabs(a_r[i] - b_r[i]) > tolerance)
            different = true;
    }
    return different;
}

void make_probe(std::vector<float>& left, std::vector<float>& right, int count)
{
    left.resize(count);
    right.resize(count);
    for (int i = 0; i < count; ++i) {
        left[i] = static_cast<float>(((i * 37) % 257) - 128) * 8.0f;
        right[i] = static_cast<float>(((i * 53) % 251) - 125) * 7.0f;
    }
}

void configure_eq3_active(CMachineInterface* machine)
{
    machine->ParameterTweak(0, 180);
    machine->ParameterTweak(1, 180);
    machine->ParameterTweak(3, 96);
    machine->ParameterTweak(4, 96);
    machine->ParameterTweak(6, 160);
    machine->ParameterTweak(7, 160);
}

int verify_effect(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback_a;
    TestCallback callback_b;
    CMachineInterface* a = create_machine();
    CMachineInterface* b = create_machine();
    std::vector<float> input_l, input_r, a_l, a_r, b_l, b_r;
    int rc = 0;

    if (!a || !a->Vals || !b || !b->Vals) {
        if (a) delete_machine(*a);
        if (b) delete_machine(*b);
        return fail(spec, "CreateMachine failed for effect regression");
    }
    apply_defaults(a, info, callback_a);
    apply_defaults(b, info, callback_b);
    make_probe(input_l, input_r, 2048);
    a_l = input_l;
    a_r = input_r;
    b_l = input_l;
    b_r = input_r;
    a->Work(a_l.data(), a_r.data(), static_cast<int>(a_l.size()), 1);
    b->Work(b_l.data(), b_r.data(), static_cast<int>(b_l.size()), 1);
    if (!finite_signal(a_l, a_r) || !finite_signal(b_l, b_r))
        rc = fail(spec, "effect produced a non-finite sample");
    else if (!same_signal(a_l, a_r, b_l, b_r, 1.0e-5f))
        rc = fail(spec, "fresh effect instances are not deterministic");
    else
        std::printf("phase5-druttis-family: deterministic effect PASS [%s]\n", spec.label);
    delete_machine(*b);
    delete_machine(*a);
    if (rc != 0 || spec.kind != Kind::Effect) return rc;

    TestCallback live_cb;
    TestCallback stale_cb;
    CMachineInterface* live = create_machine();
    CMachineInterface* stale = (std::strcmp(spec.label, "EQ-3") == 0) ? create_machine() : nullptr;
    if (!live || !live->Vals ||
            (std::strcmp(spec.label, "EQ-3") == 0 && (!stale || !stale->Vals))) {
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for live effect transition");
    }
    apply_defaults(live, info, live_cb);
    if (stale) apply_defaults(stale, info, stale_cb);
    if (stale) {
        configure_eq3_active(live);
        configure_eq3_active(stale);
    }
    live_cb.set_sample_rate(88200);
    live->SequencerTick();
    make_probe(input_l, input_r, 2048);
    a_l = input_l;
    a_r = input_r;
    live->Work(a_l.data(), a_r.data(), static_cast<int>(a_l.size()), 1);
    if (!finite_signal(a_l, a_r)) {
        if (stale) delete_machine(*stale);
        delete_machine(*live);
        return fail(spec, "live sample-rate transition produced non-finite audio");
    }

    if (stale) {
        b_l = input_l;
        b_r = input_r;
        stale->Work(b_l.data(), b_r.data(), static_cast<int>(b_l.size()), 1);
        if (!different_signal(a_l, a_r, b_l, b_r, 1.0e-4f))
            rc = fail(spec, "EQ-3 active response did not change after sample-rate transition");
        else
            std::printf("phase5-druttis-family: live rate response changed PASS [%s]\n", spec.label);
        delete_machine(*stale);
        delete_machine(*live);
        return rc;
    }

    TestCallback ref_cb;
    ref_cb.set_sample_rate(88200);
    CMachineInterface* ref = create_machine();
    if (!ref || !ref->Vals) {
        if (ref) delete_machine(*ref);
        delete_machine(*live);
        return fail(spec, "CreateMachine failed for target-rate reference");
    }
    apply_defaults(ref, info, ref_cb);
    b_l = input_l;
    b_r = input_r;
    ref->Work(b_l.data(), b_r.data(), static_cast<int>(b_l.size()), 1);
    if (!same_signal(a_l, a_r, b_l, b_r, 1.0e-4f))
        rc = fail(spec, "live sample-rate transition diverged from fresh target rate");
    else
        std::printf("phase5-druttis-family: live 44.1->88.2 effect transition PASS [%s]\n", spec.label);
    delete_machine(*ref);
    delete_machine(*live);
    return rc;
}

int render_note(const Spec& spec, CMachineInterface* machine, int count,
    std::vector<float>& left, std::vector<float>& right)
{
    machine->SeqTick(0, 69, 0, 0, 0);
    left.assign(count, 0.0f);
    right.assign(count, 0.0f);
    machine->Work(left.data(), right.data(), count, 1);
    if (!finite_signal(left, right)) return fail(spec, "generator produced non-finite audio");
    if (!nonzero_signal(left, right)) return fail(spec, "generator note render remained silent");
    return 0;
}

int verify_generator(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback_a;
    TestCallback callback_b;
    CMachineInterface* a = create_machine();
    CMachineInterface* b = create_machine();
    std::vector<float> a_l, a_r, b_l, b_r;
    int rc = 0;

    if (!a || !a->Vals || !b || !b->Vals) {
        if (a) delete_machine(*a);
        if (b) delete_machine(*b);
        return fail(spec, "CreateMachine failed for generator regression");
    }
    apply_defaults(a, info, callback_a);
    apply_defaults(b, info, callback_b);
    rc = render_note(spec, a, 8192, a_l, a_r);
    if (rc == 0) rc = render_note(spec, b, 8192, b_l, b_r);
    if (rc == 0 && spec.kind != Kind::Plucked &&
            !same_signal(a_l, a_r, b_l, b_r, 1.0e-5f))
        rc = fail(spec, "fresh generator instances are not deterministic");
    if (rc == 0)
        std::printf("phase5-druttis-family: active generator PASS [%s]\n", spec.label);
    delete_machine(*b);
    if (rc != 0) {
        delete_machine(*a);
        return rc;
    }

    a->Stop();
    callback_a.set_sample_rate(88200);
    a->SequencerTick();
    rc = render_note(spec, a, 16384, a_l, a_r);
    if (rc == 0)
        std::printf("phase5-druttis-family: live 44.1->88.2 generator transition PASS [%s]\n",
            spec.label);
    delete_machine(*a);
    return rc;
}

int test_plugin(const Spec& spec, const char* path)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-druttis-family: FAIL [%s]: dlopen: %s\n",
            spec.label, dlerror());
        return 1;
    }
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-druttis-family: FAIL [%s]: ABI exports missing: %s\n",
            spec.label, error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(spec, info);
    if (rc == 0) {
        if (spec.flags == psycle::plugin_interface::GENERATOR)
            rc = verify_generator(spec, info, create_machine, delete_machine);
        else
            rc = verify_effect(spec, info, create_machine, delete_machine);
    }
    if (dlclose(library) != 0 && rc == 0) rc = fail(spec, "dlclose failed");
    if (rc == 0) std::printf("phase5-druttis-family: PASS [%s]\n", spec.label);
    return rc;
}

} // namespace

int main(int argc, char** argv)
{
    const int count = static_cast<int>(sizeof(SPECS) / sizeof(SPECS[0]));
    if (argc != count + 1) {
        std::fprintf(stderr,
            "usage: %s EQ3.so FEEDME.so KORUZ.so PHANTOM.so PLUCKED.so SLICIT.so SUBLIME.so\n",
            argv[0]);
        return 2;
    }
    for (int i = 0; i < count; ++i) {
        if (test_plugin(SPECS[i], argv[i + 1]) != 0) return 1;
    }
    std::printf("phase5-druttis-family: PASS all %d retained source-built targets\n", count);
    return 0;
}
