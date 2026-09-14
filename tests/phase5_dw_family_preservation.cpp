/*
** PSYCLE-LINUX Phase 5C D. W. Aley family native ABI / DSP preservation.
**
** Covers the four retained source-built DW machines: dw eq, dw granulizer,
** dw IoPan and dw Tremolo. Metadata hashes are emitted by the real native ABI
** and are frozen after the first dedicated observation run.
*/

#include <algorithm>
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

enum class Kind { Eq, Granulizer, IoPan, Tremolo };

struct Spec {
    const char* label;
    const char* name;
    const char* short_name;
    int version;
    int parameters;
    int columns;
    Kind kind;
    std::uint64_t metadata_hash;
};

const Spec SPECS[] = {
    {"dw eq", "dw eq", "eq", 0x0100, 12, 3, Kind::Eq, UINT64_C(0)},
    {"dw granulizer", "dw granulizer", "granulizer", 0x0100, 50, 5, Kind::Granulizer, UINT64_C(0)},
    {"dw IoPan", "dw IoPan", "IoPan", 0x0001, 4, 2, Kind::IoPan, UINT64_C(0)},
    {"dw Tremolo", "dw Tremolo", "Tremolo", 0x0002, 8, 2, Kind::Tremolo, UINT64_C(0)},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate = 44100)
        : sample_rate_(sample_rate), bpm_(120), tpb_(4) {}
    void set_sample_rate(int value) { sample_rate_ = value; }
    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sample_rate_ / 8; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return bpm_; }
    int GetTPB() const override { return tpb_; }
    bool FileBox(bool, char[], char[]) override { return false; }
private:
    int sample_rate_;
    int bpm_;
    int tpb_;
};

int fail(const Spec& spec, const char* message)
{
    std::fprintf(stderr, "phase5-dw-family: FAIL [%s]: %s\n", spec.label, message);
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
        return fail(spec, "native API version changed");
    if (info->PlugVersion != spec.version)
        return fail(spec, "plugin version changed");
    if (info->Flags != psycle::plugin_interface::EFFECT)
        return fail(spec, "machine is no longer classified as an effect");
    if (info->numParameters != spec.parameters || !info->Parameters)
        return fail(spec, "parameter table/count changed");
    if (!info->Name || std::strcmp(info->Name, spec.name) != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, spec.short_name) != 0 ||
            !info->Author || std::strcmp(info->Author, "dw") != 0 ||
            info->numCols != spec.columns)
        return fail(spec, "historical identity metadata changed");

    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* p = info->Parameters[i];
        if (!p || !p->Name || !p->Description)
            return fail(spec, "parameter metadata contains a null entry");
        if (p->MinValue > p->MaxValue)
            return fail(spec, "parameter range is inverted");
        if (p->Flags == psycle::plugin_interface::MPF_STATE &&
                (p->DefValue < p->MinValue || p->DefValue > p->MaxValue))
            return fail(spec, "state parameter default lies outside its range");
    }

    const std::uint64_t actual = parameter_hash(info);
    std::printf("dw-metadata-hash[%s]=0x%016llx\n", spec.label,
        static_cast<unsigned long long>(actual));
    if (spec.metadata_hash != 0 && actual != spec.metadata_hash)
        return fail(spec, "frozen parameter metadata hash changed");
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

bool same_signal(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r,
    float tolerance = 1.0e-5f)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size()) return false;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        if (std::fabs(a_l[i] - b_l[i]) > tolerance ||
                std::fabs(a_r[i] - b_r[i]) > tolerance)
            return false;
    }
    return true;
}

double rms_difference(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size() || a_l.empty())
        return INFINITY;
    long double sum = 0.0;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        const long double dl = static_cast<long double>(a_l[i]) - b_l[i];
        const long double dr = static_cast<long double>(a_r[i]) - b_r[i];
        sum += dl * dl + dr * dr;
    }
    return std::sqrt(static_cast<double>(sum / (2.0L * a_l.size())));
}

