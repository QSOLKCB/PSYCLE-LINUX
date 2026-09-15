/*
** PSYCLE-LINUX Phase 5C MoreAmp EQ native preservation regression.
*/

#include <cmath>
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

constexpr int PARAM_B20 = 0;
constexpr int PARAM_B31 = 2;
constexpr int PARAM_B500 = 14;
constexpr int PARAM_B630 = 15;
constexpr int PARAM_B800 = 16;
constexpr int PARAM_B1000 = 17;
constexpr int PARAM_B1250 = 18;
constexpr int PARAM_B1600 = 19;
constexpr int PARAM_B2000 = 20;
constexpr int PARAM_B20000 = 30;
constexpr int PARAM_PREAMP = 32;
constexpr int PARAM_BANDS = 33;
constexpr int PARAM_EXTRA = 34;
constexpr int PARAM_LINK = 35;
constexpr int PARAMETER_COUNT = 36;
constexpr float IMPULSE = 100000.0f;
constexpr unsigned int DITHER_SEED = 0x4d414551u;

struct ExpectedParameter {
    const char* name;
    const char* description;
    int min_value;
    int max_value;
    int flags;
    int default_value;
};

const ExpectedParameter EXPECTED_PARAMETERS[PARAMETER_COUNT] = {
    {"20 Hz", "20 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"25 Hz", "25 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"31.5 Hz *", "* 31.5 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"40 Hz", "40 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"50 Hz", "50 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"62.5 Hz *", "* 62.5 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"80 Hz", "80 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"100 Hz", "100 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"125 Hz *", "* 125 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"160 Hz", "160 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"200 Hz", "200 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"250 Hz *", "* 250 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"315 Hz", "315 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"400 Hz", "400 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"500 Hz *", "* 500 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"630 Hz", "630 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"800 Hz", "800 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"1000 Hz *", "* 1000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"1250 Hz", "1250 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"1600 Hz", "1600 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"2000 Hz *", "* 2000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"2500 Hz", "2500 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"3150 Hz", "3150 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"4000 Hz *", "* 4000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"5000 Hz", "5000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"6300 Hz", "6300 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"8000 Hz *", "* 8000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"10000 Hz", "10000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"12500 Hz", "12500 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"16000 Hz *", "* 16000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"20000 Hz", "20000 Hz", 0, 64, psycle::plugin_interface::MPF_STATE, 32},
    {"Parametrization", "Parametrization", 0, 0, psycle::plugin_interface::MPF_LABEL, 0},
    {"Preamp", "Preamp", -16, 16, psycle::plugin_interface::MPF_STATE, 0},
    {"Bands", "Bands", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Extra filtering", "Extra filtering", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
    {"Link *", "Link", 0, 1, psycle::plugin_interface::MPF_STATE, 0},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}
    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }
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

struct StereoSignal {
    std::vector<float> left;
    std::vector<float> right;
};

struct RefCoefficients {
    double beta;
    double alpha;
    double gamma;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-moreamp-eq: FAIL: %s\n", message);
    return 1;
}

bool near(double actual, double expected, double tolerance)
{
    return std::fabs(actual - expected) <= tolerance;
}

void initialize(CMachineInterface* machine, const CMachineInfo* info,
    TestCallback* callback)
{
    std::srand(DITHER_SEED);
    machine->pCB = callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i)
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
}

StereoSignal render_impulse(CMachineInterface* machine, int samples = 256)
{
    StereoSignal signal{std::vector<float>(samples, 0.0f),
        std::vector<float>(samples, 0.0f)};
    signal.left[0] = IMPULSE;
    signal.right[0] = -IMPULSE * 0.5f;
    machine->Work(signal.left.data(), signal.right.data(), samples, 1);
    for (int i = 0; i < samples; ++i) {
        if (!std::isfinite(signal.left[i]) || !std::isfinite(signal.right[i])) {
            signal.left.clear();
            signal.right.clear();
            break;
        }
    }
    return signal;
}

