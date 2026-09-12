# PSYCLE-LINUX Roadmap

## Guiding Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

The roadmap is deliberately compatibility-first. New technology is useful only when it helps Psycle run reliably on modern Linux without discarding the workflow, file compatibility, native machines, plugin behaviour, or character that made Psycle distinct.

## Historical lineage and donor sources

PSYCLE-LINUX uses `trunk/cpsycle` r12005 as its primary implementation. Older SourceForge branches are historical and technical donors, not replacement codebases.

Primary upstream and donor locations:

- `trunk/cpsycle` — primary PSYCLE-LINUX source baseline and the later C-Psycle codebase maintained by Psycle developers, including JAZ;
- `branches/unmaintained/xpsycle` — the original 2006 Linux effort described by JAZ as functional alpha software in which songs could already be made;
- `branches/unmaintained/qpsycle` — later Qt/multiplatform UI and workflow archaeology;
- `branches/unmaintained/qpsycle2` — later QPsycle continuation/revival archaeology;
- `branches/unmaintained/wxpsycle` — alternate portable/UI experiment to inspect when it contains relevant behaviour;
- `trunk/psycle-core` — shared engine/history donor from the multiplatform restructuring era;
- `trunk/debian` — historical Debian packaging donor for install paths, package boundaries and desktop integration.

Historical evidence from JAZ's 2006 development-status posts records Linux portability, multipattern sequencing, LADSPA, native-machine ports, an event-based playback engine, sampler resampling work, a send/return Mixer, automation, VST 2.4 hosting, multi-I/O work, MIDI improvement and separation of engine/GUI for playback and offline rendering as explicit Psycle development directions.

These sources should be used with a strict compatibility rule:

1. prefer the living C-Psycle implementation when it already works;
2. consult historical branches when behaviour, intent or an unfinished feature needs clarification;
3. port a donor implementation only when its provenance/licensing is understood and it materially preserves Psycle behaviour;
4. do not replace working C-Psycle subsystems merely because an older branch used a different toolkit or architecture.

The exact direct code lineage from early `xpsycle` through the later portable-core work into `cpsycle` should continue to be documented from SVN copy/commit history rather than asserted from similarity alone.

## Phase 0 — Project Foundation

**Goal:** define the project before changing upstream code.

- [x] Establish project mission and scope.
- [x] Record the selected upstream baseline.
- [x] Record the source archive SHA-256.
- [x] Define porting principles and non-goals.
- [x] Document licensing and provenance boundaries.
- [x] Add contribution and credit documentation.
- [x] Publish the initial roadmap.

**Exit condition:** contributors can tell what PSYCLE-LINUX is, what it is not, where the source comes from, and how changes should be evaluated.

## Phase 1 — Upstream Baseline Import

**Goal:** import C-Psycle r12005 in a way that remains auditable.

- [x] Import the `r12005-trunk-cpsycle` source with upstream structure and notices intact, except explicitly documented restricted third-party SDK material.
- [x] Preserve upstream `AUTHORS`, `COPYING`, plugin notices, and third-party attribution.
- [x] Keep the first import mechanically close to upstream; do not mix it with broad refactors.
- [x] Record the exact import commit and tag it as the project baseline.
- [x] Inventory bundled third-party code and per-component licenses.
- [x] Identify generated files, vendored libraries, obsolete binaries, and build-only artifacts.
- [x] Add an initial source-tree map for maintainers.

Baseline record:

- upstream identity: SourceForge SVN `r12005` plus the recorded ZIP SHA-256;
- sanitized archival import ref: `archive/cpsycle-r12005-sanitized-import`;
- canonical audited tag: `cpsycle-r12005-baseline`;
- imported tree: `cpsycle/`;
- upstream r12005 files: 2,415;
- retained upstream files after exclusions: 2,349;
- Phase 1 licensing/provenance files added: 14;
- canonical files under `cpsycle/`: 2,363;
- upstream omissions: 66, documented in [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md).

See also [PROVENANCE.md](PROVENANCE.md), [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md), and [SOURCE_TREE.md](SOURCE_TREE.md).

**Exit condition:** the repository contains a traceable source baseline that can be compared back to r12005.

## Phase 2 — Modern Linux Build Audit

**Goal:** learn what actually breaks before redesigning anything.

Reference audit platform: Ubuntu 24.04.5 LTS x86-64, GCC/G++ 13.3.0 and GNU Make 4.3.

For Phase 2, **verified** means the target was exercised and its observed PASS/FAIL/BLOCKED state was recorded. A failed target is a completed audit result when its causal blocker is reproducible and documented.

