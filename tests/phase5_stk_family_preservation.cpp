/*
** PSYCLE-LINUX Phase 5C STK-derived native-machine preservation.
**
** Covers the three retained source-built wrappers that link against the
** supported Linux libstk-dev boundary: stk Plucked, stk Reverbs and stk Shakers.
** Complete parameter-table hashes are frozen from the first dedicated
** observation of the real source-built Linux modules.
*/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>
#include <stk/ADSR.h>
#include <stk/JCRev.h>
#include <stk/NRev.h>
#include <stk/PRCRev.h>
#include <stk/Plucked.h>
#include <stk/Shakers.h>
#include <stk/Stk.h>

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
        psycle::plugin_interface::GENERATOR, 5, 1, Kind::Plucked,
        UINT64_C(0x55e20a3f7e90f611)},
    {"stk Reverbs", "stk Reverbs", "stk Reverbs",
        "Sartorius and STK developers", 0x0110,
        psycle::plugin_interface::EFFECT, 4, 1, Kind::Reverbs,
        UINT64_C(0xf92219f73194a3ae)},
    {"stk Shakers", "stk Shakers", "Shakers",
        "Sartorius, bohan and STK 4.5.0 developers", 0x0100,
        psycle::plugin_interface::GENERATOR, 6, 1, Kind::Shakers,
        UINT64_C(0x4c29aa201e9bb317)},
};

