/*
** PSYCLE-LINUX Phase 5C Alk Muter preservation regression.
**
** Loads the retained Alk Muter through Psycle's historical native ABI and
** freezes its identity, one-parameter surface, click-avoiding mute/unmute
** ramps, live sample-rate timing update and strict Work() buffer bounds.
*/

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;
using psycle::plugin_interface::CMachineParameter;

namespace {

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

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-alk-muter: FAIL: %s\n", message);
    return 1;
}

bool near(float actual, float expected, double tolerance = 1.0e-6)
{
    return std::fabs(static_cast<double>(actual - expected)) <= tolerance;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0120 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1 || info->numParameters != 1 ||
            !info->Parameters || !info->Parameters[0]) {
        return fail("native ABI/version/type/geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, "Alk Muter") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "Muter") != 0 ||
            !info->Author || std::strcmp(info->Author, "Alk") != 0 ||
            !info->Command || std::strcmp(info->Command, "About") != 0) {
        return fail("historical identity metadata changed");
    }

    const CMachineParameter* parameter = info->Parameters[0];
    if (!parameter->Name || std::strcmp(parameter->Name, "Mute") != 0 ||
            !parameter->Description || std::strcmp(parameter->Description, "Mute off/on") != 0 ||
            parameter->MinValue != 0 || parameter->MaxValue != 1 ||
            parameter->Flags != psycle::plugin_interface::MPF_STATE ||
            parameter->DefValue != 0) {
        return fail("Mute parameter metadata changed");
    }

    std::printf("phase5-alk-muter: metadata PASS version=0x0120 parameters=1 identity=Alk-Muter\n");
    return 0;
}

void configure(CMachineInterface* machine, TestCallback* callback)
{
    machine->pCB = callback;
    machine->Vals[0] = 0;
    machine->Init();
    machine->ParameterTweak(0, 0);
}

int verify_describe(CMachineInterface* machine)
{
    char text[32] = {};
    if (!machine->DescribeValue(text, 0, 0) || std::strcmp(text, "off") != 0) {
        return fail("DescribeValue(0) changed");
    }
    std::memset(text, 0, sizeof(text));
    if (!machine->DescribeValue(text, 0, 1) || std::strcmp(text, "on") != 0) {
        return fail("DescribeValue(1) changed");
    }
    if (machine->DescribeValue(text, 0, 2)) {
        return fail("DescribeValue accepted out-of-range mute value");
    }
    std::printf("phase5-alk-muter: describe PASS off/on\n");
    return 0;
}

int verify_unity(CMachineInterface* machine)
{
    TestCallback callback(44100);
    configure(machine, &callback);
    float left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    float right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};
    const float expected_left[] = {-2.0f, -0.25f, 0.0f, 0.5f, 3.0f};
    const float expected_right[] = {1.5f, 0.25f, 0.0f, -0.75f, -4.0f};

    machine->Work(left, right, 5, 1);
    for (int i = 0; i < 5; ++i) {
        if (!near(left[i], expected_left[i]) || !near(right[i], expected_right[i])) {
            return fail("default unmuted path is no longer exact unity");
        }
    }
    std::printf("phase5-alk-muter: unity PASS mute=off\n");
    return 0;
}

int verify_mute_fade(CMachineInterface* machine, int sample_rate,
    int last_fade_index, const char* marker)
{
    TestCallback callback(sample_rate);
    std::array<float, 66> left;
    std::array<float, 66> right;
    left.fill(1.0f);
    right.fill(1.0f);
    left.front() = 1234.5f;
    right.front() = -2345.5f;
    left.back() = 3456.5f;
    right.back() = -4567.5f;

    configure(machine, &callback);
    machine->ParameterTweak(0, 1);
    machine->Work(left.data() + 1, right.data() + 1, 64, 1);

    if (!near(left.front(), 1234.5f) || !near(right.front(), -2345.5f) ||
            !near(left.back(), 3456.5f) || !near(right.back(), -4567.5f)) {
        return fail("mute fade wrote outside the host-supplied sample count");
    }
    if (!near(left[1], 1.0f) || !near(right[1], 1.0f)) {
        return fail("mute fade no longer begins from unity");
    }
    for (int i = 1; i <= last_fade_index; ++i) {
        if (!(left[static_cast<std::size_t>(i + 1)] <= left[static_cast<std::size_t>(i)] + 1.0e-7f) ||
                !near(left[static_cast<std::size_t>(i + 1)], right[static_cast<std::size_t>(i + 1)])) {
            return fail("mute fade is no longer monotonic/stereo-identical");
        }
    }
    const std::size_t first_zero = static_cast<std::size_t>(last_fade_index + 2);
    if (!(left[first_zero - 1] > 0.01f) || !near(left[first_zero], 0.0f) ||
            !near(right[first_zero], 0.0f)) {
        return fail("mute fade boundary changed");
    }
    for (std::size_t i = first_zero; i <= 64; ++i) {
        if (!near(left[i], 0.0f) || !near(right[i], 0.0f)) {
            return fail("fully muted tail is no longer exact zero");
        }
    }

    std::array<float, 8> muted_left;
    std::array<float, 8> muted_right;
    muted_left.fill(1.0f);
    muted_right.fill(-1.0f);
    machine->Work(muted_left.data(), muted_right.data(), 8, 1);
    for (std::size_t i = 0; i < muted_left.size(); ++i) {
        if (!near(muted_left[i], 0.0f) || !near(muted_right[i], 0.0f)) {
            return fail("steady mute is no longer exact zero");
        }
    }

    std::printf("phase5-alk-muter: %s PASS bounded=yes first-zero=%d\n",
        marker, last_fade_index + 1);
    return 0;
}

