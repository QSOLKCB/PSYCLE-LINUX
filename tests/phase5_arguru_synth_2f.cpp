/*
** PSYCLE-LINUX Phase 5 Arguru Synth 2f preservation regression.
**
** Loads the retained Linux native-machine shared object through Psycle's
** exported ABI, freezes its metadata/parameter contract, and exercises a
** deterministic pitched voice, note release, and live sample-rate change.
*/

#include <cmath>
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

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[] = {
    {"OSC1 Wave", "OSC1 Wave", 0, 6, psycle::plugin_interface::MPF_STATE, 1},
    {"OSC2 Wave", "OSC2 Wave", 0, 6, psycle::plugin_interface::MPF_STATE, 1},
    {"OSC2 Detune", "OSC2 Detune", -36, 36, psycle::plugin_interface::MPF_STATE, 0},
    {"OSC2 Finetune", "OSC2 Finetune", 0, 256, psycle::plugin_interface::MPF_STATE, 27},
    {"OSC2 Sync", "OSC2 Sync", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"VCA Attack", "VCA Attack", 32, 220500, psycle::plugin_interface::MPF_STATE, 32},
    {"VCA Decay", "VCA Decay", 32, 220500, psycle::plugin_interface::MPF_STATE, 6341},
    {"VCA Sustain", "VCA Sustain level", 0, 256, psycle::plugin_interface::MPF_STATE, 0},
    {"VCA Release", "VCA Release", 32, 220500, psycle::plugin_interface::MPF_STATE, 2630},
    {"VCF Attack", "VCF Attack", 32, 220500, psycle::plugin_interface::MPF_STATE, 589},
    {"VCF Decay", "VCF Decay", 32, 220500, psycle::plugin_interface::MPF_STATE, 2630},
    {"VCF Sustain", "VCF Sustain level", 0, 256, psycle::plugin_interface::MPF_STATE, 0},
    {"VCF Release", "VCF Release", 32, 220500, psycle::plugin_interface::MPF_STATE, 2630},
    {"VCF LFO Speed", "VCF LFO Speed", 1, 65536, psycle::plugin_interface::MPF_STATE, 32},
    {"VCF LFO Amplitude", "VCF LFO Amplitude", 0, 240, psycle::plugin_interface::MPF_STATE, 0},
    {"VCF Cutoff", "VCF Cutoff", 0, 240, psycle::plugin_interface::MPF_STATE, 120},
    {"VCF Resonance", "VCF Resonance", 1, 240, psycle::plugin_interface::MPF_STATE, 128},
    {"VCF Type", "VCF Type", 0, 19, psycle::plugin_interface::MPF_STATE, 0},
    {"VCF Envmod", "VCF Envmod", -240, 240, psycle::plugin_interface::MPF_STATE, 80},
    {"OSC Mix", "OSC Mix", 0, 256, psycle::plugin_interface::MPF_STATE, 128},
    {"Volume", "Volume", 0, 256, psycle::plugin_interface::MPF_STATE, 128},
    {"Arpeggiator", "Arpeggiator", 0, 16, psycle::plugin_interface::MPF_STATE, 0},
    {"Arpeggio tempo", "Arp. BPM", 32, 1024, psycle::plugin_interface::MPF_STATE, 125},
    {"Arpeggio Steps", "Arp. Steps", 0, 16, psycle::plugin_interface::MPF_STATE, 4},
    {"Glb. Detune", "Global Detune", -36, 36, psycle::plugin_interface::MPF_STATE, 1},
    {"Gbl. Finetune", "Global Finetune", -256, 256, psycle::plugin_interface::MPF_STATE, 60},
    {"Glide Depth", "Glide Depth", 0, 255, psycle::plugin_interface::MPF_STATE, 0},
    {"Resampling", "Resampling method", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
};

const int TONE_VALUES[] = {
    0, 0, 0, 27, 0,
    32, 32, 256, 32,
    32, 32, 256, 32,
    32, 0, 240, 1, 0, 0,
    0, 256, 0, 125, 4, 1, 60, 0, 0
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate)
        : sample_rate_(sample_rate) {}

    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }

    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sample_rate_ / 8; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 125; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char[]) override { return false; }

