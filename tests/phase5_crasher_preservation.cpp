/*
** PSYCLE-LINUX Phase 5C Crasher preservation regression.
**
** Crasher is intentionally a host-resilience diagnostic machine: every Work()
** callback ends by throwing std::runtime_error("crash on purpose!").  Preserve
** that historical contract while proving the callback never walks outside a
** malformed non-positive host block before producing the intended exception.
*/

#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <stdexcept>

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
    std::fprintf(stderr, "phase5-crasher: FAIL: %s\n", message);
    return 1;
}

bool exact_runtime_error(CMachineInterface* machine, float* left, float* right,
    int samples)
{
    try {
        machine->Work(left, right, samples, 1);
    } catch (const std::runtime_error& error) {
        return std::strcmp(error.what(), "crash on purpose!") == 0;
    } catch (...) {
        return false;
    }
    return false;
}

int verify_metadata(const CMachineInfo* info)
{
    if (!info) return fail("GetInfo returned null");
    if (info->APIVersion != psycle::plugin_interface::MI_VERSION ||
            info->PlugVersion != 0x0100 ||
            info->Flags != psycle::plugin_interface::EFFECT ||
            info->numCols != 1 || info->numParameters != 0) {
        return fail("ABI/version/type/parameter geometry changed");
    }
    if (!info->Name || std::strcmp(info->Name, "crasher") != 0 ||
            !info->ShortName || std::strcmp(info->ShortName, "crasher") != 0 ||
            !info->Author || std::strcmp(info->Author, "bohan") != 0) {
        return fail("historical identity changed");
    }

    std::printf("phase5-crasher: metadata PASS version=0x0100 parameters=0 identity=crasher\n");
    return 0;
}

int verify_positive_block(CMachineInterface* machine)
{
    float left[] = {101.0f, 1.0f, -2.0f, 3.0f, 202.0f};
    float right[] = {303.0f, -4.0f, 5.0f, -6.0f, 404.0f};

    if (!exact_runtime_error(machine, &left[1], &right[1], 3))
        return fail("positive Work did not produce the historical runtime_error");

    if (left[0] != 101.0f || left[4] != 202.0f ||
            right[0] != 303.0f || right[4] != 404.0f) {
        return fail("positive Work crossed the host-supplied block boundary");
    }
    if (left[1] != -1.0f || left[2] != 2.0f || left[3] != -3.0f ||
            right[1] != 4.0f || right[2] != -5.0f || right[3] != 6.0f) {
        return fail("historical stereo sign inversion changed");
    }

    std::printf("phase5-crasher: positive PASS stereo=inverted exception=runtime_error bounded=yes\n");
    return 0;
}

int verify_nonpositive_blocks(CMachineInterface* machine)
{
    float zero_left[] = {11.0f, 22.0f, 33.0f};
    float zero_right[] = {-11.0f, -22.0f, -33.0f};
    if (!exact_runtime_error(machine, &zero_left[1], &zero_right[1], 0))
        return fail("zero-length Work did not produce the historical runtime_error");
    if (zero_left[0] != 11.0f || zero_left[1] != 22.0f || zero_left[2] != 33.0f ||
            zero_right[0] != -11.0f || zero_right[1] != -22.0f || zero_right[2] != -33.0f) {
        return fail("zero-length Work modified host buffers");
    }

    float negative_left[] = {44.0f, 55.0f, 66.0f};
    float negative_right[] = {-44.0f, -55.0f, -66.0f};
    if (!exact_runtime_error(machine, &negative_left[1], &negative_right[1], -1))
        return fail("negative-length Work did not produce the historical runtime_error");
    if (negative_left[0] != 44.0f || negative_left[1] != 55.0f || negative_left[2] != 66.0f ||
            negative_right[0] != -44.0f || negative_right[1] != -55.0f || negative_right[2] != -66.0f) {
        return fail("negative-length Work touched memory before the intended exception");
    }

    std::printf("phase5-crasher: nonpositive PASS zero+negative buffers=untouched exception=runtime_error\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_CRASHER_SO\n", argv[0]);
        return 2;
    }

    using GetInfoFn = const CMachineInfo* (*)();
    using CreateMachineFn = CMachineInterface* (*)();
    using DeleteMachineFn = void (*)(CMachineInterface&);

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-crasher: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }

    GetInfoFn get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    CreateMachineFn create_machine = reinterpret_cast<CreateMachineFn>(dlsym(library, "CreateMachine"));
    DeleteMachineFn delete_machine = reinterpret_cast<DeleteMachineFn>(dlsym(library, "DeleteMachine"));
    if (!get_info || !create_machine || !delete_machine) {
        dlclose(library);
        return fail("native ABI exports missing");
    }

    const CMachineInfo* info = get_info();
    int rc = verify_metadata(info);
    CMachineInterface* machine = nullptr;
    if (rc == 0) {
        machine = create_machine();
        if (!machine) rc = fail("CreateMachine returned null");
    }

    TestCallback callback;
    if (rc == 0) {
        machine->pCB = &callback;
        machine->Init();
        rc = verify_positive_block(machine);
    }
    if (rc == 0) rc = verify_nonpositive_blocks(machine);

    if (machine) delete_machine(*machine);
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    std::printf("phase5-crasher: PASS\n");
    return 0;
}
