# Phase 2 — Modern Linux Build Audit

This document records the first reproducible modern-Linux build state for the audited C-Psycle r12005 baseline.

Phase 2 is an **audit**, not a redesign phase. A failed target is a useful result when the failure is reproducible, scoped and documented. The audit harness nevertheless returns a nonzero process status when any build stage is `FAIL` or `BLOCKED`; collecting complete evidence and reporting build health are separate concerns.

## Reference run

GitHub Actions workflow: `Phase 2 Linux build audit`

- first complete evidence run: `34607849986`;
- branch at first audit: `phase-2/modern-linux-build-audit`;
- Ubuntu: 24.04.5 LTS x86-64;
- kernel: 6.17.0-1022-azure;
- GCC/G++: 13.3.0;
- GNU Make: 4.3;
- pkg-config: 1.8.1;
- first-run audit artifact SHA-256 digest reported by GitHub: `df7228bf131e213fa03c37e60fd651c2c48c2af4a84ecba148dfbc39961324fd`.

The complete per-stage logs are produced by `scripts/phase2-build-audit.sh` and uploaded by `.github/workflows/phase2-linux-build-audit.yml`.

Review hardening added two reproducibility guarantees after the first evidence run:

1. both `make clean` and `make clean-drivers` must succeed before build stages run, and `driver/build/` is removed so a previous aggregate driver build cannot affect a rerun;
2. the workflow uploads the partial or complete `build-audit/` directory with `if: always()` even when the audit command exits nonzero.

The committed workflow runs on pull requests, manual dispatches, and pushes to the maintained `main` branch.

## Dependency result

The development-package set inherited from C-Psycle's `readme.txt` installs successfully on Ubuntu 24.04 when `pkg-config` is installed explicitly.

The first audit resolved these modules:

| pkg-config module | observed version |
| --- | --- |
| `lua` | 5.4.6 |
| `lua5.4` | 5.4.6 |
| `lilv-0` | 0.24.22 |
| `freetype2` | 26.1.20 |
| `fontconfig` | 2.15.0 |
| `x11` | 1.8.7 |
| `xft` | 2.3.6 |
| `xext` | 1.3.4 |
| `xmu` | 1.1.3 |
| `alsa` | 1.2.11 |
| `jack` | 1.9.21 |
| `sdl2` | 2.30.0 |
| `fluidsynth` | 2.3.4 |

No missing development package was the first blocker in this run. Audit report generation is shell-only and adds no Python runtime dependency.

## Stage matrix

| Stage | Result | Interpretation |
| --- | --- | --- |
| top-level `make clean` | PASS | historical clean path is reproducible |
| `make clean-drivers` | PASS | driver objects/shared libraries are explicitly cleaned before the independent audit |
| `container/src` | FAIL | direct modern-C linkage conflict; primary blocker C-01 |
| `thread/src` | PASS | static library builds |
| `script/src` | PASS | Lua 5.4 headers resolve; builds with warnings |
| `file/src` | PASS | static library builds |
| `dsp/src` | FAIL | signed/unsigned size-type mismatch; primary blocker D-01 |
| `ui/src` X11/Xft | PASS | X11 UI implementation compiles on GCC 13.3 |
| `luaui/src` | PASS | Lua UI static library builds |
| `audio/src` | FAIL | player declaration mismatch plus intentional VST2-header boundary |
| ALSA driver | BLOCKED | source compiles; core libraries are unavailable and clean direct sub-build also exposes the historical output-directory assumption |
| ALSA MIDI driver | BLOCKED | source compiles; `libcontainer` / `libdsp` unavailable |
| JACK driver | BLOCKED | source compiles; core libraries unavailable and direct sub-build assumes `driver/build/` |
| SDL2 driver | BLOCKED | source compiles; core libraries unavailable and direct sub-build assumes `driver/build/` |
| event-joystick driver | BLOCKED | source compiles; core libraries unavailable and direct sub-build assumes `driver/build/` |
| `psyplayer` | BLOCKED | build hierarchy reaches failing core prerequisites before player link |
| native X11 host | BLOCKED | build hierarchy reaches failing core prerequisites before host link |
| native plugin set | BLOCKED | many plugin sources compile; dependent links fail because `libcontainer` / `libdsp` are unavailable |
| aggregate drivers | BLOCKED | aggregate output directory is created, then links fail on unavailable core libraries |
| top-level `make all` | BLOCKED | aggregate build reproduces the known core prerequisites rather than introducing a distinct top-level defect |

