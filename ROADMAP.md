# PSYCLE-LINUX Roadmap

## Guiding Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

The roadmap is compatibility-first. New technology is useful only when it helps Psycle run reliably on modern Linux without discarding the workflow, file compatibility, native machines, plugin behaviour, or character that made Psycle distinct.

A second rule follows from Psycle's historical plugin model:

> **Preserve source broadly, but keep the default runtime surface deliberate and reliable.**

Code being present in the audited r12005 tree does not automatically mean that every retained machine must ship as a default-loaded core component before the classic Linux port can advance. Optional/legacy machines may remain preserved and buildable while being deferred from the core acceptance path.

## Historical lineage and donor sources

PSYCLE-LINUX uses SourceForge `trunk/cpsycle` revision `r12005` as its primary implementation. Older SourceForge branches are historical and technical donors, not replacement codebases.

Primary donor locations remain:

- `trunk/cpsycle` — primary source baseline;
- `branches/unmaintained/xpsycle` — original Linux-port archaeology;
- `branches/unmaintained/qpsycle` and `qpsycle2` — portable/UI/workflow archaeology;
- `branches/unmaintained/wxpsycle` — alternate portability/UI donor;
- `trunk/psycle-core` — shared engine/history donor;
- `trunk/debian` — historical packaging/install-path donor.

The retained upstream `cpsycle/doc/cpsycle-developer-guide.txt` is a primary architectural reference for C-Psycle intent. It is explicitly the **C-Version, Feb 2021 (unfinished)** guide, so pinned r12005 source and observed compatibility behavior remain authoritative if prose and code disagree. The guide documents MFC-Psycle compatibility as a goal, stepwise cross-platform separation, the audio/UI split, the platform UI bridge, 256-sample plugin work chunks, variable VST process intervals, and the 64-channel native-plugin limit. See `UPSTREAM_ARCHITECTURE.md` for the extracted project implications.

Historical evidence also records native-machine ports, LADSPA, VST 2.4 hosting, sampler/resampling work, Mixer/send-return routing, MIDI, automation, offline rendering and engine/UI separation as real Psycle development directions.

The compatibility rule for donor code remains:

1. prefer working C-Psycle behavior;
2. use the retained C-Psycle developer guide as primary architecture/intent evidence, with source behavior taking precedence where the unfinished prose differs;
3. consult historical branches when intent or compatibility needs clarification;
4. port donor implementations only with understood provenance/licensing;
5. do not replace working subsystems merely because another toolkit or architecture is newer.

---

## Phase 0 — Project Foundation

**Goal:** define the project before changing upstream code.

- [x] Establish mission and scope.
- [x] Record the selected upstream baseline.
- [x] Record source archive identity/SHA-256.
- [x] Define porting principles and non-goals.
- [x] Document licensing/provenance boundaries.
- [x] Add contribution and credit documentation.
- [x] Publish the roadmap.

**Status:** complete.

## Phase 1 — Upstream Baseline Import

**Goal:** import C-Psycle r12005 in an auditable form.

- [x] Import the r12005 C-Psycle tree mechanically close to upstream.
- [x] Preserve upstream notices and attributions.
- [x] Record the SourceForge revision and ZIP SHA-256.
- [x] Create the sanitized archival ref and canonical audited baseline tag.
- [x] Inventory bundled third-party code/licenses.
- [x] Document all whole-file omissions and provenance-safe replacements.
- [x] Document the source tree for maintainers.

Canonical baseline records remain in `PROVENANCE.md`, `UPSTREAM_OMISSIONS.md`, `THIRD_PARTY_INVENTORY.md`, and `SOURCE_TREE.md`.

Important VST2 boundary established here:

- [x] Omit Steinberg-SDK-derived `audio/src/aeffect.h`.
- [x] Omit Steinberg-SDK-derived `audio/src/aeffectx.h`.
- [x] Omit Steinberg-SDK-derived `audio/src/vstfxstore.h` rather than assuming redistribution rights.
- [x] Retain Psycle-owned/GPL VST2 host implementation outside those SDK-derived files.

**Status:** complete.

## Phase 2 — Modern Linux Build Audit

**Goal:** learn what actually breaks before redesigning anything.

- [x] Reproduce the make-based build on Ubuntu 24.04 x86-64.
- [x] Inventory development packages and `pkg-config` dependencies.
- [x] Build core libraries independently.
- [x] Capture compiler/linker failures and warnings.
- [x] Identify obsolete API, 64-bit, path and build-order assumptions.
- [x] Verify X11/Xft UI compilation.
- [x] Verify ALSA, ALSA MIDI, JACK, SDL2 and event-joystick build state.
- [x] Verify Lua and Lilv/LV2 integration points.
- [x] Audit `psyplayer` separately.
- [x] Add reproducible development-build documentation and CI evidence.

