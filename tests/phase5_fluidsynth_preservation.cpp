/*
** PSYCLE-LINUX Phase 5C FluidSynth SF2 Player native preservation regression.
*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
#include <vector>

#include <fluidsynth.h>
#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

constexpr int kParameterCount = 24;
constexpr int kStateCount = 19;
constexpr int kLabelCount = 3;
constexpr int kNullCount = 2;
constexpr int kMaxInstr = 64;
constexpr int kPathSize = 4096;

struct InstrState {
    int bank;
    int prog;
    int pitch;
    int wheel;
};

struct FluidState {
    int version;
    char sf_path[kPathSize];
    InstrState instr[kMaxInstr];
    int cur_channel;
    int reverb_on;
    int roomsize;
    int damping;
    int width;
    int reverb_level;
    int chorus_on;
    int chorus_nr;
    int chorus_level;
    int chorus_speed;
    int chorus_depth_ms;
    int chorus_type;
    int polyphony;
    int interpolation;
    int gain;
};

static_assert(sizeof(FluidState) == 5184, "historical FluidSynth SYNPAR size changed");

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter kExpected[kParameterCount] = {
    {"Channel / Instrument", "Channel", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"Midi Channel (aux col)", "Midi channel. Select with aux column", 0, 63, psycle::plugin_interface::MPF_STATE, 0},
    {"Bank", "Bank", 0, 255, psycle::plugin_interface::MPF_STATE, 0},
    {"Program", "Program", 0, 255, psycle::plugin_interface::MPF_STATE, 0},
    {"Pitch bend amount", "Max Pitch bend", 0, 16384, psycle::plugin_interface::MPF_STATE, 8192},
    {"Pitch bend Range", "Pitch Wheel sensitivity range", 0, 72, psycle::plugin_interface::MPF_STATE, 2},
    {"", "", 0, 0, psycle::plugin_interface::MPF_NULL, 0},
    {"Polyphony", "Polyphony", 1, 256, psycle::plugin_interface::MPF_STATE, 128},
    {"Reverb", "Reverb", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"On/Off", "On/Off", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Room", "Room", 0, 100, psycle::plugin_interface::MPF_STATE, 30},
    {"Damp", "Damp", 0, 100, psycle::plugin_interface::MPF_STATE, 35},
    {"Width", "Width", 0, 100, psycle::plugin_interface::MPF_STATE, 70},
    {"Level", "Level", 0, 1000, psycle::plugin_interface::MPF_STATE, 350},
    {"", "", 0, 0, psycle::plugin_interface::MPF_NULL, 0},
    {"Interpolation", "Interpolation", FLUID_INTERP_NONE, FLUID_INTERP_HIGHEST + 3, psycle::plugin_interface::MPF_STATE, FLUID_INTERP_DEFAULT},
    {"Chorus", "Chorus", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"On/Off", "On/Off", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"No", "No", 1, 99, psycle::plugin_interface::MPF_STATE, 3},
    {"Level", "Level", 1, 10000, psycle::plugin_interface::MPF_STATE, 512},
    {"Speed", "Speed", 3, 50, psycle::plugin_interface::MPF_STATE, 3},
    {"Depth", "Depth", 0, 1000, psycle::plugin_interface::MPF_STATE, 8},
    {"Mode", "Mode", FLUID_CHORUS_MOD_SINE, FLUID_CHORUS_MOD_TRIANGLE, psycle::plugin_interface::MPF_STATE, FLUID_CHORUS_MOD_SINE},
    {"Global Gain", "Global Gain", 0, 256, psycle::plugin_interface::MPF_STATE, 48},
};

class TestCallback : public CFxCallback {
public:
    TestCallback(int sample_rate, const std::string& soundfont)
        : sample_rate_(sample_rate), soundfont_(soundfont) {}

    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }
    const std::string& message() const { return message_; }

    void MessBox(const char* message, const char*, unsigned int) const override {
        message_ = message ? message : "";
    }
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return sample_rate_ / 8; }
    int GetSamplingRate() const override { return sample_rate_; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 4; }
    bool FileBox(bool, char[], char path[]) override {
        if (soundfont_.empty()) return false;
        std::strcpy(path, soundfont_.c_str());
        return true;
    }

private:
    int sample_rate_;
    std::string soundfont_;
    mutable std::string message_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-fluidsynth: FAIL: %s\n", message);
    return 1;
}

void set_parameter(CMachineInterface* machine, int index, int value)
{
    machine->Vals[index] = value;
    machine->ParameterTweak(index, value);
}

void configure_defaults(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        set_parameter(machine, i, info->Parameters[i]->DefValue);
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0101 || info->Flags != psycle::plugin_interface::GENERATOR ||
            info->numCols != 3 || info->numParameters != kParameterCount)
        return fail("ABI/version/type/geometry changed");
    if (!info->Name || std::strcmp(info->Name, "FluidSynth SF2 player") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "FluidSynth") != 0 ||
            !info->Author || !std::strstr(info->Author, "Peter Hanappe") ||
            !std::strstr(info->Author, "ported by Sartorius"))
        return fail("historical identity/credit metadata changed");

    int states = 0, labels = 0, nulls = 0;
    for (int i = 0; i < kParameterCount; ++i) {
        const CMachineParameter* actual = info->Parameters[i];
        const ExpectedParameter& expected = kExpected[i];
        if (!actual || !actual->Name || !actual->Description ||
                std::strcmp(actual->Name, expected.name) != 0 ||
                std::strcmp(actual->Description, expected.description) != 0 ||
                actual->MinValue != expected.min_value || actual->MaxValue != expected.max_value ||
                actual->Flags != expected.flags || actual->DefValue != expected.default_value) {
            std::fprintf(stderr, "phase5-fluidsynth: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->Flags == psycle::plugin_interface::MPF_STATE) ++states;
        else if (actual->Flags == psycle::plugin_interface::MPF_LABEL) ++labels;
        else if (actual->Flags == psycle::plugin_interface::MPF_NULL) ++nulls;
    }
    if (states != kStateCount || labels != kLabelCount || nulls != kNullCount)
        return fail("state/label/null partition changed");
    std::printf("phase5-fluidsynth: metadata PASS version=0x0101 slots=24 state=19 labels=3 nulls=2 identity=FluidSynth\n");
    return 0;
}

int verify_constructor_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    if (!machine || !machine->Vals) return fail("constructor did not allocate Vals");
    for (int i = 0; i < info->numParameters; ++i) {
        if (machine->Vals[i] != info->Parameters[i]->DefValue)
            return fail("constructor parameter defaults are not deterministic");
    }
    std::printf("phase5-fluidsynth: constructor PASS defaults=24 initialized-before-Init=yes\n");
    return 0;
}

int verify_state_defaults(CMachineInterface* machine, const CMachineInfo* info,
    const std::string& soundfont)
{
    TestCallback callback(44100, soundfont);
    configure_defaults(machine, info, &callback);
    FluidState state{};
    if (machine->GetDataSize() != static_cast<int>(sizeof(state)))
        return fail("historical SYNPAR size changed");
    machine->GetData(&state);
    if (state.version != 3 || state.cur_channel != 0 || state.reverb_on != 0 ||
            state.roomsize != 30 || state.damping != 35 || state.width != 70 ||
            state.reverb_level != 350 || state.chorus_on != 0 || state.chorus_nr != 3 ||
            state.chorus_level != 512 || state.chorus_speed != 3 || state.chorus_depth_ms != 8 ||
            state.chorus_type != FLUID_CHORUS_MOD_SINE || state.polyphony != 128 ||
            state.interpolation != FLUID_INTERP_DEFAULT || state.gain != 48)
        return fail("published state defaults changed");
    for (int i = 0; i < kMaxInstr; ++i) {
        if (state.instr[i].bank != 0 || state.instr[i].prog != 0 ||
                state.instr[i].pitch != 8192 || state.instr[i].wheel != 2)
            return fail("per-channel default state changed");
    }
    machine->PutData(nullptr);
    if (callback.message().find("fileversion") == std::string::npos)
        return fail("null state no longer fails safely");
    std::printf("phase5-fluidsynth: state-defaults PASS bytes=5184 damp=35 gain=48 channels=64 null-putdata=safe\n");
    return 0;
}

fluid_synth_t* make_reference(int sample_rate, const std::string& soundfont,
    fluid_settings_t** out_settings)
{
    fluid_settings_t* settings = new_fluid_settings();
    if (!settings) return nullptr;
    fluid_settings_setnum(settings, "synth.sample-rate", sample_rate);
    fluid_settings_setint(settings, "synth.polyphony", 128);
    fluid_settings_setint(settings, "synth.midi-channels", kMaxInstr);
    fluid_settings_setint(settings, "synth.threadsafe-api", 0);
    fluid_settings_setint(settings, "synth.parallel-render", 0);
    fluid_synth_t* synth = new_fluid_synth(settings);
    if (!synth) {
        delete_fluid_settings(settings);
        return nullptr;
    }
    fluid_synth_set_interp_method(synth, -1, FLUID_INTERP_DEFAULT);
    fluid_synth_set_polyphony(synth, 128);
#if FLUIDSYNTH_VERSION_MAJOR > 2 || (FLUIDSYNTH_VERSION_MAJOR == 2 && FLUIDSYNTH_VERSION_MINOR >= 2)
    fluid_synth_reverb_on(synth, -1, 0);
    fluid_synth_chorus_on(synth, -1, 0);
#else
    fluid_synth_set_reverb_on(synth, 0);
    fluid_synth_set_chorus_on(synth, 0);
#endif
    fluid_synth_set_gain(synth, 48.0 / 128.0);
    if (fluid_synth_sfload(synth, soundfont.c_str(), 1) < 0) {
        delete_fluid_synth(synth);
        delete_fluid_settings(settings);
        return nullptr;
    }
    fluid_synth_system_reset(synth);
    *out_settings = settings;
    return synth;
}

struct Render {
    std::vector<float> left;
    std::vector<float> right;
};

Render render_plugin(CMachineInterface* machine, int count)
{
    Render result{std::vector<float>(count, 0.0f), std::vector<float>(count, 0.0f)};
    machine->SeqTick(0, 60, 0, 0, 0);
    machine->Work(result.left.data(), result.right.data(), count, 1);
    return result;
}

Render render_reference(int sample_rate, const std::string& soundfont, int count)
{
    Render result{std::vector<float>(count, 0.0f), std::vector<float>(count, 0.0f)};
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = make_reference(sample_rate, soundfont, &settings);
    if (!synth) return result;
    fluid_synth_noteon(synth, 0, 60, 127);
    fluid_synth_write_float(synth, count, result.left.data(), 0, 1,
        result.right.data(), 0, 1);
    for (int i = 0; i < count; ++i) {
        result.left[i] *= 32767.0f;
        result.right[i] *= 32767.0f;
    }
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    return result;
}

double rms(const Render& signal)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < signal.left.size(); ++i)
        sum += static_cast<double>(signal.left[i]) * signal.left[i] +
            static_cast<double>(signal.right[i]) * signal.right[i];
    return std::sqrt(sum / (2.0 * signal.left.size()));
}

double max_difference(const Render& a, const Render& b)
{
    if (a.left.size() != b.left.size()) return INFINITY;
    double maximum = 0.0;
    for (std::size_t i = 0; i < a.left.size(); ++i) {
        maximum = std::max(maximum, std::fabs(static_cast<double>(a.left[i] - b.left[i])));
        maximum = std::max(maximum, std::fabs(static_cast<double>(a.right[i] - b.right[i])));
    }
    return maximum;
}

int load_soundfont(CMachineInterface* machine, TestCallback* callback)
{
    machine->Command();
    FluidState state{};
    machine->GetData(&state);
    if (std::strcmp(state.sf_path, callback->soundfont().c_str()) != 0) return 1;
    return 0;
}

int verify_audio_reference(CMachineInterface* machine, const CMachineInfo* info,
    const std::string& soundfont)
{
    TestCallback callback(44100, soundfont);
    configure_defaults(machine, info, &callback);
    machine->Command();
    FluidState state{};
    machine->GetData(&state);
    if (std::strcmp(state.sf_path, soundfont.c_str()) != 0)
        return fail("SoundFont path was not retained in opaque state");
    Render candidate = render_plugin(machine, 512);
    Render reference = render_reference(44100, soundfont, 512);
    const double diff = max_difference(candidate, reference);
    if (rms(candidate) < 1.0 || !std::isfinite(diff) || diff > 0.5)
        return fail("44.1 kHz wrapper output diverged from direct FluidSynth reference");
    std::printf("phase5-fluidsynth: audio PASS sf2=TimGM6mb note=60 velocity=127 rate=44100 direct-reference=yes rms=%.6f maxdiff=%.6f\n",
        rms(candidate), diff);
    return 0;
}

int verify_live_rate(CMachineInterface* machine, const CMachineInfo* info,
    const std::string& soundfont)
{
    TestCallback callback(44100, soundfont);
    configure_defaults(machine, info, &callback);
    machine->Command();
    callback.set_sample_rate(88200);
    machine->SequencerTick();
    Render candidate = render_plugin(machine, 512);
    Render reference88 = render_reference(88200, soundfont, 512);
    Render stale44 = render_reference(44100, soundfont, 512);
    const double target_diff = max_difference(candidate, reference88);
    const double stale_diff = max_difference(candidate, stale44);
    if (!std::isfinite(target_diff) || target_diff > 0.5 ||
            !std::isfinite(stale_diff) || stale_diff < 1.0)
        return fail("live sample-rate update does not match direct 88.2 kHz reference");
    std::printf("phase5-fluidsynth: samplerate PASS live=44100->88200 direct-reference=yes target-maxdiff=%.6f stale44-maxdiff=%.6f\n",
        target_diff, stale_diff);
    return 0;
}

int verify_boundaries(CMachineInterface* machine, const CMachineInfo* info,
    const std::string& soundfont)
{
    TestCallback callback(44100, soundfont);
    configure_defaults(machine, info, &callback);
    machine->Command();
    float left[3] = {123.0f, 456.0f, 789.0f};
    float right[3] = {-123.0f, -456.0f, -789.0f};
    machine->Work(left, right, 0, 1);
    machine->Work(left, right, -4, 1);
    if (left[0] != 123.0f || left[1] != 456.0f || left[2] != 789.0f ||
            right[0] != -123.0f || right[1] != -456.0f || right[2] != -789.0f)
        return fail("non-positive callback modified audio memory");
    machine->MidiEvent(63, 0xC0, 5 << 8);
    machine->MidiEvent(63, 0xE0, 0x2000);
    machine->SeqTick(-1, 60, 0, 0, 0);
    machine->SeqTick(0, 60, -1, 0, 0);
    if (!machine->HostEvent(psycle::plugin_interface::HE_NEEDS_AUX_COLUMN, 0, 0.0f))
        return fail("aux-column host contract changed");
    std::printf("phase5-fluidsynth: boundaries PASS nonpositive=noop midi-channel63=safe invalid-track-instrument=safe aux-column=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s PATH_TO_FLUIDSYNTH_SO PATH_TO_SF2\n", argv[0]);
        return 2;
    }
    const std::string soundfont = argv[2];
    if (!fluid_is_soundfont(soundfont.c_str())) return fail("test SoundFont is unavailable or invalid");

    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) return fail(dlerror());
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateFn = CMachineInterface* (*)();
    using DeleteFn = psycle::plugin_interface::symbols::delete_machine_function;
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(handle, "GetInfo"));
    auto create = reinterpret_cast<CreateFn>(dlsym(handle, "CreateMachine"));
    auto destroy = reinterpret_cast<DeleteFn>(dlsym(handle, "DeleteMachine"));
    if (!get_info || !create || !destroy) return fail("native ABI exports missing");

    const CMachineInfo* info = get_info();
    if (verify_metadata(info)) return 1;

    std::vector<CMachineInterface*> machines;
    for (int i = 0; i < 5; ++i) {
        CMachineInterface* machine = create();
        if (!machine) return fail("CreateMachine returned null");
        machines.push_back(machine);
    }

    int rc = 0;
    if (verify_constructor_defaults(machines[0], info) ||
            verify_state_defaults(machines[1], info, soundfont) ||
            verify_audio_reference(machines[2], info, soundfont) ||
            verify_live_rate(machines[3], info, soundfont) ||
            verify_boundaries(machines[4], info, soundfont))
        rc = 1;

    for (CMachineInterface* machine : machines) destroy(*machine);
    dlclose(handle);
    if (rc == 0) std::printf("phase5-fluidsynth: PASS\n");
    return rc;
}