int verify_unmute_fade(CMachineInterface* machine)
{
    TestCallback callback(44100);
    std::array<float, 64> left;
    std::array<float, 64> right;
    left.fill(1.0f);
    right.fill(1.0f);

    configure(machine, &callback);
    machine->ParameterTweak(0, 1);
    machine->Work(left.data(), right.data(), 64, 1);

    left.fill(1.0f);
    right.fill(1.0f);
    machine->ParameterTweak(0, 0);
    machine->Work(left.data(), right.data(), 64, 1);

    if (!(left[0] > 0.0f && left[0] < 0.01f) || !near(left[0], right[0])) {
        return fail("unmute fade no longer resumes from the muted floor");
    }
    for (std::size_t i = 1; i < left.size(); ++i) {
        if (left[i] + 1.0e-7f < left[i - 1] || !near(left[i], right[i])) {
            return fail("unmute fade is no longer monotonic/stereo-identical");
        }
    }
    if (!near(left.back(), 1.0f) || !near(right.back(), 1.0f)) {
        return fail("unmute path did not return to exact unity");
    }
    std::printf("phase5-alk-muter: unmute PASS floor-to-unity\n");
    return 0;
}

int verify_live_rate(CMachineInterface* machine)
{
    TestCallback callback(44100);
    std::array<float, 66> left;
    std::array<float, 66> right;
    left.fill(1.0f);
    right.fill(1.0f);
    left.front() = 11.0f;
    right.front() = 12.0f;
    left.back() = 13.0f;
    right.back() = 14.0f;

    configure(machine, &callback);
    callback.set_sample_rate(88200);
    machine->SequencerTick();
    machine->ParameterTweak(0, 1);
    machine->Work(left.data() + 1, right.data() + 1, 64, 1);

    if (!near(left.front(), 11.0f) || !near(right.front(), 12.0f) ||
            !near(left.back(), 13.0f) || !near(right.back(), 14.0f)) {
        return fail("88.2 kHz live-rate fade wrote outside the sample block");
    }
    if (!(left[22] > 0.01f) || !near(left[23], 0.0f) || !near(right[23], 0.0f)) {
        return fail("SequencerTick did not preserve the historical 88.2 kHz fade slope");
    }
    std::printf("phase5-alk-muter: live-rate PASS 44.1->88.2k first-zero=22\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_ALK_MUTER_SO\n", argv[0]);
        return 2;
    }

    void* library = dlopen(argv[1], RTLD_LAZY | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-alk-muter: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    dlerror();
    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    const char* error = dlerror();
    if (error || !get_info || !create_machine || !delete_machine) {
        std::fprintf(stderr, "phase5-alk-muter: FAIL: native ABI exports missing: %s\n",
            error ? error : "unknown symbol error");
        dlclose(library);
        return 1;
    }

    int rc = verify_metadata(get_info());
    CMachineInterface* describe = nullptr;
    CMachineInterface* unity = nullptr;
    CMachineInterface* mute44 = nullptr;
    CMachineInterface* unmute = nullptr;
    CMachineInterface* rate = nullptr;

    if (rc == 0) {
        describe = create_machine();
        if (!describe || !describe->Vals) rc = fail("CreateMachine returned unusable describe instance");
        else rc = verify_describe(describe);
    }
    if (rc == 0) {
        unity = create_machine();
        if (!unity || !unity->Vals) rc = fail("CreateMachine returned unusable unity instance");
        else rc = verify_unity(unity);
    }
    if (rc == 0) {
        mute44 = create_machine();
        if (!mute44 || !mute44->Vals) rc = fail("CreateMachine returned unusable 44.1 kHz mute instance");
        else rc = verify_mute_fade(mute44, 44100, 43, "mute-44k");
    }
    if (rc == 0) {
        unmute = create_machine();
        if (!unmute || !unmute->Vals) rc = fail("CreateMachine returned unusable unmute instance");
        else rc = verify_unmute_fade(unmute);
    }
    if (rc == 0) {
        rate = create_machine();
        if (!rate || !rate->Vals) rc = fail("CreateMachine returned unusable rate instance");
        else rc = verify_live_rate(rate);
    }

    if (rate) delete_machine(*rate);
    if (unmute) delete_machine(*unmute);
    if (mute44) delete_machine(*mute44);
    if (unity) delete_machine(*unity);
    if (describe) delete_machine(*describe);
    dlclose(library);

    if (rc == 0) {
        std::printf("phase5-alk-muter: PASS\n");
    }
    return rc;
}