- [x] Reproduce the original `make`-based build as closely as practical.
- [x] Inventory required development packages and `pkg-config` dependencies.
- [x] Build the core libraries independently before the full host.
- [x] Capture compiler errors and warnings without immediately suppressing them.
- [x] Identify obsolete C/C++ assumptions, removed APIs, 32-bit assumptions, path assumptions, and linker-order issues.
- [x] Verify X11/Xft UI compilation.
- [x] Verify ALSA, ALSA MIDI, JACK, SDL2, and event-joystick driver build state.
- [x] Verify Lua and Lilv/LV2 integration points.
- [x] Build `psyplayer` separately as a smaller audio-engine test target and record its blocking prerequisite.
- [x] Add a reproducible development-build document.

Phase 2 evidence and tooling:

- [PHASE2_BUILD_AUDIT.md](PHASE2_BUILD_AUDIT.md);
- [BUILDING.md](BUILDING.md);
- `scripts/phase2-build-audit.sh`;
- `.github/workflows/phase2-linux-build-audit.yml`.

Key result: the existing architecture remains a viable porting base. X11/Xft, thread, script, file and Lua UI layers compile; the first blockers were narrow declaration/type/feature-boundary issues rather than evidence for a rewrite.

**Exit condition:** satisfied.

## Phase 3 — First Native Linux Host

**Goal:** make Psycle launch and produce audio before polishing it.

- [x] Launch the native X11 host under the reproducible CI runtime smoke.
- [x] Prove writable isolated Linux configuration state and normal config save.
- [ ] Enumerate available audio and MIDI backends in a user-facing Linux session.
- [ ] Produce stable audio through physical ALSA hardware.
- [ ] Produce stable audio through a real JACK/PipeWire-JACK session.
- [ ] Verify physical ALSA MIDI input.
- [ ] Verify clean startup/shutdown and device reinitialization across real devices.
- [ ] Continue fixing x86-64 crashes and undefined behaviour as compatibility tests expose them.

Current automated runtime evidence also covers SDL2 selection, an opened SDL dummy device, a completed Psycle audio callback, X11 event-loop survival, command dispatch and normal configuration save. This is CI evidence, not a substitute for real ALSA/JACK/MIDI hardware validation.

**Exit condition:** Psycle starts on Linux, opens its native UI, accepts basic input, and produces audio reliably on supported real devices.

## Phase 4 — Core Psycle Workflow Compatibility

**Goal:** restore the normal Psycle workflow end to end.

### Established automated slices

- [x] Synthetic song creation and PSY3 save through production song I/O.
- [x] MIDI Note On → Psycle event translation → pattern insertion.
- [x] Synthetic PSY3 save/reload semantic round trip.
- [x] WAV import through the historical song/sample path.
- [x] Sampler/instrument construction for imported WAV audio.
- [x] Deterministic embedded PCM preservation across PSY3 save/reload.
- [x] Explicit sampler trigger routing after fresh reload.

### Remaining/expanding user workflow

- [ ] Machine View creation, deletion, wiring, rewiring, mute, bypass, parameter access and editor opening.
- [ ] Pattern editor note entry plus representative tracker commands/effect columns across multiple tracks.
- [ ] Keyboard shortcuts, navigation and focus behaviour required for tracker use.
- [x] Sequencer editing and playback.
- [x] Tempo, LPB/line timing, transport, loop and position behaviour.
- [ ] Normal UI sampler/sample loading workflow.
- [ ] Preset loading/saving.
- [ ] Representative historical `.psy` song load using redistributable or user-supplied fixtures.
- [x] WAV/audio render through Psycle's existing `FileOutDriver` path.
- [x] Psycle-generated WAV → Psycle Sampler compatibility regression.

Automated evidence for the completed sequencer/transport rows is provided by
`.github/workflows/phase4-sequencer-transport.yml`; automated evidence for the
render/bounce rows is provided by `.github/workflows/phase4-render-bounce-sampler.yml`.
These gates complement rather than replace the remaining UI and real-device work.

### Historical bounce-to-sampler acceptance workflow

A classic Psycle workflow must remain first-class:

> build/process a loop in Psycle → render it to WAV → load that exact WAV into Sampler → continue arranging with the rendered loop.

This was used historically to collapse CPU-heavy machine chains into samples. The regression should eventually use Psycle as both producer and consumer so WAV headers, channel layout, sample rate, PCM conversion, frame count and sampler compatibility are tested end to end.