private:
    int sample_rate_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-arguru-synth-2f: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-5)
{
    return std::fabs(static_cast<double>(actual - expected)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) {
        return fail("GetInfo returned null");
    }
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION) {
        return fail("native-machine API version changed");
    }
    if (info->PlugVersion != 0x0250) {
        return fail("Arguru Synth 2f plugin version changed");
    }
    if (info->Flags != psycle::plugin_interface::GENERATOR) {
        return fail("Arguru Synth 2f is no longer classified as a generator");
    }
    if (info->numParameters != static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
            sizeof(EXPECTED_PARAMETERS[0]))) {
        return fail("Arguru Synth 2f parameter count changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Arguru Synth 2") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Arguru Synth") != 0 ||
            !info->Author || std::strcmp(info->Author,
                "J. Arguelles (arguru) and psycledelics") != 0 ||
            info->numCols != 4) {
        return fail("Arguru Synth 2f machine identity metadata changed");
    }
    if (!info->Parameters) {
        return fail("Arguru Synth 2f parameter table is missing");
    }

    for (int i = 0; i < info->numParameters; ++i) {
        const CMachineParameter* actual = info->Parameters[i];
        const ExpectedParameter& expected = EXPECTED_PARAMETERS[i];
        if (!actual || !actual->Name || !actual->Description ||
                std::strcmp(actual->Name, expected.name) != 0 ||
                std::strcmp(actual->Description, expected.description) != 0 ||
                actual->MinValue != expected.min_value ||
                actual->MaxValue != expected.max_value ||
                actual->Flags != expected.flags ||
                actual->DefValue != expected.default_value) {
            std::fprintf(stderr,
                "phase5-arguru-synth-2f: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->DefValue < actual->MinValue ||
                actual->DefValue > actual->MaxValue) {
            return fail("Arguru Synth 2f default parameter is outside its range");
        }
    }
    return 0;
}

void apply_tone_values(CMachineInterface* machine)
{
    const int count = static_cast<int>(sizeof(TONE_VALUES) / sizeof(TONE_VALUES[0]));
    for (int i = 0; i < count; ++i) {
        machine->Vals[i] = TONE_VALUES[i];
    }
    for (int i = 0; i < count; ++i) {
        machine->ParameterTweak(i, TONE_VALUES[i]);
    }
}

void render(CMachineInterface* machine, std::vector<float>& left,
    std::vector<float>& right)
{
    const int total = static_cast<int>(left.size());
    for (int offset = 0; offset < total; offset += 256) {
        const int amount = (total - offset > 256) ? 256 : total - offset;
        machine->Work(left.data() + offset, right.data() + offset, amount, 1);
    }
}

double estimate_frequency(const std::vector<float>& samples, int sample_rate,
    int start_sample)
{
    int first = -1;
    int last = -1;
    int crossings = 0;
    for (int i = start_sample + 1; i < static_cast<int>(samples.size()); ++i) {
        if (samples[i - 1] <= 0.0f && samples[i] > 0.0f) {
            if (first < 0) {
                first = i;
            }
            last = i;
            ++crossings;
        }
    }
    if (crossings < 8 || last <= first) {
        return 0.0;
    }
    return static_cast<double>(crossings - 1) * sample_rate /
        static_cast<double>(last - first);
}

int verify_a4_pitch(CMachineInterface* machine, int sample_rate,
    bool reinitialize_from_44100)
{
    TestCallback callback(44100);
    const int sample_count = (sample_rate == 44100) ? 8192 : 16384;
    std::vector<float> left(sample_count, 0.0f);
    std::vector<float> right(sample_count, 0.0f);

    machine->pCB = &callback;
    machine->Init();
    apply_tone_values(machine);
    if (reinitialize_from_44100) {
        callback.set_sample_rate(sample_rate);
        machine->SequencerTick();
    }
    machine->SeqTick(0, 69, 0, 0, 0); /* A4; retained code subtracts 9. */
    render(machine, left, right);

    bool heard_audio = false;
    for (int i = 0; i < sample_count; ++i) {
        if (!near(left[i], right[i], 1.0e-6)) {
            return fail("Arguru Synth 2f mono voice stopped matching left/right");
        }
        if (std::fabs(left[i]) > 1.0e-3f) {
            heard_audio = true;
        }
    }
    if (!heard_audio) {
        return fail("A4 render unexpectedly produced silence");
    }

    const double frequency = estimate_frequency(left, sample_rate,
        sample_rate == 44100 ? 1024 : 2048);
    if (frequency < 435.0 || frequency > 445.0) {
        std::fprintf(stderr,
            "phase5-arguru-synth-2f: FAIL: A4 frequency %.3f Hz at %d Hz sample rate\n",
            frequency, sample_rate);
        return 1;
    }
    return 0;
}

