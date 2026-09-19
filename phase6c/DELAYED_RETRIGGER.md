# Phase 6C delayed / retrigger / extended-command observation

## Status

**Versioned deferred comparison; compatibility remains `UNKNOWN`.**

This slice follows the classified BPM/LPB/tick result and measures the next
independent matrix contract, `sequencer-delayed-retrigger`. The final PR #69
push run `35457374404` at head
`9310b08bae744093c068a4f557bbd1b80bd499c8` is now versioned under
`phase6c/evidence/sequencer-delayed-retrigger/`.

The comparison has been performed, but it deliberately does **not** promote the
matrix row. The candidate has runtime scheduling evidence; the original side
has native fixture-acceptance evidence plus separately pinned source semantics.
Those are not like-for-like execution observations, so `PASS`, `DIFFERENT`
and `MISSING` remain unsupported until a stronger original runtime execution
observer exists.

## Project-authored fixture

`tests/phase6c_delayed_retrigger_fixture.c` generates one PSY3 fixture with
BPM `137`, LPB `8`, TPB `24`, extra ticks `0`, and a built-in Sampler at
machine 0. The first row contains four independent command cases on separate
tracks:

- `FD 7F` — note delay;
- `FB 3F` — retrigger;
- `FA 42` — retrigger continue;
- `FE 04` — legacy extended command changing lines per beat to 4.

A normal note marker is placed on the following source row. C-Psycle creates,
saves and freshly reloads the fixture only to prove the intended bytes/commands
were constructed. Its event sequencer is not original-Psycle authority.

## Frozen candidate observation

`tests/phase6c_delayed_retrigger.cpp` loads the exact fixture against the
unchanged Phase 6B C++ core. It does not start playback or require an audio
driver.

The project-authored probe replaces the loaded Sampler only inside the test
harness with a recording Sampler subclass. A test-only visibility shim exposes
the frozen `Sequencer::execute_notes()` entry point; the engine implementation
itself is unchanged. The recorder captures the actual beat offsets scheduled by
the frozen command path.

The validator rederives and checks:

- the loader's `FD` parameter conversion;
- the complete `FD` scheduled offset;
- the complete `FB` retrigger offset series;
- the complete `FA` retrigger-continue offset series;
- the `FE04` effect on the following loaded pattern-row position;
- BPM, candidate tick mode and float timing values;
- exact fixture/source/probe/log identities;
- a fail-closed structured diagnostic allowlist.

Any unexpected diagnostic, malformed field, altered offset, nonzero exit,
unsafe artifact path or changed source identity makes the candidate observation
inconclusive. The exact executed probe is preserved and hash-bound in the
artifact.

## Pinned original evidence

The original side deliberately separates two evidence types.

### Native Windows load observation

A generated adapter extends the mature hash-pinned Windows observer with one
explicit `-ObserveDelayedRetrigger` fixture. Pinned Psycle 1.12.0 x86 must
accept the exact same fixture bytes under the existing installer, VC90,
DirectSound, Load Warning, runtime-identity, UI and liveness gates.

This receipt proves native runtime acceptance of the command fixture. It does
**not** pretend that visible UI text is a command execution trace.

### Pinned original source semantics

The Linux evidence job also downloads only two files from original Psycle
source commit
`7ac6d2c3553e2ee8dda55814d8e689919c345478` and validates their Git blobs:

- `Player.cpp` — `b5c962cbcc15f267b9fa1ffe1c9ab21d881b8dbc`;
- `SongStructs.hpp` — `3ad8cd1b1a0a41bc2225cfd3070bf205cbebec31`.

The source receipt freezes the original command IDs and implementation rules,
including:

- `FD`: trigger-delay counter uses
  `((parameter + 1) * SamplesPerRow()) / 256`;
- `FB`: retrigger rate is `parameter + 1`;
- `FA`: a nonzero high nibble overrides the continuing retrigger rate;
- `FE00..FE1F`: the global command changes LPB via
  `SetBPM(-1, parameter)`.

The upstream source bytes are transient CI inputs and are not vendored by this
lane.

## Versioned comparison decision

The final clean PR #69 evidence is committed as four field-preserving
projections:

- `candidate-delayed-retrigger.json` — frozen-candidate runtime scheduling;
- `original-source-delayed-retrigger.json` — pinned original source semantics;
- `original-delayed-retrigger.json` — pinned native-Windows fixture acceptance;
- `comparison.json` — the evidence-role comparison and decision.

Each projection binds the final Actions run, job/artifact identities, artifact
digest and raw receipt SHA-256. The matrix validator independently freezes the
fixture identity, candidate schedules, original source commit/blob identities,
accepted native load and the absence of an original command-execution trace.

The comparison result is:

- candidate command execution: **observed**;
- original fixture acceptance: **observed**;
- original source command semantics: **observed from pinned source**;
- original runtime command scheduling/callback timing: **not observed**;
- evidence symmetry: **source-plus-load vs candidate-runtime**;
- compatibility verdict: **`UNKNOWN`**;
- classification allowed: **false**.

This is a completed comparison decision, not an unperformed comparison. The
remaining tracker-command rung is to obtain a stronger original-Psycle execution
observation—such as runtime scheduling/callback evidence or another
deterministic execution output that can be compared like-for-like—before any
`PASS`, `DIFFERENT` or `MISSING` claim is permitted.

## Epistemic boundary

The three evidence roles remain separate:

1. candidate execution scheduling;
2. pinned original-source command semantics;
3. pinned original native-Windows fixture acceptance.

All retain `parity_status: UNKNOWN`. Source formulas plus load acceptance are
useful evidence, but they are not a native runtime command trace and are not
treated as one by the matrix gate.

This slice does not classify sampler audio output, envelopes, note lifetime,
tempo changes, pattern loops/delays, row extra ticks, mixer effects, or general
playback/render duration.