`PASS` means the imported target completed without source changes. `FAIL` means the stage itself returned nonzero after its declared prerequisites were available. `BLOCKED` means the target was still exercised but returned nonzero while one or more declared core prerequisites were unavailable. This distinction is generated directly by the harness rather than manually inferred afterward.

## CI and exit-status semantics

The audit runs every stage it can before deciding the process result. After `summary.md` is complete, any `FAIL` or `BLOCKED` build stage makes `scripts/phase2-build-audit.sh` exit nonzero.

Therefore the current Phase 2 baseline is expected to produce a **failing build-audit check** until Phase 3 clears the documented blockers. This is intentional: a successful artifact upload means evidence was preserved; it does not mean Psycle currently builds end to end.

The artifact upload step uses `if: always()` and `if-no-files-found: warn`, so unexpected harness/report failures do not discard logs that were already collected.

## Primary blocking failures

### C-01 — container inline/external linkage conflict

Target:

```text
cpsycle/container/src
```

First causal error:

```text
configuration.c:18:31: error: static declaration of
‘psy_configurationhints_make’ follows non-static declaration
```

`configuration.h` declares:

```c
psy_ConfigurationHints psy_configurationhints_make(const char* svg);
```

while `configuration.c` defines it with the project's `INLINE` macro.

This is a narrow declaration/linkage inconsistency exposed by the modern C toolchain. It blocks `libcontainer.a` and therefore most higher-level targets.

Related warning observed in the same target:

```text
tokenizer.c:286: warning: implicit declaration of function ‘stricmp’
```

`stricmp` is a Windows-style compatibility assumption and should be routed through an existing portable case-insensitive comparison or a Linux equivalent in a focused compatibility fix.

### D-01 — DSP `intptr_t` / `uintptr_t` contract mismatch

Target:

```text
cpsycle/dsp/src
```

First causal error:

```text
operations-sse2.c:166:6: error: conflicting types for ‘dsp_movmul’
```

The log also reports assignment of a function using an `intptr_t` count to a function pointer expecting `uintptr_t`.

This is a concrete signedness/type-width contract mismatch, not a reason to change the DSP architecture. The declarations and implementations need one consistent count type.

### A-01 — audio player `const` contract mismatch

Target:

```text
cpsycle/audio/src
```

First non-VST source error:

```text
player.c:925:6: error: conflicting types for ‘psy_audio_player_fade_out’
```

The header declares:

```c
void psy_audio_player_fade_out(const psy_audio_Player*);
```

while the implementation accepts:

```c
void psy_audio_player_fade_out(psy_audio_Player* self)
```

The function mutability contract must be made consistent.

### A-02 — VST2 source is still unconditional after Phase 1 header omission

The audio build also reports:

```text
fatal error: aeffectx.h: No such file or directory
```

from the VST2-related sources.

This is expected evidence from the Phase 1 licensing boundary: the legacy Steinberg-derived VST2 headers were intentionally omitted from the public baseline, but the current Linux audio makefile still compiles the VST2 translation units unconditionally.

The correct next step is **not** to restore unverified SDK headers. The Linux build needs an explicit feature boundary that excludes legacy VST2 hosting when those headers are unavailable. VST2 preservation remains a separately documented Phase 6 question.

## Build-order and path assumptions

### B-01 — driver sub-builds assume parent output directory

ALSA, JACK, SDL2 and event-joystick source compilation reached the link step, then direct per-driver builds failed with:

```text
cannot open output file ../build/<driver>.so: No such file or directory
```

The aggregate `cpsycle/driver/makefile` creates `driver/build/`, but the individual driver makefiles do not. The hardened audit explicitly removes `driver/build/` after `make clean-drivers`, ensuring repeated local runs continue to expose this clean-checkout build-order/path assumption instead of inheriting a directory from an earlier aggregate build.

### B-02 — downstream links assume core static libraries already succeeded

ALSA MIDI, aggregate drivers and many native plugin targets reach their link commands and then fail with:

```text
cannot find -lcontainer
cannot find -ldsp
```

