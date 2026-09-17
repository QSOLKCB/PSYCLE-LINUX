# Phase 6 C++ Import Audit

## Status

**Phase 6B sanitized import/build gate: COMPLETE. Phase 6C behavioural evidence collection is active.**

This document records the provenance and redistribution boundary for the pinned C++ reimplementation used by the active Psycle-on-Linux track.

Pinned upstream observation point:

```text
SourceForge SVN r12005
https://svn.code.sf.net/p/psycle/code/trunk
```

Frozen sanitized Phase 6B selection identity:

```text
00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a
```

The committed candidate source lives under:

```text
psycle-cpp-r12005-sanitized/
```

The separate C-Psycle oracle baseline remains `cpsycle-r12005-baseline`; the two histories are not conflated.

## Upstream snapshot identity

| Component | Last changed at/before r12005 | Upstream files | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| `universalis` | r12004 | 83 | `827586daad2efcfbc10466394670a4a0e5f208da94afb7b3bf72239d396a618e` |
| `psycle-core` | r10901 | 108 | `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` |
| `psycle-audiodrivers` | r12004 | 36 | `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` |
| `psycle-helpers` | r12004 | 68 | `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` |
| `psycle-player` | r10725 | 7 | `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` |
| `psycle-plugins` | r12004 | 634 | `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` |

`r12005` is the repository-wide observation revision. Different `Last Changed Rev` values only mean those paths had not changed again by r12005.

## Sanitized baseline result

Phase 6B materialized the exact retained/omitted selection from the six frozen manifests.

```text
upstream files:          936
retained component files: 291
omitted/quarantined:      645
baseline sha256:          00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a
```

The retained component set is verified by `scripts/phase6b-verify-committed-source.sh`. Historical qmake/build-system support is verified separately so compatibility repairs do not silently rewrite the frozen component identity.

Narrow modern-toolchain compatibility edits are allowed only through exact reversible normalization in the verification gates. Current approved repairs include:

- `psycle-helpers/src/psycle/helpers/endiantypes.hpp`: direct `<cmath>` dependency plus standard-qualified math calls;
- `psycle-helpers/src/psycle/helpers/filetypedetector.cpp`: direct `<cstring>` dependency plus `std::strncmp`;
- Linux qmake Boost linkage: removal of obsolete binary `boost_signals` linkage where retained source uses header-only Boost.Signals2.

The verifier reconstructs the historical bytes for identity checking; unrelated drift still fails.

## Retained historical build support

Two build inputs are required by the historical Linux player path but are staged after the frozen component-source verification rather than silently folded into the 291-file selection.

### Native plugin interface

The build stages the authentic historical header:

```text
cpsycle/plugins/psycle/plugin_interface.hpp
```

from repository archival commit:

```text
1863c4177d38d37e8eba9e4eb8857e5023debe6d
```

with exact Git blob identity:

```text
2cf4d8f756fa58098fd84b7b44d0ebe1350594c5
```

It is placed at the historical include location required by the C++ core:

```text
psycle-cpp-r12005-sanitized/psycle-plugins/src/psycle/plugin_interface.hpp
```

### `diversalis`

The historical build also stages `trunk/diversalis` at SourceForge SVN r12005. It is fetched and hashed during CI rather than being confused with the six-component retained-selection identity.

## Provenance boundary

The sanitized import preserves the project-authored GPL notices and compatible third-party lineage needed by the retained source while keeping known or conservatively unresolved material outside the baseline.

### Excluded closed-source binaries

Nine prebuilt DLLs under `psycle-plugins/closed-source/` remain excluded unless a redistribution basis is documented individually.

### Excluded historical songs

These upstream songs remain excluded because composition/sample redistribution permission was not established:

```text
src/psycle/plugins/jme/blitzn/songs/Rm-Im_in_a_place_i_dont_belong.psy
src/psycle/plugins/jme/blitzn/songs/voskomo_-_hawkeye_loader.psy
src/psycle/plugins/jme/gamefxn/songs/example.psy
```

Phase 6C therefore starts with project-authored fixtures and only later accepts redistributable/permissioned historical songs.

