# Psycle Core Parity Matrix

## Status

**Phase 6 audit scaffold — no compatibility PASS / DIFFERENT claims are accepted yet.**

This matrix compares three separately identified evidence sources:

1. **Original Psycle 1.12.0 x86** — primary behavioural reference;
2. **six-component C++ build-source family at SourceForge SVN r12005** — candidate Linux engine snapshot, including required `universalis` support code;
3. **C-Psycle r12005 regression corpus** — independent donor/oracle where semantics overlap.

See [PHASE6_REFERENCE_PROVENANCE.md](PHASE6_REFERENCE_PROVENANCE.md) for the frozen identities and [PHASE6_CPP_IMPORT_AUDIT.md](PHASE6_CPP_IMPORT_AUDIT.md) for the current import HOLD.

## Evidence rule

A row may change from `UNKNOWN` only when the result records:

- exact original-Psycle reference build/version when original behaviour is claimed;
- exact candidate C++ snapshot identity;
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
| Original Psycle | `Psycle 1.12.0 x86 / PsycleInstallerx86-1.12.0.exe` — 9,322,919 bytes — SHA-256 `f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769` | **PINNED**; observation procedures still to be executed |
| `universalis` | SVN r12005 / last changed r12004 / 83 files / manifest `827586daad2efcfbc10466394670a4a0e5f208da94afb7b3bf72239d396a618e` | **PINNED FOR AUDIT**; required build support |
| `psycle-core` | SVN r12005 / 108 files / manifest `eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b` | **PINNED FOR AUDIT**; public import HOLD |
| `psycle-audiodrivers` | SVN r12005 / 36 files / manifest `4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1` | **PINNED FOR AUDIT**; public import HOLD |
| `psycle-helpers` | SVN r12005 / 68 files / manifest `13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db` | **PINNED FOR AUDIT**; public import HOLD |
| `psycle-player` | SVN r12005 / 7 files / manifest `6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f` | **PINNED FOR AUDIT**; public import HOLD |
| `psycle-plugins` | SVN r12005 / 634 files / manifest `a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888` | **PINNED FOR AUDIT**; requires sanitization before import |
| C-Psycle oracle | repository audited baseline `cpsycle-r12005-baseline` plus merged Phase 2–5 regressions | available |

The candidate C++ snapshot is reproducible, but **not yet present in this Git repository**. A parity row cannot report candidate-engine execution until the provenance-safe sanitized import/build gate is complete.

## Engine compatibility matrix

| Subsystem / contract | Original reference evidence | Candidate C++ result | C-Psycle evidence reusable? | Status | Notes / next evidence |
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

The only implementation backlog item currently justified by Phase 6 evidence is the **sanitized C++ import/build boundary itself**:

1. materialize the exact retained-file and omission/replacement manifest for `universalis` plus the five Psycle C++ component trees;
2. preserve/restore all compatible third-party notices;
3. create an audited sanitized C++ baseline;
4. reproduce `psycle-player` on Linux;
5. only then begin moving matrix rows out of `UNKNOWN`.

A Phase 7 behavioral backlog must be generated from reproducible `DIFFERENT` / `MISSING` results, not from architectural preference.
