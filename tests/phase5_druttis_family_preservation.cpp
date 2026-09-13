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
using psycle::plugin_interface::MAX_BUFFER_LENGTH;

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
    {"EQ-3", "EQ-3", "EQ-3", 0x0110, psycle::plugin_interface::EFFECT, 12, 4, Kind::Effect, UINT64_C(0x1d7768bf2bf65d17)},
    {"FeedMe", "FeedMe 1.2", "FeedMe", 0x0120, psycle::plugin_interface::GENERATOR, 24, 5, Kind::Generator, UINT64_C(0x665df6f57aa34c4b)},
    {"Koruz", "Koruz", "Koruz", 0x0110, psycle::plugin_interface::EFFECT, 14, 2, Kind::Effect, UINT64_C(0x8c1bbabd87f42796)},
    {"Phantom", "Phantom 1.2", "Phantom", 0x0120, psycle::plugin_interface::GENERATOR, 55, 5, Kind::Generator, UINT64_C(0xbefdde29123bea07)},
    {"Plucked String", "Plucked String 1.2", "Plucked String", 0x0120, psycle::plugin_interface::GENERATOR, 7, 1, Kind::Plucked, UINT64_C(0x3be7f541e23e0f3f)},
    {"Slicit", "Slicit", "Slicit", 0x0100, psycle::plugin_interface::EFFECT, 68, 4, Kind::Slicit, UINT64_C(0x689adbb03c0fd4d3)},
    {"Sublime", "Sublime 1.1", "Sublime", 0x0110, psycle::plugin_interface::GENERATOR, 60, 4, Kind::Generator, UINT64_C(0xbaa335eeecaf0b09)},
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
    if (actual != spec.metadata_hash)
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

double rms_difference(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size() ||
            !finite_signal(a_l, a_r) || !finite_signal(b_l, b_r) || a_l.empty())
        return INFINITY;
    long double sum = 0.0;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        const long double dl = static_cast<long double>(a_l[i]) - b_l[i];
        const long double dr = static_cast<long double>(a_r[i]) - b_r[i];
        sum += dl * dl + dr * dr;
    }
    return std::sqrt(static_cast<double>(sum / (2.0L * a_l.size())));
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

void render_effect_constant(CMachineInterface* machine, int count,
    std::vector<float>& left, std::vector<float>& right)
{
    left.assign(count, 1.0f);
    right.assign(count, 1.0f);
    int offset = 0;
    while (offset < count) {
        const int remaining = count - offset;
        const int block = remaining < MAX_BUFFER_LENGTH ? remaining : MAX_BUFFER_LENGTH;
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
        offset += block;
    }
}

void configure_slicit_timing(CMachineInterface* machine)
{
    /* Program 0: two steps. Step 1 is unity/centered and step 2 is silent,
    ** so the second step boundary is directly visible in the rendered signal. */
    machine->ParameterTweak(17, 2);   // No. Steps
    machine->ParameterTweak(34, 0);   // Speed Factor x1
    machine->ParameterTweak(1, 256);  // Level 1
    machine->ParameterTweak(2, 0);    // Level 2
    machine->ParameterTweak(18, 0);   // Attack 1: 5 ms minimum
    machine->ParameterTweak(19, 0);   // Attack 2: 5 ms minimum
    machine->ParameterTweak(35, 128); // Pan 1 centered
    machine->ParameterTweak(36, 128); // Pan 2 centered
    machine->ParameterTweak(51, 0);   // Filter off
    machine->Stop();
}

