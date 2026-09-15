# Phase 6 Reference Provenance

## Purpose

Phase 6 begins by making the compatibility comparison reproducible **before** importing or modifying the C++ reimplementation.

The governing rule remains:

> **Original Psycle behaviour is the target; historical reimplementations are candidate engines and donors, not automatic authorities.**

## Primary original-Psycle reference

The primary behavioural reference is the official stable **Psycle 1.12.0** release published on 2014-10-05.

Primary executable oracle:

```text
PsycleInstallerx86-1.12.0.exe
```

Authoritative SourceForge locations:

```text
https://sourceforge.net/projects/psycle/files/psycle/1.12/
https://sourceforge.net/projects/psycle/files/psycle/1.12/PsycleInstallerx86-1.12.0.exe/download
https://sourceforge.net/p/psycle/news/2014/10/psycle-1120-officially-released/
```

The 32-bit build is primary because it best matches the historical native/VST2 ecosystem. The official x64 build may be used as a same-release architecture cross-check, but x86/x64 results must remain distinct when behaviour differs.

### Frozen binary identity

The Phase 6 discovery run established:

```text
file:    PsycleInstallerx86-1.12.0.exe
size:    9322919 bytes
sha256:  f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769
```

`scripts/phase6-upstream-audit.sh` requires both the exact size and SHA-256. A changed SourceForge payload fails the provenance gate.

The workflow downloads the official executable only long enough to validate PE `MZ` identity and the frozen hash/size, then deletes it. The installer is neither committed nor uploaded as a CI artifact.

Later 1.12.1/1.12.2 beta builds remain useful secondary references for version-specific fixes, but they do not silently replace the stable 1.12.0 matrix result.

### Observation environment

Accepted original-Psycle observations must record:

- exact Psycle version/build;
- native Windows version/architecture or explicitly identified secondary Wine environment;
- isolated Psycle configuration;
- exact machine/plugin set;
- exact song/sample/test fixture;
- observation/render procedure;
- produced receipt, render, hash, screenshot/log or other reproducible evidence where possible.

Wine may be used as a secondary reproducibility environment. A platform-sensitive Wine-only difference must not automatically be attributed to original Psycle itself.

No PASS / DIFFERENT claim may rely only on recollection.

## Frozen C++ reimplementation build-source snapshot

The first candidate engine snapshot is the **repository-wide SourceForge SVN revision `r12005`**:

```text
https://svn.code.sf.net/p/psycle/code/trunk
```

The audit now pins the complete build-source family needed for the historical C++ player path, including the top-level `universalis` support project discovered during review.

All six paths were successfully exported together at `-r 12005` on clean Ubuntu 24.04 GitHub Actions runners.

| Component | Last changed at/before r12005 | Files | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| `universalis` | r12004 | 83 | `827586daad2efcfbc10466394670a4a0e5f208da94afb7b3bf72239d396a618e` |
| `psycle-core` | r10901 | 108 | `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` |
| `psycle-audiodrivers` | r12004 | 36 | `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` |
| `psycle-helpers` | r12004 | 68 | `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` |
| `psycle-player` | r10725 | 7 | `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` |
| `psycle-plugins` | r12004 | 634 | `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` |

Pinned paths:

```text
https://svn.code.sf.net/p/psycle/code/trunk/universalis
https://svn.code.sf.net/p/psycle/code/trunk/psycle-core
https://svn.code.sf.net/p/psycle/code/trunk/psycle-audiodrivers
https://svn.code.sf.net/p/psycle/code/trunk/psycle-helpers
https://svn.code.sf.net/p/psycle/code/trunk/psycle-player
https://svn.code.sf.net/p/psycle/code/trunk/psycle-plugins
```

The differing `Last Changed Rev` values do not mean the snapshot mixes revisions. `r12005` is the repository-wide observation revision; individual paths simply had their last edit at different earlier revisions.

`qpsycle` / `qpsycle2` remain historical UI donors but are not part of this first engine snapshot because engine parity precedes Phase 8 Qt UI work.

## Automated provenance receipt

Run:

```bash
scripts/phase6-upstream-audit.sh phase6-upstream-audit
```

The audit fails if any frozen installer identity, component file count, last-changed revision or component manifest changes. It sets `LC_ALL=C` before sorting so the manifest identity is independent of host locale.

