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

The script does **not** stop at the first failed target. Each build stage is run independently and its exit code is recorded so one failure cannot conceal the rest of the Linux build state.

The same audit runs in GitHub Actions through `.github/workflows/phase2-linux-build-audit.yml`.

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

## Build layers

The host makefile builds the principal libraries in this order:

```text
script/src
thread/src
container/src
file/src
ui/src
dsp/src
luaui/src
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

The imported top-level clean target can be exercised with:

```bash
cd cpsycle
make clean
```

Because historical clean rules can themselves contain stale assumptions, the audit records clean-target failures instead of treating them as unrelated noise.

## Evidence rule

When reporting a build failure, include:

1. operating system and architecture;
2. compiler version;
3. exact build command;
4. first causal compiler/linker error rather than only the final `make` error;
5. affected source/build file;
6. whether the failure is blocking a library, driver, `psyplayer`, plugin, or the host.

Warnings should be preserved during Phase 2 unless a warning itself prevents compilation. Broad warning suppression is not an acceptable substitute for understanding the port.