int verify_slicit_timing_transition(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback live_cb;
    TestCallback stale_cb;
    TestCallback target_cb;
    target_cb.set_sample_rate(88200);
    target_cb.set_tick_length(11025);

    CMachineInterface* live = create_machine();
    CMachineInterface* stale = create_machine();
    CMachineInterface* target = create_machine();
    std::vector<float> live_l, live_r, stale_l, stale_r, target_l, target_r;
    int rc = 0;

    if (!live || !live->Vals || !stale || !stale->Vals || !target || !target->Vals) {
        if (target) delete_machine(*target);
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for Slicit timing transition");
    }

    apply_defaults(live, info, live_cb);
    apply_defaults(stale, info, stale_cb);
    apply_defaults(target, info, target_cb);
    configure_slicit_timing(live);
    configure_slicit_timing(stale);
    configure_slicit_timing(target);

    live_cb.set_sample_rate(88200);
    live_cb.set_tick_length(11025);
    live->SequencerTick();
    stale->SequencerTick();
    target->SequencerTick();

    render_effect_constant(live, 14000, live_l, live_r);
    render_effect_constant(stale, 14000, stale_l, stale_r);
    render_effect_constant(target, 14000, target_l, target_r);

    if (!finite_signal(live_l, live_r) || !finite_signal(stale_l, stale_r) ||
            !finite_signal(target_l, target_r)) {
        rc = fail(spec, "Slicit timing probe produced non-finite audio");
    } else if (!same_signal(live_l, live_r, target_l, target_r, 1.0e-6f)) {
        rc = fail(spec, "Slicit live rate/tick transition diverged from fresh target timing");
    } else if (!different_signal(live_l, live_r, stale_l, stale_r, 1.0e-4f)) {
        rc = fail(spec, "Slicit live rate/tick transition remained equivalent to stale timing");
    } else {
        std::printf("phase5-druttis-family: live rate/tick transition PASS [Slicit]\n");
    }

    delete_machine(*target);
    delete_machine(*stale);
    delete_machine(*live);
    return rc;
}

int verify_koruz_rate_transition(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback live_cb;
    TestCallback stale_cb;
    TestCallback target_cb;
    CMachineInterface* live = create_machine();
    CMachineInterface* stale = create_machine();
    CMachineInterface* target = create_machine();
    std::vector<float> input_l, input_r, live_l, live_r, stale_l, stale_r, target_l, target_r;
    int rc = 0;

    if (!live || !live->Vals || !stale || !stale->Vals || !target || !target->Vals) {
        if (target) delete_machine(*target);
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for Koruz rate transition");
    }

    apply_defaults(live, info, live_cb);
    apply_defaults(stale, info, stale_cb);
    target_cb.set_sample_rate(88200);
    target_cb.set_tick_length(11025);
    apply_defaults(target, info, target_cb);
    live_cb.set_sample_rate(88200);
    live_cb.set_tick_length(11025);
    live->SequencerTick();

    make_probe(input_l, input_r, 8192);
    live_l = input_l;
    live_r = input_r;
    stale_l = input_l;
    stale_r = input_r;
    target_l = input_l;
    target_r = input_r;
    live->Work(live_l.data(), live_r.data(), static_cast<int>(live_l.size()), 1);
    stale->Work(stale_l.data(), stale_r.data(), static_cast<int>(stale_l.size()), 1);
    target->Work(target_l.data(), target_r.data(), static_cast<int>(target_l.size()), 1);

    if (!finite_signal(live_l, live_r) || !finite_signal(stale_l, stale_r) ||
            !finite_signal(target_l, target_r)) {
        rc = fail(spec, "Koruz rate probe produced non-finite audio");
    } else {
        const double target_distance = rms_difference(live_l, live_r, target_l, target_r);
        const double stale_distance = rms_difference(live_l, live_r, stale_l, stale_r);
        std::printf("phase5-druttis-family: Koruz rate distances target=%.6f stale=%.6f\n",
            target_distance, stale_distance);
        if (!std::isfinite(target_distance) || !std::isfinite(stale_distance) ||
                stale_distance <= 1.0 || !(target_distance < stale_distance * 0.01))
            rc = fail(spec, "Koruz live rate response is not decisively closer to fresh 88.2 kHz than stale 44.1 kHz");
        else
            std::printf("phase5-druttis-family: live stochastic rate transition PASS [Koruz]\n");
    }

    delete_machine(*target);
    delete_machine(*stale);
    delete_machine(*live);
    return rc;
}

