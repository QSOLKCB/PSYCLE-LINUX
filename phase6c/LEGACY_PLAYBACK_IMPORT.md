# Phase 6C — Legacy Playback / Import Priority Lane

## Why this lane is now first

Phase 6C had been expanding one isolated playback contract at a time. That remains valid, but a historical module importer crosses many of those same boundaries at once: file decoding, sequence construction, tracker-command translation, samples, loops, sampler state, tempo/speed conversion, routing and playback.

For that reason, **new Phase 6C expansion is temporarily deferred while this lane establishes a minimal legacy-import baseline**. Existing evidence and frozen receipts are not changed.

The priority is diagnostic, not architectural: use legacy formats to expose shared playback failures early, then return to the narrower parity ladder with better information.

## Temporary roadmap

While the legacy lane is active, **this document is the working roadmap**. The broader repository roadmap remains background context and resumes after these gates are satisfied.

1. [x] **Import/translation baseline** — canonical generated IT identity, donor import/decoder coverage, pinned-original acceptance observation, and frozen-candidate direct-import boundary.
2. [x] **Sample-mode playback witness** — C-Psycle sample-mode instrument/virtual-generator mapping is restored and the canonical generated fixture has a deterministic non-silent donor playback witness.
3. [ ] **Historical `SickMaate` three-way summary — private observation pending** — the privacy-preserving original/candidate/donor workflow is implemented; the three-way observation remains pending manual dispatch with the private historical input.
4. [ ] **MIDI real-world playback corpus — current phase** — exercise the contributor-supplied SMF corpus from gentle baseline through dense timing/polyphony/routing stress, keeping native MIDI import separate from the deterministic audibility projection.
5. [ ] **Legacy-lane closeout** — isolate or hand off any remaining shared playback defects, then resume the deferred Phase 6C contract ladder.

No item in this temporary roadmap may promote original-Psycle parity without the normal paired/versioned evidence requirements.

## First historical witness: `SickMaate` (`.it`)

The first external reference is a contributor-authored Impulse Tracker module created in 2002–2003.

The exact historical file is **not committed** because its embedded PCM includes sample names/credits that indicate third-party source material. It is bound by SHA-256 in [`evidence/legacy-module-import/historical-sickmaate.json`](evidence/legacy-module-import/historical-sickmaate.json) and may be used locally/privately for observation where the operator already possesses the file.

High-value properties include:

- 81 orders / 19 patterns / 20 used channels;
- sample mode (no IT instruments), reducing the first importer scope;
- 25 IT-compressed sample payloads and 11 loops;
- fixed initial speed 4 / tempo 140 with linear slides;
- 138 IT note-cut events;
- substantial E/F/G portamento use;
- global-volume commands;
- 480 `Z58` MIDI/filter-macro events.

The historical file is evidence input, not an oracle. Original-Psycle behavior must still be observed under the pinned 1.12.0 x86 reference.


### Current historical-observation procedure

The repository now includes a **manual private-input workflow** at
`.github/workflows/phase6c-historical-it-private.yml`.

The historical module is never committed and is never uploaded as a GitHub
Actions artifact. To execute the three-way observation, the repository owner
provides a private download URL through the masked repository secret:

```text
PSYCLE_PHASE6C_HISTORICAL_IT_URL
```

Each donor/candidate/original job downloads the file independently into runner
temporary storage and first verifies all of the following before use:

- filename identity from the manifest;
- exact size: **306,462 bytes**;
- exact SHA-256:
  `cab23d8f66a6815b3248457e4f38de74f0a6d61c2691c47a78e582edb63cd432`;
- `IMPM` file magic;
- embedded title `SickMaate`.

The donor job records import structure plus a non-silent Sampulse playback
witness. The frozen C++ candidate records its direct-IT-load boundary. The
native-Windows original-reference job uses a generated historical observer that
copies the module only into its transient work root; the uploaded original
evidence contains an `external/...` identity marker and hash, never module
bytes.

A final job combines the three sanitized receipts into
`historical-sickmaate-three-way.json` with `parity_status: UNKNOWN`.
That summary is descriptive evidence only and cannot promote a compatibility
matrix row.

**Current status:** the private-input workflow and validators are implemented;
the historical three-way observation remains pending until that manual workflow
is dispatched with the private URL secret configured.

## Redistributable CI fixture