int verify_eq(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback dry_cb(44100);
    CMachineInterface* dry = create_machine();
    if (!dry || !dry->Vals) {
        if (dry) delete_machine(*dry);
        return fail(spec, "CreateMachine failed for default EQ probe");
    }
    apply_defaults(dry, info, dry_cb);
    std::vector<float> dry_l = {-1000.0f, -3.0f, 0.0f, 7.5f, 900.0f};
    std::vector<float> dry_r = {500.0f, 2.0f, 0.0f, -8.5f, -700.0f};
    const std::vector<float> ref_l = dry_l;
    const std::vector<float> ref_r = dry_r;
    process_blocks(dry, dry_l, dry_r);
    int rc = 0;
    if (!finite_signal(dry_l, dry_r) || !same_signal(dry_l, dry_r, ref_l, ref_r, 1.0e-3f))
        rc = fail(spec, "default unity response changed");
    delete_machine(*dry);
    if (rc != 0) return rc;

    TestCallback live_cb(44100), stale_cb(44100), target_cb(88200);
    CMachineInterface* live = create_machine();
    CMachineInterface* stale = create_machine();
    CMachineInterface* target = create_machine();
    if (!live || !live->Vals || !stale || !stale->Vals || !target || !target->Vals) {
        if (target) delete_machine(*target);
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for EQ rate probe");
    }
    apply_defaults(live, info, live_cb);
    apply_defaults(stale, info, stale_cb);
    apply_defaults(target, info, target_cb);
    for (CMachineInterface* machine : {live, stale, target}) {
        machine->ParameterTweak(0, 32768);
        machine->ParameterTweak(4, 65535);
    }
    live_cb.set_sample_rate(88200);
    live->SequencerTick();

    std::vector<float> live_l(256, 0.0f), live_r(256, 0.0f);
    std::vector<float> stale_l(256, 0.0f), stale_r(256, 0.0f);
    std::vector<float> target_l(256, 0.0f), target_r(256, 0.0f);
    live_l[0] = stale_l[0] = target_l[0] = 1000.0f;
    live_r[0] = stale_r[0] = target_r[0] = -500.0f;
    process_blocks(live, live_l, live_r);
    process_blocks(stale, stale_l, stale_r);
    process_blocks(target, target_l, target_r);
    const double target_distance = rms_difference(live_l, live_r, target_l, target_r);
    const double stale_distance = rms_difference(live_l, live_r, stale_l, stale_r);
    if (!finite_signal(live_l, live_r) || !finite_signal(target_l, target_r) ||
            target_distance > 1.0e-3 || stale_distance < 1.0e-2)
        rc = fail(spec, "live 88.2 kHz coefficient update no longer matches fresh target response");
    else
        std::printf("phase5-dw-family: EQ rate PASS target=%.8f stale=%.8f\n",
            target_distance, stale_distance);
    delete_machine(*target);
    delete_machine(*stale);
    delete_machine(*live);
    return rc;
}

int verify_iopan(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback cb(44100);
    CMachineInterface* machine = create_machine();
    if (!machine || !machine->Vals) {
        if (machine) delete_machine(*machine);
        return fail(spec, "CreateMachine failed");
    }
    apply_defaults(machine, info, cb);
    std::vector<float> left = {-2.0f, 0.5f, 3.0f};
    std::vector<float> right = {1.5f, -0.75f, -4.0f};
    const std::vector<float> ref_l = left;
    const std::vector<float> ref_r = right;
    process_blocks(machine, left, right);
    int rc = 0;
    if (!same_signal(left, right, ref_l, ref_r, 0.0f)) {
        rc = fail(spec, "default full-width passthrough changed");
    } else {
        machine->ParameterTweak(0, 128);
        machine->ParameterTweak(2, 0);
        left = {2.0f};
        right = {-3.0f};
        process_blocks(machine, left, right);
        if (left[0] != -3.0f || right[0] != 2.0f)
            rc = fail(spec, "historical full channel flip matrix changed");
    }
    if (rc == 0)
        std::printf("phase5-dw-family: IoPan PASS default=unity flip=full-swap\n");
    delete_machine(*machine);
    return rc;
}

