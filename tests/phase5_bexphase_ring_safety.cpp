/*
** PSYCLE-LINUX BexPhase fixed-ring safety regression.
**
** Loads the retained BexPhase native machine through the real plugin ABI,
** applies its production defaults against a tracker-line-sized tick length,
** and processes beyond both the physical 24,576-sample ring and the historical
** default refresh product (8 * 5,512 = 44,096).  Run under ASan/UBSan so any
** cursor wrap regression becomes an immediate CI failure.
*/

#include <cmath>
#include <cstdio>
#include <dlfcn.h>
#include <vector>

#include <psycle/plugin_interface.hpp>

using psycle::plugin_interface::CFxCallback;
using psycle::plugin_interface::CMachineInfo;
using psycle::plugin_interface::CMachineInterface;

namespace {

class TestCallback : public CFxCallback {
public:
    void MessBox(const char*, const char*, unsigned int) const override {}
    int CallbackFunc(int, int, int, void*) override { return 0; }
    float* unused0(int, int) override { return nullptr; }
    float* unused1(int, int) override { return nullptr; }
    int GetTickLength() const override { return 5512; }
    int GetSamplingRate() const override { return 44100; }
    int GetBPM() const override { return 120; }
    int GetTPB() const override { return 24; }
    bool FileBox(bool, char[], char[]) override { return false; }
};

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-bexphase-ring: FAIL: %s\n", message);
    return 1;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_BEXPHASE_SO\n", argv[0]);
        return 2;
    }

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-bexphase-ring: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine =
        reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine =
        reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    if (!get_info || !create_machine || !delete_machine) {
        dlclose(library);
        return fail("native ABI exports missing");
    }

    const CMachineInfo* info = get_info();
    if (!info || info->numParameters != 6 || !info->Parameters) {
        dlclose(library);
        return fail("unexpected BexPhase metadata");
    }
    if (info->Parameters[2]->DefValue != 8) {
        dlclose(library);
        return fail("BexPhase default refresh changed");
    }

    CMachineInterface* machine = create_machine();
    if (!machine || !machine->Vals) {
        if (machine) delete_machine(*machine);
        dlclose(library);
        return fail("CreateMachine returned unusable instance");
    }

    TestCallback callback;
    machine->pCB = &callback;
    machine->Init();
    for (int i = 0; i < info->numParameters; ++i) {
        machine->ParameterTweak(i, info->Parameters[i]->DefValue);
    }

    constexpr int fixed_ring = 2048 * 6 * 2;
    constexpr int default_refresh = 8;
    constexpr int tick_length = 5512;
    constexpr int requested_refresh_window = default_refresh * tick_length;
    constexpr int processed_samples = requested_refresh_window + fixed_ring;

    static_assert(fixed_ring == 24576, "BexPhase fixed ring changed");
    static_assert(requested_refresh_window == 44096,
        "BexPhase default refresh product changed");

    std::vector<float> left(processed_samples, 0.0f);
    std::vector<float> right(processed_samples, 0.0f);
    left[0] = 1.0f;
    right[0] = -1.0f;

    machine->Work(left.data(), right.data(), processed_samples, 1);

    for (int i = 0; i < processed_samples; ++i) {
        if (!std::isfinite(left[i]) || !std::isfinite(right[i])) {
            delete_machine(*machine);
            dlclose(library);
            return fail("non-finite output after fixed-ring wrap");
        }
    }

    delete_machine(*machine);
    if (dlclose(library) != 0) {
        return fail("dlclose failed");
    }

    std::printf(
        "phase5-bexphase-ring: PASS tick=%d refresh=%d requested=%d ring=%d processed=%d\n",
        tick_length, default_refresh, requested_refresh_window,
        fixed_ring, processed_samples);
    return 0;
}
