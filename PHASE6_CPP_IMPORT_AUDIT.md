# Phase 6 C++ Import Audit

## Purpose

This document records the first redistribution/provenance review of the pinned C++ reimplementation snapshot used by Phase 6.

Pinned repository revision:

```text
SourceForge SVN r12005
https://svn.code.sf.net/p/psycle/code/trunk
```

The snapshot identity is frozen by `scripts/phase6-upstream-audit.sh`; see `PHASE6_REFERENCE_PROVENANCE.md` for exact manifest hashes.

## Disposition

**Public source import status: HOLD pending a sanitized import manifest.**

The snapshot is reproducible and the notice-hint pass confirms that a large amount of the Psycle-authored C++ code carries explicit GPL-2-or-later notices. However, the five-tree snapshot also contains mixed-provenance material that must be omitted, quarantined, or separately documented before the source can be copied mechanically into this repository.

This is not a conclusion that the C++ reimplementation itself is non-redistributable. It is a conclusion that **the repository snapshot must be sanitized at file level**, exactly as the C-Psycle baseline was.

## Snapshot identity

| Component | Last changed at/before r12005 | Files | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| `psycle-core` | r10901 | 108 | `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` |
| `psycle-audiodrivers` | r12004 | 36 | `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` |
| `psycle-helpers` | r12004 | 68 | `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` |
| `psycle-player` | r10725 | 7 | `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` |
| `psycle-plugins` | r12004 | 634 | `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` |

`r12005` is the repository-wide observation revision. The component `Last Changed Rev` values simply record when each path last changed before that observation.

## Notice-hint results

The hardened audit does not retain the source files. It retains short provenance-relevant text matches so licensing and donor boundaries can be reviewed before import.

### `psycle-core`

The main `src/psycle/core/` implementation repeatedly declares itself free software under **GNU GPL version 2 or later** and credits members of the Psycle project.

This includes core engine areas such as song/sequence/player/machine/native-host and the Psycle-owned VST host wrapper files.

A separate subtree requires quarantine:

```text
src/seib/vst/
```

The receipt says several of these files are derived from the LGPL host `vsthost (1.16m)`, but it also contains explicit references to VST SDK 2.4 and, in `CVSTHost.Seib.hpp`, a `PluginLoader` described as coming from VST SDK `minihost.cpp`. Because we are not assuming redistribution rights for Steinberg SDK source expressions, **the Seib VST subtree is not cleared for the first public import**. It remains a Phase 9 archaeology/donor source until the expression-level boundary is reviewed.

The Psycle-owned files:

```text
src/psycle/core/vsthost.cpp
src/psycle/core/vsthost.h
src/psycle/core/vstplugin.cpp
src/psycle/core/vstplugin.h
```

carry GPL-2-or-later project notices in the receipt. They may be retained in a sanitized source baseline while VST2 compilation remains disabled until the Phase 9 clean-room ABI boundary exists.

`src/psycle/core/ladspa.h` is a third-party API boundary and must retain an explicit component-local license association if imported.

### `psycle-audiodrivers`

The Linux-facing ALSA/JACK and general driver implementation repeatedly carries **GPL-2-or-later Psycle project notices**.

The Windows ASIO-specific area is deliberately separated from the first Linux import:

```text
src/asio/asio.cpp
src/asio/asio.private.hpp
src/asio/asiodrivers.cpp
src/asio/asiolist.cpp
```

`asio.private.hpp` itself carries a Psycle GPL notice, but the receipt does not expose an equivalent notice for all three accompanying ASIO implementation files. Since ASIO is not required for the Linux player build and Steinberg ASIO provenance is historically sensitive, the conservative first sanitized baseline will **omit/quarantine `src/asio/`** pending a dedicated expression/provenance review.

The Psycle-side `src/psycle/audiodrivers/asiointerface.*` files do carry GPL project notices; they may be retained as historical Windows integration if they do not require restricted material to be redistributed, but they must not be part of the Linux build gate.

### `psycle-player`

The executable source under `src/psycle/player/` carries **GPL-2-or-later Psycle project notices**. Project/qmake metadata has no separate license file but does not introduce a new binary or third-party payload in the receipt.

This makes `psycle-player` a good first executable target once its required core/helper/API dependencies are imported safely.

### `psycle-helpers`

This component is intentionally treated as mixed-origin rather than stamped with one blanket label.

The receipt identifies:

- many Psycle-authored headers/sources under GPL-2-or-later;
- `math/sse_mathfun.h` identifying the **zlib license**;
- Mersenne Twister sources carrying their own redistribution conditions;
- FFT files preserving separate original-copyright lineage;
- music-DSP lineage in some math helpers.

These are not automatic exclusions. They require their existing permission/copyright text to be preserved and, where the current tree lacks a complete distribution notice, a component-local provenance/license record should be added just as was done in the C-Psycle Phase 1 audit.

### `psycle-plugins`

This tree is the most mixed-provenance part of the snapshot and should **not** be imported wholesale merely to make `psycle-player` compile.

The notice pass finds many GPL/Psycle machine sources and explicit local licenses for Audacity-derived and JME material. It also finds STK-derived machine sources that need their STK provenance retained.

One file is an immediate Steinberg-derived omission candidate:

```text
src/psycle/plugins/y_midi/gmnames.h
```