int verify_deterministic_voice(CMachineInterface* first, CMachineInterface* second)
{
    TestCallback first_callback(44100);
    TestCallback second_callback(44100);
    std::vector<float> left_a(2048, 0.0f);
    std::vector<float> right_a(2048, 0.0f);
    std::vector<float> left_b(2048, 0.0f);
    std::vector<float> right_b(2048, 0.0f);

    first->pCB = &first_callback;
    first->Init();
    apply_tone_values(first);
    first->SeqTick(0, 69, 0, 0, 0);
    render(first, left_a, right_a);

    second->pCB = &second_callback;
    second->Init();
    apply_tone_values(second);
    second->SeqTick(0, 69, 0, 0, 0);
    render(second, left_b, right_b);

    for (int i = 0; i < 2048; ++i) {
        if (!near(left_a[i], left_b[i], 1.0e-6) ||
                !near(right_a[i], right_b[i], 1.0e-6)) {
            return fail("fresh fixed-waveform A4 renders are no longer deterministic");
        }
    }
    return 0;
}

int verify_note_release(CMachineInterface* machine)
{
    TestCallback callback(44100);
    std::vector<float> pre_release(1024, 0.0f);
    std::vector<float> left(128, 0.0f);
    std::vector<float> right(128, 0.0f);

    machine->pCB = &callback;
    machine->Init();
    apply_tone_values(machine);
    machine->SeqTick(0, 69, 0, 0, 0);
    render(machine, pre_release, pre_release);

    machine->SeqTick(0, psycle::plugin_interface::NOTE_NOTEOFF, 0, 0, 0);
    render(machine, left, right);

    for (int i = 64; i < 128; ++i) {
        if (!near(left[i], 0.0f, 1.0e-7) || !near(right[i], 0.0f, 1.0e-7)) {
            return fail("minimum VCA release no longer reaches silence promptly");
        }
    }
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ARGURU_SYNTH_2F_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-arguru-synth-2f: FAIL: dlopen: %s\n", dlerror());
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
            "phase5-arguru-synth-2f: FAIL: native ABI exports missing: %s\n",
            symbol_error ? symbol_error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* pitch_441 = nullptr;
    CMachineInterface* pitch_882 = nullptr;
    CMachineInterface* deterministic_a = nullptr;
    CMachineInterface* deterministic_b = nullptr;
    CMachineInterface* release_machine = nullptr;

    if (rc == 0) {
        pitch_441 = create_machine();
        if (!pitch_441 || !pitch_441->Vals) {
            rc = fail("CreateMachine did not provide a usable 44.1 kHz instance");
        } else {
            rc = verify_a4_pitch(pitch_441, 44100, false);
        }
    }
    if (rc == 0) {
        pitch_882 = create_machine();
        if (!pitch_882 || !pitch_882->Vals) {
            rc = fail("CreateMachine did not provide a usable 88.2 kHz instance");
        } else {
            rc = verify_a4_pitch(pitch_882, 88200, true);
        }
    }
    if (rc == 0) {
        deterministic_a = create_machine();
        deterministic_b = create_machine();
        if (!deterministic_a || !deterministic_b ||
                !deterministic_a->Vals || !deterministic_b->Vals) {
            rc = fail("CreateMachine did not provide deterministic-test instances");
        } else {
            rc = verify_deterministic_voice(deterministic_a, deterministic_b);
        }
    }
    if (rc == 0) {
        release_machine = create_machine();
        if (!release_machine || !release_machine->Vals) {
            rc = fail("CreateMachine did not provide a release-test instance");
        } else {
            rc = verify_note_release(release_machine);
        }
    }

    if (release_machine) delete_machine(*release_machine);
    if (deterministic_b) delete_machine(*deterministic_b);
    if (deterministic_a) delete_machine(*deterministic_a);
    if (pitch_882) delete_machine(*pitch_882);
    if (pitch_441) delete_machine(*pitch_441);

    if (dlclose(library) != 0 && rc == 0) {
        rc = fail("dlclose failed after native-machine test");
    }
    if (rc != 0) {
        return rc;
    }

    std::printf("phase5-arguru-synth-2f: PASS\n");
    std::printf("machine: Arguru Synth 2\n");
    std::printf("parameters: 28\n");
    std::printf("abi: GetInfo/CreateMachine/DeleteMachine\n");
    std::printf("dsp: deterministic A4 + note release + 88.2 kHz retune\n");
    return 0;
}
