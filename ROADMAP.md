# PSYCLE-LINUX Roadmap

## Guiding Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

The target is the **behaviour and workflow of original Psycle**, not whichever historical reimplementation happens to be easiest to compile.

A second rule follows from Psycle's plugin history:

> **Preserve source broadly, but keep the default runtime surface deliberate and reliable.**

Optional machines and third-party plugins are part of Psycle's history, but they must not make the host fragile merely because they exist in a scan directory.

---

## Architecture Clarification — September 2026

Direct clarification from long-time Psycle maintainer **JosepMa / JAZ** established a more precise upstream lineage than this project originally assumed.

### Original Psycle

`psycle/` is the original C++ Psycle application.

It is strongly tied to Microsoft Visual Studio and MFC, with additional Windows-oriented SDKs and libraries. It remains the primary **behavioural and UI reference**, but it is not a practical direct Linux implementation base without replacing substantial MFC UI/platform code.

Because original Psycle evolved across releases, “original Psycle” is not by itself a reproducible oracle. Before any PASS / DIFFERENT compatibility claim is accepted, Phase 6 must identify the exact original-Psycle source revision and/or executable build used for that observation, record its provenance and checksum where practical, and document the runtime/observation method.

### First cross-platform reimplementation

The first C++ reimplementation was split across projects/directories including:

- `psycle-core`
- `psycle-audiodrivers`
- `psycle-helpers`
- `psycle-player`
- `psycle-plugins`

The historical C++ build also depends on the top-level `universalis` support project, so Phase 6 pins it as part of the coherent build-source snapshot.

According to maintainer clarification, that reimplementation was buildable on Debian Linux and could play most Psycle songs, but it was not fully up to date with original Psycle playback and provided a player rather than the full tracker application.

**This family is now the leading candidate engine base for a faithful Linux Psycle.**

### Later C-Psycle reimplementation

`cpsycle/` is a later C reimplementation with its own UI toolkit, tracker and event/sequencer architecture.

C-Psycle shares substantial concepts and compatibility goals with Psycle, but it is **not the original Psycle port** and intentionally diverged in some areas. Its own developer guide describes it as a C-version/variant and documents differences such as the event-oriented sequencer.

The current repository's extensive work on C-Psycle remains valuable as:

- an audited preservation baseline;
- a Linux implementation donor;
- a machine/plugin behaviour oracle;
- a `.psy`/preset/state test corpus;
- a timing/sampler compatibility laboratory;
- a source of regression infrastructure;
- an independent reference when checking `psycle-core` compatibility.

It is **not discarded**, but it is no longer treated as the final implementation architecture.

### Evidence hierarchy

When compatibility questions arise, use this order:

1. a **version-pinned original Psycle source/build and reproducible observation** where legally and technically available;
2. reproducible historical songs/tests and preserved machine contracts tied to a known Psycle version where applicable;
3. `psycle-core` family behaviour as the candidate cross-platform engine;
4. C-Psycle source plus the extensive regression evidence built in this repository;
5. retained developer documentation and historical branches/posts as architecture/intent evidence.

If multiple original Psycle versions behave differently, the parity report must record that difference rather than collapsing them into one unnamed “original Psycle” result.

The retained `cpsycle/doc/cpsycle-developer-guide.txt` remains important, but it is explicitly **C-Version, Feb 2021 (unfinished)**. See `UPSTREAM_ARCHITECTURE.md`.

---

# Completed Preservation Track — C-Psycle

The work below remains part of PSYCLE-LINUX because it establishes compatibility evidence that will be reused during engine convergence.

## Phase 0 — Project Foundation

- [x] Establish mission and scope.
- [x] Record the selected C-Psycle baseline.
- [x] Record source archive identity/SHA-256.
- [x] Define porting principles and non-goals.
- [x] Document licensing/provenance boundaries.
- [x] Add contribution and credit documentation.

**Status:** complete.

## Phase 1 — Audited C-Psycle Baseline

