# Phase 6C delayed / retrigger / extended-command observation

## Status

**Original runtime execution-attempt observer implemented; compatibility remains `UNKNOWN`.**

This slice follows the classified BPM/LPB/tick result and measures the next
independent matrix contract, `sequencer-delayed-retrigger`. The final PR #69
push run `35457374404` at head
`9310b08bae744093c068a4f557bbd1b80bd499c8` remains the versioned comparison
under `phase6c/evidence/sequencer-delayed-retrigger/`.

That comparison still deliberately does **not** promote the matrix row. The
candidate has runtime scheduling evidence; the versioned original side has
native fixture-acceptance evidence plus separately pinned source semantics.
This follow-on adds the missing *execution-attempt observer* for original
runtime behaviour. The pinned reference must first pass the existing clean-load
gate before the observer touches the renderer. A renderer failure is retained as
an original-runtime observation, not converted into command-execution evidence.
No new `PASS`, `DIFFERENT` or `MISSING` verdict is permitted until a
command-bearing original runtime output exists and is compared like-for-like.

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

## Additive original execution witness

The stronger observer deliberately does **not** modify
`phase6c-delayed-retrigger.psy` or any of the PR #69/#70 versioned evidence.
Instead, `tests/phase6c_delayed_retrigger_execution_fixture.c` creates a
separate execution-only PSY3 witness with the same BPM/LPB/TPB family and a
deterministic 512-frame sample routed through the built-in Sampler. Its
four-frame impulse begins at frame 256, beyond the retained Sampler's default
~5 ms / ~221-frame attack at 44.1 kHz, so the witness remains observable after
the legacy attack ramp.

The commands are spaced into independent one-beat windows:

- beat 0: `FD 7F` on note 60;
- beat 1: `FB 3F` on note 60;
- beat 2: `FA 42` on note 60;
- beat 3: `FE 04`;
- beat 3.125: an ordinary note marker after the LPB command.

The native-Windows observer first applies the existing accepted-load, Load
Warning, VC90 identity and liveness gates. Only then does the new helper invoke
the source-pinned Psycle 1.12.0 File-menu command `Render as Wav...`
(command ID `32894`) and the exact `Render as Wav File` controls documented
by the pinned original resource definitions. Each render is forced to:

- output to file;
- record the entire song;
- 44.1 kHz;
- 16-bit PCM;
- mono mix;
- dither off;
- separated track/wire/generator outputs off.

The first render attempt records the exact process and output state after Save
Wave is dispatched. If that render succeeds and the reference remains alive,
the observer repeats the render in the same process. Only two byte-identical
WAV files that pass RIFF/PCM validation and expose multiple distinct impulse
onsets in both the `FB` and `FA` beat windows are allowed to set
`runtime_command_execution_observed: true`.

If the reference process exits during the verified render attempt, the observer
instead retains the pre-render clean-load predicate, non-zero process exit code,
exact diagnostic and hash/size binding for any created output. That path writes
`runtime_command_execution_observed: false` and remains `UNKNOWN`; it cannot
be used as a delayed/retrigger execution trace.

The first pinned-Windows attempt exposed exactly that second case: Psycle
1.12.0 reached the clean accepted-load gate and verified Save Wave dispatch,
then exited during offline rendering before a valid waveform was produced.
This is useful failure evidence, but it does **not** complete the original
command-execution observation. The next rung remains obtaining and versioning a
reproducible command-bearing original runtime output from this sampled witness,
then collecting candidate execution evidence from the **same** witness with a
comparable rendered-onset procedure before any timing classification. The
already-frozen PR #69 one-beat callback schedule remains preservation evidence;
it is not a like-for-like comparison partner for this separate four-beat
sampled witness.

## Original render-failure isolation

The first full sampled-witness render ends in a pinned-reference access
violation after verified Save Wave dispatch. That failure is not enough to say
whether the trigger is the shared sampled fixture/render path, one command
family, or an interaction that exists only when the commands are combined.

The next additive diagnostic lane therefore keeps the PR #71 full witness and
all frozen PR #69/#70 evidence unchanged and generates five separate PSY3
fixtures with the same BPM/LPB/TPB, built-in Sampler, 512-frame sample,
frame-256 impulse and Sampler-to-Master routing:

- `control`: ordinary notes only;
- `fd`: only `FD 7F` differs from the control;
- `fb`: only `FB 3F` differs from the control;
- `fa`: only `FA 42` differs from the control;
- `fe`: only `FE 04` plus its post-command marker differs from the control.

Each fixture is loaded and rendered in a **fresh Psycle process** under the
same installer, VC90, registry, machine/plugin, Load Warning and UI-identity
gates used by the full witness. The diagnostic render is attempted once. A
successful path must retain a hash-bound mono 44.1 kHz / 16-bit PCM output; a
reference-process exit must retain its non-zero exit code and hash/size binding
for any created output.

`scripts/phase6c-delayed-retrigger-render-isolation.py` derives only a failure
localization result. In particular:

- a control crash keeps the cause at the shared sampled-fixture/render layer;
- a clean control plus one-command crashes isolates a command-associated
  failure for follow-up;
- clean individual variants do not prove the full witness and instead leave a
  combined-command interaction or other full-witness difference open.

None of these diagnostic outcomes can change `sequencer-delayed-retrigger`
from `UNKNOWN`, and none is a substitute for the required same-witness,
comparable-procedure original/candidate execution pair.

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
