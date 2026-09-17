# Source Provenance

PSYCLE-LINUX is a continuation and Linux-porting effort built from the existing Psycle source lineage. This file records the source baselines selected for the project and the rules for preserving their history.

## Selected Initial Baseline

The initial technical reference supplied for the port is:

- Project: Psycle / C-Psycle
- Upstream host: SourceForge
- Revision: `r12005`
- Revision-pinned tree: `https://sourceforge.net/p/psycle/code/12005/tree/trunk/cpsycle/`
- Revision-pinned snapshot: `https://sourceforge.net/p/psycle/code/12005/tree/trunk/cpsycle/?format=zip`
- Canonical SVN path: `https://svn.code.sf.net/p/psycle/code/trunk/cpsycle`
- Snapshot/archive label: `r12005-trunk-cpsycle`
- Archive filename: `psycle-code-r12005-trunk-cpsycle.zip`
- SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`

The revision-pinned SourceForge URLs above are the retrieval references for this baseline. Do not use the mutable `HEAD` tree when reproducing the selected source.

The same source revision can also be exported directly with Subversion:

```bash
svn export -r 12005 https://svn.code.sf.net/p/psycle/code/trunk/cpsycle cpsycle-r12005
```

The SHA-256 identifies the exact SourceForge ZIP archive used during the initial PSYCLE-LINUX documentation and source audit. Verify a downloaded snapshot before treating it as the selected baseline:

```bash
sha256sum psycle-code-r12005-trunk-cpsycle.zip
```

Expected result:

```text
2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc  psycle-code-r12005-trunk-cpsycle.zip
```

A raw `svn export` reproduces the r12005 source revision but is not expected to have the ZIP archive's byte-for-byte checksum because the archive container/metadata differ. Future upstream snapshots must receive their own provenance record rather than silently replacing this baseline.

### Architectural role of this baseline

The r12005 C-Psycle baseline remains the project's first audited preservation corpus, but it is **not now claimed to be the final implementation base for original Psycle on Linux**.

Upstream maintainer clarification distinguishes:

- `psycle/` — original C++/MFC Psycle and the primary behavioural/UI reference;
- the `psycle-core` family — earlier C++ cross-platform reimplementation and current candidate Linux engine;
- `cpsycle/` — later C reimplementation, retained here as a tested donor/reference implementation and regression corpus.

Phase 6A separately pinned the six-component C++ r12005 build-source family and its public-import boundary. Phase 6B then materialized and committed the provenance-cleared candidate under `psycle-cpp-r12005-sanitized/` without changing the frozen C-Psycle baseline below. Its frozen sanitized baseline SHA-256 is `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`; detailed retained/omitted-source evidence remains in `PHASE6B_SANITIZED_BASELINE.md`, `PHASE6B_CPP_THIRD_PARTY_NOTICES.md`, and `PHASE6_CPP_IMPORT_AUDIT.md`.

## Upstream Identity, Sanitized Archive, and Canonical Audited Baseline

Phase 1 records two states for different provenance purposes.

### Upstream identity and sanitized archival import

- Upstream revision: SourceForge SVN `r12005`
- Audited ZIP SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Sanitized archival ref: `archive/cpsycle-r12005-sanitized-import`

The exact upstream source identity is the pinned SourceForge revision plus the recorded archive checksum. The original bootstrap Git commit is not retained by a public archival ref because it contained artifacts later determined unsuitable for public redistribution: two prebuilt 7-Zip binaries lacking corresponding source in the bundle and eight `.psy` songs with unresolved composition/sample permissions.

The sanitized archival ref preserves the mechanically imported upstream source after all **66 public-baseline omissions** and before the Phase 1-added licensing/provenance files. It therefore contains **2,349 retained upstream paths** and none of the omitted artifacts; `orderedtable.lua` and `qsort.c` at those paths use the provenance-safe Phase 1 replacement implementations documented below. The legacy branch name `archive/cpsycle-r12005-raw-import`, if present, is intentionally repointed to the same sanitized commit and must not be used to expose the unsanitized bootstrap tree.

### Canonical audited Phase 1 baseline

- Canonical tag: `cpsycle-r12005-baseline`

The canonical tag identifies the final audited Phase 1 public baseline before any Linux compatibility fixes. This document deliberately does **not** embed the canonical commit SHA: the tag itself is the stable identifier. That avoids a self-referential record becoming stale when audit-only metadata is corrected before the baseline is frozen.

The canonical baseline is constructed from r12005 as follows:

- upstream r12005 files: **2,415**;
- intentionally omitted upstream files: **66**;
- retained upstream files: **2,349**;
- Phase 1 licensing/provenance files added: **14**;
- canonical files under `cpsycle/`: **2,363**.

The 66 whole-file omissions are:

- 41 files under the bundled Steinberg ASIO SDK 2.3 directory;
- three legacy Steinberg VST2 SDK-derived headers;
- two prebuilt 7-Zip Windows binaries (`7z.exe` and `7z.dll`) for which the r12005 C-Psycle bundle does not provide corresponding source or an equivalent same-place source-access mechanism;
- eight bundled `.psy` demo/example songs whose composition/sample redistribution permissions have not yet been established for the canonical public baseline. Their upstream paths remain documented, but the files are omitted from all public archival and canonical refs.
- six MAKK M3 plugin files whose source carries an informal restriction but no sufficiently clear redistribution grant;
- six TinyBoxes skin files (the skin readme, preset and four derived skin assets) whose note identifies derivation from 404's RGB artwork without recording redistribution/modification permission.

Two additional upstream source paths remain at their original paths but their source expressions are replaced with independently written GPL-2.0-or-later-compatible implementations: `cpsycle/luascripts/psycle/orderedtable.lua` and `cpsycle/container/src/qsort.c`. The aggregate plugin makefile is narrowed only to stop referencing the omitted M3 directory.

The fourteen Phase 1-added licensing/provenance files are:

- `cpsycle/ui/src/scintilla/License.txt`;
- `cpsycle/audio/src/LADSPA-LICENSE.txt`;
- `cpsycle/luascripts/psycle/SERPENT-LICENSE.txt`;
- `cpsycle/audio/src/EXS24-RENOISE-LICENSE.txt`;
- `cpsycle/luascripts/psycle/socket/LICENSE`;
- `cpsycle/luascripts/psycle/MOBDEBUG-LICENSE.txt`;
- `cpsycle/build-systems/mswindows-installer/INNOTOOLS-DOWNLOADER-LICENSE.txt`;
- `cpsycle/file/src/LIBXML2-COPYRIGHT.txt`;
- `cpsycle/driver/wasapi/PORTAUDIO-LICENSE.txt`;
- `cpsycle/detail/SDCC-STRLWR-LICENSE.txt`;
- `cpsycle/plugins/zephod_super_fm/ZEPHOD-SUPERFM-LICENSE-PROVENANCE.txt`;
- `cpsycle/dsp/src/FFT-SOURCES-NOTICE.txt`;
- `cpsycle/dsp/src/MODPLUG-FILTER-PROVENANCE.txt`;
- `cpsycle/audio/src/SAMPULSE-PROVENANCE.txt`;

The Scintilla restoration is required because all three imported Scintilla headers explicitly refer to a `License.txt` that is absent from the r12005 C-Psycle snapshot. The inventory records the mixed copyright provenance: `Scintilla.h` and `SciLexer.h` carry 1998-2003 notices, while `Sci_Position.h` carries a 2015 notice and refers to the same named Scintilla/SciTE license file.

The LADSPA boundary file does not change `ladspa.h`; it makes the header's existing LGPL-2.1-or-later declaration explicit at the component boundary and points recipients to the complete license terms. Phase 1 also restores missing distribution/provenance notices for bundled Serpent, EXS24 For Renoise-derived code, LuaSocket, MobDebug/RemDebug, Inno Tools Downloader, libxml2-derived encoding code, PortAudio WASAPI portions, SDCC `strlwr`, Zephod SuperFM, the mixed-origin FFT implementation, and the ModPlug-derived IT filter. The musl MIT grant is restored directly in `cpsycle/detail/strcasestr.h`, so it does not add another file to the count.

The exact omission paths and deterministic reconstruction procedure are recorded in [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md).

The remainder of the baseline is intentionally mechanically close to r12005. The only Phase 1 source-expression replacements are the provenance-safe `orderedtable.lua` and `qsort.c` implementations, plus removal of the M3 aggregate-build entry necessitated by that component omission; no mass formatting, framework migration, DSP redesign, or general Linux compatibility patching is mixed into the audited baseline.

Third-party material and historical build artifacts are inventoried in [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md). An initial maintainer map is in [SOURCE_TREE.md](SOURCE_TREE.md).

## Upstream Architectural Reference

The retained r12005 documentation includes:

- `cpsycle/doc/cpsycle-developer-guide.txt`
- title: **Psycle Developer Guide**
- edition: **C-Version, Feb 2021 (unfinished)**

The imported Visual Studio 2019 solution also names `doc/cpsycle-developer-guide.docx` as a solution documentation item. This makes the retained text file strong upstream evidence of **C-Psycle** architectural intent and terminology, rather than a PSYCLE-LINUX reconstruction.

The guide records, among other things:

- compatibility with MFC-Psycle as an explicit C-Psycle goal;
- step-by-step separation of Win32-specific and platform-independent code;
- the audio-engine/UI subsystem split and runtime-loaded audio/event drivers;
- `Workspace` ownership/references including `Player`, `MachineFactory`, `PluginCatcher` and `Song`;
- the player's deliberate **256-sample** `psy_audio_MAX_STREAM_SIZE` split for Psycle-plugin compatibility;
- tracker-line splitting and `SeqTick()` compatibility behaviour inside the newer beat/event sequencer;
- the expectation that VST plugins tolerate variable process-block lengths;
- the **64-channel** native-plugin limit and `LogicalChannel` mapping;
- the host/UI bridge intended to allow platform-specific implementations;
- the 2021 state in which Visual Studio was current while GCC was described as out of date.

Because the document labels itself unfinished and describes the later C reimplementation, it is not treated as a frozen specification for the final Linux product. The project-wide compatibility/evidence order is:

1. original Psycle source/behaviour where legally and technically available;
2. trustworthy historical songs, documented formats and frozen machine/plugin contracts;
3. the selected `psycle-core` family as the candidate engine to measure and repair;
4. C-Psycle source plus the PSYCLE-LINUX regression corpus where semantics overlap;
5. the retained C-Psycle developer guide as primary architecture/intent evidence for C-Psycle itself;
6. older branches, posts, release notes and related historical donors as supporting context.

For claims specifically about the r12005 C-Psycle baseline, pinned r12005 source and reproducible C-Psycle behaviour remain authoritative over unfinished prose. See [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md) for the project-facing extraction of these constraints.

## Why This Baseline

The r12005 C-Psycle tree already contains substantial cross-platform and Linux-specific work. Relevant areas include:

- `ui/src/imps/x11/`
- `driver/alsa/`
- `driver/alsamidi/`
- `driver/jack/`
- `driver/sdl2/`
- `driver/evjoystick/`
- `host/`
- `audio/`
- `player/`
- `plugins/`
- `presets/`
- `luaui/`
- `luascripts/`
- `doc/`

The top-level makefile already defines host, plugin, driver and player targets. The Linux driver makefile includes SDL2, ALSA, ALSA MIDI, JACK and Linux event joystick targets. The host and UI build files reference X11/Xft and related libraries.

That made C-Psycle a strong first preservation/Linux-validation baseline and enabled the completed Phases 0–5 regression program. It does **not** mean the final product must retain C-Psycle's engine, sequencer or X11 UI architecture. The active roadmap measures the separately imported C++ `psycle-core` family as the candidate engine while keeping all C-Psycle work as donor and compatibility evidence.

## Upstream Authorship

The r12005 `AUTHORS` file identifies Psycle as copyright 2000-2021 by a community of developers and plugin authors.

It identifies, among others:

- Josep Maria Antolin Segura — `jaz001`
- Johan Boule — `johan-boule`
- Stefan Nattkemper — `stefan001`
- ShadowBane — `baneofshadow`
- Juan Antonio Arguelles Rius — `arguru`
- Mats Hojlund — `cbr`
- Daniel Arena — `dubdub`
- Mark McCormack
- Marcin Kowalski — `FideLoop`
- Lukasz Langa — `kSh`
- Jeremy Evers — `pooplog`
- Martin Etnestad Johansen — `lobywang`
- James Redfern — `alkenstein`
- and additional contributors recorded in the upstream file.

The upstream `AUTHORS` file, copyright headers and per-plugin notices are authoritative and must be preserved with imported source.

## Arguru Machines in r12005

The selected C-Psycle snapshot contains source directories for the following Arguru machines:

- `plugins/arguru-compressor/`
- `plugins/arguru-distortion/`
- `plugins/arguru-goaslicer/`
- `plugins/arguru-reverb/`
- `plugins/arguru-synth-2f/`
- `plugins/arguru-xfilter/`

These remain historical preservation/compatibility targets. Their completed regression evidence may be reused when the selected `psycle-core`/plugin family exposes equivalent identities and contracts.

## Import Rules

When upstream source is introduced or refreshed in PSYCLE-LINUX:

1. preserve the directory structure as closely as practical in the raw import;
2. preserve `AUTHORS`, `COPYING`, copyright headers, README files and component notices;
3. do not mass-format or mechanically rename source in the import commit;
4. do not claim PSYCLE-LINUX authorship over upstream code;
5. record every intentionally omitted upstream file and why;
6. keep pinned upstream revision/checksum identities separate from sanitized archival refs and audited public-baseline tags when redistribution corrections are required;
7. keep third-party source boundaries and licensing visible;
8. record later upstream cherry-picks or source refreshes separately rather than moving frozen baseline identities;
9. give each separately imported upstream family (`cpsycle`, `psycle-core` family, etc.) its own provenance record rather than conflating their histories.

Phase 1 satisfies these rules for C-Psycle with SourceForge SVN `r12005` plus the recorded ZIP SHA-256 as the exact upstream identity, `archive/cpsycle-r12005-sanitized-import` for the public mechanically comparable source state after all omissions, and `cpsycle-r12005-baseline` for the audited public state. The canonical tag is not to be moved after Phase 1 is merged and frozen.

Phase 6B separately satisfies the candidate-family import rule through the frozen six-component r12005 manifests, explicit sanitization/omission records, retained third-party notices, and the committed `psycle-cpp-r12005-sanitized/` baseline. Neither family may be silently relabelled or have its frozen identity moved to absorb later fixes.

## Historical Community

Psycle was more than its source repository. Developers, testers, musicians, plugin authors and users shared builds, songs, machines, bug reports and knowledge through the project's community channels, including the historical `#Psycle` IRC community.

PSYCLE-LINUX aims to preserve that history respectfully while producing a maintainable Linux port.