int verify_effect(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    const bool is_koruz = std::strcmp(spec.label, "Koruz") == 0;
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
    if (!finite_signal(a_l, a_r) || !finite_signal(b_l, b_r)) {
        rc = fail(spec, "effect produced a non-finite sample");
    } else if (is_koruz) {
        if (!different_signal(a_l, a_r, input_l, input_r) ||
                !different_signal(b_l, b_r, input_l, input_r))
            rc = fail(spec, "Koruz stochastic effect path did not engage");
        else
            std::printf("phase5-druttis-family: stochastic finite effect PASS [Koruz]\n");
    } else if (!same_signal(a_l, a_r, b_l, b_r, 1.0e-5f)) {
        rc = fail(spec, "fresh effect instances are not deterministic");
    } else {
        std::printf("phase5-druttis-family: deterministic effect PASS [%s]\n", spec.label);
    }
    delete_machine(*b);
    delete_machine(*a);
    if (rc != 0) return rc;

    if (spec.kind == Kind::Slicit)
        return verify_slicit_timing_transition(spec, info, create_machine, delete_machine);
    if (spec.kind != Kind::Effect) return rc;
    if (is_koruz)
        return verify_koruz_rate_transition(spec, info, create_machine, delete_machine);

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
    live_cb.set_tick_length(11025);
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
    ref_cb.set_tick_length(11025);
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
    /* Sublime's Voice::NoteOn consumes m_globals.m_ticklength, which the
    ** retained machine initializes only from SequencerTick(). Ensure every
    ** fresh/stale/target reference has defined host timing before SeqTick(). */
    if (std::strcmp(spec.label, "Sublime") == 0)
        machine->SequencerTick();
    machine->SeqTick(0, 69, 0, 0, 0);
    left.assign(count, 0.0f);
    right.assign(count, 0.0f);

    /* The native Psycle ABI guarantees Work() blocks no larger than
    ** MAX_BUFFER_LENGTH (256 samples per channel). Sublime and other retained
    ** machines keep fixed-size scratch buffers at exactly that size, so long
    ** observations must be rendered as a sequence of host-faithful blocks. */
    int offset = 0;
    while (offset < count) {
        const int remaining = count - offset;
        const int block = remaining < MAX_BUFFER_LENGTH ? remaining : MAX_BUFFER_LENGTH;
        machine->Work(left.data() + offset, right.data() + offset, block, 1);
        offset += block;
    }

    if (!finite_signal(left, right)) return fail(spec, "generator produced non-finite audio");
    if (!nonzero_signal(left, right)) return fail(spec, "generator note render remained silent");
    return 0;
}

bool stochastic_generator(const Spec& spec)
{
    return std::strcmp(spec.label, "Phantom") == 0;
}

int verify_sublime_rate_transition(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    std::vector<float> live_l, live_r, stale_l, stale_r, target_l, target_r;
    int rc = 0;

    /* Sublime owns global band-limited wavetable storage. UpdateWaveforms(sr)
    ** may reallocate that storage, while live Voice/Globals objects retain
    ** pointers into it. Psycle changes the engine rate globally, so it never
    ** keeps 44.1- and 88.2-kHz Sublime instances alive simultaneously. Record
    ** the three observations sequentially to preserve that real lifecycle. */
    {
        TestCallback stale_cb;
        CMachineInterface* stale = create_machine();
        if (!stale || !stale->Vals) {
            if (stale) delete_machine(*stale);
            return fail(spec, "CreateMachine failed for sequential stale Sublime reference");
        }
        apply_defaults(stale, info, stale_cb);
        rc = render_note(spec, stale, 16384, stale_l, stale_r);
        delete_machine(*stale);
        if (rc != 0) return rc;
    }

    {
        TestCallback target_cb;
        target_cb.set_sample_rate(88200);
        target_cb.set_tick_length(11025);
        CMachineInterface* target = create_machine();
        if (!target || !target->Vals) {
            if (target) delete_machine(*target);
            return fail(spec, "CreateMachine failed for sequential target Sublime reference");
        }
        apply_defaults(target, info, target_cb);
        rc = render_note(spec, target, 16384, target_l, target_r);
        delete_machine(*target);
        if (rc != 0) return rc;
    }

    {
        TestCallback live_cb;
        CMachineInterface* live = create_machine();
        if (!live || !live->Vals) {
            if (live) delete_machine(*live);
            return fail(spec, "CreateMachine failed for sequential live Sublime transition");
        }
        apply_defaults(live, info, live_cb);
        /* Initialize retained tick-dependent voice timing at the source rate
        ** before exercising the live host-rate transition. */
        live->SequencerTick();
        live_cb.set_sample_rate(88200);
        live_cb.set_tick_length(11025);
        live->SequencerTick();
        rc = render_note(spec, live, 16384, live_l, live_r);
        delete_machine(*live);
        if (rc != 0) return rc;
    }

    const double target_distance = rms_difference(live_l, live_r, target_l, target_r);
    const double stale_distance = rms_difference(live_l, live_r, stale_l, stale_r);
    std::printf("phase5-druttis-family: generator rate distances [Sublime] target=%.6f stale=%.6f\n",
        target_distance, stale_distance);
    if (!std::isfinite(target_distance) || !std::isfinite(stale_distance) ||
            stale_distance <= 1.0e-6 || !(target_distance < stale_distance))
        return fail(spec, "Sublime live rate response is not closer to fresh 88.2 kHz than stale 44.1 kHz");

    std::printf("phase5-druttis-family: live rate-sensitive generator PASS [Sublime]\n");
    return 0;
}