bool same_signal(const StereoSignal& a, const StereoSignal& b,
    double tolerance = 1.0e-3)
{
    if (a.left.size() != b.left.size() || a.right.size() != b.right.size())
        return false;
    for (std::size_t i = 0; i < a.left.size(); ++i) {
        if (!near(a.left[i], b.left[i], tolerance) ||
                !near(a.right[i], b.right[i], tolerance))
            return false;
    }
    return true;
}

double reference_preamp(int value)
{
    return 9.9999946497217584440165E-01 *
        std::exp(6.9314738656671842642609E-02 * static_cast<double>(value + 20)) +
        3.7119444716771825623636E-07;
}

double reference_gain(int raw_value)
{
    const double db = (static_cast<double>(raw_value) - 32.0) * 0.5;
    return 2.5220207857061455181125E-01 *
        std::exp(8.0178361802353992349168E-02 * db) -
        2.5220207852836562523180E-01;
}

RefCoefficients reference_coefficients(int sample_rate, double f0,
    double octave_percent)
{
    const double pi = 3.14159265358979323846;
    const double gain_f0 = 1.0;
    const double gain_f1 = 1.0 / std::sqrt(2.0);
    const double octave_factor = std::pow(2.0, octave_percent / 2.0);
    const double f1 = f0 / octave_factor;
    const double tf0 = 2.0 * pi * f0 / static_cast<double>(sample_rate);
    const double tf = 2.0 * pi * f1 / static_cast<double>(sample_rate);
    const auto sq = [](double x) { return x * x; };
    const double a = sq(gain_f1) * sq(std::cos(tf0)) -
        2.0 * sq(gain_f1) * std::cos(tf) * std::cos(tf0) + sq(gain_f1) -
        sq(gain_f0) * sq(std::sin(tf));
    const double b = 2.0 * sq(gain_f1) * sq(std::cos(tf)) +
        sq(gain_f1) * sq(std::cos(tf0)) -
        2.0 * sq(gain_f1) * std::cos(tf) * std::cos(tf0) - sq(gain_f1) +
        sq(gain_f0) * sq(std::sin(tf));
    const double c = 0.25 * sq(gain_f1) * sq(std::cos(tf0)) -
        0.5 * sq(gain_f1) * std::cos(tf) * std::cos(tf0) +
        0.25 * sq(gain_f1) - 0.25 * sq(gain_f0) * sq(std::sin(tf));
    const double k = c - ((b * b) / (4.0 * a));
    const double h = -(b / (2.0 * a));
    const double radius = std::sqrt(-(k / a));
    const double x0 = std::fmin(h - radius, h + radius);
    return {2.0 * x0, 0.5 - x0,
        2.0 * (0.5 + x0) * std::cos(tf0)};
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0100 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 3 || info->numParameters != PARAMETER_COUNT)
        return fail("MoreAmp EQ ABI/version/type/geometry changed");
    if (!info->Name || std::strcmp(info->Name, "MoreAmp EQ") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "maEQ") != 0 ||
            !info->Author || std::strcmp(info->Author,
                "Felipe Rivera/pmisteli/Sartorius") != 0)
        return fail("MoreAmp EQ historical identity changed");

    int state_count = 0;
    int label_count = 0;
    for (int i = 0; i < PARAMETER_COUNT; ++i) {
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
                "phase5-moreamp-eq: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
        if (actual->Flags == psycle::plugin_interface::MPF_STATE) ++state_count;
        if (actual->Flags == psycle::plugin_interface::MPF_LABEL) ++label_count;
    }
    if (state_count != 35 || label_count != 1)
        return fail("MoreAmp EQ 35-state/one-label partition changed");
    std::printf("phase5-moreamp-eq: metadata PASS version=0x0100 slots=36 state=35 labels=1 identity=maEQ\n");
    return 0;
}

