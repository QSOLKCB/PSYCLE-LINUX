# Phase 6 Reference Provenance

## Purpose

Phase 6 begins by making the compatibility comparison reproducible **before** importing or modifying the C++ reimplementation.

This document pins the first observation candidates for both sides of the parity matrix and defines what must be true before either can be treated as authoritative evidence.

The governing rule remains:

> **Original Psycle behaviour is the target; historical reimplementations are candidate engines and donors, not automatic authorities.**

## Primary original-Psycle reference

### Version

The primary reference version is **Psycle 1.12.0**, the official stable 1.12 release published on 2014-10-05.

Primary executable build:

```text
PsycleInstallerx86-1.12.0.exe
```

Authoritative SourceForge release location:

```text
https://sourceforge.net/projects/psycle/files/psycle/1.12/PsycleInstallerx86-1.12.0.exe/download
```

Release index:

```text
https://sourceforge.net/projects/psycle/files/psycle/1.12/
```

Release announcement:

```text
https://sourceforge.net/p/psycle/news/2014/10/psycle-1120-officially-released/
```

The 32-bit build is the primary executable oracle because historical Psycle native machines and third-party VST2 usage were strongly associated with the 32-bit Windows ecosystem. The official x64 build from the same release may be used as an architecture cross-check, but results from x86 and x64 must not be silently merged when they differ.

### Why 1.12.0 rather than a later beta

SourceForge also carries later 1.12.1/1.12.2 beta material, including a July 2018 1.12.2 build that JosepMa/JAZ later described as a build he used for testing and playing songs.

Those builds are valuable secondary references for version-specific bug fixes, but **1.12.0 is the primary matrix reference because it is the official stable 1.12 release**. If a parity question is known to differ in 1.12.1/1.12.2, the later result must be recorded as a separate version-specific observation.

### Binary identity

The Phase 6 provenance workflow downloads the official SourceForge file only long enough to compute:

- effective download URL;
- file size;
- SHA-256;
- basic PE magic validation.

The executable is then deleted. It is **not** committed to this repository and is **not** uploaded in the CI artifact.

The first successful discovery run establishes the observed SHA-256. A follow-up commit must freeze that digest in the audit configuration before the executable identity is considered fully pinned.

### Observation environment

Parity results against original Psycle must record the environment used. The preferred reference environment is:

- Windows x86/x64 environment capable of running the selected 1.12.0 build natively;
- isolated Psycle configuration/profile;
- no unrecorded third-party plugins;
- exact test song/fixture identity;
- exact plugin/native-machine set;
- exact sample/audio fixture identity;
- audio-device-independent/offline rendering where the question permits it.

Wine may be used as a **secondary reproducibility environment**, but a Wine-only difference must not automatically be attributed to original Psycle itself. Platform-sensitive observations should be confirmed against a native Windows reference when practical.

No PASS / DIFFERENT result may be based only on recollection. Human historical knowledge remains useful for choosing tests, but accepted matrix entries require a versioned build/source and a reproducible artifact or procedure.

## C++ reimplementation snapshot candidate

The first coherent snapshot candidate is the **repository-wide SourceForge SVN revision `r12005`**, matching the already-audited C-Psycle baseline revision.

SVN root:

```text
https://svn.code.sf.net/p/psycle/code/trunk
```

Pinned engine-family paths:

```text
https://svn.code.sf.net/p/psycle/code/trunk/psycle-core
https://svn.code.sf.net/p/psycle/code/trunk/psycle-audiodrivers
https://svn.code.sf.net/p/psycle/code/trunk/psycle-helpers
https://svn.code.sf.net/p/psycle/code/trunk/psycle-player
https://svn.code.sf.net/p/psycle/code/trunk/psycle-plugins
```

All five paths are evaluated at the same repository revision:

```text
-r 12005
```

Using one repository-wide SVN revision prevents a pseudo-snapshot assembled from unrelated mutable `HEAD` states.

Historical maintainer evidence also identifies `qpsycle`/`qpsycle2` as UI work associated with the C++ reimplementation. They are **not** part of this first engine snapshot because the active UI plan is to establish engine parity first and implement the Linux tracker UI in Phase 8. They remain historical donors and may be audited separately if a specific behaviour or interface requires them.

Maintainer context:

```text
https://sourceforge.net/p/psycle/bugs/81/
```

## Automated snapshot receipt

Run:

```bash
scripts/phase6-upstream-audit.sh phase6-upstream-audit
```

The audit:

1. verifies every pinned component with `svn info -r 12005`;
2. exports each component into a temporary directory;
3. creates a sorted per-file SHA-256 manifest;
4. hashes that manifest to produce a tree-content identity;
5. records the component's last-changed SVN revision;
6. records candidate licensing/notice files;
7. records dependency-path hints for focused third-party review;
8. downloads and hashes the original-Psycle reference executable;
9. deletes all exported source and downloaded executable material before completion.

The uploaded CI artifact therefore contains **receipts only**, not imported upstream code or reference binaries.

## Licensing and import boundary

### Original Psycle executable

The official executable is used as an observation oracle. This project does not infer a redistribution right for the installer merely from its public availability. The installer is not mirrored in this repository.

### `psycle-core` family

The SourceForge release documentation describes Psycle as open source and records the project's historical free-source intent, but that prose is **not sufficient by itself to certify every file and bundled dependency in the five C++ component trees for republication**.

Before source is imported into PSYCLE-LINUX, the Phase 6 audit must inspect at minimum:

- component/root COPYING/LICENSE/NOTICE material;
- per-file copyright and licence headers where present;
- bundled or copied third-party code;
- Steinberg ASIO/VST material or derived headers;
- Boost and other external-library boundaries;
- prebuilt binaries/assets;
- songs/samples/presets with unclear redistribution permission.

The same preservation rule used for the C-Psycle import applies: restricted or unclear third-party material is omitted or independently replaced rather than casually copied into the public repository.

**Current import status: HOLD.**

The r12005 component paths are pinned for audit, but no C++ source is imported by this PR. Import becomes eligible only after the generated receipt is reviewed and the licensing/third-party boundary is explicitly documented.

## Relationship to original Psycle

The C++ reimplementation is not assumed to be a source-equivalent extraction of Psycle 1.12. Historical project material records that the multiplatform engine could play existing songs but diverged from the Windows application, and later project notes describe unresolved choices about how `psycle-core` should relate to the MFC engine.

Therefore:

- original Psycle 1.12.0 remains the primary behavioural reference;
- `psycle-core` r12005 is the first candidate engine snapshot;
- C-Psycle r12005 is a separate tested donor/oracle;
- differences are evidence to investigate, not automatically bugs in one side or the other.

## Phase 6A acceptance

Phase 6A is complete only when:

- the original 1.12.0 reference digest is frozen and reproducible;
- all five C++ component paths are proven to exist at r12005;
- content manifests are recorded;
- the licensing/third-party audit has a documented disposition;
- any public import contains only material cleared for redistribution;
- the resulting identities are referenced by `PSYCLE_CORE_PARITY.md` and the active-track CI.
