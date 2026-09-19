# Phase 6C sequence / pattern-order observation contract

This slice measures the canonical matrix row `sequencer-pattern-order` without
assuming that C-Psycle's later sequencer architecture is authoritative.

The compatibility status remains **UNKNOWN** until a versioned original/candidate
comparison is committed. This document defines the observation lane only.

## Fixture scope

`tests/phase6c_sequence_order_fixture.c` generates a project-authored PSY3
fixture with one ordinary legacy play-order list. It deliberately avoids
C-Psycle's multi-sequence extension.

The fixture contains three patterns and the non-monotonic play order:

```text
00 -> pattern 00
01 -> pattern 02
02 -> pattern 01
03 -> pattern 02
```

The expected original-Psycle order-list labels are therefore:

```text
00: 00
01: 02
02: 01
03: 02
```

Pattern lengths are distinct (1, 2 and 3 beats), so the generator also verifies
that order offsets follow the referenced pattern lengths. Those C-Psycle checks
protect fixture construction; they are not original-Psycle compatibility
evidence.

## Candidate observation

`tests/phase6c_sequence_order.cpp` is a separate project-authored probe linked
against the verified historical C++ core. It does not patch or replace engine
source.

With `PSYCLE_THREADS=1`, the probe loads the exact fixture without starting
playback and records every loaded `SequenceLine` entry in model order,
including its sequence position, pattern ID and pattern name. The evidence
validator requires:

- the frozen Phase 6B candidate identity;
- exact fixture and committed source hashes;
- the probe executable hash;
- a hash-bound fixture-generator transcript;
- the exact PSY3 load warning;
- a fail-closed diagnostic allowlist with required loader/warning/Master
  construction markers;
- a clean process exit.

Candidate evidence remains `parity_status: UNKNOWN`.

## Original Psycle observation

The maintained Windows reference observer can enable `-ObserveSequenceOrder`.
It loads the exact same fixture into pinned Psycle 1.12.0 x86 using the existing
installer/runtime/bootstrap controls.

No order-list input is injected. The observer only reads process-owned UI
Automation text and extracts the complete visible set matching `NN: NN`.
A sequence-order observation is conclusive only after exactly four such labels
remain unchanged for at least four polls. The actual labels are always recorded;
a stable difference is evidence, not a harness failure.

The existing PSY3 Load Warning is still verified and dismissed through the
already bounded process-owned UI Automation path. UI automation, runtime
identity, load or liveness contamination keeps the observation inconclusive.

Original evidence remains `parity_status: UNKNOWN`.

## Maintained execution

```sh
bash scripts/phase6c-sequence-order-candidate.sh phase6c-evidence
python3 scripts/phase6c-sequence-order-evidence.py candidate phase6c-evidence
python3 scripts/phase6c-sequence-order-evidence.py original \
  phase6c-candidate-artifact phase6c-original-evidence
python3 tests/phase6c_sequence_order.py
```

The Phase 6C workflow runs this lane after the existing candidate build and
before artifact upload, then runs the original observer and both validators on
Windows.

## Classification boundary

A later comparison may classify `PASS` only if both pinned implementations
conclusively expose the same order for the exact fixture and all receipt
identities/procedures are versioned. A stable differing order may support a
scoped `DIFFERENT` result. Missing, partial, ambiguous or contaminated evidence
must stay `UNKNOWN`.

This slice does not measure BPM/LPB/tick timing, delayed/retrigger commands,
audio playback, multi-sequence behaviour, or UI editing semantics.
