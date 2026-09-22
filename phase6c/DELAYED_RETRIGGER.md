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

The first complete diagnostic run, workflow `35723698458` at head
`1a513153f8caf994114a14a510ca9efc683541ba`, passed the candidate and original
evidence validators and produced `shared-sampled-fixture-or-render-path-failure`.
The ordinary-note control and all four single-command variants reached their
clean pre-render load gates, verified Save Wave dispatch, then exited with
Windows status `0xC0000005` (`-1073741819`) and retained zero-byte outputs.
Because the control fails identically, this evidence does **not** attribute the
crash to `FD`, `FB`, `FA` or `FE`; the next investigation boundary is the
shared sampled-fixture/original-render path itself.

## Original render-substrate isolation

The follow-on substrate ladder keeps every PR #69/#70 receipt, the PR #71 full
execution witness and the PR #72 command-isolation witnesses unchanged. It
creates four cumulative PSY3 fixtures and runs each in a fresh pinned Psycle
process:

1. `master-only` — Master plus the four-beat song geometry only;
2. `sampler-empty` — adds the built-in Sampler wired to Master;
3. `sample-state` — adds the deterministic 512-frame sample and instrument
   state but no note;
4. `ordinary-note` — adds one ordinary note at beat 0.

The first complete substrate observation, workflow `35730075034` at evidence
head `0a99e8a7af3aa2d714e1c85480d16ddaf62c9ab0`, establishes a narrower
runtime boundary.

The first three rungs all:

- pass the clean original-reference load/identity gate;
- reach verified Save Wave dispatch;
- keep the Psycle process alive with no exit code;
- produce the same finalized mono 44.1 kHz / 16-bit PCM WAV;
- retain file size `154412` bytes and SHA-256
  `bfcdb0b89773f22484650ad80fc52354ee7c584f89c272d0e8f67ee5546a6e8c`;
- contain `77184` zero-valued PCM frames (about 1.7502 seconds);
- remain byte-stable for at least four polls.

Those controls are deliberately recorded as
`stable-finalized-output-process-alive`, **not** as a normally completed
Render-dialog transaction: the `Render as Wav File` dialog remained open and
did not reach the helper's expected terminal Close state.

The fourth rung changes only by adding one ordinary note. It reaches the same
clean pre-render gate and verified Save Wave dispatch, then the reference
process exits with Windows status `0xC0000005` (`-1073741819`) while the
requested WAV remains zero bytes.

Therefore the evidence does not implicate mere Sampler presence or embedded
sample/instrument state. It narrows the observed process-exit boundary to an
**ordinary-note-present Sampler execution path after three stable-output
controls**. This is an association boundary, not a claim that note playback has
already been proven as the unique root cause.

Pinned original source is consistent with that next focus:
`Player::StartRecording()` changes the offline sample rate before recording,
`Player::SampleRate()` propagates that rate to every present machine, and
`Player::Work()` executes sequencer notes before recursively processing the
Master graph and writing the recording buffer. The next diagnostic step should
therefore isolate ordinary-note routing / Sampler voice startup before
returning to delayed/retrigger timing.

This substrate evidence still does **not** provide a command-bearing original
runtime output and does not change `sequencer-delayed-retrigger` from
`UNKNOWN`.

## Sampler voice-startup isolation

The next additive lane binds its interpretation directly to original Psycle
source commit `7ac6d2c3553e2ee8dda55814d8e689919c345478`. The evidence receipt pins
`Sampler.cpp` blob `6cc0bd7328d01131c3d41b68f4e5d4189959e364` and
`Song.cpp` blob `9ed00469d29d0a5714cc7b86175d0dc4c6048e3a`.

The pinned source establishes these gates:

- the Sampler constructor initializes every `lastInstrument[]` entry to 255;
- instrument `FF` with no previous instrument returns before sample lookup;
- a disabled/missing sample slot returns before voice selection;
- an enabled sample advances through `GetFreeVoice()` into `Voice::Tick()`;
- original Song construction allocates a default legacy `Instrument` for every slot;
- `Voice::Tick()` resolves that instrument and binds the enabled sample before audio work.

Four fresh-process PSY3 fixtures advance through those boundaries:

1. `note-no-previous-inst` — note 60, instrument `FF`; sample 0 is present but
   the source-pinned no-previous-instrument gate must return first;
2. `note-missing-sample` — note 60, instrument 1; sample 1 is absent, so the
   source-pinned sample-enabled gate must return before voice selection;
