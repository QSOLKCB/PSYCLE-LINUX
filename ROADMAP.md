# PSYCLE-LINUX Roadmap

## Guiding Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

The roadmap is deliberately compatibility-first. New technology is useful only when it helps Psycle run reliably on modern Linux without discarding the workflow, file compatibility, native machines, or character that made Psycle distinct.

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

Initial reference platform: current Ubuntu x86-64 with a modern GCC toolchain.

- [ ] Reproduce the original `make`-based build as closely as practical.
- [ ] Inventory required development packages and `pkg-config` dependencies.
- [ ] Build the core libraries independently before the full host.
- [ ] Capture compiler errors and warnings without immediately suppressing them.
- [ ] Identify obsolete C/C++ assumptions, removed APIs, 32-bit assumptions, path assumptions, and linker-order issues.
- [ ] Verify X11/Xft UI compilation.
- [ ] Verify ALSA, ALSA MIDI, JACK, SDL2, and event-joystick driver builds.
- [ ] Verify Lua and Lilv/LV2 integration points.
- [ ] Build `psyplayer` separately as a smaller audio-engine test target.
- [ ] Add a reproducible development-build document.

**Exit condition:** a clean machine can reproduce the known build state and every blocking failure is documented.

## Phase 3 — First Native Linux Host

**Goal:** make Psycle launch and produce audio before polishing it.

- [ ] Launch the native X11 host without fatal startup errors.
- [ ] Make configuration paths writable and Linux-appropriate.
- [ ] Enumerate available audio and MIDI backends.
- [ ] Produce stable audio through ALSA.
- [ ] Produce stable audio through JACK.
- [ ] Verify ALSA MIDI input.
- [ ] Verify clean startup/shutdown and device reinitialization.
- [ ] Fix obvious x86-64 crashes and undefined behaviour exposed by the port.

**Exit condition:** Psycle starts on Linux, opens its native UI, accepts basic input, and produces audio reliably.

## Phase 4 — Core Psycle Workflow Compatibility

**Goal:** restore the normal Psycle workflow end to end.

- [ ] Machine View creation, deletion, wiring, rewiring, mute, bypass, and parameter access.
- [ ] Pattern editor note entry and tracker commands.
- [ ] Sequencer editing and playback.
- [ ] Tempo, line timing, transport, loop, and position behaviour.
- [ ] Sampler and sample loading.
- [ ] Preset loading/saving.
- [ ] Song creation and save.
- [ ] Existing `.psy` song load.
- [ ] Save/reload round-trip tests.
- [ ] WAV/audio render or equivalent existing export path.
- [ ] Keyboard shortcuts and focus behaviour required for tracker use.

Existing Psycle example songs in the upstream tree should become regression fixtures where licensing permits.

**Exit condition:** a user can create, edit, save, reopen, play, and render a real Psycle song on Linux.

## Phase 5 — Native Machine Preservation

**Goal:** make Psycle's native machines first-class Linux citizens.

Priority includes the Arguru machines present in r12005:

- [ ] Arguru Compressor
- [ ] Arguru Distortion
- [ ] Arguru Goaslicer
- [ ] Arguru Reverb
- [ ] Arguru Synth 2f
- [ ] Arguru XFilter

Also:

- [ ] Build the broader native-machine set.
- [ ] Validate parameter ranges, presets, timing, state serialization, and song reload.
- [ ] Add small deterministic regression songs for representative generators and effects.
- [ ] Compare behaviour with historical Psycle where a trustworthy reference is available.
- [ ] Document machines that cannot yet be made bit-for-bit or behaviourally compatible.

**Exit condition:** core native machines load, process audio, save state, and reopen reliably in Linux sessions.

## Phase 6 — Plugin Hosting

**Goal:** support useful Linux plugin ecosystems without derailing the core port.

Order of work should follow what the existing source already supports and what can be maintained legally and technically.

- [ ] Audit current Lilv/LV2 implementation and complete or repair it where practical.
- [ ] Audit historical LADSPA-related code/documentation.
- [ ] Define plugin search paths using Linux conventions.
- [ ] Make failed or missing plugins non-fatal when loading songs.
- [ ] Preserve plugin identity and automation mapping across save/load.
- [ ] Document the status of legacy VST hosting separately.

Possible later additions, only after the core port is stable:

- [ ] VST3 evaluation.
- [ ] CLAP evaluation.

**VST2 note:** legacy VST2 support is a preservation and licensing problem, not a prerequisite for the first Linux release. Do not import or redistribute SDK material without an explicit licensing review.

**Exit condition:** at least one maintained Linux-native external plugin format works reliably without compromising Psycle's native-machine path.

## Phase 7 — Modern Linux Integration

**Goal:** make Psycle feel at home on a current Linux desktop without rewriting its identity.

- [ ] Verify operation under PipeWire through JACK/ALSA compatibility layers.
- [ ] Consider a native PipeWire backend only if it solves a demonstrated problem.
- [ ] XDG-compliant configuration, data, cache, preset, sample, and plugin paths.
- [ ] Desktop entry and MIME integration for `.psy` files where appropriate.
- [ ] Application icon and launcher integration.
- [ ] HiDPI and scaling audit.
- [ ] File-dialog and clipboard behaviour audit.
- [ ] Multi-monitor and window-placement audit.

**Exit condition:** Psycle integrates cleanly with a modern Linux desktop while preserving its existing UI model.

## Phase 8 — Build System and Packaging

**Goal:** make installation boring and reproducible.

The existing makefiles are the starting point. A build-system migration is not a Phase 1 requirement.

- [ ] Stabilize the current build first.
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

## Phase 10 — PSYCLE-LINUX 1.0

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
- [ ] Core native machines work, including the preserved Arguru set.
- [ ] Installation package is available.
- [ ] Known incompatibilities are documented rather than hidden.

## Non-Goals

Unless the requirements change, the following are explicitly **not** objectives of the initial Linux port:

- rewriting Psycle as a web application;
- replacing the tracker with a piano-roll-first workflow;
- redesigning Machine View into a generic modern DAW interface;
- converting the entire codebase to another language merely for modernization;
- replacing X11 before the X11 implementation is made functional;
- adding fashionable plugin formats before native Psycle functionality works;
- breaking `.psy` compatibility to simplify implementation;
- renaming or replacing historical native machines without a technical reason;
- performing broad cosmetic refactors in the same changes that establish compatibility.

## Decision Test for New Work

Before adding a major dependency, subsystem, framework, or rewrite, ask:

1. Does this directly help Psycle run correctly on modern Linux?
2. Can the existing implementation be repaired instead?
3. Does it preserve existing songs, machines, and workflow?
4. Can it be introduced incrementally and tested?
5. Will future maintainers understand why it was necessary?

If the answer is mostly no, it probably does not belong in the port yet.