int verify_generator_rate_transition(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    if (std::strcmp(spec.label, "Sublime") == 0)
        return verify_sublime_rate_transition(spec, info, create_machine, delete_machine);

    TestCallback live_cb;
    TestCallback stale_cb;
    TestCallback target_cb;
    CMachineInterface* live = create_machine();
    CMachineInterface* stale = create_machine();
    CMachineInterface* target = create_machine();
    std::vector<float> live_l, live_r, stale_l, stale_r, target_l, target_r;
    int rc = 0;

    if (!live || !live->Vals || !stale || !stale->Vals || !target || !target->Vals) {
        if (target) delete_machine(*target);
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for generator rate transition");
    }

    apply_defaults(live, info, live_cb);
    apply_defaults(stale, info, stale_cb);
    target_cb.set_sample_rate(88200);
    target_cb.set_tick_length(11025);
    apply_defaults(target, info, target_cb);
    live_cb.set_sample_rate(88200);
    live_cb.set_tick_length(11025);
    live->SequencerTick();

    if ((rc = render_note(spec, live, 16384, live_l, live_r)) == 0 &&
            (rc = render_note(spec, stale, 16384, stale_l, stale_r)) == 0 &&
            (rc = render_note(spec, target, 16384, target_l, target_r)) == 0) {
        const double target_distance = rms_difference(live_l, live_r, target_l, target_r);
        const double stale_distance = rms_difference(live_l, live_r, stale_l, stale_r);
        std::printf("phase5-druttis-family: generator rate distances [%s] target=%.6f stale=%.6f\n",
            spec.label, target_distance, stale_distance);
        if (!std::isfinite(target_distance) || !std::isfinite(stale_distance) ||
                stale_distance <= 1.0e-6 || !(target_distance < stale_distance))
            rc = fail(spec, "live generator rate response is not closer to fresh 88.2 kHz than stale 44.1 kHz");
        else
            std::printf("phase5-druttis-family: live rate-sensitive generator PASS [%s]\n", spec.label);
    }

    delete_machine(*target);
    delete_machine(*stale);
    delete_machine(*live);
    return rc;
}

int verify_generator(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback_a;
    TestCallback callback_b;
    CMachineInterface* a = create_machine();
    CMachineInterface* b = nullptr;
    std::vector<float> a_l, a_r, b_l, b_r;
    int rc = 0;

    if (!a || !a->Vals) {
        if (a) delete_machine(*a);
        return fail(spec, "CreateMachine failed for first generator regression instance");
    }

    /* Several retained Druttis generators initialize shared static wavetable
    ** state only when the first live instance is initialized. Preserve that
    ** host lifecycle: initialize/render A before constructing B. */
    apply_defaults(a, info, callback_a);
    rc = render_note(spec, a, 8192, a_l, a_r);
    if (rc != 0) {
        delete_machine(*a);
        return rc;
    }

    b = create_machine();
    if (!b || !b->Vals) {
        if (b) delete_machine(*b);
        delete_machine(*a);
        return fail(spec, "CreateMachine failed for second generator regression instance");
    }
    apply_defaults(b, info, callback_b);
    rc = render_note(spec, b, 8192, b_l, b_r);
    if (rc == 0 && !stochastic_generator(spec) &&
            !same_signal(a_l, a_r, b_l, b_r, 1.0e-5f))
        rc = fail(spec, "fresh generator instances are not deterministic");
    if (rc == 0 && stochastic_generator(spec))
        std::printf("phase5-druttis-family: stochastic active generator PASS [%s]\n", spec.label);
    else if (rc == 0)
        std::printf("phase5-druttis-family: deterministic active generator PASS [%s]\n", spec.label);
    delete_machine(*b);
    delete_machine(*a);
    if (rc != 0) return rc;

    return verify_generator_rate_transition(spec, info, create_machine, delete_machine);
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
