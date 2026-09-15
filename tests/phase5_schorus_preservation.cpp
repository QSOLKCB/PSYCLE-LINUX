/*
** PSYCLE-LINUX Phase 5C Sartorius SChorus preservation regression.
*/

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <string>
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
    {"Dry", "Dry", -32768, 32768, psycle::plugin_interface::MPF_STATE, 32768},
    {"Wet", "Wet", -32768, 32768, psycle::plugin_interface::MPF_STATE, 0},
    {"Feedback left", "Feedback left", -32768, 32768, psycle::plugin_interface::MPF_STATE, 16384},
    {"Feedback right", "Feedback right", -32768, 32768, psycle::plugin_interface::MPF_STATE, 16384},
    {"Min Delay", "Min Delay", 1, 6000, psycle::plugin_interface::MPF_STATE, 1},
    {"Max Delay", "Max Delay", 1, 6000, psycle::plugin_interface::MPF_STATE, 5},
    {"Rate", "Rate", 0, 1000, psycle::plugin_interface::MPF_STATE, 1},
    {"Delayer", "Delayer", 1, 40000, psycle::plugin_interface::MPF_STATE, 40000},
};

class TestCallback : public CFxCallback {
public:
    explicit TestCallback(int sample_rate) : sample_rate_(sample_rate) {}

    void set_sample_rate(int sample_rate) { sample_rate_ = sample_rate; }
    const std::string& message() const { return message_; }
    const std::string& title() const { return title_; }

    void MessBox(const char* message, const char* title, unsigned int) const override {
        message_ = message ? message : "";
        title_ = title ? title : "";
    }
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
    mutable std::string message_;
    mutable std::string title_;
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-schorus: FAIL: %s\n", message);
    return 1;
}

