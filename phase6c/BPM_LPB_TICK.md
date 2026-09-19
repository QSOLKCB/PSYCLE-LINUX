# Phase 6C BPM / LPB / Tick Timing

## Status

**Scoped classification: `DIFFERENT`.**

PR #67 established the paired observation lane for one project-authored PSY3 timing fixture. The final clean workflow run `35453296036` observed both the frozen Phase 6B C++ candidate and pinned Psycle 1.12.0 x86 against the exact same fixture bytes. This follow-on classification versions those receipts and the comparison under `phase6c/evidence/sequencer-bpm-lpb-tick/`.

The verdict is intentionally narrow: BPM and LPB agree, but the loaded tick cadence does not.

## Fixture

`bpm-lpb-tick/phase6c-bpm-lpb-tick.psy` is generated during CI from `tests/phase6c_bpm_lpb_tick_fixture.c` and contains no third-party musical material.

Its frozen timing inputs are:

- BPM: `137`;
- lines per beat: `8`;
- ticks per beat: `24`;
- extra ticks: `0`;
- four note markers at beat positions `0`, `0.125`, `0.25`, and `0.375`.

The marker spacing independently exposes LPB `8`; C-Psycle is used only to construct/reload the fixture and is not treated as original-Psycle timing authority.

## Pinned original observation

The native-Windows lane targets pinned Psycle 1.12.0 x86. It verifies the process-owned **Song Information** dialog and reads Psycle's source-defined native edit controls without modifying them.

Final run `35453296036` recorded four identical clean polls:

- Tempo: `137`;
- Lines per beat: `8`;
- Ticks per beat: `24`;
- Extra tick per line: `0`;
- Real tempo: `137`;
- Real ticks per beat: `24`.

The load was accepted, the fixture marker remained stable for 45 polls, runtime/UI diagnostics were empty, and the receipt remained `parity_status: UNKNOWN` until this separate comparison.

Versioned projection: `phase6c/evidence/sequencer-bpm-lpb-tick/original-bpm-lpb-tick.json`.

## Frozen candidate observation

The candidate probe is linked against the unchanged Phase 6B C++ core and executes the exact preserved/hash-bound probe binary from the evidence artifact.

For the same fixture it records:

- BPM: `137.0`;
- marker-derived LPB: `8.0`;
- `tick_speed = 8`;
- `is_ticks = true`;
- at 44.1 kHz: `samples_per_beat = 19313.869140625`, `samples_per_tick = 2414.23364257812`;
- at 48 kHz: `samples_per_beat = 21021.8984375`, `samples_per_tick = 2627.7373046875`.

The frozen source explains the result: `Psy3Filter::LoadSNGIv0` reads the PSY3 **lines-per-beat** value into `CoreSong::tick_speed()`. `PlayerTimeInfo::setTicksSpeed(song.tick_speed(), song.is_ticks())` then receives `8` with `is_ticks=true`, so `PlayerTimeInfo::recalcSPT()` computes one tick as `samples_per_beat / 8`.

Versioned receipt: `phase6c/evidence/sequencer-bpm-lpb-tick/candidate-bpm-lpb-tick.json`.

## Classification

The versioned comparison records:

- BPM match: **yes** — `137`;
- LPB match: **yes** — `8`;
- original ticks per beat: `24`;
- original extra tick per line: `0`;
- candidate timing tick cadence: `8` per beat because `is_ticks=true`;
- tick-semantics match: **no**.

Therefore `sequencer-bpm-lpb-tick` is **`DIFFERENT`** for this exact fixture and procedure.

This is not a claim that all sequencer timing is wrong. It identifies a concrete compatibility gap: the frozen candidate conflates the persisted LPB value with the legacy tick-cadence field used by `PlayerTimeInfo`, whereas original Psycle keeps LPB `8` and TPB `24` distinct.

Comparison receipt: `phase6c/evidence/sequencer-bpm-lpb-tick/comparison.json`.

## Classification boundary

This verdict does **not** classify:

- delayed-note or retrigger commands;
- extended tracker commands;
- tempo changes inside patterns;
- playback/render duration;
- Sampler PS1 or XMSampler tick processing;
- multi-sequence timing;
- UI editing semantics;
- sample-accurate equivalence beyond the two recorded sample rates.

Those remain independent contracts. The next Phase 6C evidence slice is the delayed/retrigger/extended-command contract.
