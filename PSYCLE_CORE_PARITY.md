# Psycle Core Parity Matrix

## Status

**Phase 6C active — PSY2 and PSY3 parse/load acceptance plus sequence/pattern order are scoped PASS results, serialization/save capability is MISSING, BPM/LPB/tick timing is DIFFERENT for the exact hash-bound timing fixture, and 15 contracts remain UNKNOWN.**

This document is the human-readable view of the machine-readable contract in [`phase6c/compatibility-matrix.json`](phase6c/compatibility-matrix.json). The matrix compares three separately identified evidence sources:

1. **Original Psycle 1.12.0 x86** — primary behavioural reference;
2. **sanitized SourceForge SVN r12005 C++ family** — candidate Linux engine, frozen by the Phase 6B baseline identity;
3. **C-Psycle r12005 regression corpus** — independent donor/oracle where semantics overlap.

Phase 6B is complete. The sanitized C++ source is committed under `psycle-cpp-r12005-sanitized/`, the historical qmake path builds `psycle-player` on Ubuntu 24.04, the missing historical support inputs are staged with pinned identities, and the final Phase 6B historical-player CI run completed successfully.

Phase 6C has crossed from evidence collection into evidence-backed classification. Versioned original/candidate comparisons classify PSY2 and PSY3 parse/load acceptance for one fixture each, classify sequence/pattern order as PASS for the exact legacy single-sequence `0,2,1,2` fixture, classify the tested PSY3 serialization/save capability as MISSING, and classify the scoped BPM/LPB/tick contract as DIFFERENT. For the timing fixture, both sides preserve BPM 137 and LPB 8, but original Psycle reports TPB 24 while the candidate loads LPB 8 into `tick_speed` with `is_ticks=true`, making `PlayerTimeInfo` use a beat/8 tick cadence. Delayed/retrigger commands, playback, sampler tick processing, multi-sequence behaviour and editing semantics remain separate; byte identity and complete semantic round-trip state are explicitly outside the save-capability verdict.

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
| Original Psycle | `Psycle 1.12.0 x86 / PsycleInstallerx86-1.12.0.exe` — 9,322,919 bytes — SHA-256 `f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769` | **PINNED**; executable is not redistributed; PSY2/PSY3 parse/load acceptance, scoped sequence/pattern order, Save As/fresh reopen, and scoped Song Information BPM/LPB/TPB timing are versioned; broader behavioural observations remain open |
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

### PSY3 parse receipt and process lifetime

PR #61 run `35364406445` supplies the unchanged original and candidate process
observations. The subsequently derived `candidate-psy3-parse.json` is explicitly
an analysis of those archived bytes, not a newly executed player run. See the
[PSY3 evidence procedure](phase6c/evidence/project-io-psy3-parse/README.md).

The frozen player emits `playing...` only after `song.load()` returns true. The
PSY3 loader can nevertheless report damage as a warning and return true, so
`scripts/phase6c-candidate-parse.py` requires the exact fixture hash, matching
load paths, dummy driver, ordered single load/loader/Sampler/Master/playing
markers, and only explicitly scoped warnings across the complete log. Missing
or ambiguous evidence, other warnings, errors, and abnormal process results
remain inconclusive. The sidecar retains `parity_status: UNKNOWN`; only the
versioned comparison against accepted original evidence classifies this row.

The maintained candidate lane emits and checks this sidecar on subsequent runs.
The matrix validator independently rederives the archived sidecar and keeps the
raw candidate receipt pinned byte-for-byte. This does not establish correct
playback, serialized state, clean termination, or general PSY3 support.

## Engine compatibility matrix