- [x] Import SourceForge `trunk/cpsycle` revision `r12005` mechanically close to upstream.
- [x] Preserve notices and attributions.
- [x] Create sanitized archival and canonical audited references.
- [x] Inventory bundled third-party code/licenses.
- [x] Document whole-file omissions and provenance-safe replacements.
- [x] Map the source tree for maintainers.

Important VST2 provenance boundary established here:

- [x] Do not redistribute Steinberg-SDK-derived `aeffect.h`.
- [x] Do not redistribute Steinberg-SDK-derived `aeffectx.h`.
- [x] Do not redistribute Steinberg-SDK-derived `vstfxstore.h`.
- [x] Preserve Psycle-owned/GPL host-side code separately from omitted SDK material.

**Status:** complete.

## Phase 2 — Modern Linux Build Audit

- [x] Reproduce the C-Psycle make-based build on Ubuntu 24.04 x86-64.
- [x] Inventory development packages and build-order assumptions.
- [x] Build core libraries independently.
- [x] Audit X11/Xft, Lua, Lilv/LV2 and Linux driver integration.
- [x] Audit ALSA, ALSA MIDI, JACK, SDL2 and event-joystick targets.
- [x] Audit `psyplayer` separately.
- [x] Add reproducible build documentation and CI evidence.

**Status:** complete for the C-Psycle baseline.

## Phase 3 — Native C-Psycle Linux Host

Automated host/runtime work:

- [x] Build and launch the C-Psycle native X11 host in CI.
- [x] Prove writable isolated Linux configuration state.
- [x] Dynamically select/load SDL2 audio.
- [x] Complete real host audio callbacks using SDL dummy audio.
- [x] Exercise editor/focus/tracker/FileView paths and clean X11 shutdown.

Physical hardware work remains useful as donor validation but is no longer the immediate architectural priority:

- [ ] Physical ALSA acceptance.
- [ ] Real JACK/PipeWire-JACK acceptance.
- [ ] Physical ALSA MIDI acceptance.

## Phase 4 — C-Psycle Workflow Compatibility

The following automated behaviours are established and should be reused as compatibility tests where they apply to the next engine:

- [x] PSY3 song creation/save/reload.
- [x] MIDI Note On → event → pattern insertion.
- [x] Machine creation/deletion/wiring/rewiring/mute/bypass/positions.
- [x] Tracker note/effect editing, navigation and undo/redo.
- [x] Sequencer/transport/tempo/LPB/loop behaviour.
- [x] WAV import and sample/instrument construction.
- [x] Embedded PCM persistence.
- [x] Preset persistence including opaque machine state.
- [x] Project-authored historical PSY2-layout fixture.
- [x] WAV render through the retained render path.
- [x] Psycle-rendered WAV → Psycle Sampler round trip.

Classic bounce workflow preserved as an acceptance concept:

> build/process a loop → render to WAV → load the rendered WAV into Sampler → continue arranging.

## Phase 5 — Native Machine Preservation Corpus

### Completed families/slices

- [x] Arguru family.
- [x] Pooplog family.
- [x] Druttis family.
- [x] JM Drum.
- [x] JME family.
- [x] Zephod SuperFM.
- [x] Yezar Freeverb.
- [x] DW family.
- [x] STK-derived retained machines where provenance permits.
- [x] Alk Muter.
- [x] BexPhase.
- [x] Audacity Compressor.
- [x] Audacity Phaser.
- [x] Audacity WahWah.
- [x] Crasher.
- [x] Dalay Delay.
- [x] ayeternal Dist! Distortion.
- [x] ayeternal Gainer.
- [x] LADSPA GVerb.
- [x] ThunderPalace SoftSat.
- [x] Haas.
- [x] LegaSynth TB303.
- [x] MoreAmp EQ.
- [x] Negative.
- [x] Ninereeds Fractal 7900s Port.
- [x] Sartorius SChorus.
- [x] FluidSynth SF2 Player.

The resulting regression corpus covers native ABI/identity, parameters, deterministic/tolerance DSP, timing/sample-rate behaviour, preset/state persistence, opaque-state layouts and fresh PSY3 reopen for representative machines.

### Deferred optional retained machines

These remain preserved in the C-Psycle source tree but are not immediate blockers:

