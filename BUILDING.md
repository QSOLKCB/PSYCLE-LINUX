# Building PSYCLE-LINUX on Linux

This document describes the reproducible development environment used by **Phase 2 — Modern Linux Build Audit**.

The project rule still applies:

> **Port Psycle to Linux. Do not reinvent Psycle.**

Phase 2 starts from the existing C-Psycle makefiles and records their behaviour on a current Linux toolchain before broader build-system changes are considered.

## Reference environment

The automated reference environment is:

- Ubuntu 24.04 LTS x86-64;
- GCC/G++ supplied by Ubuntu `build-essential`;
- GNU Make;
- `pkg-config`;
- the existing C-Psycle make-based build under `cpsycle/`.

The developer workstation target for the project remains current Ubuntu x86-64. Differences between Ubuntu releases should be recorded rather than hidden with ad-hoc local workarounds.

## Development packages

Install the upstream C-Psycle dependency set:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  pkg-config \
  liblilv-dev \
  liblua5.4-dev \
  libfreetype-dev \
  libfontconfig-dev \
  libgl-dev \
  libx11-dev \
  libxft-dev \
  libxext-dev \
  libxmu-dev \
  libasound2-dev \
  libjack-jackd2-dev \
  libstk-dev \
  libsdl2-dev \
  libfluidsynth-dev
```

This intentionally follows the imported C-Psycle `readme.txt` closely. If Phase 2 discovers an additional required package, it must be documented with the source file or build target that requires it.

The audit summary is generated entirely by POSIX/GNU shell tooling already supplied by the development environment; Python is not required.

## Run the Phase 2 audit

From the repository root:

```bash
chmod +x scripts/phase2-build-audit.sh
scripts/phase2-build-audit.sh build-audit
```

The audit writes:

```text
build-audit/summary.md
build-audit/logs/*.log
```

The generated `build-audit/` tree is ignored by Git so local environment/compiler evidence cannot be accidentally committed.

The script does **not** stop at the first failed build target. Each build stage is run and its result is recorded so one failure cannot conceal the rest of the Linux build state.

The exit status is nevertheless meaningful: after the complete report has been generated, the script exits nonzero if any audited build stage is `FAIL` or `BLOCKED`. This means the Phase 2 reference baseline currently produces a nonzero audit result because the blockers documented in `PHASE2_BUILD_AUDIT.md` are real. The report is evidence; a green process exit is reserved for an all-PASS build state.

The same audit runs in GitHub Actions through `.github/workflows/phase2-linux-build-audit.yml`. The workflow uploads `build-audit/` with `if: always()`, so logs remain available even when the audit step fails as expected on the current baseline.

## Direct upstream-style build

To exercise the imported makefiles directly:

```bash
cd cpsycle
make
```

The original top-level targets remain available:

```bash
make all
make host
make plugins
make drivers
make player
make debug
make host-debug
make plugins-debug
make player-debug
```

Do not assume all of these targets are currently clean on a modern toolchain. Phase 2 exists specifically to document their current state before Phase 3 begins compatibility repair.

## Optional VST2 hosting on Linux

VST2 hosting remains a supported Psycle feature. It is **disabled by default** in the public Linux baseline only because Phase 1 intentionally omitted the legacy Steinberg SDK-derived headers from redistribution.

If you have a compatible local VST2 SDK/header boundary that you are entitled to use, enable the existing VST2 host with:

```bash
cd cpsycle
make clean
make ENABLE_VST2=1 CPPFLAGS="-I/path/to/local/vst2/headers"
```

The supplied include directory must make the VST2 headers expected by the imported Psycle sources available locally. `ENABLE_VST2=1` propagates `PSYCLE_ENABLE_VST2`, which restores the VST2 machine factory, scanner and host sources on Linux. The SDK itself remains outside this repository.

Windows retains the historical Psycle VST2 feature gate; PSYCLE-LINUX does not treat Windows as a release target, but Linux-port changes should not silently disable an existing upstream capability.

## Build layers

The host makefile builds the principal libraries in this order:

```text
script/src
thread/src
container/src
file/src
ui/src
luaui/src
dsp/src
audio/src
host/src
```

For diagnosis, build an individual layer with:

```bash
cd cpsycle
make -C container/src
make -C dsp/src
make -C ui/src
make -C audio/src
```

Linux drivers can be checked independently:

```bash
make -C driver/alsa
make -C driver/alsamidi
make -C driver/jack
make -C driver/sdl2
make -C driver/evjoystick
```

`psyplayer` is deliberately tested separately because it gives the project a smaller executable target for validating the audio/song engine before the full X11 host.

## `pkg-config` integration points

The Phase 2 audit records whether these modules are visible and which flags they provide:

```text
lua
lua5.4
lilv-0
freetype2
fontconfig
x11
xft
xext
xmu
alsa
jack
sdl2
fluidsynth
```

The imported makefiles do not use all of those names consistently. Phase 2 records the observed behaviour first; compatibility fixes belong in a focused follow-up commit rather than being silently hidden in this document.

## Cleaning

A repeatable audit must start without stale outputs. The imported top-level `make clean` target does **not** clean the driver tree, so the audit first performs both historical cleanup paths:

```bash
cd cpsycle
make clean
make clean-drivers
```

Those historical rules are not complete enough for a repeatability claim. Some generated objects can survive in nested plugin directories or under output names the makefiles no longer enumerate. After both clean targets pass, the audit therefore runs a separate **Generated artifact purge** stage.

That purge scans `cpsycle/` for untracked native build outputs (`*.o`, `*.a`, `*.so`, `*.lo`, `*.gch`, plus generated `psycle`/`psyplayer` executables) and removes them regardless of directory depth. It also removes the generated `cpsycle/driver/build/` directory so an earlier run cannot hide the clean-checkout driver output-path failure.

The purge is deliberately Git-aware: if any matching artifact is tracked by the repository, the audit refuses to delete it and treats cleanup as failed. This prevents the repeatability step from silently removing baseline content.

If either historical clean command or the generated-artifact purge fails, the audit writes the partial report and exits before running build stages. It will not treat possibly stale artifacts as fresh evidence.

## PASS, FAIL, and BLOCKED

The generated summary distinguishes three states:

- **PASS** — the stage completed successfully;
- **FAIL** — the stage itself returned nonzero and its declared prerequisites had passed;
- **BLOCKED** — the stage returned nonzero while one or more declared core prerequisites were unavailable.

Blocked targets are still executed. The label records causal status rather than skipping the command, so their logs can show how far the target progressed without misrepresenting a downstream prerequisite failure as a new independent defect.

## Evidence rule

When reporting a build failure, include:

1. operating system and architecture;
2. compiler version;
3. exact build command;
4. first causal compiler/linker error rather than only the final `make` error;
5. affected source/build file;
6. whether the failure is blocking a library, driver, `psyplayer`, plugin, or the host.

Warnings should be preserved during Phase 2 unless a warning itself prevents compilation. Broad warning suppression is not an acceptable substitute for understanding the port.