| Subsystem / contract | Original reference evidence | Candidate C++ evidence | C-Psycle evidence reusable? | Status | Next evidence |
| --- | --- | --- | --- | --- | --- |
| PSY2 parsing | [`original-psy2.json`](phase6c/evidence/project-io-psy2-parse/original-psy2.json): pinned 1.12.0 accepted the exact fixture with stable marker evidence and no harness/runtime diagnostic | [`candidate-psy2.json`](phase6c/evidence/project-io-psy2-parse/candidate-psy2.json): pinned r12005 player loaded the same fixture and exited 0 | yes; project fixture exists | PASS | expand separately to serialization, state/playback semantics and additional PSY2 fixtures; see [`comparison.json`](phase6c/evidence/project-io-psy2-parse/comparison.json) |
| PSY3 parsing | [`original-psy3.json`](phase6c/evidence/project-io-psy3-parse/original-psy3.json): pinned 1.12.0 accepted the exact fixture after verified `Load Warning` dismissal, with 40 stable marker polls | [`candidate-psy3-parse.json`](phase6c/evidence/project-io-psy3-parse/candidate-psy3-parse.json): ordered load markers and scoped diagnostics establish parse acceptance; raw process evidence remains `timeout` / exit `124` | yes; project fixture exists | PASS | serialization/state/playback remain separate; see [`comparison.json`](phase6c/evidence/project-io-psy3-parse/comparison.json) |
| Serialization / round-trip | [`serialization-save.json`](phase6c/evidence/project-io-serialization-roundtrip/serialization-save.json): pinned 1.12.0 saved the exact PSY3 input to a fresh PSY3 output and a fresh process accepted it | [`candidate-serialization.json`](phase6c/evidence/project-io-serialization-roundtrip/candidate-serialization.json): `CoreSong::save` versions 2, 3 and 4 each returned false, emitted no output and exited 0 | yes | MISSING | implement the evidenced save capability in Phase 7, then measure semantic round-trip state separately; see [`comparison.json`](phase6c/evidence/project-io-serialization-roundtrip/comparison.json) |
| Malformed-file behaviour | pending | pending | partial | UNKNOWN | define rejection/recovery fixtures and original behaviour |
| Sequence / pattern order | [`original-sequence-order.json`](phase6c/evidence/sequencer-pattern-order/original-sequence-order.json): pinned 1.12.0 accepted the exact fixture and exposed stable labels `00: 00, 01: 02, 02: 01, 03: 02` for 38 polls | [`candidate-sequence-order.json`](phase6c/evidence/sequencer-pattern-order/candidate-sequence-order.json): clean probe exit with canonical Master line and musical play order `0,2,1,2` | fixture construction only | PASS | exact legacy single-sequence order only; timing, playback, multi-sequence and editing remain separate; see [`comparison.json`](phase6c/evidence/sequencer-pattern-order/comparison.json) |
| BPM / LPB / tick timing | [`original-bpm-lpb-tick.json`](phase6c/evidence/sequencer-bpm-lpb-tick/original-bpm-lpb-tick.json): pinned 1.12.0 accepted the exact fixture and stably exposed BPM 137 / LPB 8 / TPB 24 / extra tick 0 | [`candidate-bpm-lpb-tick.json`](phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json): clean probe observes BPM 137 / derived LPB 8 but `tick_speed=8`, `is_ticks=true`, so `PlayerTimeInfo` uses beat/8 tick cadence | fixture construction only | DIFFERENT | restore distinct legacy TPB/extra-tick semantics without conflating LPB; delayed/retrigger commands remain a separate contract; see [`comparison.json`](phase6c/evidence/sequencer-bpm-lpb-tick/comparison.json) |
| Delayed / retrigger / extended commands | [`original-delayed-retrigger.json`](phase6c/evidence/sequencer-delayed-retrigger/original-delayed-retrigger.json): pinned 1.12.0 accepts the exact FD/FB/FA/FE04 fixture with 38 stable marker polls and clean runtime/UI identity; [`original-source-delayed-retrigger.json`](phase6c/evidence/sequencer-delayed-retrigger/original-source-delayed-retrigger.json) separately binds the pinned original command IDs/formulas, but `runtime_execution_trace` remains `not-observed` | [`candidate-delayed-retrigger.json`](phase6c/evidence/sequencer-delayed-retrigger/candidate-delayed-retrigger.json): frozen-core runtime probe observes FD note-delay scheduling, complete FB/FA retrigger series and FE04 marker geometry with the exact final PR #69 fixture | fixture construction only | UNKNOWN | the additive original execution observer now has two diagnostic boundaries: PR #72 shows FD/FB/FA/FE are not the differentiator, and workflow `35730075034` narrows the shared render failure further because Master-only, empty-Sampler and sample/instrument-state controls each produce the same finalized silent PCM output while Psycle remains alive, whereas adding one ordinary note reproduces `0xC0000005`. Investigate ordinary-note routing / Sampler voice startup, then obtain a command-bearing original output and collect candidate render evidence from the same witness before timing classification. The frozen PR #69 callback schedule remains preservation evidence, not the comparison partner; [`comparison.json`](phase6c/evidence/sequencer-delayed-retrigger/comparison.json) remains frozen. |
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

- sequencer timing and event-ordering semantics beyond the classified single-sequence play-order and scoped BPM/LPB/tick fixtures;
- tracker tick/line timing details beyond the classified LPB/TPB distinction;
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
2. preserve the scoped PSY2 and PSY3 parse/load PASS comparisons, including the PSY3 timeout as a separate process-lifetime result;
3. preserve the scoped serialization/save MISSING comparison from final PR #63 run `35439069516`; byte identity and complete semantic round-trip state remain separate;
4. preserve the scoped sequence/pattern-order PASS from final PR #65 run `35443357239`, including its exact single-sequence fixture boundary;
5. preserve the scoped BPM/LPB/tick DIFFERENT result from final PR #67 run `35453296036`: BPM 137 and LPB 8 match, while original TPB 24 differs from the candidate beat/8 tick cadence;
6. preserve the versioned delayed/retrigger/extended-command evidence from final PR #69 run `35457374404`: frozen-candidate scheduling, pinned original-source semantics, and guarded native-Windows fixture acceptance remain separate roles with compatibility `UNKNOWN`;
7. preserve the versioned deferred comparison: source semantics plus native load acceptance are not promoted into original runtime execution. The additive original-render diagnostics now exclude FD/FB/FA/FE as the crash differentiator and narrow the observed `0xC0000005` boundary to the ordinary-note-present Sampler execution path after Master-only, empty-Sampler and embedded sample/instrument controls each produce the same finalized silent PCM output with the reference process alive. Investigate ordinary-note routing / Sampler voice startup next; only after obtaining a command-bearing original output should candidate render evidence be collected from the same witness for like-for-like timing. Do not compare that sampled output directly with the frozen PR #69 one-beat callback schedule;
8. continue to routing, sampler, native-state, WAV and render contracts as evidence becomes reproducible, and classify only rows with sufficient like-for-like original + candidate evidence;
9. extend Phase 7 only from confirmed `DIFFERENT` / `MISSING` results.

### First Phase 7 implementation backlog

Two evidence-backed engine items are now frozen. First, add the missing candidate PSY3 serialization/save capability required to perform the tested original Save As operation and produce an output that can be reopened; byte-for-byte identity is not required, and complete semantic state preservation must be measured separately. Second, separate legacy TPB/extra-tick timing from LPB in the C++ timing model so an LPB 8 song with TPB 24 no longer drives `PlayerTimeInfo` at a beat/8 tick cadence. Delayed/retrigger semantics must still be measured independently before broad tracker-command changes.

No Phase 7 engine change is justified merely because C-Psycle and the C++ candidate differ. Original Psycle remains the compatibility target.
