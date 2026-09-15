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

The Phase 6 discovery run on GitHub Actions established:

```text
file:    PsycleInstallerx86-1.12.0.exe
size:    9322919 bytes
sha256:  f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769
```

`scripts/phase6-upstream-audit.sh` now requires both the exact size and SHA-256. A changed SourceForge payload fails the provenance gate.

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

## Frozen C++ reimplementation snapshot

The first candidate engine snapshot is the **repository-wide SourceForge SVN revision `r12005`**:

```text
https://svn.code.sf.net/p/psycle/code/trunk
```

The five component paths were successfully exported together at `-r 12005` on a clean Ubuntu 24.04 GitHub Actions runner.

| Component | Last changed at/before r12005 | Files | Manifest SHA-256 |
| --- | ---: | ---: | --- |
| `psycle-core` | r10901 | 108 | `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` |
| `psycle-audiodrivers` | r12004 | 36 | `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` |
| `psycle-helpers` | r12004 | 68 | `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` |
| `psycle-player` | r10725 | 7 | `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` |
| `psycle-plugins` | r12004 | 634 | `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` |

Pinned paths:

```text
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

The audit now fails if any frozen installer identity, component file count, last-changed revision or component manifest changes. It retains only:

- SVN metadata;
- sorted file-hash manifests;
- licensing/notice filename candidates;
- dependency-path hints;
- redistribution-review filename hints;
- short provenance-relevant text matches.

All exported source and downloaded binary material lives in a temporary directory and is deleted before artifact upload.

Initial discovery evidence:

```text
workflow run: 35005522858
artifact id: 10410908445
artifact size: 44404 bytes
artifact sha256: 8c812ade65c82749442dbcc84114c0fa8723a126af5d1e28d7fc190c61fdb0fa
```

That run passed and established the frozen identities above. A later exact-head run is required after the hardcoded pins and notice-hint audit changes.

## Licensing and import boundary

The official Psycle 1.12.0 readme describes Psycle as open source and records the project's historical licensing intent. That is useful project-level evidence but is **not treated as a blanket redistribution grant for every bundled file** in the five sibling component trees.

The first receipt already found concrete mixed-provenance material, including nine prebuilt DLLs under `psycle-plugins/closed-source/` and three historical `.psy` song files. Those cannot be mechanically mirrored into the public repository without separate clearance.

ASIO-, VST/Seib-, LADSPA-, STK- and other third-party boundaries also require file-level review.

See [PHASE6_CPP_IMPORT_AUDIT.md](PHASE6_CPP_IMPORT_AUDIT.md).

**Current public source import status: HOLD.**

No C++ source is imported by this PR. A future sanitized import must have an explicit omission/replacement inventory and its own archival/canonical baseline identity; it must not mutate the existing C-Psycle baseline.

## Relationship to original Psycle

Historical maintainer evidence says the multiplatform C++ version could play existing songs but remained incomplete and developed incompatibilities. Therefore:

- Psycle 1.12.0 x86 is the primary behavioural reference;
- the five-component r12005 C++ snapshot is the first candidate engine;
- C-Psycle r12005 remains a separate tested donor/oracle;
- reproducible differences are evidence to classify, not automatically defects in either side.

## Phase 6A status

Completed:

- [x] select the primary original-Psycle reference version/build;
- [x] freeze its official SourceForge origin, size and SHA-256;
- [x] define the observation/evidence rules;
- [x] pin all five C++ component paths to repository-wide SVN r12005;
- [x] freeze component file counts, last-changed revisions and manifest hashes;
- [x] establish receipt-only CI that redistributes neither source nor original binaries;
- [x] create the initial `PSYCLE_CORE_PARITY.md` evidence matrix;
- [x] record the initial import disposition and concrete blockers.

Still required before public source import:

- [ ] finish file-level notice/third-party review from the hardened receipt;
- [ ] produce the exact sanitized omission/replacement list;
- [ ] demonstrate that the sanitized set can support the Phase 6B Linux-player build or document provenance-safe replacements.