These are downstream effects of C-01 and D-01. The generated summary records them as `BLOCKED` when the core prerequisite stages failed.

### B-03 — duplicate plugin aggregate entry

The imported plugin aggregate makefile reports:

```text
target 'legasynth/src' given more than once in the same rule
```

This does not currently stop `make`, but it is a concrete historical makefile defect worth removing when build repair begins.

## X11 / Xft result

`make -C ui/src` completes successfully on Ubuntu 24.04 / GCC 13.3 using the imported X11 implementation under:

```text
cpsycle/ui/src/imps/x11/
```

This is an important Phase 2 result: the project does **not** need a new GUI toolkit merely to obtain a modern Linux compilation baseline.

Warnings remain, notably incompatible callback-pointer types and ignored `fread` results. They are recorded rather than globally suppressed.

## Lua result

Both of these modules are visible through `pkg-config`:

```text
lua
lua5.4
```

The script library and Lua UI library build successfully against Lua 5.4.6.

The script layer emits a warning involving `luaL_newlib` and `sizeof` on an array function parameter. It is non-blocking in the Phase 2 reference build and should be reviewed separately rather than suppressed globally.

## Lilv / LV2 result

`lilv-0` 0.24.22 is present and the audio compile command successfully resolves the Lilv include path. The bulk audio compile reaches `lv2param.c` and `lv2plugin.c` without an LV2-specific missing-header error before unrelated A-01/A-02 failures stop the library build.

Full LV2 link/runtime verification remains blocked by the audio-library failures. Phase 2 therefore records the integration point as **headers/tooling present; final library link blocked upstream**.

## Linux driver result

The audit compiles driver source far enough to reach linking for:

- ALSA;
- ALSA MIDI;
- JACK;
- SDL2;
- event joystick.

The observed failures are output-directory/core-library prerequisites described as B-01/B-02, not missing Linux API headers. This is evidence that the existing driver architecture remains usable as the starting point.

## `psyplayer` result

The standalone player build is present and reproducible as a distinct target, but its build hierarchy depends on `container`, `dsp` and `audio`. It currently stops at the known core prerequisites before `psyplayer` can link and is therefore reported as `BLOCKED`.

This makes `psyplayer` a good Phase 3 smoke target once C-01, D-01, A-01 and the VST2 feature boundary are repaired.

## Warning classes recorded for follow-up

The first audit also exposes non-blocking issues that should not be hidden by broad compiler flags:

- implicit `stricmp` declaration on Linux;
- ignored `fread` return values in UI/audio serialization paths;
- incompatible callback/function-pointer types in UI code;
- Lua 5.4 `sizeof` warning around `luaL_newlib` use;
- potential format overflow reported in `psy2loader.c`;
- duplicate `legasynth/src` aggregate make target.

Warnings that affect file parsing or function-pointer type safety deserve focused review after the primary build blockers are removed.

## What Phase 2 establishes

1. The documented development packages are obtainable on a current Ubuntu reference runner.
2. The historical make-based architecture is still executable and diagnosable; replacing it wholesale is not justified by the audit.
3. X11/Xft, Lua UI, thread, script and file layers already build.
4. The first blockers are narrow source-contract and feature-boundary issues rather than architectural failures.
5. Linux driver source reaches linking with modern ALSA/JACK/SDL2 headers.
6. `psyplayer`, host and plugin failures are largely downstream of a small number of core blockers.
7. The VST2 failure is an intentional licensing/build-feature boundary, not a reason to reintroduce omitted SDK-derived headers.
8. The audit can be rerun without stale driver artifacts influencing results, and its generated summary preserves `PASS` / `FAIL` / `BLOCKED` causality.

## Handoff to Phase 3

Phase 3 should begin with small compatibility PRs in this order:

1. resolve C-01 and the Linux `stricmp` portability warning;
2. resolve D-01 without changing DSP behaviour;
3. resolve A-01;
4. make legacy VST2 compilation conditional on an explicitly available/legal VST2 SDK boundary;
5. make individual driver output paths self-contained;
6. rebuild `psyplayer` before the full host;
7. rebuild drivers and native machines;
8. launch the existing X11 host and begin runtime/audio validation.

No evidence from Phase 2 supports rewriting Psycle, replacing X11, replacing the audio architecture, or abandoning the existing native machines.