- [ ] Surround.
- [ ] Ring Modulator.
- [ ] Flanger.
- [ ] 2-pole Filter.

Do not delete them. Revisit them only when the compatibility matrix or real historical songs demonstrate that they matter to the target release.

### Preservation matrix status

Already established across representative machines:

- [x] discovery/instantiation;
- [x] parameter metadata;
- [x] audio behaviour;
- [x] presets;
- [x] timing/tracker commands where applicable;
- [x] public/opaque state serialization;
- [x] `.psy` save/reload and topology restoration.

Still important for the final product:

- [ ] robust missing-machine placeholders;
- [ ] historical-song compatibility using legally usable references.

**Status:** C-Psycle preservation corpus is mature enough to become an oracle for the next implementation track.

---

# Active Implementation Track — Original Psycle Compatibility on Linux

## Phase 6 — `psycle-core` Provenance and Parity Audit

**Goal:** determine exactly how close the existing C++ reimplementation already is to original Psycle before writing new engine or UI code.

### 6A — pin both sides of the parity comparison

At the start of Phase 6A the repository did **not** import the C++ engine family or the original-Psycle executable. Phase 6A pinned both identities and defined the public-import boundary without redistributing the original executable.

#### Original Psycle reference

- [x] Select and record the primary original Psycle reference version/build: **Psycle 1.12.0 x86** / `PsycleInstallerx86-1.12.0.exe`.
- [x] Record the authoritative SourceForge origin, release/version identity, exact size, and SHA-256.
- [x] Separate observation rights from redistribution: the executable is downloaded transiently for identity verification and is never committed or uploaded as an artifact.
- [x] Document the reproducible observation environment/evidence requirements, including native-Windows preference and secondary Wine handling.
- [x] Require machine-readable receipts, hashes, renders, dumps, screenshots/logs, or another reproducible procedure rather than memory-only claims.
- [x] Require additional Psycle releases to be identified independently and recorded as version-specific observations rather than overwriting the primary reference.

#### C++ reimplementation build-source snapshot

