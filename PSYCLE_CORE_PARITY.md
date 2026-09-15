# Psycle Core Parity Matrix

## Status

**Phase 6 audit scaffold — no compatibility PASS / DIFFERENT claims are accepted yet.**

This matrix compares three separately identified evidence sources:

1. **Original Psycle 1.12.0 x86** — primary behavioural reference;
2. **`psycle-core` family at SourceForge SVN r12005** — candidate Linux engine snapshot;
3. **C-Psycle r12005 regression corpus** — independent donor/oracle where semantics overlap.

See [PHASE6_REFERENCE_PROVENANCE.md](PHASE6_REFERENCE_PROVENANCE.md) for the acquisition and authority rules.

## Evidence rule

A row may change from `UNKNOWN` only when the result records:

- exact original-Psycle reference build/version when original behaviour is claimed;
- exact `psycle-core` snapshot identity;
- fixture/input identity;
- observation procedure;
- produced receipt, hash, render, dump, log, or other reproducible evidence;
- tolerance/invariant when byte-exact comparison is inappropriate.

`PASS` means the tested compatibility contract matches the pinned original reference for the stated fixture/procedure. It does **not** mean the whole subsystem is proven equivalent.

`DIFFERENT` means a reproducible difference exists. It is not automatically a defect until the expected compatibility contract is established.

`MISSING` means the candidate engine lacks a capability required to perform the corresponding original-Psycle operation.

`UNKNOWN` means evidence has not yet established the result.

## Pinned inputs

| Role | Identity | Status |
| --- | --- | --- |
| Original Psycle | `Psycle 1.12.0 x86 / PsycleInstallerx86-1.12.0.exe` | SHA-256 discovery pending Phase 6 provenance CI |
| Candidate C++ engine | SourceForge SVN `r12005`: `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, `psycle-plugins` | tree manifests pending Phase 6 provenance CI |
| C-Psycle oracle | repository audited baseline `cpsycle-r12005-baseline` plus merged Phase 2–5 regressions | available |

## Engine compatibility matrix

| Subsystem / contract | Original reference evidence | `psycle-core` result | C-Psycle evidence reusable? | Status | Notes / next evidence |
| --- | --- | --- | --- | --- | --- |
| PSY2 parsing | pending | pending | yes, project fixture exists | UNKNOWN | establish shared fixture and field-level receipt |
| PSY3 parsing | pending | pending | yes | UNKNOWN | compare song topology and state |
| PSY serialization / round-trip | pending | pending | yes | UNKNOWN | distinguish format compatibility from byte identity |
| Sequence / pattern order | pending | pending | partial | UNKNOWN | original pattern-sequence semantics are authoritative |
| BPM / LPB / tick timing | pending | pending | partial | UNKNOWN | C-Psycle event sequencer may diverge |
| Delayed / retrigger commands | pending | pending | partial | UNKNOWN | use minimal deterministic pattern fixtures |
| Sampler PS1 | pending | pending | yes | UNKNOWN | compare pitch, envelopes, looping, commands |
| XMSampler / Sampulse-related playback | pending | pending | partial | UNKNOWN | pin feature/version scope carefully |
| Mixer / Master routing | pending | pending | partial | UNKNOWN | compare graph gain/send semantics |
| Native-machine ABI / identity | pending | pending | strong Phase 5 corpus | UNKNOWN | map C++ host ABI against original first |
| Native-machine parameter/state persistence | pending | pending | strong Phase 5 corpus | UNKNOWN | reuse source-derived plugin receipts where valid |
| Plugin opaque state persistence | pending | pending | yes | UNKNOWN | VST2 host restoration is later Phase 9 work |
| MIDI routing | pending | pending | partial | UNKNOWN | separate engine routing from platform driver input |
| Automation / tweak commands | pending | pending | partial | UNKNOWN | record exact command semantics |
| WAV/sample loading | pending | pending | yes | UNKNOWN | start with project-authored PCM fixture |
| Offline render / bounce | pending | pending | yes | UNKNOWN | compare deterministic renders where practical |
| Bounce → Sampler workflow | pending | pending | yes | UNKNOWN | reuse Phase 4 acceptance concept |
| Missing-machine recovery | pending | pending | partial | UNKNOWN | define placeholder contract before implementation |
| Historical song playback | pending | pending | not yet | UNKNOWN | only use redistributable/permissioned songs |

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

The current regression corpus should be reused deliberately rather than copied wholesale.

### Likely portable compatibility contracts

- `.psy` fixture construction and structural round-trip checks;
- machine identity/parameter/state receipts where the C++ and C hosts consume the same native ABI;
- deterministic DSP oracles for preserved native machines;
- WAV/sample fixture generation;
- render → Sampler workflow concepts;
- missing-machine recovery concepts;
- sample-rate and non-positive-buffer boundary tests where applicable.

### Requires original-Psycle confirmation first

- sequencer/event ordering;
- tracker tick/line timing details;
- delayed/retrigger command implementation;
- mixer/send semantics;
- automation routing;
- UI-facing behavior.

### C-Psycle-only historical evidence

- C-Psycle X11 host behavior;
- C-Psycle platform UI bridge implementation details;
- C-Psycle's newer event-sequencer internals when they differ from original Psycle.

## First implementation backlog

This section remains intentionally empty until the pinned SourceForge receipts and first build/parity observations exist.

A Phase 7 backlog must be generated from reproducible `DIFFERENT` / `MISSING` results, not from architectural preference.