### Steinberg-derived / Windows SDK boundary

The following remain outside the first candidate baseline:

```text
psycle-core/src/seib/vst/
psycle-audiodrivers/src/asio/
psycle-plugins/src/psycle/plugins/y_midi/gmnames.h
```

`gmnames.h` identifies itself as VST Plug-Ins SDK material. The Seib VST subtree and Windows ASIO subtree remain quarantined pending their dedicated expression/provenance review. None is needed for the Phase 6 native Linux engine/player parity work.

VST2 restoration remains Phase 9 scope and must not be achieved by reintroducing excluded SDK expressions.

## Component notes

### `universalis`

Required historical support project. The r12005 receipt is dominated by Psycle project GPL-2-or-later notices and contains no binary/song redistribution hints relevant to the retained baseline.

### `psycle-core`

The retained engine source carries Psycle GPL-2-or-later notices. Psycle-owned native/song/sequence/player machinery is available for Phase 6C. The Seib VST subtree remains excluded.

### `psycle-audiodrivers`

Linux-facing ALSA/JACK/general driver code is retained where cleared. The Windows ASIO subtree remains excluded.

### `psycle-helpers`

Mixed-origin helper code remains subject to its component-local provenance/notice requirements. The baseline preserves compatible lineage such as zlib-licensed SSE math, Mersenne Twister and FFT attribution rather than applying one blanket label.

### `psycle-player`

The historical CLI player is retained and is now the first executable candidate used by Phase 6C. It supports the `dummy` output driver, making project/song-load evidence practical in headless CI.

### `psycle-plugins`

The 634-file upstream plugin tree was not imported wholesale. The first candidate baseline keeps the minimum audited native API surface required by the engine/player work; broader machine history remains covered by the C-Psycle preservation corpus and can be imported later only through explicit provenance-safe slices.

## Build evidence

Phase 6B now proves more than source materialization:

- sanitized source identity reproduction: PASS;
- qmake/build-support identity verification: PASS;
- authentic historical native plugin interface staging: PASS;
- r12005 `diversalis` staging: PASS;
- qmake configuration on Ubuntu 24.04: PASS;
- `universalis` / helpers / audio-driver / core chain build: PASS;
- historical `psycle-player` link: PASS;
- `psycle-player --help`: PASS;
- `psycle-player --version`: PASS.

Final green historical-player evidence before Phase 6C:

```text
workflow: Phase 6B historical psycle-player build
run:      35196962690
result:   success
```

The build blockers discovered on the way—missing math declarations, missing C string declaration, missing historical plugin-interface header and obsolete Boost.Signals binary linkage—were repaired narrowly and are now captured by CI rather than left as undocumented local folklore.

## Phase 6B completion checklist

- [x] Materialize exact retained-file and omission arithmetic.
- [x] Preserve/restore compatible provenance and third-party notices.
- [x] Create a separate sanitized C++ baseline identity.
- [x] Keep VST/ASIO/binary/song quarantine material out.
- [x] Reproduce the historical Debian/Linux qmake build path.
- [x] Build `psycle-player` with its required support components.
- [x] Record and repair concrete modern compiler/linker blockers narrowly.
- [x] Verify the resulting historical player in Ubuntu 24.04 CI.
- [x] Establish a headless-capable `dummy` driver path for Phase 6C fixture execution.

## Phase 6C handoff

The import question is no longer the blocker. The active question is behavioural compatibility.

Phase 6C uses:

- `phase6c/compatibility-matrix.json` as the machine-readable contract inventory;
- `scripts/phase6c-validate-matrix.py` to prevent unsupported parity claims;
- `.github/workflows/phase6c-compatibility-matrix.yml` as the maintained candidate evidence lane;
- project-authored PSY2/PSY3 fixtures as the first shared executable inputs;
- the pinned original Psycle 1.12.0 x86 build as the behavioural authority before any row may become PASS / DIFFERENT / MISSING.

**Result:** the provenance-safe C++ candidate is imported and buildable. Phase 6C can now measure it rather than speculate about it.