bool near(float a, float b, double tolerance = 1.0e-5)
{
    return std::fabs(static_cast<double>(a - b)) <= tolerance;
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

void process_chunked(CMachineInterface* machine, std::vector<float>& left,
    std::vector<float>& right)
{
    const int total = static_cast<int>(left.size());
    for (int offset = 0; offset < total; offset += psycle::plugin_interface::MAX_BUFFER_LENGTH) {
        const int count = std::min(psycle::plugin_interface::MAX_BUFFER_LENGTH, total - offset);
        machine->Work(left.data() + offset, right.data() + offset, count, 1);
    }
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0100 || info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 4) {
        return fail("ABI/version/type/column metadata changed");
    }
    if (!info->Name || std::strcmp(info->Name, "SChorus") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "SChorus") != 0 ||
            !info->Author || std::strcmp(info->Author, "Sartorius") != 0) {
        return fail("historical identity metadata changed");
    }
    const int expected_count = static_cast<int>(sizeof(EXPECTED_PARAMETERS) /
        sizeof(EXPECTED_PARAMETERS[0]));
    if (info->numParameters != expected_count || !info->Parameters)
        return fail("parameter table geometry changed");
    for (int i = 0; i < expected_count; ++i) {
        const CMachineParameter* actual = info->Parameters[i];
        const ExpectedParameter& expected = EXPECTED_PARAMETERS[i];
        if (!actual || !actual->Name || !actual->Description ||
                std::strcmp(actual->Name, expected.name) != 0 ||
                std::strcmp(actual->Description, expected.description) != 0 ||
                actual->MinValue != expected.min_value || actual->MaxValue != expected.max_value ||
                actual->Flags != expected.flags || actual->DefValue != expected.default_value) {
            std::fprintf(stderr, "phase5-schorus: FAIL: parameter %d metadata changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-schorus: metadata PASS version=0x0100 parameters=8 state=8 identity=SChorus\n");
    return 0;
}

int verify_constructor_defaults(CMachineInterface* machine, const CMachineInfo* info)
{
    if (!machine || !machine->Vals) return fail("constructor did not allocate Vals");
    for (int i = 0; i < info->numParameters; ++i) {
        if (machine->Vals[i] != info->Parameters[i]->DefValue) {
            std::fprintf(stderr,
                "phase5-schorus: FAIL: constructor slot %d expected %d got %d\n",
                i, info->Parameters[i]->DefValue, machine->Vals[i]);
            return 1;
        }
    }
    std::printf("phase5-schorus: constructor PASS defaults=8 initialized-before-Init=yes\n");
    return 0;
}

int verify_descriptions_and_about(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    char text[128];
    if (!machine->DescribeValue(text, 0, 16384) || std::strcmp(text, "1.000000") != 0)
        return fail("Dry description changed");
    if (!machine->DescribeValue(text, 1, 16384) || std::strcmp(text, "0.500000") != 0)
        return fail("Wet description changed");
    if (!machine->DescribeValue(text, 2, -8192) || std::strcmp(text, "-0.250000") != 0)
        return fail("feedback description changed");
    if (!machine->DescribeValue(text, 4, 5) || std::strcmp(text, "5 ms") != 0)
        return fail("delay description changed");
    if (!machine->DescribeValue(text, 6, 25) || std::strcmp(text, "25 ms/s") != 0)
        return fail("Rate description changed");
    if (machine->DescribeValue(text, 7, 1024))
        return fail("Delayer unexpectedly acquired a formatted description");
    machine->Command();
    if (callback.title() != "SChorus" ||
            callback.message() != "Sartorius Chorus\nBe carefull with wet and delayer!!!")
        return fail("historical About text changed");
    std::printf("phase5-schorus: describe-about PASS dry=1 wet=0.5 fb=-0.25 delay=5ms rate=25ms/s warning=retained\n");
    return 0;
}

int verify_nonpositive(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    float left[3] = {123.25f, 777.0f, 888.0f};
    float right[3] = {-456.5f, -777.0f, -888.0f};
    machine->Work(left, right, 0, 1);
    machine->Work(left, right, -3, 1);
    if (left[0] != 123.25f || left[1] != 777.0f || left[2] != 888.0f ||
            right[0] != -456.5f || right[1] != -777.0f || right[2] != -888.0f)
        return fail("non-positive callbacks modified audio memory");
    std::printf("phase5-schorus: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

int verify_max_block_identity(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    set_parameter(machine, 0, 16384);
    set_parameter(machine, 1, 0);
    set_parameter(machine, 2, 0);
    set_parameter(machine, 3, 0);

    std::vector<float> left(258);
    std::vector<float> right(258);
    std::vector<float> expected_left(256);
    std::vector<float> expected_right(256);
    left[0] = 123456.0f;
    right[0] = -123456.0f;
    for (int i = 0; i < 256; ++i) {
        left[i + 1] = static_cast<float>((i % 31) * 173 - 2400);
        right[i + 1] = static_cast<float>(2100 - (i % 29) * 149);
        expected_left[i] = left[i + 1];
        expected_right[i] = right[i + 1];
    }
    left[257] = 654321.0f;
    right[257] = -654321.0f;
    machine->Work(left.data() + 1, right.data() + 1, 256, 1);
    if (left[0] != 123456.0f || right[0] != -123456.0f ||
            left[257] != 654321.0f || right[257] != -654321.0f)
        return fail("maximum host block crossed canary boundaries");
    for (int i = 0; i < 256; ++i) {
        if (left[i + 1] != expected_left[i] || right[i + 1] != expected_right[i])
            return fail("dry identity configuration changed");
    }
    std::printf("phase5-schorus: dry PASS mix=dry-only feedback=0 max-block=256 exact-unity=yes bounded=yes\n");
    return 0;
}

int verify_default_oracle(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    float left[8] = {1000.0f, 0, 0, 0, 0, 0, 0, 0};
    float right[8] = {-500.0f, 0, 0, 0, 0, 0, 0, 0};
    const float expected_left[8] = {2000.0f, 1000.0f, 500.0f, 250.0f, 125.0f, 62.5f, 31.25f, 15.625f};
    const float expected_right[8] = {-1000.0f, -500.0f, -250.0f, -125.0f, -62.5f, -31.25f, -15.625f, -7.8125f};
    machine->Work(left, right, 8, 1);
    for (int i = 0; i < 8; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i]) {
            std::fprintf(stderr, "phase5-schorus: FAIL: default oracle sample %d changed\n", i);
            return 1;
        }
    }
    std::printf("phase5-schorus: oracle PASS default-feedback-trace=8 source-derived=yes\n");
    return 0;
}

void configure_delay_probe(CMachineInterface* machine)
{
    set_parameter(machine, 0, 16384);
    set_parameter(machine, 1, 32768);
    set_parameter(machine, 2, 0);
    set_parameter(machine, 3, 0);
    set_parameter(machine, 4, 1);
    set_parameter(machine, 5, 1);
    set_parameter(machine, 6, 1);
    set_parameter(machine, 7, 256);
}

int verify_delay_path(CMachineInterface* machine, const CMachineInfo* info)
{
    TestCallback callback(44100);
    configure_defaults(machine, info, &callback);
    configure_delay_probe(machine);
    std::vector<float> left(300, 0.0f);
    std::vector<float> right(300, 0.0f);
    left[0] = 1000.0f;
    right[0] = 2000.0f;
    process_chunked(machine, left, right);
    if (!near(left[0], 1000.0f) || !near(left[44], 1000.0f) ||
            !near(left[88], 1000.0f) || !near(right[0], 2000.0f) ||
            !near(right[256], 2000.0f))
        return fail("wet delay impulse markers changed");
    for (int i = 1; i < 44; ++i) {
        if (!near(left[i], 0.0f)) return fail("left wet echo arrived before 1 ms marker");
    }
    std::printf("phase5-schorus: delay PASS rate=44100 left-echo=44 right-ring-echo=256 wet-active=yes\n");
    return 0;
}

bool same_signal(const std::vector<float>& a, const std::vector<float>& b,
    double tolerance = 1.0e-5)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!near(a[i], b[i], tolerance)) return false;
    }
    return true;
}

