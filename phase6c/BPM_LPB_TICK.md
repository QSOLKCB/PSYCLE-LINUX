# Phase 6C BPM / LPB / Tick Observation Lane

## Status

**Observation lane under construction; compatibility remains `UNKNOWN`.**

This lane follows the scoped sequence/pattern-order classification without extending that verdict into timing. It measures one project-authored PSY3 fixture on the frozen Phase 6B C++ candidate and defines the corresponding pinned-original observation contract. Candidate or C-Psycle evidence cannot classify parity by itself.

## Fixture

`bpm-lpb-tick/phase6c-bpm-lpb-tick.psy` is generated during CI from `tests/phase6c_bpm_lpb_tick_fixture.c` and contains no third-party musical material.

Its frozen timing inputs are:

- BPM: `137`;
- lines per beat: `8`;
- ticks per beat: `24`;
- extra ticks: `0`;
- four simple note markers at beat positions `0`, `0.125`, `0.25`, and `0.375`.

The note markers make LPB observable from persisted pattern spacing instead of trusting metadata alone. C-Psycle generates and reloads the fixture only as construction evidence; its event-sequencer timing is not treated as original-Psycle authority.

## Candidate observation

`scripts/phase6c-bpm-lpb-tick-candidate.sh` builds a separate probe against the unchanged frozen C++ core. The probe loads the exact fixture without starting playback and records:

- loaded BPM;
- the candidate's loaded `tick_speed` and `is_ticks` mode;
- marker positions and LPB derived from their spacing;
- `samples_per_beat`, `samples_per_tick`, and fixture-line sample intervals at 44.1 and 48 kHz;
- loader reports and complete diagnostics.

`scripts/phase6c-bpm-lpb-tick-evidence.py` rederives the receipt from the raw probe bytes and log. Wrong numeric types, altered positions, incorrect timing formulae, unknown diagnostics, nonzero process results, unsafe paths, or changed source hashes force an inconclusive observation.

The receipt always retains `parity_status: UNKNOWN`.

## Original-reference contract

The matching original observation targets pinned Psycle 1.12.0 x86 and the same fixture bytes. A conclusive observation must identify the process-owned **Song Information** window and stably bind these controls:

- Tempo;
- Lines per beat;
- Ticks per beat;
- Extra tick per line;
- Real tempo;
- Real ticks per beat.

The fixture-derived expected values are `137`, `8`, `24`, `0`, `137`, and `24`. The Windows harness must first identify exactly one process-owned **Song Information** window or open it through the unique process-owned **File > Song Properties** menu item, then verify that exactly one such dialog exists before polling. At least four identical complete polls plus the existing clean accepted-load, runtime-identity and liveness gates are required. Missing, ambiguous, contradictory, unstable, or automation-derived values remain inconclusive.

A clean stable original observation is retained even when one or more values differ from the fixture-derived expectations. In that case `matches_fixture_expected` is `false`; the observed values remain authoritative original-reference evidence and `parity_status` remains `UNKNOWN` until a separately versioned comparison classifies the scoped contract. The dedicated observation workflow requires an actual `observed` result and cannot pass merely because an inconclusive receipt was well-formed.

The original receipt shape is validated now, but no original value is committed or treated as observed until a clean native-Windows artifact is produced.

## Classification boundary

This lane does not classify:

- playback/render duration;
- tempo changes inside a pattern;
- note delay, retrigger or extended commands;
- multi-sequence timing;
- UI editing semantics;
- sample-accurate equivalence outside the two recorded sample rates.

A later classification PR must version the exact original and candidate receipts, pin their workflow/artifact identities, add a comparison receipt, and then determine whether the scoped contract is `PASS`, `DIFFERENT`, or `MISSING`.
