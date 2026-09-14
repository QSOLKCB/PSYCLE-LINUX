/*
** PSYCLE-LINUX Phase 5C STK-derived native-machine preservation.
**
** Covers the three retained source-built wrappers that link against the
** supported Linux libstk-dev boundary: stk Plucked, stk Reverbs and stk Shakers.
** Metadata hashes are emitted during the first dedicated observation run and
** frozen only after the real Linux modules have been observed.
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

enum class Kind { Plucked, Reverbs, Shakers };

struct Spec {
    const char* label;
    const char* name;
    const char* short_name;
    const char* author;
    int version;
    int flags;
    int parameters;
    int columns;
    Kind kind;
    std::uint64_t metadata_hash;
};

const Spec SPECS[] = {
    {"stk Plucked", "stk Plucked", "stk Plucked",
        "Sartorius, Bohan and STK 4.2.0 developers", 0x0100,
        psycle::plugin_interface::GENERATOR, 5, 1, Kind::Plucked, UINT64_C(0)},
    {"stk Reverbs", "stk Reverbs", "stk Reverbs",
        "Sartorius and STK developers", 0x0110,
        psycle::plugin_interface::EFFECT, 4, 1, Kind::Reverbs, UINT64_C(0)},
    {"stk Shakers", "stk Shakers", "Shakers",
        "Sartorius, bohan and STK 4.5.0 developers", 0x0100,
        psycle::plugin_interface::GENERATOR, 6, 1, Kind::Shakers, UINT64_C(0)},
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
    std::fprintf(stderr, "phase5-stk-family: FAIL [%s]: %s\n", spec.label, message);
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
    if (info->PlugVersion != spec.version || info->Flags != spec.flags)
        return fail(spec, "plugin version/type changed");
    if (info->numParameters != spec.parameters || !info->Parameters)
        return fail(spec, "parameter table/count changed");
    if (!info->Name || std::strcmp(info->Name, spec.name) != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, spec.short_name) != 0 ||
            !info->Author || std::strcmp(info->Author, spec.author) != 0 ||
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
    std::printf("stk-metadata-hash[%s]=0x%016llx\n", spec.label,
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

bool same_signal(const std::vector<float>& a_l, const std::vector<float>& a_r,
    const std::vector<float>& b_l, const std::vector<float>& b_r,
    float tolerance = 0.0f)
{
    if (a_l.size() != b_l.size() || a_r.size() != b_r.size()) return false;
    if (!finite_signal(a_l, a_r) || !finite_signal(b_l, b_r)) return false;
    for (std::size_t i = 0; i < a_l.size(); ++i) {
        if (std::fabs(a_l[i] - b_l[i]) > tolerance ||
                std::fabs(a_r[i] - b_r[i]) > tolerance)
            return false;
    }
    return true;
}

double signal_energy(const std::vector<float>& left, const std::vector<float>& right)
{
    if (!finite_signal(left, right)) return INFINITY;
    long double energy = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        energy += static_cast<long double>(left[i]) * left[i];
        energy += static_cast<long double>(right[i]) * right[i];
    }
    return static_cast<double>(energy);
}

int verify_plucked(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback(44100);
    CMachineInterface* machine = create_machine();
    if (!machine || !machine->Vals) {
        if (machine) delete_machine(*machine);
        return fail(spec, "CreateMachine failed");
    }
    apply_defaults(machine, info, callback);

    std::vector<float> left(256, 0.0f), right(256, 0.0f);
    process_blocks(machine, left, right);
    int rc = 0;
    if (!silent_signal(left, right))
        rc = fail(spec, "idle generator is no longer silent");

    if (rc == 0) {
        machine->SeqTick(0, 60, 0, 0, 0);
        machine->SeqTick(0, 255, 0, 0x0c, 0);
        left.assign(256, 0.0f); right.assign(256, 0.0f);
        process_blocks(machine, left, right);
        if (!silent_signal(left, right))
            rc = fail(spec, "historical 0C00 volume mute changed");
    }

    if (rc == 0) {
        machine->SeqTick(0, 255, 0, 0x0c, 255);
        left.assign(2048, 0.0f); right.assign(2048, 0.0f);
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-8)
            rc = fail(spec, "active plucked note produced no finite signal");
    }

    if (rc == 0) {
        machine->Stop();
        left.assign(256, 0.0f); right.assign(256, 0.0f);
        process_blocks(machine, left, right);
        if (!silent_signal(left, right))
            rc = fail(spec, "Stop no longer clears the plucked generator");
    }

    if (rc == 0) {
        callback.set_sample_rate(88200);
        machine->SequencerTick();
        machine->SeqTick(0, 60, 0, 0x0c, 255);
        left.assign(2048, 0.0f); right.assign(2048, 0.0f);
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-8)
            rc = fail(spec, "live 88.2 kHz plucked note became silent/non-finite");
        callback.set_sample_rate(44100);
        machine->SequencerTick();
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Plucked PASS idle=zero 0C00=zero Stop=zero live-rate=finite\n");
    delete_machine(*machine);
    return rc;
}

int verify_reverbs(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback(44100);
    CMachineInterface* machine = create_machine();
    if (!machine || !machine->Vals) {
        if (machine) delete_machine(*machine);
        return fail(spec, "CreateMachine failed");
    }
    apply_defaults(machine, info, callback);
    int rc = 0;

    machine->ParameterTweak(2, 0);
    std::vector<float> left = {-3.0f, 0.5f, 7.0f};
    std::vector<float> right = {4.0f, -2.0f, 1.25f};
    const std::vector<float> ref_l = left;
    const std::vector<float> ref_r = right;
    process_blocks(machine, left, right);
    if (!same_signal(left, right, ref_l, ref_r))
        rc = fail(spec, "Dry/Wet=0 exact bypass changed");

    for (int algorithm = 0; rc == 0 && algorithm < 3; ++algorithm) {
        machine->ParameterTweak(0, algorithm);
        machine->ParameterTweak(1, 80);
        machine->ParameterTweak(2, 100);
        machine->ParameterTweak(3, 0);
        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        left[0] = 1.0f;
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-12)
            rc = fail(spec, "selected reverb produced no finite impulse response");
        else if (!silent_signal(right, right, 1.0e-7f))
            rc = fail(spec, "independent reverb routing leaked into silent right input");
    }

    if (rc == 0) {
        machine->ParameterTweak(0, 1);
        machine->ParameterTweak(1, 80);
        machine->ParameterTweak(2, 100);
        machine->ParameterTweak(3, 1);
        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        left[0] = 1.0f;
        process_blocks(machine, left, right);
        long double right_energy = 0.0;
        for (float value : right) right_energy += static_cast<long double>(value) * value;
        if (!finite_signal(left, right) || !(right_energy > 1.0e-12L))
            rc = fail(spec, "mixed-channel reverb no longer crossfeeds the opposite channel");
    }

    if (rc == 0) {
        callback.set_sample_rate(88200);
        machine->SequencerTick();
        machine->ParameterTweak(0, 1);
        machine->ParameterTweak(2, 100);
        machine->ParameterTweak(3, 0);
        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        left[0] = 1.0f;
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-12)
            rc = fail(spec, "live 88.2 kHz reverb became silent/non-finite");
        callback.set_sample_rate(44100);
        machine->SequencerTick();
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Reverbs PASS dry=unity algorithms=3 routing=independent+mixed live-rate=finite\n");
    delete_machine(*machine);
    return rc;
}

int verify_shakers(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    TestCallback callback(44100);
    CMachineInterface* machine = create_machine();
    if (!machine || !machine->Vals) {
        if (machine) delete_machine(*machine);
        return fail(spec, "CreateMachine failed");
    }
    apply_defaults(machine, info, callback);
    int rc = 0;

    std::vector<float> left(256, 0.0f), right(256, 0.0f);
    process_blocks(machine, left, right);
    if (!silent_signal(left, right))
        rc = fail(spec, "idle shaker generator is no longer silent");

    if (rc == 0) {
        machine->SeqTick(0, 48, 0, 0, 0);
        machine->SeqTick(0, 255, 0, 0x0c, 0);
        left.assign(256, 0.0f); right.assign(256, 0.0f);
        process_blocks(machine, left, right);
        if (!silent_signal(left, right))
            rc = fail(spec, "historical shaker 0C00 mute changed");
    }

    if (rc == 0) {
        machine->SeqTick(0, 255, 0, 0x0c, 255);
        left.assign(4096, 0.0f); right.assign(4096, 0.0f);
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-8)
            rc = fail(spec, "mapped shaker note produced no finite signal");
    }

    if (rc == 0) {
        machine->Stop();
        left.assign(256, 0.0f); right.assign(256, 0.0f);
        process_blocks(machine, left, right);
        if (!silent_signal(left, right))
            rc = fail(spec, "Stop no longer silences shaker tracks");
    }

    if (rc == 0) {
        machine->SeqTick(0, 71, 0, 0, 0);
        left.assign(256, 0.0f); right.assign(256, 0.0f);
        process_blocks(machine, left, right);
        if (!silent_signal(left, right))
            rc = fail(spec, "note above historical shaker mapping range became active");
    }

    if (rc == 0) {
        callback.set_sample_rate(88200);
        machine->SequencerTick();
        machine->SeqTick(0, 48, 0, 0x0c, 255);
        left.assign(4096, 0.0f); right.assign(4096, 0.0f);
        process_blocks(machine, left, right);
        const double energy = signal_energy(left, right);
        if (!std::isfinite(energy) || energy <= 1.0e-8)
            rc = fail(spec, "live 88.2 kHz shaker became silent/non-finite");
        callback.set_sample_rate(44100);
        machine->SequencerTick();
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Shakers PASS map=48..70 0C00=zero Stop=zero live-rate=finite\n");
    delete_machine(*machine);
    return rc;
}

int verify_kind(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    switch (spec.kind) {
    case Kind::Plucked: return verify_plucked(spec, info, create_machine, delete_machine);
    case Kind::Reverbs: return verify_reverbs(spec, info, create_machine, delete_machine);
    case Kind::Shakers: return verify_shakers(spec, info, create_machine, delete_machine);
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
            "usage: %s STK_PLUCKED_SO STK_REVERBS_SO STK_SHAKERS_SO\n", argv[0]);
        return 2;
    }

    for (int i = 0; i < spec_count; ++i) {
        const Spec& spec = SPECS[i];
        void* library = dlopen(argv[i + 1], RTLD_LAZY | RTLD_LOCAL);
        if (!library) {
            std::fprintf(stderr, "phase5-stk-family: FAIL [%s]: dlopen: %s\n",
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
                "phase5-stk-family: FAIL [%s]: native ABI exports missing: %s\n",
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
        std::printf("phase5-stk-family: machine PASS [%s]\n", spec.label);
    }

    std::printf("phase5-stk-family: PASS machines=3\n");
    return 0;
}