**Status:** complete. See `PHASE2_BUILD_AUDIT.md` and `BUILDING.md`.

## Phase 3 — First Native Linux Host

**Goal:** make Psycle launch and produce audio before polishing it.

### Automated/native-host work

- [x] Build and launch the native X11 host in CI.
- [x] Prove writable isolated Linux configuration state and clean config save.
- [x] Select/load the SDL2 audio driver dynamically.
- [x] Open SDL2 dummy audio and complete a real Psycle host callback.
- [x] Exercise native editor/focus, tracker input, FileView sample loading and clean X11 shutdown.

### Real-device work still required

- [ ] Enumerate available audio/MIDI backends in a user-facing Linux session.
- [ ] Produce stable audio through physical ALSA hardware.
- [ ] Produce stable audio through a real JACK/PipeWire-JACK session.
- [ ] Verify physical ALSA MIDI input.
- [ ] Verify clean startup/shutdown and device reinitialization across real devices.
- [ ] Continue fixing x86-64 crashes/UB as real-device testing exposes them.

**Status:** automated host path established; physical hardware acceptance remains open.

## Phase 4 — Core Psycle Workflow Compatibility

**Goal:** restore the normal Psycle workflow end to end.

- [x] Synthetic song creation and PSY3 save/reload.
- [x] MIDI Note On → Psycle event → pattern insertion.
- [x] Machine View create/delete/wire/rewire/mute/bypass/parameter/position behavior.
- [x] Live Machine editor opening and interaction.
- [x] Tracker note/effect command editing, navigation, undo/redo and reload.
- [x] Sequencer editing, playback, tempo, LPB, transport, loops and position.
- [x] WAV import through the historical sample/instrument path.
- [x] Embedded PCM preservation across PSY3 round-trip.
- [x] Normal FileView sample/instrument workflow.
- [x] Preset loading/saving including opaque plugin state.
- [x] Project-authored historical PSY2-layout compatibility fixture.
- [x] WAV render through `FileOutDriver`.
- [x] Psycle-rendered WAV → Psycle Sampler acceptance workflow.

The classic bounce workflow remains a first-class acceptance path:

> build/process a loop → render to WAV → load that WAV into Sampler → continue arranging.

**Status:** automated Phase 4 compatibility scope complete. Physical ALSA/JACK/MIDI remains Phase 3 real-device work.

---

## Phase 5 — Classic Native Machine Preservation

**Goal:** make the characteristic Psycle native-machine ecosystem reliable on Linux without casually changing old-song behavior.

### 5A — Arguru family

- [x] Arguru Compressor.
- [x] Arguru Distortion.
- [x] Arguru Goaslicer.
- [x] Arguru Reverb.
- [x] Arguru Synth 2f.
- [x] Arguru XFilter / CrossDelay.

Coverage includes native ABI/metadata, representative DSP/timing/synthesis behavior, preset/state persistence and fresh PSY3 reopen.

**Status:** complete.

### 5B — Pooplog family

- [x] FM Laboratory.
- [x] FM Light.
- [x] FM UltraLight.
- [x] Delay.
- [x] Delay Light.
- [x] Filter.
- [x] Autopan.
- [x] Lofi Processor.
- [x] Scratch Master.

Coverage includes complete metadata hashes, representative DSP/timing behavior, preset/state persistence, opaque FM state and one-song multi-machine PSY3 reopen.

**Status:** complete.

### 5C — broader classic Psycle ecosystem

Family/group slices:

- [x] Druttis family — EQ-3, FeedMe, Koruz, Phantom, Plucked String, Slicit and Sublime.
- [x] JM Drum.
- [x] JME family — Blitz/GameFX retained variants.
- [x] Zephod SuperFM.
- [x] Yezar Freeverb.
- [x] DW family — EQ, Granulizer, IoPan and Tremolo.
- [x] STK-derived retained machines where provenance permits.

Focused retained-machine slices completed after the original roadmap checkpoint:

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

The FluidSynth slice freezes its 24-slot ABI (19 state / 3 labels / 2 nulls), 5184-byte `SYNPAR` v3 opaque state, SoundFont path/channel state, tracker-note behavior, direct FluidSynth audio equivalence, live sample-rate transition, independent preset restore and fresh PSY3 reopen.

#### Deferred retained optional machines — not blockers for 5D

The following source-built machines remain preserved in the audited r12005 tree and may receive dedicated gates later, but they are **not prerequisites for moving the classic/core port into Phase 5D**:

- [ ] Surround.
- [ ] Ring Modulator.
- [ ] Flanger.
- [ ] 2-pole Filter.

