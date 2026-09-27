# Phase 6C — Sampler PS1 Compatibility

## Status

The broad **Sampler PS1 behaviour** row remains **UNKNOWN**.

This rung establishes a source-pinned semantic baseline only. It does not turn source
similarity or source differences into a runtime parity verdict.

The baseline is mechanically frozen in:

- `scripts/phase6c-sampler-ps1-contract.py`
- `phase6c/evidence/sampler-ps1/source-contract.json`
- `tests/phase6c_sampler_ps1_contract.py`

## Reference identities

Original Psycle remains the authoritative compatibility target:

- reference build: **Psycle 1.12.0 x86**
- source repository: `jpaquim/psycle`
- source commit: `7ac6d2c3553e2ee8dda55814d8e689919c345478`
- `psycle/src/psycle/host/Sampler.cpp` Git blob:
  `6cc0bd7328d01131c3d41b68f4e5d4189959e364`
- `psycle/src/psycle/host/Sampler.hpp` Git blob:
  `46da9fa80757b11ed146a21a529c70dbc6364f12`

The frozen C++ candidate remains the imported Phase 6B r12005 family:

- baseline:
  `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`
- `psycle-core/src/psycle/core/sampler.cpp` Git blob:
  `9edc00fb9013fbfde0bb0977398b934265b9c463`
- `psycle-core/src/psycle/core/sampler.h` Git blob:
  `f8df31889b2c0c4d89413db13fc5180fd5da4d53`

C-Psycle is retained only as supporting shared-contract source:

- `cpsycle/audio/src/sampler.c` Git blob:
  `475c96cc0742091b3b34aad634bd8d989caf83c8`
- `cpsycle/audio/src/sampler.h` Git blob:
  `e74dd3270f5581e17104006efe874299e974e94b`
- `cpsycle/audio/src/samplerdefs.h` Git blob:
  `88e4b0fa1d720e1dd567386270694052df0ecd61`

## Source-established common surface

All three source families retain the classic PS1 command identities used by this
baseline:

- `01` portamento up
- `02` portamento down
- `03` tone portamento
- `08` panning
- `09` sample offset
- `0C` volume
- `0E` extended command
- `15` retrigger
- extended `Cx` note-off
- extended `Dx` note-delay

Original and frozen candidate also retain 16 maximum voices and 8 default voices.

These are source facts, not proof that the commands execute identically.

## Source-level preservation boundaries

The baseline deliberately records differences rather than hiding them.

### Pitch / sample-rate basis

Pinned original Psycle derives ordinary note speed using the **sample's own sample
rate divided by output sample rate**.

The frozen C++ candidate uses a **44.1 kHz constant divided by output sample
rate** in its older Sampler path.

C-Psycle's current PS1 implementation again uses the sample's own sample rate.

This makes a non-44.1-kHz sample the highest-value first runtime witness. The
source discrepancy is **not** classified as `DIFFERENT` until like-for-like
original and candidate execution is captured.

### Extended note timing

Pinned original Psycle expresses PS1 extended `Cx` / `Dx` timing in fixed
sixths of a **row**.

The frozen candidate source spells those calculations as
`samplesPerTick()/6`. For **loaded PSY3 songs**, however, the retained loader
places LPB in `tick_speed`, `PlayerTimeInfo` computes
`samplesPerTick = samplesPerBeat / tick_speed`, and the committed BPM/LPB
candidate receipt verifies at both 44.1 kHz and 48 kHz that
`samples_per_tick == samples_per_fixture_line`. In that qualified input mode,
the candidate's effective interval is therefore **row/6**, matching the original
timing basis despite the different API name.

The early candidate `VoiceTick` branch special-cases `E-D0`; the later
note-initialization path assigns `_triggerNoteDelay` for nonzero `E-Dx`.

C-Psycle's PS1 path also uses row-based sixths.

A dedicated `E-Dx` / `E-Cx` runtime witness is still useful to confirm command
execution, but it is no longer justified as a row-versus-tick discrepancy for
loaded PSY3 songs.

### Machine-state chunk generation

The pinned source generations are not identical:

- original Psycle Sampler internal version: **2**
- frozen C++ candidate Sampler internal version: **1**
- current C-Psycle PS1 Sampler version: **3**

Original version 2 persists both corrected-C4 state and linear-slide mode.
The frozen candidate version 1 persists corrected-C4 state but predates that
second field.

This source fact does not by itself establish save/reopen incompatibility; a
runtime state-roundtrip fixture is required.

## Shared source semantics retained for later witnesses

The baseline also pins source markers for:

- 44.1-kHz-normalized amplitude-envelope scaling;
- normal-loop wrap at the loop end;
- the historical 0.5-per-side panning destination clamp;
- the classic PS1 command table and polyphony defaults.

Runtime fixtures must still prove the externally visible behaviour.

## Next evidence ladder

The next Sampler PS1 rungs are intentionally ordered by the strongest
source-indicated risk:

1. **Non-44.1-kHz pitch witness** — use one project-authored sample whose embedded
   sample rate is not 44.1 kHz, render the same note under pinned original Psycle
   and the frozen candidate, and compare pitch/duration with identity-bound
   receipts.
2. **PS1 extended timing witness** — exercise `E-Dx` and `E-Cx` on a dedicated
   PS1 Sampler fixture and retain exact onset/off timing from both sides.
3. **Envelope / loop / panning / offset / volume / retrigger** — add one narrow
   fixture per behaviour rather than one opaque omnibus song.
4. **Sampler state round trip** — save/reopen corrected-C4, slide mode,
   polyphony/resampling state where the relevant source generation supports it.

Only evidence-backed subcontracts may become `PASS`, `DIFFERENT`, or
`MISSING`. The broad `sampler-ps1` matrix row remains `UNKNOWN` until the
required runtime surface is covered.