- [x] Identify the coherent SourceForge SVN r12005 build-source set: `universalis`, `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, and `psycle-plugins`.
- [x] Record revision-pinned paths, per-component file counts, last-changed revisions, and locale-stable SHA-256 manifest identities.
- [x] Audit first-pass licensing, dependency, notice, binary/song, and third-party boundaries before importing anything.
- [x] Record upstream authorship/license evidence and require retained notices to survive the future sanitized import.
- [x] Identify concrete exclusions/quarantines, including closed-source DLLs, unresolved songs, Steinberg-derived material, the Seib VST subtree, and the Windows ASIO subtree.
- [x] Define a conservative sanitized-import policy and keep public source import on **HOLD** until the exact retained-file/omission manifest is materialized.
- [x] Document the relationship between the pinned C++ candidate, the Psycle 1.12.0 behavioural reference, and the separate C-Psycle r12005 oracle.

**Phase 6A status: complete.** The two parity inputs are reproducibly pinned and the first public-import boundary is documented.

### 6B — sanitized import and historical Linux player build

- [x] Materialize the exact retained-file list and omission/replacement arithmetic from the frozen six-component manifests.
- [x] Restore/add complete compatible third-party permission/provenance notices required by retained helper/API code.
- [x] Create separate sanitized archival/canonical baseline identities for the C++ family without changing `cpsycle-r12005-baseline`.
- [x] Import only the provenance-cleared source needed for the first engine/player build; keep quarantined VST/ASIO/binary/song material out.
- [x] Reproduce the Debian/Linux build path without redesigning it first.
- [x] Build `psycle-player` and its required `universalis`/core/audio-driver/helper/plugin components.
- [x] Record compiler/linker/runtime blockers.
- [x] Establish deterministic CLI/headless playback where practical.
- [x] Confirm candidate loading behaviour for project-authored historical formats: PSY2 loads and exits cleanly; PSY3 reaches the version-3 loader, constructs Sampler/Master and begins playback, while the current noninteractive termination procedure times out. This is candidate evidence only, not original-Psycle parity.

**Phase 6B status: complete.** The sanitized C++ baseline is imported and frozen, the historical Linux player builds reproducibly, and the first PSY2/PSY3 candidate observations are recorded. The PSY3 timeout is retained as evidence for later parity work rather than hidden by the completion status.

### 6C — build a three-way compatibility matrix

Compare:

1. **pinned original Psycle reference build/version** — behavioural reference;
2. **pinned C++ build-source family** — candidate Linux engine;
3. **C-Psycle regression corpus** — independent donor/oracle where semantics overlap.

Completed matrix infrastructure:

- [x] Establish the canonical machine-readable 20-contract matrix in `phase6c/compatibility-matrix.json`.
- [x] Pin the original Psycle and Phase 6B candidate identities in the matrix validator.
- [x] Require versioned original and candidate receipts plus a comparison verdict before any row leaves `UNKNOWN`.
- [x] Collect reusable candidate PSY2/PSY3 receipts in maintained CI.
- [x] Keep candidate-only and C-Psycle-only evidence from becoming original-Psycle parity claims.
- [x] Version the first original/candidate comparison receipts and classify `project-io-psy2-parse` as a scoped `PASS` for the exact project-authored fixture.

Every PASS / DIFFERENT / MISSING entry must identify the original-Psycle reference version/build and the observation artifacts/procedures that support it. “Original Psycle did X” without a versioned reference is not a reproducible matrix result.

Audit at minimum:

- [x] PSY2/PSY3 parsing and the first serialization capability contract against the pinned original reference;
  - [x] PSY2 parse/load acceptance for the exact project-authored PSY2SONG fixture is classified `PASS` against pinned Psycle 1.12.0 x86.
  - [x] PSY3 parse/load acceptance for the exact project-authored Phase 4 fixture is classified `PASS` using PR #61 run `35364406445`: original 1.12.0 accepted after verified warning dismissal; a separately derived candidate parse receipt proves ordered load completion without disqualifying diagnostics. The unchanged candidate process receipt remains `timeout` / exit `124`; playback and termination are not classified.
  - [x] Serialization/save capability is classified `MISSING` for the exact PSY3 fixture; byte identity and complete semantic round-trip state remain separate and unmeasured.
    - [x] Add a separate runtime probe for candidate `CoreSong::save` versions 2/3/4 and a guarded original Save As/fresh-reopen observation with independent receipt validation. See `phase6c/SERIALIZATION.md`.
    - [x] Record paired runtime observations: candidate save versions 2/3/4 return false without output; original Save As succeeds and a fresh process accepts the saved output. The final PR #63 run `35439069516` reproduces this at head `b6c3a7d4f026200477929c96d641592ef52384c2`.
    - [x] Commit a versioned comparison and classify the required save capability `MISSING`; keep byte equality and complete semantic state preservation outside that verdict. See `phase6c/evidence/project-io-serialization-roundtrip/comparison.json`.
- [x] sequence/pattern order;
  - [x] Add a project-authored single-sequence PSY3 fixture with non-monotonic order `0,2,1,2`, a candidate model probe, and a guarded original UI Automation order-list observer. See `phase6c/SEQUENCE_ORDER.md`.
  - [x] Record paired runtime observations in final PR #65 Phase 6C run `35443357239` at head `0a9eebcba7028bd808bfa32c3e654828db51d7fa`: original Psycle exposes stable labels `00:00,01:02,02:01,03:02` and the candidate extracts play order `0,2,1,2` from the canonical musical sequence line.
  - [x] Commit a versioned comparison and classify `sequencer-pattern-order` as scoped `PASS` for that exact single-sequence fixture. See `phase6c/evidence/sequencer-pattern-order/comparison.json`.
- [x] BPM/LPB/tick behaviour;
  - [x] Establish the paired observation lane with a project-authored BPM 137 / LPB 8 / TPB 24 / extra-tick 0 fixture and clean Linux-candidate + pinned-Windows-original receipts in final PR #67 run `35453296036`.
  - [x] Version the exact paired receipts and classify `sequencer-bpm-lpb-tick` as scoped `DIFFERENT`: BPM and LPB agree, but original TPB 24 is not equivalent to the candidate's `tick_speed=8`, `is_ticks=true` beat/8 cadence. Delayed/retrigger commands and broader playback timing remain separate.
- [ ] tracker commands and delayed/retrigger events;
- [ ] Sampler PS1 behaviour;
- [ ] XMSampler behaviour;
- [ ] mixer/master/routing semantics;
- [ ] native-machine ABI/state;
- [ ] plugin parameter/state persistence;
- [ ] MIDI routing and automation;
- [ ] WAV/sample loading;
- [ ] render/bounce behaviour;
- [ ] missing-machine behaviour;
- [ ] old-song playback where a legal reference is available.

Do **not** assume C-Psycle's event sequencer is authoritative when it differs from original Psycle. The point of this phase is to discover and document differences.

**Phase 6C status: active.** PSY2 and PSY3 parse/load acceptance plus sequence/pattern order are scoped `PASS` results for exact project-authored fixtures, serialization/save capability is scoped `MISSING`, and BPM/LPB/tick timing is scoped `DIFFERENT`; 15 contracts remain `UNKNOWN`. The timing verdict is limited to the exact BPM 137 / LPB 8 / TPB 24 fixture: original Psycle preserves TPB 24 while the candidate feeds LPB 8 into its legacy tick-speed path with `is_ticks=true`. It does not classify delayed/retrigger commands, sampler tick processing, playback duration, tempo changes, multi-sequence behaviour or UI editing. The confirmed `MISSING` serializer gap and `DIFFERENT` tick-semantics gap now both justify narrow Phase 7 convergence backlog items. The next Phase 6C evidence slice is delayed/retrigger/extended tracker-command behaviour.

### 6D — parity report

- [x] Produce `PSYCLE_CORE_PARITY.md` with PASS / DIFFERENT / MISSING / UNKNOWN for each subsystem.
- [x] Mechanically require the original-Psycle version/build and versioned evidence receipts for every non-`UNKNOWN` row; the classified rows are scoped PSY2, PSY3 and sequence/pattern-order `PASS` results, scoped serialization/save-capability `MISSING`, and scoped BPM/LPB/tick `DIFFERENT`.
- [x] Separate engine gaps from UI-only gaps.
- [x] Identify which existing C-Psycle tests are portable shared contracts, which need original-Psycle confirmation, and which remain C-Psycle-only historical evidence.
- [x] Add `scripts/phase6d-validate-parity-report.py` plus a maintained Phase 6D workflow so the human-readable 20-row report cannot drift from the canonical matrix status inventory.
- [x] Freeze the first Phase 7 implementation backlog from confirmed `DIFFERENT` / `MISSING` evidence rather than intuition. The first item is the scoped PSY3 save-capability gap classified in Phase 6C.

**Phase 6D status: evidence-backed implementation backlog frozen and growing.** The human report projects three scoped `PASS` results (PSY2 parse/load, PSY3 parse/load and sequence/pattern order), one scoped serialization/save `MISSING` result, and one scoped BPM/LPB/tick `DIFFERENT` result. Phase 7 may implement the confirmed serializer capability gap and the narrowly defined TPB/LPB timing gap while Phase 6C continues collecting evidence for the remaining 15 contracts.

### 6E — active-track CI foundation

As soon as the provenance-safe C++ family is imported, the new implementation path must have CI of its own rather than relying on the retained C-Psycle Phase 2–5 workflows.

- [x] Add a maintained GitHub Actions workflow that builds the pinned C++ family and `psycle-player` on the reference Linux runner.
- [x] Run deterministic/headless player smoke and the currently portable Phase 6 PSY2/PSY3 fixture slice in CI.
- [x] Upload the receipts/logs required to diagnose the current candidate evidence slice; add renders when a later comparison contract requires them.
- [ ] Make the active C++ engine build/parity workflow a required merge signal before Phase 7 implementation PRs are accepted. (`main` is currently unprotected, so this governance control is not yet in force.)
- [ ] Require every Phase 7 parity regression to execute in the maintained CI path; no release-blocking regression may remain local-only.
- [ ] Extend the active-track CI as later phases arrive: Qt build/headless interaction smoke in Phase 8, and VST2 ABI/real-plugin/scanner/song-load containment tests in Phase 9.

**Phase 6E status: active foundation.** The build/evidence workflows exist and run on the canonical branch, but required-check governance and future-phase coverage remain open.

**Exit condition:** the original-Psycle reference and C++ candidate inputs are pinned and reproducible; the parity matrix identifies exactly what the candidate engine already provides and what must change; and the active engine/build/parity suite runs in maintained CI.

---

## Phase 7 — Engine Convergence

**Goal:** make the C++ cross-platform engine behave like original Psycle before building the full tracker UI.

- [ ] Close song-format compatibility gaps found in Phase 6.
- [ ] Close timing/sequencer/tracker-command gaps.
  - [ ] Separate legacy ticks-per-beat / extra-tick timing from LPB in the C++ candidate so the classified BPM 137 / LPB 8 / TPB 24 fixture no longer drives a beat/8 tick cadence.
  - [ ] Do not generalize that fix to delayed/retrigger/extended commands until their separate Phase 6C contract is classified.
- [ ] Bring Sampler/XMSampler behaviour to the required compatibility level.
- [ ] Restore mixer/master/routing semantics required by real songs.
- [ ] Preserve native-machine and plugin state contracts.
- [ ] Port/adapt Linux audio/MIDI drivers where the existing `psycle-audiodrivers` path is incomplete.
- [ ] Reuse C-Psycle implementations only where behaviour is demonstrated equivalent or where they provide a clean Linux platform implementation independent of divergent tracker semantics.
- [ ] Add regressions for every fixed parity gap.
- [ ] Run every new parity regression in the maintained active-track GitHub Actions workflow before merge.
- [ ] Keep the CI reference-version metadata synchronized with the parity report so a green result always identifies what original-Psycle behaviour it is protecting.

**Exit condition:** the engine/player can reproduce representative behaviour from the pinned original-Psycle reference within documented tolerances without requiring a GUI, and the parity suite gates regressions in CI.

---

## Phase 8 — Linux Tracker UI

**Goal:** restore the recognizable Psycle application workflow on top of the compatible engine.

The pinned original `psycle/` reference remains the visual/interaction reference; its MFC implementation is not copied blindly into a Linux toolkit.

### 8A — toolkit proof

**Qt is the leading candidate.** Start with Qt Widgets because Psycle is a desktop tracker with custom grids, machine graphs, editors, mixers and tool windows that map naturally to widget/custom-painting patterns.

- [ ] Build a minimal Qt host shell against the converged engine.
- [ ] Prove event loop, audio-engine ownership and clean shutdown.
- [ ] Prove keyboard focus/input behaviour suitable for tracker editing.
- [ ] Add Qt build/headless smoke coverage to the maintained active-track CI before expanding the UI surface.
- [ ] Evaluate QML only where it offers a concrete advantage; do not require QML merely for modernity.

### 8B — core views

Implement in compatibility order:

- [ ] Pattern/Tracker view.
- [ ] Machine View / graph routing.
- [ ] Parameter windows.
- [ ] Sequence/order workflow required by original Psycle songs.
- [ ] Instrument/Sampler window.
- [ ] Wave editor.
- [ ] Mixer/Master view.
- [ ] Preset/program UI.
- [ ] Settings/audio/MIDI/plugin-path UI.

### 8C — interaction fidelity

- [ ] Keyboard mappings and tracker navigation.
- [ ] Machine creation/wiring/mute/bypass interactions.
- [ ] Native-machine editor integration.
- [ ] Drag/drop, clipboard and file-dialog behaviour where historically important.
- [ ] Preserve the compact desktop-workstation character rather than redesigning Psycle as a generic modern DAW.

VST editor embedding or safe external-editor handling is intentionally **not** a Phase 8 exit requirement; it depends on the restored VST host lifecycle and is scheduled in Phase 9.

**Exit condition:** a Psycle user can create, edit, play, save and reopen songs using a recognizably Psycle workflow on Linux with native machines and core engine features, with the critical Qt host/build/interaction path protected by CI. Third-party VST editor integration is completed with the VST host in Phase 9.

---

## Phase 9 — Plugin Hosting, VST2 Compatibility, and Isolation

**Goal:** restore the third-party plugin ecosystem real Psycle projects relied on without restoring historical startup or song-loading fragility.

### 9A — startup-safe discovery and song-load containment

- [ ] Define a persistent plugin metadata cache keyed by path plus change detection.
- [ ] Start from cached metadata without instantiating every plugin on every boot.
- [ ] Rescan only new/changed binaries or on explicit request.
- [ ] Probe untrusted plugins in a separate scanner process.
- [ ] Add scan timeout/hang detection.
- [ ] Record scanner crashes without crashing Psycle.
- [ ] Quarantine repeatedly failing binaries with a user-visible reason.
- [ ] Allow retry/unquarantine/rescan.
- [ ] Preserve missing/quarantined song nodes as recoverable placeholders.
- [ ] Instantiate third-party plugins for song loading under a crash/hang-contained worker or bridge boundary before attaching them to the live graph.
- [ ] Restore song-specific opaque/plugin state under the same containment boundary and timeout policy.
- [ ] If a safely restored instance cannot be handed off without losing containment, keep that plugin behind the process boundary for runtime use.
- [ ] Do not implicitly open third-party plugin editors during song load; editor creation is an explicit post-load action.
- [ ] Exercise scanner crash, scanner hang, instantiation crash/hang, and state-restore crash/hang cases in CI.

This containment is part of the Phase 9 song-loading guarantee, not an optional later enhancement. Broader runtime isolation may still be expanded for additional native/legacy formats, but plugin discovery and song-specific instantiation/state restore must not be able to terminate or indefinitely hang the main Psycle process.

### 9B — clean-room VST2 ABI boundary and host restoration

Established facts from the C-Psycle audit:

- [x] historical Steinberg-derived `aeffect.h` is excluded from the public repository;
- [x] historical Steinberg-derived `aeffectx.h` is excluded;
- [x] historical Steinberg-derived `vstfxstore.h` is excluded;
- [x] Psycle-owned host-side VST2 code survives separately;
- [x] the retained host depends on the `aeffectx` ABI surface;
- [x] no current dependency on `vstfxstore.h` has been demonstrated; Psycle already has its own FXP-writing code.

Implementation plan:

- [ ] Inventory only the VST2 ABI types/constants/opcodes/layouts actually required by the chosen engine/host.
- [ ] Write independent project-authored compatibility definitions without copying Steinberg SDK source expressions.
- [ ] Add `sizeof` / `offsetof` / calling-convention assertions.
- [ ] Add a project-authored dummy VST2 test plugin for controlled ABI/error-path coverage.
- [ ] Restore native Linux VST2 hosting behind that boundary.
- [ ] Preserve Psycle's documented variable positive process-block lengths.
- [ ] Select at least one **independently maintained free/open Linux VST2 plugin** from a pinned, provenance-documented source/build that does **not** compile against PSYCLE-LINUX's compatibility headers.
- [ ] Do not commit proprietary Steinberg SDK material merely to build the independent validation plugin; use a legally/provenance-safe source/build path and record it.
- [ ] Validate the independent plugin through the restored host with **real audio processing** (nontrivial process callback/audio path), not only discovery/metadata.
- [ ] Where applicable, validate MIDI/note input, parameter changes, program/preset behaviour, opaque/chunk state, variable block lengths, song save/reopen, and missing-plugin fallback against that independent plugin.
- [ ] Keep the project-authored dummy fixture and the independent real-plugin test as separate gates: the first diagnoses our ABI implementation; the second proves interoperability with an implementation that did not share our header definitions.
- [ ] Add both the controlled ABI fixture and the independent real-plugin processing/state tests to maintained CI where licensing/distribution permits; otherwise build/fetch the pinned external fixture reproducibly during CI without vendoring restricted material.
- [ ] Add VST editor embedding or safe external-editor handling only after the host lifecycle/state path is working.
- [ ] Ensure editor creation/teardown failures are contained and cannot corrupt or terminate the main host.
- [ ] Audit `.fxp` / `.fxb` behaviour.
- [ ] Implement `vstfxstore`-equivalent structures only if an audited dependency requires them.
- [ ] Complete a licensing/provenance review before release.

### 9C — maintained Linux formats

- [ ] Preserve/strengthen LADSPA where useful.
- [ ] Evaluate LV2 integration against the final engine architecture.
- [ ] Evaluate VST3 after classic compatibility is stable.
- [ ] Evaluate CLAP after classic compatibility is stable.
- [ ] Apply the same discovery/song-load containment rules to every third-party format admitted to the release surface.

### 9D — optional Windows legacy bridge

- [ ] Evaluate an out-of-process Wine bridge for historical Windows VST2 `.dll` plugins.
- [ ] Ensure bridge/plugin failure cannot terminate the main Psycle process.
- [ ] Preserve historical plugin identity/state where technically and legally possible.

**Exit condition:** plugin discovery and song loading survive missing, crashing or hanging third-party plugins; plugin state can be restored or replaced by a recoverable placeholder without terminating the host; native Linux VST2 interoperability is proven against both controlled project fixtures and at least one independent real plugin exercising audio processing; editor lifecycle works behind the clean provenance boundary; and the critical plugin-host/isolation regressions run in CI.

---

## Phase 10 — Linux Integration, Packaging, and Hardware Acceptance

- [ ] Physical ALSA audio validation.
- [ ] Real JACK/PipeWire-JACK validation.
- [ ] Physical ALSA MIDI validation.
- [ ] XDG-compliant configuration/data/cache/plugin locations with migration support.
- [ ] Desktop entry and MIME integration.
- [ ] HiDPI/font/theme audit without redesigning the UI identity.
- [ ] Reproducible source/binary packaging.
- [ ] Clean install/upgrade/uninstall tests.
- [ ] Preserve the simplest useful build path; modernize the build system only when evidence justifies it.

**Exit condition:** installation and real Linux audio/MIDI use are boring and reproducible.

---

## Phase 11 — Reliability Suite and PSYCLE-LINUX 1.0

A 1.0 release means **usable, compatible Psycle on Linux**, not merely "it compiles."

Required acceptance areas:

- [ ] representative original `.psy` songs load and play correctly against explicitly identified reference versions;
- [ ] engine parity matrix has no unexplained release-blocking differences;
- [ ] tracker/Machine View workflow is usable and recognizably Psycle;
- [ ] native-machine state and representative sound behaviour are preserved;
- [ ] render/bounce → Sampler workflow works;
- [ ] missing/broken plugins degrade safely;
- [ ] plugin scanner/cache/quarantine and song-load instantiation/state-restore containment are dependable;
- [ ] VST2 compatibility status, independent real-plugin evidence, and provenance boundary are documented;
- [ ] real audio/MIDI hardware paths are validated;
- [ ] packaging/install/uninstall are reproducible;
- [ ] long-session/stress testing is complete;
- [ ] user and developer documentation are release quality;
- [ ] maintained CI covers the active engine/parity suite, Qt host smoke, plugin ABI/independent-plugin processing, and plugin failure-containment regressions;
- [ ] release-blocking regressions are required merge checks or otherwise enforced before release—not merely documented local tests.

---

## Decision Test for New Work

Before adding or replacing a major dependency, subsystem, engine or UI path, ask:

1. Does this move us closer to **original Psycle behaviour on Linux**?
2. Is the behaviour established by a **version-pinned** original Psycle reference, a trustworthy song/test, or other reproducible evidence?
3. Can an existing `psycle-core` implementation be repaired instead of rewritten?
4. Can proven C-Psycle code/tests be reused without importing C-Psycle-specific behavioural divergence?
5. Does the change preserve old songs, machines and plugin state?
6. Does it reduce or contain startup/runtime failure risk?
7. Is the licensing/provenance boundary clear?
8. Can it be introduced incrementally and regression-tested in maintained CI?

If the answer is unclear, preserve evidence first and avoid a broad rewrite.