This is a scope/prioritization decision, not a deletion decision. Their sources remain available and buildable. The default/runtime plugin set should be chosen deliberately because every automatically scanned/loaded machine increases startup failure surface.

**5C status:** complete for the current classic/core preservation scope. Optional retained extras are deferred.

### 5D — preservation matrix

The 5A–5C gates already establish the following matrix rows across representative classic machines:

- [x] Discovery and instantiation.
- [x] Parameter ranges, names, flags and defaults.
- [x] Deterministic/tolerance-based audio behavior where practical.
- [x] Preset behavior.
- [x] Timing, sample-rate and tracker-command behavior where applicable.
- [x] Public/opaque state serialization.
- [x] `.psy` save/reload and topology restoration.
- [ ] Broad missing-machine behavior, including useful placeholders without startup failure.
- [ ] Historical-song compatibility using trustworthy, legally redistributable references where available.

The upstream developer guide independently documents the player's 256-sample `psy_audio_MAX_STREAM_SIZE` split as a Psycle-plugin compatibility measure and the 64-channel native-plugin limit. Those are preservation constraints for 5D unless a separately versioned compatibility design proves a safe extension.

5D should now consolidate the existing machine evidence rather than require every retained optional effect to become a default/core runtime dependency.

Fix crashes, undefined behavior, 64-bit assumptions and serialization defects when evidence exposes them, but do not casually rewrite DSP equations and change the sound of old songs.

**Phase 5 exit condition:** classic native machines load, process audio, preserve state and reopen reliably; missing/optional machines do not prevent a song or host session from being recoverable.

---

## Phase 6 — Plugin Hosting, VST2 Compatibility, and Isolation

**Goal:** restore the plugin ecosystem that real Psycle projects used while making plugin discovery substantially safer than the historical all-in-process startup model.

Third-party plugins are historically important to Psycle, but a broken native/VST binary must not be able to make the whole host unusable merely because it is present in a scan directory.

### 6A — startup-safe plugin discovery

- [ ] Define a persistent plugin metadata cache keyed by path plus a change detector such as mtime/size and/or content hash.
- [ ] Start Psycle from cached metadata without instantiating every third-party plugin on every boot.
- [ ] Rescan only new/changed binaries or when explicitly requested.
- [ ] Probe untrusted plugins in a separate scanner process rather than the main Psycle process.
- [ ] Add scan timeout/hang detection.
- [ ] Record scanner crashes without crashing Psycle.
- [ ] Quarantine repeatedly failing binaries with an explicit user-visible reason/status.
- [ ] Preserve missing/quarantined nodes as recoverable placeholders when opening songs.
- [ ] Allow deliberate retry/unquarantine/rescan.

Longer term, runtime plugin-process isolation should be considered for especially fragile legacy plugins so a plugin crash need not destroy the open Psycle session.

### 6B — VST2 provenance and clean-room ABI boundary

Phase 1 established these facts:

- [x] Steinberg-SDK-derived `aeffect.h` is not redistributed.
- [x] Steinberg-SDK-derived `aeffectx.h` is not redistributed.
- [x] Steinberg-SDK-derived `vstfxstore.h` is not redistributed.
- [x] Psycle-owned/GPL VST2 host translation units remain in the tree behind `PSYCLE_USE_VST2`.
- [x] Current retained host code directly depends on the `aeffectx.h` ABI surface.
- [x] No retained code dependency on `vstfxstore.h` has been identified; Psycle already implements FXP writing in `presetio.c`.

Implementation plan:

- [ ] Produce a neutral inventory/specification of **only** the VST2 ABI surface the retained Psycle host consumes.
- [ ] Independently implement the required scalar types, `AEffect`-compatible binary layout, callback signatures, dispatcher/host opcodes, flags, event/MIDI structures, time-info structures and entry-point contract without copying Steinberg SDK source expressions.
- [ ] Keep the compatibility implementation clearly project-authored and provenance-documented.
- [ ] Add compile-time `sizeof` / `offsetof` / calling-convention assertions for the binary contract.
- [ ] Add a project-authored dummy VST2 fixture so the host ABI can be tested without redistributing a proprietary SDK/plugin.
- [ ] Restore the retained Psycle VST2 host behind this compatibility boundary on Linux.
- [ ] Preserve Psycle's documented variable process-block behavior: tracker/newline splits may make VST process intervals unequal, and restored VST2 hosting must accept arbitrary positive sample counts correctly rather than forcing fixed blocks.
- [ ] Test an externally obtained or source-built free/open Linux VST2 plugin.
- [ ] Verify discovery/load, audio, pattern/MIDI playback, parameters, state, PSY3 reload and missing-plugin handling.
- [ ] Audit `.fxp` / `.fxb` behavior.
- [ ] Implement any `vstfxstore`-equivalent structures only if an audited real dependency proves them necessary.
- [ ] Complete a licensing/provenance review before distributing the VST2 compatibility layer as a release feature.