`scripts/phase6c-generate-it-import-fixture.py` generates a 4,439-byte, project-owned IT 2.14 fixture from first principles. It contains no historical sample bytes.

It deliberately exercises:

- sample-mode IT loading;
- deterministic signed 8-bit PCM with a 4,096-frame forward loop, used by the current sample-mode playback witness after the missing instrument/virtual-generator mapping is restored;
- speed 4 / tempo 140;
- linear-slide mode;
- `E10` portamento down;
- `F10` portamento up;
- `G08` tone portamento;
- `V40` global volume;
- `Z58` MIDI/filter macro;
- direct IT note-cut (`254`);
- `C00` pattern break.

Canonical generated identity:

```text
size:   4,439 bytes
sha256: f02f5b8d1de98d4c6bcf00e2d6a04617e0714e4bcd7e299b5bc56d057526cae1
```

The fixture is generated during CI rather than committed as an opaque binary.

## C-Psycle donor boundary

The retained C-Psycle source recognizes the `IMPM` signature and dispatches to `ITModule2`. Its importer already contains compressed-sample decoding, sample loops and sample-rate metadata, speed-to-LPB conversion, E/F/G command translation, global-volume translation and Zxx MIDI/filter-macro translation.

It also contains an explicit compatibility compromise:

```text
IT note cut 254 -> Psycle release
```

because the donor comments that Psycle has no distinct note-cut note. The generated runtime probe freezes that **donor import/translation observation only**. It does not claim that original Psycle behaved the same way.

### Current phase — sample-mode playback witness

The merged import baseline deliberately stopped short of audible sample-mode playback. The retained importer loaded sample PCM correctly but did not materialize the Sampulse instrument and virtual-generator route required for sample-mode IT notes to start a voice.

The current phase repairs only that missing donor mapping:

- each successfully loaded sample-mode IT sample receives a Sampulse instrument entry in instrument group 1;
- the instrument entry maps the full default key range to the imported sample;
- the sample number is bound to a virtual generator targeting the imported Sampulse machine;
- the existing imported pattern note is required to route through that virtual generator.

The playback witness then loads the **unchanged canonical generated IT fixture** twice from fresh state and renders the same first note through the real imported Sampulse path. Completion requires:

- a materialized sample-mode instrument;
- a materialized virtual-generator route;
- a non-zero audio block;
- two fresh 512-frame renders that are bit-identical;
- a separate hash-bound donor playback receipt.

This remains **C-Psycle donor evidence only**. It does not classify original Psycle or the frozen C++ candidate, and the compatibility matrix remains unchanged.

## Candidate boundary

The sanitized C++ candidate contains substantial IT-compatible XMSampler semantics, including MIDI macro/filter logic, but this lane has not yet established a corresponding `.it`/`IMPM` file-loader entry point in the retained candidate tree.

The next candidate question is intentionally narrow:

> Can the frozen candidate directly accept the generated `.it` fixture, or is legacy module import a missing host/song-I/O capability around an otherwise capable sampler?

Do not classify `MISSING` until that runtime/source boundary is recorded in a versioned receipt.

## Original reference boundary

Use pinned Psycle 1.12.0 x86 and the same observation discipline as the existing Phase 6C lanes.

For both the generated fixture and the hash-bound historical module, record separately: file acceptance/rejection; imported pattern/order geometry; sample count/decode success; speed/tempo result; command translation where observable; playback/render outcome; and any crash, warning, normalization or unsupported feature.

A remembered historical behavior is useful motivation, not a PASS verdict.

## MIDI reference corpus

[`reference-corpus/manifest.json`](reference-corpus/manifest.json) is now an executable schema-v2 evidence contract for six contributor-supplied MIDI stem sets. The real archive bytes remain external to the repository and Actions artifacts; their archive SHA-256 identities, aggregate source observations and stress roles are frozen in the manifest.

### Current MIDI procedure

The private real-world workflow is:

`.github/workflows/phase6c-midi-corpus-private.yml`

It is manual (`workflow_dispatch`) and uses the masked repository secret:

```text
PSYCLE_PHASE6C_MIDI_CORPUS_URL
```

The URL points to an **outer transport ZIP** containing the six exact contributor archives. The outer ZIP has no evidence identity of its own. Each inner archive is located by its frozen filename and accepted only if its SHA-256 matches the manifest.

The stress progression is intentionally:

