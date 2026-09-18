# Psycle Core Parity Matrix

## Status

**Phase 6C active — the first original-Psycle compatibility row is now classified: PSY2 parsing is PASS for one exact hash-bound fixture/procedure; 19 contracts remain UNKNOWN.**

This document is the human-readable view of the machine-readable contract in [`phase6c/compatibility-matrix.json`](phase6c/compatibility-matrix.json). The matrix compares three separately identified evidence sources:

1. **Original Psycle 1.12.0 x86** — primary behavioural reference;
2. **sanitized SourceForge SVN r12005 C++ family** — candidate Linux engine, frozen by the Phase 6B baseline identity;
3. **C-Psycle r12005 regression corpus** — independent donor/oracle where semantics overlap.

Phase 6B is complete. The sanitized C++ source is committed under `psycle-cpp-r12005-sanitized/`, the historical qmake path builds `psycle-player` on Ubuntu 24.04, the missing historical support inputs are staged with pinned identities, and the final Phase 6B historical-player CI run completed successfully.

Phase 6C has now crossed from evidence collection into evidence-backed classification. The first versioned original/candidate comparison classifies only the PSY2 parsing contract; broader Project I/O semantics remain open.

## Evidence rule

A row may change from `UNKNOWN` only when the result records:

- exact original-Psycle reference build/version when original behaviour is claimed;
- exact candidate C++ snapshot identity;
- fixture/input identity;
- observation procedure;
- produced receipt, hash, render, dump, log, screenshot, or another reproducible observation;
- tolerance/invariant when byte-exact comparison is inappropriate.

`PASS` means the tested compatibility contract matches the pinned original reference for the stated fixture/procedure. It does **not** mean the whole subsystem is proven equivalent.

`DIFFERENT` means a reproducible difference exists. It is not automatically a defect until the expected compatibility contract is established.

`MISSING` means the candidate engine lacks a capability required to perform the corresponding original-Psycle operation, with that requirement grounded in the original-reference evidence.

`UNKNOWN` means the evidence is incomplete.

Candidate-only or C-Psycle-only observations may be useful and may expose implementation gaps, but they do **not** move an overall compatibility row out of `UNKNOWN` by themselves. `scripts/phase6c-validate-matrix.py` enforces that rule mechanically.

## Pinned inputs

| Role | Identity | Status |
| --- | --- | --- |
| Original Psycle | `Psycle 1.12.0 x86 / PsycleInstallerx86-1.12.0.exe` — 9,322,919 bytes — SHA-256 `f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769` | **PINNED**; executable is not redistributed; PSY2 parse/load acceptance is versioned and classified, broader behavioural observations remain open |
| Candidate C++ | SourceForge SVN r12005 sanitized Phase 6B baseline `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a` | **IMPORTED / BUILDABLE** |
| Historical player build | Ubuntu 24.04 qmake build; Phase 6B run `35196962690` | **PASS** as a build/runtime smoke, not an original-Psycle parity claim |
| C-Psycle oracle | repository baseline `cpsycle-r12005-baseline` plus merged Phase 2–5 regressions | **AVAILABLE** |

See [`PHASE6_REFERENCE_PROVENANCE.md`](PHASE6_REFERENCE_PROVENANCE.md) for the frozen upstream identities and [`PHASE6_CPP_IMPORT_AUDIT.md`](PHASE6_CPP_IMPORT_AUDIT.md) for the sanitized import boundary.

## Phase 6C executable evidence lane

The maintained Phase 6C workflow is `.github/workflows/phase6c-compatibility-matrix.yml`.

Its first portable evidence slice deliberately reuses project-authored fixtures rather than upstream demo songs:

- `scripts/phase4-historical-psy2-smoke.sh` creates and validates the existing deterministic PSY2SONG fixture through C-Psycle;
- `scripts/phase4-core-workflow-smoke.sh` creates and validates the existing deterministic PSY3 workflow fixture through C-Psycle;
- the pinned historical `psycle-player` is rebuilt from the sanitized C++ baseline;
- `scripts/phase6c-candidate-fixtures.sh` runs that player with the `dummy` audio driver against both fixtures;
- the evidence artifact records fixture SHA-256, player exit code, observation classification and log SHA-256 for each candidate run.

Candidate observation receipts still do not self-promote parity. For PSY2, a separate versioned original-reference receipt and comparison verdict now bind the exact fixture hash to an original accepted load and a candidate clean load; `scripts/phase6c-validate-matrix.py` additionally requires those concrete parse semantics before accepting PASS.

## Engine compatibility matrix