int verify_constructor_defaults(CMachineInterface* machine,
    const CMachineInfo* info)
{
    if (!machine || !machine->Vals)
        return fail("constructor parameter storage missing");
    for (int i = 0; i < PARAMETER_COUNT; ++i) {
        if (machine->Vals[i] != info->Parameters[i]->DefValue) {
            std::fprintf(stderr,
                "phase5-moreamp-eq: FAIL: constructor slot %d expected %d got %d\n",
                i, info->Parameters[i]->DefValue, machine->Vals[i]);
            return 1;
        }
    }
    std::printf("phase5-moreamp-eq: defaults PASS constructor=published-36-slot-state\n");
    return 0;
}

int expect_description(CMachineInterface* machine, int param, int value,
    const char* expected)
{
    char text[128] = {};
    if (!machine->DescribeValue(text, param, value) ||
            std::strcmp(text, expected) != 0) {
        std::fprintf(stderr,
            "phase5-moreamp-eq: FAIL: description param=%d value=%d expected='%s' got='%s'\n",
            param, value, expected, text);
        return 1;
    }
    return 0;
}

int verify_descriptions_and_link(CMachineInterface* machine,
    const CMachineInfo* info)
{
    TestCallback callback(44100);
    initialize(machine, info, &callback);
    if (expect_description(machine, PARAM_B20, 32, "--") ||
            expect_description(machine, PARAM_B31, 64, "16.0 dB") ||
            expect_description(machine, PARAM_PREAMP, -6, "-6 dB") ||
            expect_description(machine, PARAM_BANDS, 0, "10") ||
            expect_description(machine, PARAM_BANDS, 1, "31") ||
            expect_description(machine, PARAM_EXTRA, 1, "On") ||
            expect_description(machine, PARAM_LINK, 0, "Off"))
        return 1;
    machine->ParameterTweak(PARAM_BANDS, 1);
    if (expect_description(machine, PARAM_B20, 32, "0.0 dB")) return 1;

    machine->ParameterTweak(PARAM_LINK, 1);
    machine->ParameterTweak(PARAM_B1000, 44);
    if (machine->Vals[PARAM_B500] != 32 || machine->Vals[PARAM_B630] != 36 ||
            machine->Vals[PARAM_B800] != 40 || machine->Vals[PARAM_B1000] != 44 ||
            machine->Vals[PARAM_B1250] != 40 || machine->Vals[PARAM_B1600] != 36 ||
            machine->Vals[PARAM_B2000] != 32)
        return fail("linked B1000 interpolation geometry changed");

    std::printf("phase5-moreamp-eq: describe-link PASS mode10=inactive-dashes mode31=all-bands link1000=32/36/40/44/40/36/32\n");
    return 0;
}

int verify_default_and_preamp(CMachineInterface* flat, CMachineInterface* boosted,
    const CMachineInfo* info)
{
    TestCallback flat_cb(44100);
    TestCallback boost_cb(44100);
    initialize(flat, info, &flat_cb);
    initialize(boosted, info, &boost_cb);
    boosted->ParameterTweak(PARAM_PREAMP, 6);

    const StereoSignal a = render_impulse(flat);
    const StereoSignal b = render_impulse(boosted);
    if (a.left.empty() || b.left.empty()) return fail("preamp render became non-finite");
    if (!near(a.left[0], IMPULSE, 0.1) ||
            !near(a.right[0], -IMPULSE * 0.5, 0.1))
        return fail("flat default path is no longer unity gain");
    const double expected_scale = reference_preamp(6) * 0.25;
    if (!near(b.left[0], IMPULSE * expected_scale, 0.2) ||
            !near(b.right[0], -IMPULSE * 0.5 * expected_scale, 0.2))
        return fail("source-derived +6 dB preamp first-sample oracle changed");

    std::printf("phase5-moreamp-eq: preamp PASS default=unity plus6-scale=1.51571666\n");
    return 0;
}