1. **FM Doom** — gentler real-world baseline;
2. **Celestial Mechanics** — older composition/export behaviour;
3. **Deterministic Pattern** — controlled-generation/follow-tempo behaviour;
4. **Blue Glare** — dense modern tempo-map workload;
5. **Polyrhythmic Patterns** — overlap/polyphony/event-ordering stress;
6. **NGC3603 Quantum Demoscene** — final routing stress, including all 16 MIDI channels.

For every stem, the private-input analyser independently parses the Standard MIDI File and freezes the observable source structure: SMF type, PPQN, note balance, note count, same-note overlaps, cross-track polyphony, tempo map, channel use, duration, zero-duration pairs and malformed key-signature metadata. The real-world exporter edge is retained rather than normalized away.

### C-Psycle donor boundary

C-Psycle has a real `MThd`/`MidiLoader` song-I/O path. At the start of this phase the retained loader source was already compiled into `libaudio`, but dispatch was disabled by the preserved `PSYCLE_USE_MIDI_FILE` feature gate in `cpsycle/detail/psyconf.h`. This lane restores that existing gate rather than adding a replacement MIDI importer.

Native MIDI import creates sequence tracks and tracker events, but it **does not choose or instantiate a sound-generating machine**. Therefore this lane records two different claims:

1. **native import evidence** — load acceptance plus a digest/count of the untouched imported event graph;
2. **execution projection** — after the untouched graph is frozen, imported note/release/MIDI-CC events are routed through one deterministic project-owned Sampulse substrate and rendered through the production Player/FileOutDriver path.

The projection keeps imported note numbers, event offsets, track geometry and tempo commands intact. It supplies only the missing sound source needed for audibility and is explicitly **not** evidence of native MIDI instrument-selection semantics.

Real-corpus renders are bounded to the first 16 beats per imported pattern. The WAV files are used transiently for non-silence/hash observations and then deleted; uploaded evidence contains only sanitized JSON, the exact probe binary and the retained `midiloader.c` source used.

### Frozen C++ candidate boundary

The frozen sanitized C++ candidate has internal MIDI-event machinery but no retained Standard MIDI File loader entry point (`MThd`/`MidiLoader`). The private workflow therefore runs one manifest-bound representative stem from each set against the frozen player and records the direct-load capability boundary only. A direct-load rejection does not imply that its internal MIDI event or plugin semantics are absent.

### Original-reference boundary

Pinned Psycle 1.12.0 x86 is **not observed in this corpus phase**. This lane is real-world donor robustness evidence, not a new parity row. If an original-reference MIDI claim is later required, it must be added as a separately version-pinned observation rather than inferred from historical memory or C-Psycle behaviour.

### Continuous CI boundary

Ordinary PR CI does not need the private corpus. It generates a small project-owned SMF1/480 fixture and runs the same C-Psycle probe end-to-end through:

```text
SongReader / MidiLoader
  -> untouched event-graph digest
  -> deterministic Sampulse projection
  -> Player
  -> FileOutDriver
  -> non-silent WAV validation
```

The contract tests also verify malformed-key-signature preservation, cross-track polyphony, candidate source boundaries and raw-MIDI/ZIP privacy rejection.

This gives the project two complementary layers:

1. **minimal generated fixtures** that continuously isolate and exercise the execution path;
2. **real musical corpora** that reveal interactions and audible regressions under private, identity-bound observation.

## Exit criteria for the temporary priority hold

Resume the previously deferred Phase 6C expansion after all of the following are true:

- [x] generated IT fixture identity is CI-gated;
- [x] C-Psycle donor importer loads and validates the generated fixture's import/translation contract;
- [x] pinned original Psycle has a versioned generated-fixture acceptance observation;
- [x] frozen C++ candidate has a versioned direct-import capability observation;
- [x] C-Psycle sample-mode import has a deterministic non-silent playback witness from the unchanged canonical fixture;
- [ ] the historical `SickMaate` witness has a hash-bound original/candidate/C-Psycle observation summary without redistributing its samples;
- [ ] the contributor-supplied MIDI corpus has begun real-world playback observations after the minimal IT witness is stable;
- [ ] any discovered shared playback defect has either been isolated or explicitly handed off to a narrow fix/evidence PR.

Only then return to the remaining Sampler PS1, XMSampler, routing and other Phase 6C rows.
