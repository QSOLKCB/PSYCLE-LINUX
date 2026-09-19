# Phase 6C sequence / pattern-order observation contract

This slice measures the canonical matrix row `sequencer-pattern-order` without
assuming that C-Psycle's later sequencer architecture is authoritative.

The versioned comparison committed after PR #65 now classifies
`sequencer-pattern-order` as a scoped **PASS** for the exact project-authored
single-sequence PSY3 fixture. The original and candidate runtime receipts remain
observation-only `parity_status: UNKNOWN` records by design; the committed
comparison is the object that establishes the matrix verdict.

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

Candidate runtime evidence remains `parity_status: UNKNOWN`; the versioned
comparison binds that observation to the original reference before the matrix
can classify it.

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

Original runtime evidence remains `parity_status: UNKNOWN`; it does not
self-promote the compatibility row.

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

## Versioned classification

Final PR #65 Phase 6C run `35443357239` at head
`0a9eebcba7028bd808bfa32c3e654828db51d7fa` produced clean paired evidence
for the identical fixture SHA-256
`6a0c5073fb54b7bb7496a6dda85ece80abefe8ba6098491b1a06972fe619d5c0`.

The pinned original Psycle 1.12.0 x86 observation exposed these stable order
labels for 38 polls:

```text
00: 00
01: 02
02: 01
03: 02
```

The frozen candidate probe exited 0 and extracted the canonical musical
`SequenceLine` play order:

```text
0, 2, 1, 2
```

The versioned receipts and comparison are committed under
`phase6c/evidence/sequencer-pattern-order/`. The matrix therefore records
a scoped **PASS**.

This PASS does not measure BPM/LPB/tick timing, delayed/retrigger commands,
audio playback, multi-sequence behaviour, or UI editing semantics. Those remain
separate compatibility contracts and evidence tasks.