constexpr double PLUCKED_OFFSET = -36.3763165623;
constexpr unsigned int PLUCKED_SEED = 0x51a7u;
constexpr unsigned int SHAKER_RATE_SEED = 0x5a17u;
const int SHAKER_OLD_TO_NEW[] = {
    0, 1, 2, 19, 21, 5, 3, 4, 8, 9, 20, 6,
    7, 12, 13, 14, 15, 16, 17, 18, 10, 11, 22
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
    if (actual != spec.metadata_hash)
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

bool same_mono(const std::vector<float>& actual, const std::vector<float>& expected,
    float tolerance)
{
    if (actual.size() != expected.size()) return false;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (!std::isfinite(actual[i]) || !std::isfinite(expected[i]) ||
                std::fabs(actual[i] - expected[i]) > tolerance)
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

double mono_energy(const std::vector<float>& signal)
{
    long double energy = 0.0;
    for (float value : signal) {
        if (!std::isfinite(value)) return INFINITY;
        energy += static_cast<long double>(value) * value;
    }
    return static_cast<double>(energy);
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

template <typename Reverb>
std::vector<float> render_direct_reverb_channel(int sample_rate, int channel,
    std::size_t samples)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    Reverb reverb;
    reverb.setT60(static_cast<stk::StkFloat>(80.0 * 0.03125));
    reverb.setEffectMix(1.0);
    reverb.clear();
    std::vector<float> output(samples, 0.0f);
    for (std::size_t i = 0; i < samples; ++i) {
        const stk::StkFloat input = i == 0 ? 1.0 : 0.0;
        output[i] = static_cast<float>(reverb.tick(input, static_cast<unsigned int>(channel)));
    }
    return output;
}

std::vector<float> render_direct_reverb(int algorithm, int sample_rate, int channel,
    std::size_t samples)
{
    switch (algorithm) {
    case 0:
        return render_direct_reverb_channel<stk::JCRev>(sample_rate, channel, samples);
    case 1:
        return render_direct_reverb_channel<stk::NRev>(sample_rate, channel, samples);
    case 2:
        return render_direct_reverb_channel<stk::PRCRev>(sample_rate, channel, samples);
    default:
        return std::vector<float>();
    }
}

void render_direct_shaker(int sample_rate, int instrument, std::size_t samples,
    unsigned int seed, std::vector<float>& left, std::vector<float>& right)
{
    stk::Stk::setSampleRate(static_cast<stk::StkFloat>(sample_rate));
    stk::Shakers shaker;
    shaker.controlChange(2, 64.0);
    shaker.controlChange(4, 64.0);
    shaker.controlChange(11, 10.0);
    shaker.controlChange(1, 64.0);
    shaker.controlChange(128, 64.0);
    const stk::StkFloat frequency = static_cast<stk::StkFloat>(
        220.0 * std::pow(2.0, (static_cast<double>(instrument) + 7.0) / 12.0));
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

int verify_plucked(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    stk::Stk::setSampleRate(44100.0);
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

    delete_machine(*machine);
    machine = nullptr;

    if (rc == 0) {
        stk::Stk::setSampleRate(44100.0);
        TestCallback rate_callback(44100);
        CMachineInterface* rate_machine = create_machine();
        if (!rate_machine || !rate_machine->Vals) {
            if (rate_machine) delete_machine(*rate_machine);
            return fail(spec, "rate-test CreateMachine failed");
        }
        apply_defaults(rate_machine, info, rate_callback);
        rate_callback.set_sample_rate(88200);
        rate_machine->SequencerTick();
        if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - 88200.0) > 0.5) {
            rc = fail(spec, "SequencerTick did not propagate 88.2 kHz to STK");
        } else {
            std::srand(PLUCKED_SEED);
            rate_machine->SeqTick(0, 60, 0, 0x0c, 255);
            left.assign(8192, 0.0f); right.assign(8192, 0.0f);
            process_blocks(rate_machine, left, right);
            std::vector<float> ref_l, ref_r;
            render_direct_plucked(88200, 60, left.size(), PLUCKED_SEED, ref_l, ref_r);
            if (!same_signal(left, right, ref_l, ref_r, 1.0e-3f))
                rc = fail(spec, "live 88.2 kHz plucked render diverged from direct STK reference");
        }
        delete_machine(*rate_machine);
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Plucked PASS idle=zero 0C00=zero Stop=zero live-rate=STK-reference\n");
    return rc;
}

int verify_reverbs(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    stk::Stk::setSampleRate(44100.0);
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

    std::vector<std::vector<float>> algorithm_refs;
    for (int algorithm = 0; rc == 0 && algorithm < 3; ++algorithm) {
        machine->ParameterTweak(0, algorithm);
        machine->ParameterTweak(1, 80);
        machine->ParameterTweak(2, 100);
        machine->ParameterTweak(3, 0);

        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        left[0] = 1.0f;
        process_blocks(machine, left, right);
        const std::vector<float> expected_left =
            render_direct_reverb(algorithm, 44100, 0, left.size());
        algorithm_refs.push_back(expected_left);
        if (!same_mono(left, expected_left, 1.0e-6f) ||
                !silent_signal(right, right, 1.0e-7f)) {
            rc = fail(spec, "left-input independent reverb diverged from selected STK algorithm");
            break;
        }

        machine->ParameterTweak(0, algorithm);
        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        right[0] = 1.0f;
        process_blocks(machine, left, right);
        const std::vector<float> expected_right =
            render_direct_reverb(algorithm, 44100, 1, right.size());
        if (!same_mono(right, expected_right, 1.0e-6f) ||
                !silent_signal(left, left, 1.0e-7f)) {
            rc = fail(spec, "right-input independent reverb diverged from selected STK algorithm");
            break;
        }
    }

    if (rc == 0 && algorithm_refs.size() == 3) {
        const double d01 = mono_energy(algorithm_refs[0]);
        const double d11 = mono_energy(algorithm_refs[1]);
        const double d21 = mono_energy(algorithm_refs[2]);
        if (!std::isfinite(d01) || !std::isfinite(d11) || !std::isfinite(d21) ||
                d01 <= 1.0e-12 || d11 <= 1.0e-12 || d21 <= 1.0e-12 ||
                same_mono(algorithm_refs[0], algorithm_refs[1], 1.0e-7f) ||
                same_mono(algorithm_refs[0], algorithm_refs[2], 1.0e-7f) ||
                same_mono(algorithm_refs[1], algorithm_refs[2], 1.0e-7f))
            rc = fail(spec, "direct STK reverb references are not three distinct responses");
    }

    if (rc == 0) {
        machine->ParameterTweak(0, 1);
        machine->ParameterTweak(1, 80);
        machine->ParameterTweak(2, 100);
        machine->ParameterTweak(3, 1);

        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        left[0] = 1.0f;
        process_blocks(machine, left, right);
        if (!finite_signal(left, right) || !(mono_energy(right) > 1.0e-12))
            rc = fail(spec, "mixed reverb lost left-to-right crossfeed");

        machine->ParameterTweak(0, 1);
        left.assign(65536, 0.0f); right.assign(65536, 0.0f);
        right[0] = 1.0f;
        process_blocks(machine, left, right);
        if (!finite_signal(left, right) || !(mono_energy(left) > 1.0e-12))
            rc = fail(spec, "mixed reverb lost right-to-left crossfeed");
    }

    if (rc == 0) {
        callback.set_sample_rate(88200);
        machine->SequencerTick();
        if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - 88200.0) > 0.5) {
            rc = fail(spec, "SequencerTick did not propagate 88.2 kHz to STK");
        } else {
            machine->ParameterTweak(0, 1);
            machine->ParameterTweak(1, 80);
            machine->ParameterTweak(2, 100);
            machine->ParameterTweak(3, 0);
            left.assign(65536, 0.0f); right.assign(65536, 0.0f);
            left[0] = 1.0f;
            process_blocks(machine, left, right);
            const std::vector<float> expected =
                render_direct_reverb(1, 88200, 0, left.size());
            if (!same_mono(left, expected, 1.0e-6f) ||
                    !silent_signal(right, right, 1.0e-7f))
                rc = fail(spec, "live 88.2 kHz reverb diverged from direct STK reference");
        }
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Reverbs PASS dry=unity algorithms=STK-reference routing=bidirectional live-rate=STK-reference\n");
    delete_machine(*machine);
    return rc;
}

int verify_shaker_mapping(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    const int map_count = static_cast<int>(sizeof(SHAKER_OLD_TO_NEW) /
        sizeof(SHAKER_OLD_TO_NEW[0]));
    for (int index = 0; index < map_count; ++index) {
        const int note = 48 + index;
        const int instrument = SHAKER_OLD_TO_NEW[index];
        const unsigned int seed = 0x600du + static_cast<unsigned int>(note);
        stk::Stk::setSampleRate(44100.0);
        TestCallback callback(44100);
        CMachineInterface* machine = create_machine();
        if (!machine || !machine->Vals) {
            if (machine) delete_machine(*machine);
            return fail(spec, "mapping-test CreateMachine failed");
        }
        apply_defaults(machine, info, callback);
        std::srand(seed);
        machine->SeqTick(0, note, 0, 0x0c, 255);
        std::vector<float> left(4096, 0.0f), right(4096, 0.0f);
        process_blocks(machine, left, right);
        std::vector<float> ref_left, ref_right;
        render_direct_shaker(44100, instrument, left.size(), seed, ref_left, ref_right);
        const bool matches = same_signal(left, right, ref_left, ref_right, 1.0e-3f);
        delete_machine(*machine);
        if (!matches) {
            std::fprintf(stderr,
                "phase5-stk-family: FAIL [%s]: note %d no longer maps to STK instrument %d\n",
                spec.label, note, instrument);
            return 1;
        }
    }
    return 0;
}

int verify_shakers(const Spec& spec, const CMachineInfo* info,
    CMachineInterface* (*create_machine)(), void (*delete_machine)(CMachineInterface&))
{
    stk::Stk::setSampleRate(44100.0);
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

    delete_machine(*machine);
    machine = nullptr;

    if (rc == 0)
        rc = verify_shaker_mapping(spec, info, create_machine, delete_machine);

    if (rc == 0) {
        stk::Stk::setSampleRate(44100.0);
        TestCallback rate_callback(44100);
        CMachineInterface* rate_machine = create_machine();
        if (!rate_machine || !rate_machine->Vals) {
            if (rate_machine) delete_machine(*rate_machine);
            return fail(spec, "rate-test CreateMachine failed");
        }
        apply_defaults(rate_machine, info, rate_callback);
        rate_callback.set_sample_rate(88200);
        rate_machine->SequencerTick();
        if (std::fabs(static_cast<double>(stk::Stk::sampleRate()) - 88200.0) > 0.5) {
            rc = fail(spec, "SequencerTick did not propagate 88.2 kHz to STK");
        } else {
            std::srand(SHAKER_RATE_SEED);
            rate_machine->SeqTick(0, 48, 0, 0x0c, 255);
            left.assign(8192, 0.0f); right.assign(8192, 0.0f);
            process_blocks(rate_machine, left, right);
            std::vector<float> ref_left, ref_right;
            render_direct_shaker(88200, SHAKER_OLD_TO_NEW[0], left.size(),
                SHAKER_RATE_SEED, ref_left, ref_right);
            if (!same_signal(left, right, ref_left, ref_right, 1.0e-3f))
                rc = fail(spec, "live 88.2 kHz shaker render diverged from direct STK reference");
        }
        delete_machine(*rate_machine);
    }

    if (rc == 0)
        std::printf("phase5-stk-family: Shakers PASS map=48..70->STK-reference 0C00=zero Stop=zero live-rate=STK-reference\n");
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