Its own header identifies it as `VST Plug-Ins SDK` material created by Steinberg Media Technologies. It is therefore not eligible for the first public sanitized import absent a separately established redistribution basis.

The first engine/player import should prefer the **minimum native plugin API surface actually required to compile and test the engine**, then bring historical machine sources across in audited slices rather than importing all 634 files as one legal/provenance unit.

## Definite public-import exclusions

### Closed-source/prebuilt plugin binaries

The pinned `psycle-plugins` tree contains nine `.dll` files under `closed-source/`:

```text
closed-source/cyanphase/CyanPhase_VibraSynth_1P.dll
closed-source/j-hamaide/SingleFrequency.dll
closed-source/recovered-or-reversed-engineered/arguru/arguru compressor.dll
closed-source/recovered-or-reversed-engineered/ninereeds.and.7900/NRS_7900_Fractal.dll
closed-source/sond/s_filter.dll
closed-source/sond/s_phaser.dll
closed-source/sond/s_reverb.dll
closed-source/sond/s_vld.dll
closed-source/sond/softsynth_psycle_plugin.dll
```

They are **excluded from any mechanical public import** unless a clear redistribution basis is documented individually.

### Historical `.psy` songs

```text
src/psycle/plugins/jme/blitzn/songs/Rm-Im_in_a_place_i_dont_belong.psy
src/psycle/plugins/jme/blitzn/songs/voskomo_-_hawkeye_loader.psy
src/psycle/plugins/jme/gamefxn/songs/example.psy
```

These same songs were conservatively omitted from the C-Psycle public baseline because composition/sample redistribution permission was not established. They remain excluded unless permission is established.

### Steinberg VST SDK-derived header candidate

```text
src/psycle/plugins/y_midi/gmnames.h
```

The file identifies itself as Steinberg VST Plug-Ins SDK material and is excluded from the proposed first public baseline.

## Quarantined pending expression-level review

These are not declared permanently non-redistributable; they are simply **not cleared for the first import**:

```text
psycle-core/src/seib/vst/
psycle-audiodrivers/src/asio/
```

Both are nonessential to the first native Linux `psycle-player` build goal. Deferring them lets Phase 6 establish engine/song parity without dragging Phase 9 VST2 or Windows ASIO licensing questions into the engine baseline.

## Project-level licensing evidence

The official Psycle 1.12.0 release readme calls Psycle open source, records Arguru's original Psycle 1.0 public-domain grant, and describes the later team's intent to keep Psycle source freely visible/modifiable and redistributed as derived Psycle work.

That historical prose supports the project's open-source intent, but it is **not used as a blanket override for third-party files, SDK-derived expressions, binaries, songs or assets**.

The file-level notice receipt is stronger for the candidate engine: much of the project-authored C++ source explicitly states GPL-2-or-later. The sanitized import should preserve those headers verbatim and add clear boundary notices for compatible third-party material where necessary.

## Proposed first sanitized-import policy

The next import PR should be deliberately smaller than the five-tree SourceForge snapshot:

1. import the cleared Psycle-authored portions of `psycle-core`;
2. import the cleared portions of `psycle-helpers` with their third-party notices preserved/restored;
3. import `psycle-player`;
4. import only Linux-relevant/cleared `psycle-audiodrivers` for the first build, with `src/asio/` quarantined;
5. import only the minimum audited `psycle-plugins` API surface required by the engine/player build initially;
6. keep the nine closed-source DLLs, three songs, Steinberg `gmnames.h`, Seib VST subtree and ASIO subtree out of that baseline;
7. add exact omission/replacement arithmetic and a new C++ archival/canonical baseline identity;
8. keep VST2 restoration in Phase 9 rather than using excluded historical SDK expressions to make Phase 6 compile.

## Sanitized-import rule

A future C++ import must:

1. start from the frozen r12005 manifests;
2. preserve all retained authorship/license headers;
3. record each omission/quarantine and reason;
4. restore full compatible third-party permission notices where the source only contains abbreviated lineage text;
5. independently replace material only when technically necessary and provenance-safe;
6. record resulting file-count arithmetic and content identity;
7. create separate archival/canonical refs for the C++ family;
8. never mutate or conflate `cpsycle-r12005-baseline`.

## Phase 6A audit status

Completed in this PR:

- [x] freeze the five component identities;
- [x] freeze the original Psycle 1.12.0 x86 executable identity;
- [x] review file-name licensing/dependency/redistribution hints;
- [x] review provenance-relevant source/header notice hints;
- [x] identify closed-source binary exclusions;
- [x] identify unresolved historical-song exclusions;
- [x] identify a Steinberg VST SDK-derived plugin-header exclusion candidate;
- [x] quarantine the Seib VST and ASIO subtrees from the first Linux baseline;
- [x] define the minimum/sanitized first-import policy.

Still required in the next import/build slice:

- [ ] materialize the exact proposed retained-file list and omission arithmetic;
- [ ] restore/add any required full third-party notices for retained helper/API code;
- [ ] create the sanitized C++ archival/canonical baseline;
- [ ] prove that baseline builds `psycle-player` on Linux or document the next narrow build blockers.

**Result:** Phase 6A has produced a reproducible, conservative import boundary. The correct next step is a sanitized engine/player import and build audit—not a bulk five-tree copy and not Qt UI work yet.