int verify_10_band(CMachineInterface* flat, CMachineInterface* active,
    CMachineInterface* ignored, const CMachineInfo* info)
{
    TestCallback flat_cb(44100);
    TestCallback active_cb(44100);
    TestCallback ignored_cb(44100);
    initialize(flat, info, &flat_cb);
    initialize(active, info, &active_cb);
    initialize(ignored, info, &ignored_cb);
    active->ParameterTweak(PARAM_B1000, 64);
    ignored->ParameterTweak(PARAM_B20, 64);

    const StereoSignal baseline = render_impulse(flat);
    const StereoSignal boosted = render_impulse(active);
    const StereoSignal unstarred = render_impulse(ignored);
    if (baseline.left.empty() || boosted.left.empty() || unstarred.left.empty())
        return fail("10-band render became non-finite");
    if (same_signal(boosted, baseline, 0.1))
        return fail("10-band 1 kHz gain no longer affects audio");
    if (!same_signal(unstarred, baseline, 0.1))
        return fail("10-band mode no longer ignores unstarred 20 Hz control");

    const RefCoefficients c = reference_coefficients(44100, 1000.0, 1.0);
    const double x = IMPULSE * reference_preamp(0);
    const double gain = reference_gain(64);
    const double y0 = c.alpha * x;
    const double y1 = c.gamma * y0;
    const double y2 = -c.alpha * x + c.gamma * y1 - c.beta * y0;
    const double expected[3] = {
        x * 0.25 + y0 * gain,
        y1 * gain,
        y2 * gain
    };
    for (int i = 0; i < 3; ++i) {
        if (!near(boosted.left[i], expected[i], 2.0)) {
            std::fprintf(stderr,
                "phase5-moreamp-eq: FAIL: 10-band 1k marker %d expected %.9g got %.9g\n",
                i, expected[i], boosted.left[i]);
            return 1;
        }
    }

    std::printf("phase5-moreamp-eq: band10 PASS active=1000Hz unstarred20=ignored markers=3\n");
    return 0;
}

int verify_31_band(CMachineInterface* flat, CMachineInterface* active,
    const CMachineInfo* info)
{
    TestCallback flat_cb(44100);
    TestCallback active_cb(44100);
    initialize(flat, info, &flat_cb);
    initialize(active, info, &active_cb);
    flat->ParameterTweak(PARAM_BANDS, 1);
    active->ParameterTweak(PARAM_BANDS, 1);
    active->ParameterTweak(PARAM_B20, 64);

    const StereoSignal baseline = render_impulse(flat);
    const StereoSignal boosted = render_impulse(active);
    if (baseline.left.empty() || boosted.left.empty())
        return fail("31-band render became non-finite");
    if (same_signal(boosted, baseline, 0.1))
        return fail("31-band 20 Hz gain no longer affects audio");

    const RefCoefficients c = reference_coefficients(44100, 20.0, 1.0 / 3.0);
    const double x = IMPULSE * reference_preamp(0);
    const double expected0 = x * 0.25 + c.alpha * x * reference_gain(64);
    if (!near(boosted.left[0], expected0, 2.0))
        return fail("source-derived 31-band 20 Hz first-sample oracle changed");

    std::printf("phase5-moreamp-eq: band31 PASS active=20Hz marker0=source-derived\n");
    return 0;
}

int verify_extra_filter(CMachineInterface* single, CMachineInterface* extra,
    const CMachineInfo* info)
{
    TestCallback single_cb(44100);
    TestCallback extra_cb(44100);
    initialize(single, info, &single_cb);
    initialize(extra, info, &extra_cb);
    single->ParameterTweak(PARAM_B1000, 64);
    extra->ParameterTweak(PARAM_B1000, 64);
    extra->ParameterTweak(PARAM_EXTRA, 1);

    const StereoSignal once = render_impulse(single);
    const StereoSignal twice = render_impulse(extra);
    if (once.left.empty() || twice.left.empty())
        return fail("extra-filter render became non-finite");
    if (same_signal(once, twice, 0.1))
        return fail("extra-filter cascade no longer changes the EQ response");

    const RefCoefficients c = reference_coefficients(44100, 1000.0, 1.0);
    const double x = IMPULSE * reference_preamp(0);
    const double gain = reference_gain(64);
    const double stage1 = c.alpha * x * gain;
    const double expected0 = x * 0.25 + stage1 + c.alpha * stage1 * gain;
    if (!near(twice.left[0], expected0, 2.0))
        return fail("source-derived extra-filter first-sample oracle changed");

    std::printf("phase5-moreamp-eq: extra PASS cascade=two-pass differs=single marker0=source-derived\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    initialize(machine, info, &callback);
    machine->ParameterTweak(PARAM_EXTRA, 1);
    float left[7] = {11, 12, 13, 14, 15, 16, 17};
    float right[7] = {-11, -12, -13, -14, -15, -16, -17};
    const float expected_left[7] = {11, 12, 13, 14, 15, 16, 17};
    const float expected_right[7] = {-11, -12, -13, -14, -15, -16, -17};
    machine->Work(left + 2, right + 2, 0, 1);
    machine->Work(left + 2, right + 2, -7, 1);
    for (int i = 0; i < 7; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i])
            return fail("non-positive callback modified guarded buffers");
    }
    std::printf("phase5-moreamp-eq: nonpositive PASS extra=on zero+negative strict-noop\n");
    return 0;
}

