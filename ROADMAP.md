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

### First cross-platform reimplementation

The first C++ reimplementation was split across projects/directories including:

- `psycle-core`
- `psycle-audiodrivers`
- `psycle-helpers`
- `psycle-player`
- `psycle-plugins`

According to maintainer clarification, that reimplementation was buildable on Debian Linux and could play most Psycle songs, but it was not fully up to date with original Psycle playback and provided a player rather than the full tracker application.

**This family is now the leading candidate engine base for a faithful Linux Psycle.**

### Later C-Psycle reimplementation

`cpsycle/` is a later C reimplementation with its own UI toolkit, tracker and event/sequencer architecture.

C-Psycle shares substantial concepts and compatibility goals with Psycle, but it is **not the original Psycle port** and intentionally diverges in some areas. Its own developer guide describes it as a C-version/variant and documents differences such as the event-oriented sequencer.

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

1. original Psycle behaviour/source where legally and technically available;
2. reproducible historical songs/tests and preserved machine contracts;
3. `psycle-core` family behaviour as the candidate cross-platform engine;
4. C-Psycle source plus the extensive regression evidence built in this repository;
5. retained developer documentation and historical branches/posts as architecture/intent evidence.

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

### 6A — acquire and pin the correct upstream source

The current repository does **not** yet import the `psycle-core` family.

- [ ] Identify the exact SourceForge revisions/paths for `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player` and `psycle-plugins` that form a coherent buildable state.
- [ ] Record revision-pinned URLs and checksums where appropriate.
- [ ] Audit licences and third-party boundaries before importing anything.
- [ ] Preserve upstream notices/authorship.
- [ ] Import or vendor only after the provenance boundary is clear.
- [ ] Document the relationship between the selected C++ reimplementation snapshot and original Psycle versions it was intended to emulate.

### 6B — reproduce the historical Linux player build

- [ ] Reproduce the Debian/Linux build path without redesigning it first.
- [ ] Build `psycle-player` and its required core/audio-driver/helper/plugin components.
- [ ] Record compiler/linker/runtime blockers.
- [ ] Establish deterministic CLI/headless playback where practical.
- [ ] Confirm which historical `.psy` versions load.

### 6C — build a three-way compatibility matrix

Compare:

1. **original Psycle** — behavioural reference;
2. **`psycle-core` family** — candidate Linux engine;
3. **C-Psycle regression corpus** — independent donor/oracle where semantics overlap.

Audit at minimum:

- [ ] PSY2/PSY3 parsing and serialization;
- [ ] sequence/pattern timing;
- [ ] BPM/LPB/tick behaviour;
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

### 6D — parity report

- [ ] Produce `PSYCLE_CORE_PARITY.md` with PASS / DIFFERENT / MISSING / UNKNOWN for each subsystem.
- [ ] Separate engine gaps from UI-only gaps.
- [ ] Identify which existing C-Psycle tests can be ported unchanged, which need original-Psycle oracles, and which should remain C-Psycle-only historical tests.
- [ ] Freeze the first implementation backlog from evidence rather than intuition.

**Exit condition:** we know exactly what `psycle-core` already provides and what must change to reach original-Psycle playback compatibility.

---

## Phase 7 — Engine Convergence

**Goal:** make the C++ cross-platform engine behave like original Psycle before building the full tracker UI.

- [ ] Close song-format compatibility gaps found in Phase 6.
- [ ] Close timing/sequencer/tracker-command gaps.
- [ ] Bring Sampler/XMSampler behaviour to the required compatibility level.
- [ ] Restore mixer/master/routing semantics required by real songs.
- [ ] Preserve native-machine and plugin state contracts.
- [ ] Port/adapt Linux audio/MIDI drivers where the existing `psycle-audiodrivers` path is incomplete.
- [ ] Reuse C-Psycle implementations only where behaviour is demonstrated equivalent or where they provide a clean Linux platform implementation independent of divergent tracker semantics.
- [ ] Add regressions for every fixed parity gap.

**Exit condition:** the engine/player can reproduce representative original-Psycle songs within documented tolerances without requiring a GUI.

---

## Phase 8 — Linux Tracker UI

**Goal:** restore the recognizable Psycle application workflow on top of the compatible engine.

Original `psycle/` remains the visual/interaction reference; its MFC implementation is not copied blindly into a Linux toolkit.

### 8A — toolkit proof