Do **not** reintroduce the eight omitted upstream demo/example `.psy` songs merely to obtain fixtures. Prefer project-authored deterministic fixtures; real showcase/demo material can be added later only with clear rights.

**Exit condition:** a user can create, edit, save, reopen, play and render a real Psycle song on Linux.

## Phase 5 — Classic Native Machine Preservation

**Goal:** make Psycle's native-machine ecosystem first-class Linux citizens without changing their characteristic behaviour merely for modernization.

### 5A — Arguru family

- [ ] Arguru Compressor
- [ ] Arguru Distortion
- [ ] Arguru Goaslicer
- [ ] Arguru Reverb
- [ ] Arguru Synth 2f
- [ ] Arguru XFilter

### 5B — Pooplog family

Preserve the Jeremy Evers/Pooplog native-machine family present in the imported source, including representative synth and effect variants such as:

- [ ] Pooplog FM / FM Laboratory synth family
- [ ] Pooplog Delay
- [ ] Pooplog Filter
- [ ] Pooplog Autopan
- [ ] Pooplog Lofi Processor
- [ ] Pooplog Scratch

### 5C — broader classic Psycle ecosystem

- [ ] Druttis machines
- [ ] JM machines, including JAZ's JM Drum
- [ ] JME machines
- [ ] Zephod machines
- [ ] Yezar machines
- [ ] DW machines
- [ ] STK-derived Psycle machines where licensing/provenance permits
- [ ] Remaining retained native generators/effects in the audited r12005 set

### 5D — preservation matrix

For representative machines, validate:

- [ ] discovery and instantiation;
- [ ] parameter ranges and parameter naming;
- [ ] deterministic/tolerance-based audio behaviour where practical;
- [ ] presets;
- [ ] timing and tracker-command behaviour;
- [ ] state serialization;
- [ ] `.psy` save/reload;
- [ ] missing-machine behaviour;
- [ ] historical-song compatibility where a trustworthy legal reference exists.

Fix crashes, undefined behaviour, 64-bit assumptions and serialization defects, but do not casually rewrite DSP equations and thereby change the sound of old songs.

**Exit condition:** classic native machines load, process audio, preserve state and reopen reliably in Linux sessions.

## Phase 6 — Plugin Hosting

**Goal:** support useful Linux plugin ecosystems while preserving Psycle's historical plugin workflow.

- [ ] Audit current Lilv/LV2 implementation and complete or repair it where practical.
- [ ] Audit historical LADSPA implementation and xpsycle/QPsycle behaviour.
- [ ] Define plugin search paths using Linux conventions.
- [ ] Make failed or missing plugins non-fatal when loading songs.
- [ ] Preserve plugin identity, parameters, MIDI and automation mapping across save/load.

### VST2 preservation

VST2 is an explicit compatibility feature because real Psycle users and songs depend on it. The C-Psycle VST2 host was still receiving real-world compatibility fixes from JAZ in 2021.

- [ ] Keep Linux VST2 support available as an opt-in build feature.
- [ ] Keep Steinberg SDK-derived headers/material out of the public repository unless redistribution rights are independently established.
- [ ] Define/use a legally clean local or clean-room compatibility boundary.
- [ ] Test an externally obtained or source-built free/open Linux VST2 plugin.
- [ ] Verify discovery/load, pattern/MIDI playback, parameter/state save, PSY3 reload and missing-plugin handling.
- [ ] Audit `.fxp`/`.fxb` preset behaviour where supported.

Possible later additions, only after the core port is stable:

- [ ] VST3 evaluation.
- [ ] CLAP evaluation.

**Exit condition:** maintained Linux-native plugin hosting works reliably and legacy VST2 compatibility remains available without compromising the repository's licensing boundary.

## Phase 7 — Modern Linux Integration

**Goal:** make Psycle feel at home on a current Linux desktop without rewriting its identity.

- [ ] Verify operation under PipeWire through JACK/ALSA compatibility layers.
- [ ] Consider a native PipeWire backend only if it solves a demonstrated problem.
- [ ] XDG-compliant configuration, data, cache, preset, sample and plugin paths.
- [ ] Desktop entry and MIME integration for `.psy` files where appropriate.
- [ ] Application icon and launcher integration.
- [ ] HiDPI and scaling audit.
- [ ] File-dialog and clipboard behaviour audit.
- [ ] Multi-monitor and window-placement audit.

**Exit condition:** Psycle integrates cleanly with a modern Linux desktop while preserving its existing UI model.

