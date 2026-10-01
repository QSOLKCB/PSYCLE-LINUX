# Phase 6C — Legacy Playback / Import Priority Lane

## Why this lane is now first

Phase 6C had been expanding one isolated playback contract at a time. That remains valid, but a historical module importer crosses many of those same boundaries at once: file decoding, sequence construction, tracker-command translation, samples, loops, sampler state, tempo/speed conversion, routing and playback.

For that reason, **new Phase 6C expansion is temporarily deferred while this lane establishes a minimal legacy-import baseline**. Existing evidence and frozen receipts are not changed.

The priority is diagnostic, not architectural: use legacy formats to expose shared playback failures early, then return to the narrower parity ladder with better information.

## Temporary roadmap

While the legacy lane is active, **this document is the working roadmap**. The broader repository roadmap remains background context and resumes after these gates are satisfied.

1. [x] **Import/translation baseline** — canonical generated IT identity, donor import/decoder coverage, pinned-original acceptance observation, and frozen-candidate direct-import boundary.
2. [ ] **Sample-mode playback witness — current phase** — restore the missing C-Psycle sample-mode instrument/virtual-generator mapping and prove two fresh renders of the canonical generated fixture are bit-identical and non-silent.
3. [ ] **Historical `SickMaate` three-way summary** — run the hash-bound historical witness through the available original/candidate/donor observation paths without redistributing its samples.
4. [ ] **MIDI real-world playback corpus** — begin with the gentler baseline and then exercise dense timing/polyphony/routing cases.
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

[`reference-corpus/manifest.json`](reference-corpus/manifest.json) freezes the identities and roles of six contributor-supplied MIDI stem sets. They are not the first CI gate, but they are now the follow-on real-world playback corpus once the minimal legacy loader fixture is stable.

The corpus spans controlled generation, gentle real-world arrangements, dense polyphony and same-note overlaps, 16-channel routing stress, complex tempo maps, and malformed-but-usable key-signature metadata from a modern exporter.

This gives the project two complementary layers:

1. **minimal generated fixtures** that isolate one rule;
2. **real musical corpora** that reveal interactions and audible regressions.

## Exit criteria for the temporary priority hold

Resume the previously deferred Phase 6C expansion after all of the following are true:

- [x] generated IT fixture identity is CI-gated;
- [x] C-Psycle donor importer loads and validates the generated fixture's import/translation contract;
- [x] pinned original Psycle has a versioned generated-fixture acceptance observation;
- [x] frozen C++ candidate has a versioned direct-import capability observation;
- [ ] C-Psycle sample-mode import has a deterministic non-silent playback witness from the unchanged canonical fixture;
- [ ] the historical `SickMaate` witness has a hash-bound original/candidate/C-Psycle observation summary without redistributing its samples;
- [ ] the contributor-supplied MIDI corpus has begun real-world playback observations after the minimal IT witness is stable;
- [ ] any discovered shared playback defect has either been isolated or explicitly handed off to a narrow fix/evidence PR.

Only then return to the remaining Sampler PS1, XMSampler, routing and other Phase 6C rows.
