# Phase 6B Sanitized C++ Baseline

## Purpose

Phase 6A froze the complete six-component SourceForge SVN `r12005` observation and established the provenance boundary. Phase 6B starts by converting those frozen manifests into an exact, machine-checkable first engine/player baseline before any historical C++ source is committed.

This is intentionally narrower than a bulk import. It answers one question first:

> Exactly which frozen upstream files belong to the first Linux `psycle-player` baseline, and exactly which files remain excluded, quarantined, or deferred?

The source of truth remains the receipts produced by `scripts/phase6-upstream-audit.sh`. `scripts/phase6b-sanitized-manifest.sh` consumes those receipts and refuses any count or content-identity drift.

## Frozen arithmetic

| Component | Upstream r12005 files | Retained for first baseline | Omitted / quarantined / deferred |
| --- | ---: | ---: | ---: |
| `universalis` | 83 | 83 | 0 |
| `psycle-core` | 108 | 100 | 8 |
| `psycle-audiodrivers` | 36 | 32 | 4 |
| `psycle-helpers` | 68 | 68 | 0 |
| `psycle-player` | 7 | 7 | 0 |
| `psycle-plugins` | 634 | 1 | 633 |
| **Total** | **936** | **291** | **645** |

The canonical retained-manifest identity is:

```text
SHA-256: 00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a
```

That digest is the SHA-256 of the locale-stable aggregate `retained-all.sha256` receipt. It identifies the exact upstream bytes selected for the first Phase 6B baseline without redistributing those bytes in the audit artifact.

## Selection policy

### Fully retained

The first baseline retains all frozen files from:

- `universalis` — 83 / 83;
- `psycle-helpers` — 68 / 68;
- `psycle-player` — 7 / 7.

These remain subject to the notice-preservation requirements already recorded by Phase 6A. Retention here means “selected for the sanitized baseline,” not “all licensing work is permanently finished.”

### `psycle-core`

Retain 100 / 108 files.

Quarantine exactly the eight files under:

```text
src/seib/vst/
```

Reason: expression-level VST/SDK provenance review remains deliberately deferred to the later VST2 work. The Psycle-owned core VST host files outside that subtree remain in the baseline, but VST2 compilation stays disabled until the clean-room ABI boundary exists.

### `psycle-audiodrivers`

Retain 32 / 36 files.

Quarantine exactly the four files under:

```text
src/asio/
```

Reason: ASIO expression/provenance review is not required to establish the Linux player. The remaining driver tree stays available to reproduce the historical build logic; the Linux build gate must not require the quarantined ASIO implementation.

### `psycle-plugins`

Retain only:

```text
src/psycle/plugins/plugin.hpp
```

This is the minimum frozen native-plugin API surface selected for the first engine/player baseline. The other 633 plugin-tree files are not bulk-imported.

Within those 633 files, Phase 6A already established 13 explicit exclusion cases:

- nine closed-source/prebuilt DLLs — excluded;
- three historical `.psy` songs with unresolved redistribution permission — excluded;
- `src/psycle/plugins/y_midi/gmnames.h` — excluded as Steinberg VST SDK-derived material.

The remaining 620 plugin-tree files are **deferred**, not declared non-redistributable. They can return through separately audited preservation slices after the player/core baseline is established. This keeps Phase 6B from re-importing the whole classic plugin ecosystem merely to satisfy an include dependency.

## Reproduction

Generate a fresh Phase 6A receipt and then materialize the Phase 6B selection receipt:

```bash
scripts/phase6-upstream-audit.sh phase6-upstream-audit
bash scripts/phase6b-sanitized-manifest.sh \
  phase6-upstream-audit \
  phase6b-sanitized-manifest
```

The second command fails if:

- any frozen manifest is missing;
- any manifest entry is malformed;
- per-component accounting fails;
- the six-component total is not 936 files;
- the retained total is not 291 files;
- the omitted/quarantined/deferred total is not 645 files;
- the retained aggregate identity differs from `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`.

## Baseline receipt layout

The generated directory contains only receipts:

- `counts.tsv` — exact component arithmetic;
- `retained/<component>.sha256` — frozen retained file/hash pairs;
- `omitted/<component>.tsv` — omitted path, SHA-256, and explicit reason;
- `retained-all.sha256` — locale-stable aggregate retained identity input;
- `baseline.sha256` — canonical first-baseline manifest identity;
- `summary.md` — human-readable totals.

No SourceForge source, binaries, songs, SDK material, or assets are uploaded by the Phase 6B manifest workflow.

## Phase 6B status after this slice

Completed here:

- [x] materialize the exact retained-file list and omission/defer arithmetic from all six frozen r12005 manifests;
- [x] freeze a deterministic first-baseline retained-manifest identity;
- [x] add CI that replays Phase 6A before accepting the Phase 6B selection receipt.

Still required before Phase 6B is complete:

- [ ] restore/add full compatible third-party notices required by retained helper/API code;
- [ ] commit the selected 291 upstream files on a separate C++ archival/canonical baseline without changing `cpsycle-r12005-baseline`;
- [ ] reproduce the historical Debian/Linux `psycle-player` build against that sanitized tree;
- [ ] record and fix only the narrow compiler/linker/runtime blockers exposed by that build;
- [ ] establish deterministic CLI/headless playback where practical and begin historical `.psy` load checks.

This document therefore freezes **Phase 6B.1**, not the whole Phase 6 roadmap. The next source-bearing commit must match this receipt exactly or deliberately update this policy and its identity with reviewable evidence.