void configure_tremolo(CMachineInterface* machine)
{
    machine->ParameterTweak(0, 1000); // depth 100%
    machine->ParameterTweak(1, 1);    // triangle
    machine->ParameterTweak(2, 100);  // neutral gravity
    machine->ParameterTweak(3, 360);  // symmetric skew
    machine->ParameterTweak(4, 5000); // visible rate
    machine->ParameterTweak(5, 0);    // aligned stereo phase basis
    machine->ParameterTweak(6, 0);
    machine->ParameterTweak(7, 1);    // restart LFO
}

int verify_tremolo(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback unity_cb(44100);
    CMachineInterface* unity = create_machine();
    if (!unity || !unity->Vals) {
        if (unity) delete_machine(*unity);
        return fail(spec, "CreateMachine failed for unity probe");
    }
    apply_defaults(unity, info, unity_cb);
    unity->ParameterTweak(0, 0);
    std::vector<float> unity_l = {-2.0f, 0.5f, 3.0f};
    std::vector<float> unity_r = {1.5f, -0.75f, -4.0f};
    const std::vector<float> ref_l = unity_l;
    const std::vector<float> ref_r = unity_r;
    process_blocks(unity, unity_l, unity_r);
    int rc = 0;
    if (!same_signal(unity_l, unity_r, ref_l, ref_r, 0.0f))
        rc = fail(spec, "Depth=0 unity response changed");
    delete_machine(*unity);
    if (rc != 0) return rc;

    TestCallback live_cb(44100), stale_cb(44100), target_cb(88200);
    CMachineInterface* live = create_machine();
    CMachineInterface* stale = create_machine();
    CMachineInterface* target = create_machine();
    if (!live || !live->Vals || !stale || !stale->Vals || !target || !target->Vals) {
        if (target) delete_machine(*target);
        if (stale) delete_machine(*stale);
        if (live) delete_machine(*live);
        return fail(spec, "CreateMachine failed for Tremolo rate probe");
    }
    apply_defaults(live, info, live_cb);
    apply_defaults(stale, info, stale_cb);
    apply_defaults(target, info, target_cb);
    configure_tremolo(live);
    configure_tremolo(stale);
    configure_tremolo(target);

    std::vector<float> live_pre_l(441, 1.0f), live_pre_r(441, 1.0f);
    std::vector<float> stale_pre_l(441, 1.0f), stale_pre_r(441, 1.0f);
    std::vector<float> target_pre_l(882, 1.0f), target_pre_r(882, 1.0f);
    process_blocks(live, live_pre_l, live_pre_r);
    process_blocks(stale, stale_pre_l, stale_pre_r);
    process_blocks(target, target_pre_l, target_pre_r);
    live_cb.set_sample_rate(88200);
    live->SequencerTick();

    std::vector<float> live_l(512, 1.0f), live_r(512, 1.0f);
    std::vector<float> stale_l(512, 1.0f), stale_r(512, 1.0f);
    std::vector<float> target_l(512, 1.0f), target_r(512, 1.0f);
    process_blocks(live, live_l, live_r);
    process_blocks(stale, stale_l, stale_r);
    process_blocks(target, target_l, target_r);
    const double target_distance = rms_difference(live_l, live_r, target_l, target_r);
    const double stale_distance = rms_difference(live_l, live_r, stale_l, stale_r);
    if (!finite_signal(live_l, live_r) || target_distance > 2.0e-4 || stale_distance < 1.0e-3)
        rc = fail(spec, "live LFO sample-rate scaling diverged from fresh 88.2 kHz wall-clock phase");
    else
        std::printf("phase5-dw-family: Tremolo rate PASS target=%.8f stale=%.8f\n",
            target_distance, stale_distance);
    delete_machine(*target);
    delete_machine(*stale);
    delete_machine(*live);
    return rc;
}