int verify_live_rate(CMachineInterface* transitioned, CMachineInterface* fresh88,
    CMachineInterface* fresh44, const CMachineInfo* info)
{
    TestCallback transition_callback(44100);
    TestCallback callback88(88200);
    TestCallback callback44(44100);
    configure_defaults(transitioned, info, &transition_callback);
    configure_defaults(fresh88, info, &callback88);
    configure_defaults(fresh44, info, &callback44);
    configure_delay_probe(transitioned);
    configure_delay_probe(fresh88);
    configure_delay_probe(fresh44);

    transition_callback.set_sample_rate(88200);
    transitioned->SequencerTick();

    std::vector<float> trans_left(128, 0.0f), trans_right(128, 0.0f);
    std::vector<float> ref88_left(128, 0.0f), ref88_right(128, 0.0f);
    std::vector<float> ref44_left(128, 0.0f), ref44_right(128, 0.0f);
    trans_left[0] = ref88_left[0] = ref44_left[0] = 1000.0f;
    trans_right[0] = ref88_right[0] = ref44_right[0] = 2000.0f;
    process_chunked(transitioned, trans_left, trans_right);
    process_chunked(fresh88, ref88_left, ref88_right);
    process_chunked(fresh44, ref44_left, ref44_right);

    if (!same_signal(trans_left, ref88_left) || !same_signal(trans_right, ref88_right))
        return fail("live 44.1->88.2 kHz sweep state diverged from fresh 88.2 kHz");
    if (same_signal(trans_left, ref44_left) && same_signal(trans_right, ref44_right))
        return fail("sample-rate oracle is not rate-sensitive");
    if (!near(trans_left[88], 1000.0f) || !near(ref44_left[44], 1000.0f))
        return fail("sample-rate delay markers changed");

    std::printf("phase5-schorus: samplerate PASS sweep-preserved=yes live=44100->88200 left-echo=88 fresh-reference=yes stale44-differs=yes\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_SCHORUS_SO\n", argv[0]);
        return 2;
    }
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
    for (int i = 0; i < 9; ++i) {
        CMachineInterface* machine = create();
        if (!machine) return fail("CreateMachine returned null");
        machines.push_back(machine);
    }

    int rc = 0;
    if (verify_constructor_defaults(machines[0], info) ||
            verify_descriptions_and_about(machines[1], info) ||
            verify_nonpositive(machines[2], info) ||
            verify_max_block_identity(machines[3], info) ||
            verify_default_oracle(machines[4], info) ||
            verify_delay_path(machines[5], info) ||
            verify_live_rate(machines[6], machines[7], machines[8], info))
        rc = 1;

    for (CMachineInterface* machine : machines) destroy(*machine);
    dlclose(handle);
    if (rc == 0) std::printf("phase5-schorus: PASS\n");
    return rc;
}