It retains only:

- SVN metadata;
- sorted file-hash manifests;
- licensing/notice filename candidates;
- broad dependency-path hints;
- redistribution-review filename hints;
- short provenance-relevant text matches.

All exported source and downloaded binary material lives in a temporary directory and is deleted before artifact upload.

### Discovery / hardening evidence

Initial five-component discovery:

```text
workflow run: 35005522858
artifact id: 10410908445
artifact sha256: 8c812ade65c82749442dbcc84114c0fa8723a126af5d1e28d7fc190c61fdb0fa
```

Hardened notice-hint pass:

```text
workflow run: 35006165361
artifact id: 10411079593
artifact sha256: d1cd408b7bc703895b969452a128f5571474739fd45cc2968431094cce5eb305
```

`universalis` discovery after review identified the missing required build input:

```text
workflow run: 35006794691
artifact id: 10411942992
artifact sha256: 937522f061519202bd07c1392e39fe420f45c94f2a3663aa2793181cf5bfe372
```

That run established `universalis` as r12004 / 83 files / manifest `827586daad2efcfbc10466394670a4a0e5f208da94afb7b3bf72239d396a618e` at the repository-wide r12005 observation point.

A final exact-head run is required after freezing that identity.

## Licensing and import boundary

The official Psycle 1.12.0 readme describes Psycle as open source and records the project's historical licensing intent. That is useful project-level evidence but is **not treated as a blanket redistribution grant for every bundled file** in the sibling component trees.

The file-level receipt indicates that `universalis`, `psycle-player`, much of `psycle-core`, and the Linux-facing driver code carry explicit GPL-2-or-later Psycle notices. It also exposes concrete mixed-provenance boundaries that require sanitization.

Definite or conservative first-import exclusions/quarantines currently include:

- nine prebuilt DLLs under `psycle-plugins/closed-source/`;
- three unresolved historical `.psy` songs;
- `psycle-plugins/src/psycle/plugins/y_midi/gmnames.h`, which identifies itself as Steinberg VST Plug-Ins SDK material;
- `psycle-core/src/seib/vst/`, pending expression-level VST SDK review;
- `psycle-audiodrivers/src/asio/`, pending expression-level ASIO provenance review.

Helper/STK/LADSPA and other third-party boundaries require their compatible notices to be preserved or restored explicitly.

See [PHASE6_CPP_IMPORT_AUDIT.md](PHASE6_CPP_IMPORT_AUDIT.md).

**Current public source import status: HOLD pending the exact sanitized retained/omission manifest.**

No C++ source is imported by this PR. A future sanitized import must have an explicit omission/replacement inventory and its own archival/canonical baseline identity; it must not mutate the existing C-Psycle baseline.

## Relationship to original Psycle

Historical maintainer evidence says the multiplatform C++ version could play existing songs but remained incomplete and developed incompatibilities. Therefore:

- Psycle 1.12.0 x86 is the primary behavioural reference;
- the six-component r12005 build-source snapshot is the first candidate engine input;
- C-Psycle r12005 remains a separate tested donor/oracle;
- reproducible differences are evidence to classify, not automatically defects in either side.

## Phase 6A status

Completed in this PR:

- [x] select the primary original-Psycle reference version/build;
- [x] freeze its official SourceForge origin, size and SHA-256;
- [x] define the observation/evidence rules;
- [x] identify the complete six-component historical C++ build-source set, including `universalis`;
- [x] pin all six paths to repository-wide SVN r12005;
- [x] freeze component file counts, last-changed revisions and manifest hashes;
- [x] establish receipt-only CI that redistributes neither source nor original binaries;
- [x] review filename/dependency/provenance notice hints;
- [x] create the initial `PSYCLE_CORE_PARITY.md` evidence matrix;
- [x] record the import disposition and concrete sanitization blockers;
- [x] define the conservative first sanitized-import policy.

Still required in Phase 6B before candidate-engine execution evidence can begin:

- [ ] materialize the exact retained-file and omission/replacement manifest;
- [ ] restore/add any required compatible third-party notices;
- [ ] create the sanitized C++ archival/canonical baseline;
- [ ] demonstrate that the sanitized set builds `psycle-player` on Linux or document the next narrow build blockers.