## Phase 8 — Build System and Packaging

**Goal:** make installation boring and reproducible.

The existing makefiles are the starting point. A build-system migration is justified only by demonstrated maintenance need.

- [ ] Stabilize the current build first.
- [ ] Inspect `trunk/debian` as a historical packaging donor for package boundaries, paths and integration decisions.
- [ ] Decide whether the existing makefiles remain maintainable.
- [ ] If justified, introduce a modern build system incrementally rather than via a source rewrite.
- [ ] Produce a Debian/Ubuntu package or reproducible `.deb` workflow.
- [ ] Produce an AppImage or similarly low-friction portable build if practical.
- [ ] Evaluate Flatpak after plugin discovery and filesystem access are understood.
- [ ] Document distro-packager requirements.

**Exit condition:** a user can install and remove a packaged build without manually copying libraries around the filesystem.

## Phase 9 — Reliability, CI, and Compatibility Suite

**Goal:** stop regressions from undoing the port.

- [ ] CI build on supported Linux compiler versions.
- [ ] Debug and release build jobs.
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer jobs where compatible with the audio path.
- [ ] Headless/library-level tests where UI testing is unnecessary.
- [ ] `.psy` fixture load tests.
- [ ] Save/reload round-trip tests.
- [ ] Native-machine discovery and state tests.
- [ ] Audio render checksum or tolerance-based regression tests where deterministic output is realistic.
- [ ] Dependency and packaging smoke tests.

**Exit condition:** major compatibility regressions are caught automatically before merge.

## Phase 10 — New Native Machines After Preservation

**Goal:** add new Psycle-native instruments only after the historical workflow and machine ecosystem are dependable.

A proposed first new machine is a simple rhythm-station/step-drum machine inspired by the immediacy of classic software rhythm stations and Psycle's own JM Drum heritage, without cloning proprietary software or replacing JM Drum.

Possible staged capabilities:

- [ ] compact 8-lane step sequencer;
- [ ] 16/32/64-step patterns;
- [ ] WAV/sample and simple synthesized drum voices;
- [ ] per-step velocity/accent and later probability/pitch options;
- [ ] swing and Psycle BPM/LPB/transport sync;
- [ ] MIDI triggering and native parameter automation;
- [ ] complete state persistence inside `.psy`;
- [ ] render pattern/bars to WAV through Psycle's established render path;
- [ ] optional **Render → Send to Sampler** workflow after render/sampler compatibility is proven.

The interface should remain immediate: click steps, tweak drums, press play. New-machine work must not delay restoration of classic Psycle machines.

## Phase 11 — PSYCLE-LINUX 1.0

A 1.0 release should mean **usable Psycle on Linux**, not merely "it compiles."

Minimum release criteria:

- [ ] Builds from documented source on a supported Linux distribution.
- [ ] Native graphical host launches reliably.
- [ ] ALSA and/or JACK audio works reliably.
- [ ] MIDI input works.
- [ ] Machine View and pattern editing are usable.
- [ ] New songs can be created and saved.
- [ ] Existing representative `.psy` songs can be loaded.
- [ ] Songs can be reopened without corrupting machine state.
- [ ] Audio can be rendered/exported.
- [ ] Core native machines work, including representative Arguru, Pooplog, Druttis/JM and other retained classic families.
- [ ] VST2 compatibility status is documented and usable where enabled.
- [ ] Installation package is available.
- [ ] Known incompatibilities are documented rather than hidden.

## Non-Goals

Unless requirements change, the following are explicitly **not** objectives of the initial Linux port:

- rewriting Psycle as a web application;
- replacing the tracker with a piano-roll-first workflow;
- redesigning Machine View into a generic modern DAW interface;
- converting the entire codebase to another language merely for modernization;
- replacing X11 before the existing X11 implementation is made functional and compatibility-tested;
- adding fashionable plugin formats before native Psycle functionality works;
- removing VST2 merely because it is old when users/songs still depend on it;
- breaking `.psy` compatibility to simplify implementation;
- renaming or replacing historical native machines without a technical reason;
- performing broad cosmetic refactors in the same changes that establish compatibility.

## Decision Test for New Work

Before adding a major dependency, subsystem, framework or rewrite, ask:

1. Does this directly help Psycle run correctly on modern Linux?
2. Can the existing implementation be repaired instead?
3. Does it preserve existing songs, machines and workflow?
4. Can it be introduced incrementally and tested?
5. Will future maintainers understand why it was necessary?

If the answer is mostly no, it probably does not belong in the port yet.
