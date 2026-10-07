# Phase 6C — Legacy-lane closeout

The IT/MIDI detour has an **implementation closeout**, recorded in
[`legacy-closeout.json`](legacy-closeout.json). The repaired donor paths and
remaining capability/translation boundaries have explicit dispositions and
follow-up actions. **Private observation closeout remains pending.** This is
the intended v0.2.0 implementation checkpoint, not a new parity verdict.

## Repair and handoff ledger

| Boundary | Disposition | Retained scope / follow-up |
| --- | --- | --- |
| IT sample-mode instruments and routes | Repaired in C-Psycle | Canonical fixture, two identical non-silent 512-frame renders |
| Disabled/dormant SMF import path | Repaired in C-Psycle | Existing loader restored; pre-projection event graph recorded |
| MIDI releases and voice allocation | Repaired in C-Psycle | Equal-tick/terminal releases, channel matching, high notes and 64-voice reuse |
| MIDI timing/event semantics | Repaired in C-Psycle | Exact ticks, tempo maps, initial BPM, SysEx, running status, note/CC digests |
| Ended Sampulse foreground voice | Repaired in C-Psycle | Avoid period recalculation without a live sample controller |
| Frozen candidate direct IT/SMF loading | Handed off | Narrow host/song-I/O capability lane; establish original requirements first |
| IT note-cut → release compromise | Handed off | Pin original note-cut behavior before translation changes |
| Native MIDI instrument selection | Handed off | Deterministic Sampler projection proves execution; instrument choice remains separate |
| Original Sampler startup / render attribution | Handed off | Continue paired PS1 and delayed/retrigger evidence; donor fixes do not settle original behavior |

The ledger's `references` point to the retained source, procedures and regression
probes. `REPAIRED_DONOR_ONLY` describes the scoped donor repair, never original
Psycle compatibility. `HANDED_OFF` means the boundary has a named next action;
it does not mean the capability is implemented or verified.

The compatibility matrix, historical-input manifest and six-set corpus manifest
remain byte-identical to the PR #89 checkpoint. The closeout validator checks
their SHA-256 identities. Future changes must explicitly revise the closeout
snapshot rather than presenting changed inputs as this checkpoint.

## Validate the checkpoint

```bash
python3 tests/phase6c_legacy_closeout.py
python3 scripts/phase6c-legacy-closeout.py --output /tmp/legacy-closeout.json
```

Ordinary PR CI runs the new controls alongside the existing IT, historical and
MIDI contracts and the native donor execution probes. Its new
`phase6c-legacy-lane-closeout` artifact reports the implementation handoff with
both private observation gates pending. A successful validator exit means the
encoded closeout contract passed; it does not mean the private corpus ran.

## Outstanding private observations

| Lane | Masked repository secret | Combined artifact |
| --- | --- | --- |
| Historical `SickMaate` | `PSYCLE_PHASE6C_HISTORICAL_IT_URL` | `phase6c-historical-sickmaate-three-way-summary` |
| Six MIDI stem sets | `PSYCLE_PHASE6C_MIDI_CORPUS_URL` | `phase6c-midi-real-world-corpus-summary` |

Dispatch the existing manual workflows documented in
[`LEGACY_PLAYBACK_IMPORT.md`](LEGACY_PLAYBACK_IMPORT.md), then download and
extract each **combined** artifact. Raw IT/MIDI/ZIP/WAV bytes stay outside the
public evidence roots. Recheck the summary against its complete component files:

```bash
python3 scripts/phase6c-legacy-closeout.py \
  --historical-artifact /path/to/historical-summary \
  --midi-artifact /path/to/midi-summary \
  --output /tmp/legacy-closeout-observations.json
```

The tool reruns the existing summary/component validators, checks the recreated
summary matches, rejects missing or changed support files, and audits the roots
for private payloads and symlinks. MIDI evidence is revision-bound by its
existing validator: use a checkout of the exact workflow head to revalidate a
downloaded corpus artifact, rather than relabeling old evidence as a new run.

`COMPONENTS_REVALIDATED` means summary integrity under those existing contracts.
It is narrower than an independent execution replay or a new original-reference
behavioral verdict. Missing artifacts stay `PENDING_PRIVATE_DISPATCH`; malformed
supplied evidence fails validation. Neither a self-reported success flag nor an
inconclusive original observation clears the priority hold automatically.

## Return to the main evidence ladder

Once the private outcomes have been reviewed against the existing priority-hold
exit criteria and any new defect has been isolated or handed off, record the
observation identities and update the working roadmap. The return point is the
pending **Sampler PS1 E-D3/E-C3 paired runtime witness**, then the narrow
envelope/loop/panning/offset/volume/retrigger/state ladder in
[`SAMPLER_PS1.md`](SAMPLER_PS1.md). XMSampler and routing follow under normal
Phase 6C evidence rules. The confirmed Phase 7 serializer and TPB/extra-tick
backlog remains separate.

This implementation checkpoint leaves all existing matrix verdicts untouched:
three scoped `PASS`, one `MISSING`, one `DIFFERENT` and fifteen `UNKNOWN`.
The closeout report itself always retains `parity_status: UNKNOWN` and
`priority_hold: AWAITING_OBSERVATION_REVIEW`; it cannot authorize engine
convergence or deferred evidence expansion by itself.