void configure_granulizer_probe(CMachineInterface* machine)
{
    machine->ParameterTweak(1, 10);     // 10-sample grain at 44.1 kHz
    machine->ParameterTweak(2, 1000);   // no second grain in short probe
    machine->ParameterTweak(3, 0);      // no attack envelope
    machine->ParameterTweak(4, 0);      // no decay envelope
    machine->ParameterTweak(5, 1000);   // unity pitch
    machine->ParameterTweak(6, 1000);
    machine->ParameterTweak(7, 0);      // pitch link off
    machine->ParameterTweak(8, 1);      // one layer
    machine->ParameterTweak(41, 32768); // limiter effectively open for probe
    machine->ParameterTweak(42, 100);   // unity integer gain
}

int verify_granulizer(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    int rc = 0;
    for (int sample_rate : {44100, 88200}) {
        TestCallback cb(sample_rate);
        CMachineInterface* machine = create_machine();
        if (!machine || !machine->Vals) {
            if (machine) delete_machine(*machine);
            return fail(spec, "CreateMachine failed");
        }
        apply_defaults(machine, info, cb);
        configure_granulizer_probe(machine);
        std::vector<float> left(25, 1000.0f), right(25, -1000.0f);
        process_blocks(machine, left, right);
        const int expected_active = sample_rate == 44100 ? 10 : 20;
        for (int i = 0; i < 25; ++i) {
            const bool active = i < expected_active;
            if (active) {
                if (left[static_cast<std::size_t>(i)] != 1000.0f ||
                        right[static_cast<std::size_t>(i)] != -1000.0f) {
                    rc = fail(spec, "fixed-grain unity capture/output changed");
                    break;
                }
            } else if (left[static_cast<std::size_t>(i)] != 0.0f ||
                    right[static_cast<std::size_t>(i)] != 0.0f) {
                rc = fail(spec, "grain duration/sample-rate scaling changed");
                break;
            }
        }
        delete_machine(*machine);
        if (rc != 0) return rc;
    }
    std::printf("phase5-dw-family: Granulizer PASS fixed-grain=10@44.1k/20@88.2k random-mod=off\n");
    return 0;
}

int verify_kind(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    switch (spec.kind) {
    case Kind::Eq: return verify_eq(spec, info, create_machine, delete_machine);
    case Kind::Granulizer: return verify_granulizer(spec, info, create_machine, delete_machine);
    case Kind::IoPan: return verify_iopan(spec, info, create_machine, delete_machine);
    case Kind::Tremolo: return verify_tremolo(spec, info, create_machine, delete_machine);
    }
    return fail(spec, "unknown test kind");
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    const int spec_count = static_cast<int>(sizeof(SPECS) / sizeof(SPECS[0]));
    if (argc != spec_count + 1) {
        std::fprintf(stderr,
            "usage: %s DW_EQ_SO DW_GRANULIZER_SO DW_IOPAN_SO DW_TREMOLO_SO\n", argv[0]);
        return 2;
    }

    for (int i = 0; i < spec_count; ++i) {
        const Spec& spec = SPECS[i];
        void* library = dlopen(argv[i + 1], RTLD_LAZY | RTLD_LOCAL);
        if (!library) {
            std::fprintf(stderr, "phase5-dw-family: FAIL [%s]: dlopen: %s\n",
                spec.label, dlerror());
            return 1;
        }
        dlerror();
        GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
        CreateMachineFn create_machine =
            reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
        DeleteMachineFn delete_machine =
            reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
        const char* error = dlerror();
        if (error || !get_info || !create_machine || !delete_machine) {
            std::fprintf(stderr,
                "phase5-dw-family: FAIL [%s]: native ABI exports missing: %s\n",
                spec.label, error ? error : "unknown symbol error");
            dlclose(library);
            return 1;
        }

        const CMachineInfo* info = get_info();
        int rc = verify_metadata(spec, info);
        if (rc == 0) rc = verify_kind(spec, info, create_machine, delete_machine);
        if (dlclose(library) != 0 && rc == 0)
            rc = fail(spec, "dlclose failed");
        if (rc != 0) return rc;
        std::printf("phase5-dw-family: machine PASS [%s]\n", spec.label);
    }

    std::printf("phase5-dw-family: PASS machines=4\n");
    return 0;
}
