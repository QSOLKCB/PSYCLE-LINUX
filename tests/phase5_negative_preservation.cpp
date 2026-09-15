/*
** PSYCLE-LINUX Phase 5C Negative native preservation regression.
**
** The retained Negative effect is intentionally tiny: zero public parameters
** and exact stereo sign inversion.  Exercise the actual shared object's
** higher-level plugin.hpp ABI with its declared reference signatures, then
** exercise the same retained source directly so malformed non-positive host
** callback counts cannot regress into the historical backwards walk.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <limits>
#include <sstream>

#include <psycle/plugin.hpp>
#include "../cpsycle/plugins/negative/src/negative.cpp"

using psycle::plugin::Plugin;

namespace {

int fail(const char* message)
{
    std::fprintf(stderr, "phase5-negative: FAIL: %s\n", message);
    return 1;
}

int verify_metadata(const Plugin::Information& info)
{
    if (info.APIVersion != psycle::plugin_interface::MI_VERSION ||
            info.PlugVersion != 0x0100 ||
            info.Flags != psycle::plugin_interface::EFFECT ||
            info.numCols != 1 || info.numParameters != 0)
        return fail("ABI/version/type/zero-parameter geometry changed");
    if (!info.Name || std::strcmp(info.Name, "Negative") != 0 ||
            !info.ShortName || std::strcmp(info.ShortName, "Negative") != 0 ||
            !info.Author || std::strcmp(info.Author, "who cares") != 0)
        return fail("historical Negative identity changed");

    std::printf("phase5-negative: metadata PASS version=0x0100 parameters=0 identity=Negative author=who-cares\n");
    return 0;
}

int verify_help(psycle::plugin::Negative& machine)
{
    std::ostringstream out;
    machine.help(out);
    if (out.str() != "just a Negative (out = -in)\n")
        return fail("historical help text changed");
    std::printf("phase5-negative: help PASS contract=out-equals-minus-in\n");
    return 0;
}

int verify_positive(psycle::plugin::Negative& machine)
{
    float left[258];
    float right[258];
    left[0] = 123456.0f;
    right[0] = -654321.0f;
    left[257] = -111111.0f;
    right[257] = 222222.0f;
    for (int i = 0; i < 256; ++i) {
        left[i + 1] = static_cast<float>(i * 3 - 271);
        right[i + 1] = static_cast<float>(419 - i * 5);
    }

    machine.Work(&left[1], &right[1], 256, 1);
    if (left[0] != 123456.0f || right[0] != -654321.0f ||
            left[257] != -111111.0f || right[257] != 222222.0f)
        return fail("positive callback crossed the 256-sample host boundary");
    for (int i = 0; i < 256; ++i) {
        const float expected_left = -static_cast<float>(i * 3 - 271);
        const float expected_right = -static_cast<float>(419 - i * 5);
        if (left[i + 1] != expected_left || right[i + 1] != expected_right)
            return fail("positive callback no longer performs exact stereo negation");
    }

    float edge_left[] = {0.0f, -0.0f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};
    float edge_right[] = {-0.0f, 0.0f, -17.5f, 23.25f};
    machine.Work(edge_left, edge_right, 4, 1);
    if (!std::signbit(edge_left[0]) || std::signbit(edge_left[1]) ||
            !std::isinf(edge_left[2]) || !std::signbit(edge_left[2]) ||
            !std::isinf(edge_left[3]) || std::signbit(edge_left[3]) ||
            std::signbit(edge_right[0]) || !std::signbit(edge_right[1]) ||
            edge_right[2] != 17.5f || edge_right[3] != -23.25f)
        return fail("IEEE sign inversion edge behavior changed");

    std::printf("phase5-negative: inversion PASS max-block=256 stereo=exact signed-zero=yes infinity=yes bounded=yes\n");
    return 0;
}

int verify_nonpositive(psycle::plugin::Negative& machine)
{
    float left[] = {11.0f, 22.0f, 33.0f, 44.0f, 55.0f};
    float right[] = {-11.0f, -22.0f, -33.0f, -44.0f, -55.0f};
    const float expected_left[] = {11.0f, 22.0f, 33.0f, 44.0f, 55.0f};
    const float expected_right[] = {-11.0f, -22.0f, -33.0f, -44.0f, -55.0f};

    machine.Work(&left[2], &right[2], 0, 1);
    machine.Work(&left[2], &right[2], -7, 1);
    for (int i = 0; i < 5; ++i) {
        if (left[i] != expected_left[i] || right[i] != expected_right[i])
            return fail("non-positive callback modified guarded host buffers");
    }

    std::printf("phase5-negative: nonpositive PASS zero+negative strict-noop\n");
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s PATH_TO_NEGATIVE_SO\n", argv[0]);
        return 2;
    }

    using GetInfoFn = const Plugin::Information& (*)();
    using CreateFn = Plugin& (*)();
    using DeleteFn = void (*)(Plugin&);

    void* library = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!library) {
        std::fprintf(stderr, "phase5-negative: FAIL: dlopen: %s\n", dlerror());
        return 1;
    }
    auto get_info = reinterpret_cast<GetInfoFn>(dlsym(library, "GetInfo"));
    auto create = reinterpret_cast<CreateFn>(dlsym(library, "CreateMachine"));
    auto destroy = reinterpret_cast<DeleteFn>(dlsym(library, "DeleteMachine"));
    if (!get_info || !create || !destroy) {
        dlclose(library);
        return fail("native ABI exports missing");
    }

    int rc = verify_metadata(get_info());
    if (rc == 0) {
        Plugin& instance = create();
        destroy(instance);
        std::printf("phase5-negative: abi PASS create-delete=exact-reference-signatures\n");
    }
    if (dlclose(library) != 0 && rc == 0) rc = fail("dlclose failed");
    if (rc != 0) return rc;

    psycle::plugin::Negative machine;
    if (verify_metadata(psycle::plugin::Negative::information()) ||
            verify_help(machine) ||
            verify_positive(machine) ||
            verify_nonpositive(machine))
        return 1;

    std::printf("phase5-negative: PASS\n");
    return 0;
}