int verify_sample_rate_transition(CMachineInterface* machine,
    const CMachineInfo* info)
{
    TestCallback callback(44100);
    initialize(machine, info, &callback);
    machine->ParameterTweak(PARAM_BANDS, 1);
    machine->ParameterTweak(PARAM_B20000, 64);

    const StereoSignal active44 = render_impulse(machine);
    if (active44.left.empty()) return fail("44.1 kHz high-band render became non-finite");
    const RefCoefficients c44 = reference_coefficients(44100, 20000.0, 1.0 / 3.0);
    const double x = IMPULSE * reference_preamp(0);
    const double expected44 = x * 0.25 + c44.alpha * x * reference_gain(64);
    if (!near(active44.left[0], expected44, 2.0))
        return fail("44.1 kHz 20 kHz-band first-sample oracle changed");

    callback.set_sample_rate(32000);
    machine->SequencerTick();
    const StereoSignal disabled32 = render_impulse(machine);
    if (disabled32.left.empty()) return fail("32 kHz transitioned render became non-finite");
    if (!near(disabled32.left[0], IMPULSE, 0.2) ||
            !near(disabled32.right[0], -IMPULSE * 0.5, 0.2))
        return fail("20 kHz band was not disabled above the 32 kHz Nyquist limit");
    if (near(active44.left[0], disabled32.left[0], 10.0))
        return fail("sample-rate transition retained stale 44.1 kHz high-band coefficients");

    std::printf("phase5-moreamp-eq: samplerate PASS 20k-band active@44100 disabled@32000 history=reset\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_MAEQ_SO\n", argv[0]);
        return 2;
    }
    void* handle = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!handle) return fail(dlerror());

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateFn = CMachineInterface* (*)();
    using DeleteFn = void (*)(CMachineInterface*);
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(handle, "GetInfo"));
    auto create = reinterpret_cast<CreateFn>(dlsym(handle, "CreateMachine"));
    auto destroy = reinterpret_cast<DeleteFn>(dlsym(handle, "DeleteMachine"));
    if (!get_info || !create || !destroy) return fail("native ABI exports missing");

    const CMachineInfo* info = get_info();
    if (verify_metadata(info)) return 1;

    std::vector<CMachineInterface*> machines;
    for (int i = 0; i < 13; ++i) {
        CMachineInterface* machine = create();
        if (!machine) return fail("CreateMachine returned null");
        machines.push_back(machine);
    }

    int rc = 0;
    if (verify_constructor_defaults(machines[0], info) ||
            verify_descriptions_and_link(machines[0], info) ||
            verify_default_and_preamp(machines[1], machines[2], info) ||
            verify_10_band(machines[3], machines[4], machines[5], info) ||
            verify_31_band(machines[6], machines[7], info) ||
            verify_extra_filter(machines[8], machines[9], info) ||
            verify_nonpositive(machines[10], info) ||
            verify_sample_rate_transition(machines[11], info)) {
        rc = 1;
    }

    for (CMachineInterface* machine : machines) destroy(machine);
    dlclose(handle);
    if (rc == 0) std::printf("phase5-moreamp-eq: PASS\n");
    return rc;
}