### 6C — maintained Linux plugin formats

- [ ] Strengthen LV2 host coverage using the existing Lilv integration.
- [ ] Evaluate VST3 only after the classic/core host is stable.
- [ ] Evaluate CLAP only after the classic/core host is stable.
- [ ] Keep LADSPA compatibility where it remains useful.

### 6D — optional legacy Windows-plugin bridge

Only after native Linux VST2 hosting and process isolation are dependable:

- [ ] Evaluate an out-of-process Wine bridge for historical Windows VST2 `.dll` plugins.
- [ ] Ensure a bridged plugin crash/hang cannot terminate the main Psycle process.
- [ ] Preserve legacy song/plugin identity and opaque state where technically and legally possible.

**Phase 6 exit condition:** maintained Linux plugin hosting works reliably; legacy VST2 interoperability is available through a clean provenance boundary; broken or missing third-party plugins do not prevent Psycle from starting.

---

## Phase 7 — Modern Linux Integration

**Goal:** make Psycle feel at home on a current Linux desktop without rewriting its identity.

- [ ] Desktop entry and application metadata.
- [ ] Icons and MIME integration for Psycle song/preset formats where appropriate.
- [ ] XDG-compliant config/data/cache locations while preserving migration compatibility.
- [ ] User-visible audio/MIDI backend selection and diagnostics.
- [ ] HiDPI/font/theme checks without replacing the established UI model.
- [ ] File-dialog/path behavior appropriate for modern Linux.

**Exit condition:** Psycle integrates cleanly with a modern Linux desktop while preserving its existing UI model.

## Phase 8 — Build System and Packaging

**Goal:** make installation boring and reproducible.

- [ ] Preserve the make-based path as a compatibility/reference build.
- [ ] Decide whether a secondary modern build-system front end materially helps maintenance.
- [ ] Define install prefixes and runtime resource/plugin locations.
- [ ] Produce repeatable source and binary packages.
- [ ] Package required runtime dependencies without silently bundling restricted SDK material.
- [ ] Verify clean install, upgrade and uninstall behavior.

**Exit condition:** users can install/remove packaged builds without manually copying libraries around the filesystem.

## Phase 9 — Reliability, CI, and Compatibility Suite

**Goal:** stop regressions from undoing the port.

- [x] Linux build audit CI.
- [x] Native X11/runtime smoke CI.
- [x] Core Phase 4 workflow compatibility gates.
- [x] Extensive Phase 5 native-machine preservation gates.
- [ ] Real-device ALSA/JACK/MIDI acceptance matrix.
- [ ] Consolidated Phase 5D matrix/report.
- [ ] Plugin scanner crash/hang/quarantine regressions.
- [ ] Clean-room VST2 ABI/host compatibility regression suite.
- [ ] Curated legal historical-song compatibility corpus where rights allow.
- [ ] Release-level long-session/stress testing.

**Exit condition:** major compatibility regressions are caught automatically before merge/release.

## Phase 10 — New Native Machines After Preservation

**Goal:** add new Psycle-native instruments/effects only after the historical workflow and machine ecosystem are dependable.

- [ ] Define a modern, documented native-machine contribution template.
- [ ] Keep new machines clearly separate from preserved historical identities.
- [ ] Require deterministic tests, state compatibility and licensing/provenance records.
- [ ] Avoid new-machine work delaying classic compatibility work.

The interface should remain immediate: click steps, tweak machines, press play.

## Phase 11 — PSYCLE-LINUX 1.0

A 1.0 release means **usable Psycle on Linux**, not merely “it compiles.”

Release acceptance should include:

- [ ] reliable native Linux startup/shutdown on supported desktops;
- [ ] real ALSA/JACK/PipeWire-JACK and MIDI validation;
- [ ] classic create/edit/play/save/reopen/render workflow;
- [ ] representative classic native-machine preservation matrix complete;
- [ ] safe behavior for missing/broken/quarantined plugins;
- [ ] dependable plugin discovery/cache behavior;
- [ ] documented plugin-format support and VST2 provenance boundary;
- [ ] reproducible packaging/install/uninstall;
- [ ] user and developer documentation suitable for a maintained release.

---

## Decision Test

Before adding or replacing a major dependency, subsystem or plugin path, ask:

1. Does this make Psycle work better on Linux?
2. Does it preserve compatibility with old songs, machines and workflows?
3. Does it reduce or at least contain startup/runtime failure risk?
4. Can we test it reproducibly?
5. Is the licensing/provenance boundary clear?
6. Is it necessary now, or can it remain an optional/deferred component?

If the answer is unclear, preserve evidence first and avoid a broad rewrite.