**Qt is the leading candidate.** Start with Qt Widgets because Psycle is a desktop tracker with custom grids, machine graphs, editors, mixers and tool windows that map naturally to widget/custom-painting patterns.

- [ ] Build a minimal Qt host shell against the converged engine.
- [ ] Prove event loop, audio-engine ownership and clean shutdown.
- [ ] Prove keyboard focus/input behaviour suitable for tracker editing.
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
- [ ] Native/VST editor embedding or safe external editor handling.
- [ ] Drag/drop, clipboard and file-dialog behaviour where historically important.
- [ ] Preserve the compact desktop-workstation character rather than redesigning Psycle as a generic modern DAW.

**Exit condition:** a Psycle user can create, edit, play, save and reopen songs using a recognizably Psycle workflow on Linux.

---

## Phase 9 — Plugin Hosting, VST2 Compatibility, and Isolation

**Goal:** restore the third-party plugin ecosystem real Psycle projects relied on, without restoring the historical startup fragility.

### 9A — startup-safe discovery

- [ ] Define a persistent plugin metadata cache keyed by path plus change detection.
- [ ] Start from cached metadata without instantiating every plugin on every boot.
- [ ] Rescan only new/changed binaries or on explicit request.
- [ ] Probe untrusted plugins in a separate scanner process.
- [ ] Add scan timeout/hang detection.
- [ ] Record scanner crashes without crashing Psycle.
- [ ] Quarantine repeatedly failing binaries with a user-visible reason.
- [ ] Allow retry/unquarantine/rescan.
- [ ] Preserve missing/quarantined song nodes as recoverable placeholders.

Longer term, consider runtime process isolation for especially fragile legacy plugins.

### 9B — clean-room VST2 ABI boundary

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
- [ ] Add a project-authored dummy VST2 test plugin.
- [ ] Restore native Linux VST2 hosting behind that boundary.
- [ ] Preserve Psycle's documented variable positive process-block lengths.
- [ ] Verify MIDI, parameters, opaque state, presets and song reopen.
- [ ] Audit `.fxp` / `.fxb` behaviour.
- [ ] Implement `vstfxstore`-equivalent structures only if an audited dependency requires them.
- [ ] Complete a licensing/provenance review before release.

### 9C — maintained Linux formats

- [ ] Preserve/strengthen LADSPA where useful.
- [ ] Evaluate LV2 integration against the final engine architecture.
- [ ] Evaluate VST3 after classic compatibility is stable.
- [ ] Evaluate CLAP after classic compatibility is stable.

### 9D — optional Windows legacy bridge

- [ ] Evaluate an out-of-process Wine bridge for historical Windows VST2 `.dll` plugins.
- [ ] Ensure bridge/plugin failure cannot terminate the main Psycle process.
- [ ] Preserve historical plugin identity/state where technically and legally possible.

**Exit condition:** real Psycle plugin workflows are available without making startup or song loading fragile.

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

- [ ] representative original `.psy` songs load and play correctly;
- [ ] engine parity matrix has no unexplained release-blocking differences;
- [ ] tracker/Machine View workflow is usable and recognizably Psycle;
- [ ] native-machine state and representative sound behaviour are preserved;
- [ ] render/bounce → Sampler workflow works;
- [ ] missing/broken plugins degrade safely;
- [ ] plugin scanner/cache/quarantine behaviour is dependable;
- [ ] VST2 compatibility status and provenance boundary are documented;
- [ ] real audio/MIDI hardware paths are validated;
- [ ] packaging/install/uninstall are reproducible;
- [ ] long-session/stress testing is complete;
- [ ] user and developer documentation are release quality.

---

## Decision Test for New Work

Before adding or replacing a major dependency, subsystem, engine or UI path, ask:

1. Does this move us closer to **original Psycle behaviour on Linux**?
2. Is the behaviour established by original Psycle, a trustworthy song/test, or reproducible evidence?
3. Can an existing `psycle-core` implementation be repaired instead of rewritten?
4. Can proven C-Psycle code/tests be reused without importing C-Psycle-specific behavioural divergence?
5. Does the change preserve old songs, machines and plugin state?
6. Does it reduce or contain startup/runtime failure risk?
7. Is the licensing/provenance boundary clear?
8. Can it be introduced incrementally and regression-tested?

If the answer is unclear, preserve evidence first and avoid a broad rewrite.