3. `note-sample-default-inst` — note 60, sample 0 enabled, no serialized
   C-Psycle instrument state; original Psycle uses its constructor-created
   default legacy Instrument and enters voice startup;
4. `note-sample-serialized-inst` — the same enabled sample plus serialized
   instrument state.

Workflow `35734490424` at evidence head
`c4c4b55900d1444cbe644345e559067d3996c532` completes both candidate and
pinned-Windows validation. The first two controls keep the reference process
alive and each produce the same finalized 154,412-byte silent mono PCM output
(SHA-256
`bfcdb0b89773f22484650ad80fc52354ee7c584f89c272d0e8f67ee5546a6e8c`,
77,184 zero-valued frames). Both enabled-sample rungs instead reach verified
Save Wave dispatch, exit with Windows status `0xC0000005`
(`-1073741819`), and retain zero-byte outputs.

The derived diagnosis is
`enabled-sample-voice-startup-associated-exit`. In particular, serialized
C-Psycle instrument state is **not** the differentiator: the constructor-default
original Instrument crashes at the same boundary. The next diagnostic rung
must therefore split the operations after `samples.IsEnabled()`: voice
selection / `Voice::Tick()` setup versus the first `Voice::Work()` audio
operation. This remains an association boundary rather than proof that any one
statement inside those functions is the unique fault.

This result still does **not** provide a command-bearing delayed/retrigger
runtime output and does not change `sequencer-delayed-retrigger` from
`UNKNOWN`.

## Sampler Voice::Tick / Voice::Work boundary isolation

The next additive lane keeps the same pinned original Psycle commit
`7ac6d2c3553e2ee8dda55814d8e689919c345478` and binds its interpretation to
`Sampler.cpp` blob `6cc0bd7328d01131c3d41b68f4e5d4189959e364`,
`Sampler.hpp` blob `46da9fa80757b11ed146a21a529c70dbc6364f12`, and
`SongStructs.hpp` blob `3ad8cd1b1a0a41bc2225cfd3070bf205cbebec31`.

The source-bound ladder separates four points:

1. `release-no-active-voice` — enabled sample 0 plus release note 120 on an
   empty track. Original `Sampler::Tick()` passes the sample-enabled gate but
   returns because no voice is active, before `Voice::Tick()`;
2. `delayed-note-short` — note 60 with Sampler-local extended command
   `0x0E/0xDF` in a one-row song. This is distinct from global
   `PatternCmd::NOTE_DELAY = 0xFD`, so `Player::ExecuteNotes()` forwards the
   event into `Sampler::Tick()`. In `Voice::Tick()`, `E-DF` sets
   `_triggerNoteDelay = 15/6` rows = 2.5 rows and leaves the envelopes
   `ENV_OFF`; because the song is only one row, the delay cannot expire and
   `Voice::Work()` must return before `controller.Work()`;
3. `delayed-note-long` — the same `E-DF` event in a four-beat song, long
   enough for the 2.5-row delay to expire and make sample work reachable;
4. `ordinary-note-short` — a one-row ordinary note, making sample work
   reachable immediately.

Workflow `35752301481` at evidence head
`4cf71eba6f7b5f6ba14be6089cf37422b16b5ca6` completes the candidate and
pinned-Windows lanes. The release/no-active-voice control keeps the reference
process alive and retains a finalized 4,868-byte mono PCM WAV with 2,412
zero-valued frames, SHA-256
`f40e6f9f280c993b8c0ecdf6a54ba8314462803b7075e46d7db0c4a2bd9dfbae`.
The other three fixtures all reach verified Save Wave dispatch, exit with
Windows status `0xC0000005` (`-1073741819`), and retain zero-byte WAVs.

The derived diagnosis is `voice-tick-initialization-associated-exit`. The
one-row delayed fixture is the key discriminator: the crash occurs even though
its 2.5-row delay cannot expire before the song ends, so no successful
`Voice::Work()` path can reach `controller.Work()`. The current boundary is
therefore inside `Voice::Tick()` after voice selection and before normal audio
work. The remaining source order to isolate is legacy instrument/sample binding
and pitch/speed calculation, resampler allocation/update, then
envelope/pan/filter initialization.

This remains an association boundary, not proof that any specific
`Voice::Tick()` statement is the unique fault. It still does **not** provide a
command-bearing delayed/retrigger runtime output and does not change
`sequencer-delayed-retrigger` from `UNKNOWN`.

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