| Subsystem / contract | Original reference evidence | Candidate C++ evidence | C-Psycle evidence reusable? | Status | Next evidence |
| --- | --- | --- | --- | --- | --- |
| PSY2 parsing | [`original-psy2.json`](phase6c/evidence/project-io-psy2-parse/original-psy2.json): pinned 1.12.0 accepted the exact fixture with stable marker evidence and no harness/runtime diagnostic | [`candidate-psy2.json`](phase6c/evidence/project-io-psy2-parse/candidate-psy2.json): pinned r12005 player loaded the same fixture and exited 0 | yes; project fixture exists | PASS | expand separately to serialization, state/playback semantics and additional PSY2 fixtures; see [`comparison.json`](phase6c/evidence/project-io-psy2-parse/comparison.json) |
| PSY3 parsing | PR #61 run `35364406445`: pinned 1.12.0 dismissed the exact verified `Load Warning`, then exposed `phase4-first.psy` for 40 stable polls with no application/harness/runtime diagnostic | candidate receipt still says `timeout` / exit `124`; log reaches version-3 loader, creates Sampler/Master and reaches `playing...` | yes; project fixture exists | UNKNOWN | version a candidate parse-specific receipt that separates successful parse/load evidence from the player's noninteractive lifetime timeout, then compare against the clean original receipt |
| Serialization / round-trip | pending | pending | yes | UNKNOWN | distinguish semantic round-trip from byte identity |
| Malformed-file behaviour | pending | pending | partial | UNKNOWN | define rejection/recovery fixtures and original behaviour |
| Sequence / pattern order | pending | pending | partial | UNKNOWN | original sequence semantics are authoritative |
| BPM / LPB / tick timing | pending | pending | partial | UNKNOWN | capture deterministic timing receipts from original |
| Delayed / retrigger / extended commands | pending | pending | partial | UNKNOWN | use minimal deterministic patterns |
| Sampler PS1 | pending | pending | yes | UNKNOWN | compare pitch, envelopes, looping and commands |
| XMSampler / Sampulse-related playback | pending | pending | partial | UNKNOWN | pin exact feature/version scope |
| Mixer / Master / routing / mute / bypass | pending | pending | partial | UNKNOWN | compare graph defaults, gain and send semantics |
| Native-machine ABI / identity | pending | pending | strong Phase 5 corpus | UNKNOWN | bind shared ABI receipts to original 1.12.0 observations |
| Native-machine defaults / tweak / state | pending | pending | strong Phase 5 corpus | UNKNOWN | reuse source-derived plugin receipts where valid |
| Plugin opaque-state persistence | pending | pending | yes | UNKNOWN | VST2 host restoration remains later Phase 9 scope |
| Native rescan / cache | pending | pending | partial | UNKNOWN | separate native discovery from later VST2 scanning |
| MIDI routing | pending | pending | partial | UNKNOWN | separate engine routing from platform-driver input |
| Automation / tweak commands | pending | pending | partial | UNKNOWN | record exact event ordering and command semantics |
| WAV / sample loading | pending | pending | yes | UNKNOWN | use a project-authored PCM fixture |
| Offline render / bounce | pending | pending | yes | UNKNOWN | compare deterministic/tolerance-bounded renders |
| Missing-machine recovery | pending | pending | partial | UNKNOWN | establish original placeholder/recovery contract |
| Historical song playback | pending | pending | not yet | UNKNOWN | use only redistributable, permissioned or project-authored material |

The machine-readable matrix contains the canonical row IDs and exact evidence-gating rules. This table is a readable projection, not a second source of truth.

## UI-only gaps

Phase 6 separates engine gaps from UI gaps. The absence of a full tracker UI in `psycle-player` is expected and is not an engine parity failure by itself.

| UI area | Original Psycle reference | Candidate engine responsibility | Planned phase |
| --- | --- | --- | --- |
| Pattern/tracker grid | visual + interaction oracle | expose required model/edit commands | Phase 8 |
| Machine View | visual + routing interaction oracle | expose graph/routing operations | Phase 8 |
| Parameter windows | visual + parameter semantics | expose parameters/programs/state | Phase 8 |
| Instrument/Sampler editor | visual + editing semantics | expose sample/instrument model | Phase 8 |
| Wave editor | visual + editing semantics | expose wave/sample operations | Phase 8 |
| Mixer/Master view | visual + routing semantics | expose mixer state/commands | Phase 8 |
| VST editor lifecycle | historical interaction oracle | host lifecycle must exist first | Phase 9 |

## Existing C-Psycle evidence classification

### Likely portable compatibility contracts

- project-authored `.psy` fixture construction and structural round-trip checks;
- machine identity/parameter/state receipts where the C++ and C hosts consume the same native ABI;
- deterministic DSP oracles for preserved native machines;
- WAV/sample fixture generation;
- render-to-Sampler workflow concepts;
- missing-machine recovery concepts;
- sample-rate and non-positive-buffer boundary tests where applicable.

### Requires original-Psycle confirmation first

- sequencer/event ordering;
- tracker tick/line timing details;
- delayed/retrigger command implementation;
- mixer/send semantics;
- automation routing;
- UI-facing behaviour.

### C-Psycle-only historical evidence

- C-Psycle X11 host behaviour;
- C-Psycle platform UI bridge implementation details;
- C-Psycle's newer event-sequencer internals when they differ from original Psycle.

## Current implementation backlog

Phase 6C now justifies this evidence backlog:

1. keep the machine-readable matrix and validation gate green;
2. preserve the first classified PSY2 parse receipt/verdict and keep its scoped PASS mechanically validated;
3. preserve the clean original PSY3 observation from run `35364406445`, then make the candidate receipt express parse/load acceptance separately from its noninteractive timeout before attempting a PSY3 comparison; keep serialization separate from parse acceptance;
4. classify only additional rows for which versioned original + candidate receipts and a comparison verdict are sufficient;
5. expand to timing, tracker commands, routing, sampler, native-state, WAV and render fixtures in that order as evidence becomes reproducible;
6. generate the first Phase 7 behavioural backlog only from confirmed `DIFFERENT` / `MISSING` results.

No Phase 7 engine change is justified merely because C-Psycle and the C++ candidate differ. Original Psycle remains the compatibility target.
