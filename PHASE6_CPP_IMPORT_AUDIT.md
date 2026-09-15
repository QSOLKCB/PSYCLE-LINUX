# Phase 6 C++ Import Audit

## Purpose

This document records the first redistribution/provenance review of the pinned C++ reimplementation snapshot used by Phase 6.

Pinned repository revision:

```text
SourceForge SVN r12005
https://svn.code.sf.net/p/psycle/code/trunk
```

The snapshot identity is frozen by `scripts/phase6-upstream-audit.sh`; see `PHASE6_REFERENCE_PROVENANCE.md` for the exact manifest hashes.

## Disposition

**Public source import status: HOLD.**

The snapshot is reproducible and technically suitable for audit, but the first receipt exposes material that must be omitted or separately cleared before the five trees can be copied into this public repository.

This is not a conclusion that the Psycle-authored C++ source is non-redistributable. It is a conclusion that **the repository-wide component snapshot is mixed-provenance and cannot be mirrored mechanically without a file-level sanitization pass**.

## Snapshot identity

| Component | Last changed at/before r12005 | Files | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| `psycle-core` | r10901 | 108 | `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` |
| `psycle-audiodrivers` | r12004 | 36 | `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` |
| `psycle-helpers` | r12004 | 68 | `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` |
| `psycle-player` | r10725 | 7 | `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` |
| `psycle-plugins` | r12004 | 634 | `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` |

The different `Last Changed Rev` values are expected: `r12005` is the repository-wide observation revision, while individual component paths were last modified at different earlier revisions.

## Definite public-import exclusions discovered by filename inventory

### Closed-source/prebuilt plugin binaries

The pinned `psycle-plugins` tree contains nine `.dll` files under an explicitly named `closed-source/` area:

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

These are **not eligible for a mechanical public import** merely because they are present in SourceForge SVN. Any future sanitized import must omit them unless a clear redistribution basis is documented individually.

### Historical `.psy` songs

The pinned `psycle-plugins` tree also contains:

```text
src/psycle/plugins/jme/blitzn/songs/Rm-Im_in_a_place_i_dont_belong.psy
src/psycle/plugins/jme/blitzn/songs/voskomo_-_hawkeye_loader.psy
src/psycle/plugins/jme/gamefxn/songs/example.psy
```

The same song paths were already treated conservatively during the C-Psycle provenance audit because composition/sample redistribution permission was not established. They remain excluded from any new public import unless permission is established.

## Areas requiring targeted source/header review

The receipt deliberately identifies paths for review without treating path names as licensing conclusions.

### `psycle-audiodrivers`

ASIO-related files are present:

```text
src/asio/asio.cpp
src/asio/asio.private.hpp
src/asio/asiodrivers.cpp
src/asio/asiolist.cpp
src/psycle/audiodrivers/asiointerface.cpp
src/psycle/audiodrivers/asiointerface.h
```

Before import, determine which of these are Psycle-authored host integration and which, if any, contain or derive from restricted Steinberg ASIO SDK expressions. The C-Psycle audit already demonstrated that Steinberg SDK material must not be assumed redistributable.

### `psycle-core`

VST-hosting areas include:

```text
src/psycle/core/vsthost.cpp
src/psycle/core/vsthost.h
src/psycle/core/vstplugin.cpp
src/psycle/core/vstplugin.h
src/seib/vst/CVSTHost.Seib.cpp
src/seib/vst/CVSTHost.Seib.hpp
src/seib/vst/CVSTPreset.cpp
src/seib/vst/CVSTPreset.hpp
src/seib/vst/EffectWnd.cpp
src/seib/vst/EffectWnd.hpp
src/seib/vst/JBridgeEnabler.cpp
src/seib/vst/JBridgeEnabler.hpp
```

These need authorship/license review separately from the later Phase 9 clean-room VST2 ABI work. A Psycle-owned VST host can be valuable while any Steinberg-derived SDK definitions remain excluded.

The tree also contains `src/psycle/core/ladspa.h`, whose upstream licensing boundary should be made explicit if retained.

### `psycle-plugins`

The first filename-level license candidates include component-local notices for Audacity-derived material, JME machines and Vincenzo De Masi plugins. STK-derived machine sources are also present and require their own preserved upstream notice/provenance treatment.

Graphics such as plugin bitmaps/JPEGs are not assumed to share source-code licensing automatically and require asset provenance review where they would be redistributed.

## Project-level licensing evidence

The official Psycle 1.12.0 release readme describes Psycle as open source and explains the historical licensing intent. It also says Arguru's Psycle 1.0 sources were public domain while later team policy was intended to keep Psycle source freely visible/modifiable and distributable as derived Psycle work.

That historical prose is useful evidence of project intent, but it is **not used here as a blanket license override for every file in the r12005 sibling trees**. In particular, it cannot by itself clear bundled third-party code, binaries, songs, SDK-derived material or assets.

The automated receipt found no root `COPYING*` / `LICENSE*` / `LICENCE*` / `NOTICE*` candidate in `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, or `psycle-player`. This observation does **not** prove those sources lack licensing notices: the next audit pass also records provenance-relevant source/header text hints so file-level notices can be reviewed without mirroring the source first.

## Sanitized-import rule

A future C++ import must follow the same discipline used for the C-Psycle baseline:

1. start from the frozen r12005 manifests in this document;
2. classify file-level authorship/license notices;
3. omit restricted or unresolved binaries, SDK material, songs and assets;
4. preserve upstream notices for retained code;
5. independently replace only where technically necessary and provenance-safe;
6. record exact omission/replacement paths and resulting file-count arithmetic;
7. create a separate archival/canonical baseline identity for the C++ family;
8. never mutate or conflate the existing `cpsycle-r12005-baseline` record.

## Next acceptance gate

Before the import HOLD can be lifted:

- [ ] review the new `notice-hints.txt` receipts for all five component trees;
- [ ] classify ASIO/VST/Seib/LADSPA/STK and other third-party boundaries;
- [x] identify prebuilt/closed-source binary exclusions;
- [x] identify unresolved historical-song exclusions;
- [ ] produce the proposed exact sanitized omission list;
- [ ] verify the sanitized tree can still support the Phase 6B Linux player build or document required legal/provenance-safe replacements.

Until those items are complete, Phase 6 should continue with **audit evidence**, not a bulk source import.
