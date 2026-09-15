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

### Established end-to-end user workflow

- [x] Machine View command/model path: creation, deletion, wiring, rewiring, mute, bypass, parameter access and persisted machine positions.
- [x] Live Machine View editor opening and embedded editor interaction through the native X11 parameter/tool frame.
- [x] Tracker Grid command/model path: note entry plus representative tracker/effect commands across multiple tracks, including undo/redo and PSY3 reload.
- [x] Keyboard shortcuts, navigation and focus behaviour required for tracker use.
- [x] Sequencer editing and playback.
- [x] Tempo, LPB/line timing, transport, loop and position behaviour.
- [x] Normal UI sampler/sample loading workflow through FileView and Workspace sample/instrument insertion.
- [x] Preset loading/saving, including integer parameters and opaque plugin-state payloads.
- [x] Representative historical `.psy` song load using a project-authored PSY2SONG fixture derived from the documented Psycle 1.66-era layout.
- [x] WAV/audio render through Psycle's existing `FileOutDriver` path.
- [x] Psycle-generated WAV → Psycle Sampler compatibility regression.

Automated evidence for Machine View/Tracker command-model compatibility is provided
by `.github/workflows/phase4-interactive-editing.yml`; sequencer/transport evidence
is provided by `.github/workflows/phase4-sequencer-transport.yml`; render/bounce
evidence is provided by `.github/workflows/phase4-render-bounce-sampler.yml`; and
preset serialization evidence is provided by `.github/workflows/phase4-preset-roundtrip.yml`;
legacy PSY2 evidence is provided by `.github/workflows/phase4-historical-psy2.yml`; and
the native editor/focus/FileView evidence is exercised by `scripts/phase3-runtime-smoke.sh` inside the Linux audit.
Phase 3 physical ALSA/JACK/MIDI validation remains separate real-device work and is not claimed by these Phase 4 gates.

### Historical bounce-to-sampler acceptance workflow

A classic Psycle workflow must remain first-class:

> build/process a loop in Psycle → render it to WAV → load that exact WAV into Sampler → continue arranging with the rendered loop.

This was used historically to collapse CPU-heavy machine chains into samples. The regression now uses Psycle as both producer and consumer so WAV headers, channel layout, sample rate, PCM conversion, frame count and sampler compatibility are tested end to end.

Do **not** reintroduce the eight omitted upstream demo/example `.psy` songs merely to obtain fixtures. Prefer project-authored deterministic fixtures; real showcase/demo material can be added later only with clear rights.

**Exit condition:** satisfied for the automated Phase 4 compatibility scope: create, edit, save, reopen, play, render, preset persistence, normal sample loading, live editor/focus behavior and legacy PSY2 loading are all gated on Linux. Physical ALSA/JACK/MIDI validation remains tracked separately in Phase 3.

## Phase 5 — Classic Native Machine Preservation

**Goal:** make Psycle's native-machine ecosystem first-class Linux citizens without changing their characteristic behaviour merely for modernization.

### 5A — Arguru family

- [x] Arguru Compressor
  - [x] Build the retained source as a Linux native-machine `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze machine identity/version/type plus all six parameter names, ranges, flags and defaults.
  - [x] Verify deterministic Ratio=0 unity bypass and retained 2x Input Gain behaviour without changing the DSP equations.
  - [x] Verify six-parameter preset/state serialization and full `.psy` save/reload through production `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.
- [x] Arguru Distortion
  - [x] Build the retained source as a Linux native-machine `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze machine identity/version/type plus all four historical parameter names, descriptions, ranges, flags and defaults.
  - [x] Verify deterministic hard clipping, stereo phase inversion and retained stateful saturate-mode response without changing the DSP equations.
  - [x] Verify four-parameter preset/state serialization and a fresh production `.psy` save/reload through `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.
- [x] Arguru Goaslicer
  - [x] Build the retained source as a Linux native-machine `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze machine identity/version/type plus the historical `Length` and `Slope` parameter names, descriptions, ranges, flags and defaults.
  - [x] Verify deterministic 44.1 kHz gate/fade-down behaviour, `SequencerTick()` release/fade-up behaviour, and retained 88.2 kHz sample-rate scaling.
  - [x] Verify two-parameter preset/state serialization and a fresh production `.psy` save/reload through `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.
- [x] Arguru Reverb
  - [x] Build the retained source as a Linux native-machine `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze machine identity/version/type plus all eight historical parameter names, descriptions, ranges, flags and defaults, including the retained `Absortion` spelling.
  - [x] Verify exact dry-path unity, retained wet stereo pre-delay behaviour and `SequencerTick()` sample-rate reinitialization at 88.2 kHz without changing the DSP equations.
  - [x] Verify eight-parameter preset/state serialization and a fresh production `.psy` save/reload through `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.
- [x] Arguru Synth 2f
  - [x] Build the retained source as a Linux native-generator `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze generator identity/version/type plus all 28 historical parameter names, descriptions, ranges, flags and defaults.
  - [x] Verify deterministic fixed-waveform A4 rendering, minimum-release Note Off behaviour and retained 88.2 kHz wavetable/sample-rate reinitialization without changing synthesis equations.
  - [x] Verify 28-parameter preset/state serialization and a fresh production `.psy` save/reload through `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.
- [x] Arguru XFilter
  - [x] Build the retained source as a Linux native-machine `.so` and load it through Psycle's `GetInfo` / `CreateMachine` / `DeleteMachine` ABI.
  - [x] Freeze the historical `Arguru CrossDelay` identity/version/type plus all six parameter names, descriptions, ranges, flags and defaults.
  - [x] Verify exact dry-path unity, retained sample-delay stereo offset, 88.2 kHz sample-rate scaling, and Lines-mode tracker-tick reconfiguration without changing the DSP equations.
  - [x] Verify six-parameter preset/state serialization and a fresh production `.psy` save/reload through `PluginCatcher`, `MachineFactory`, preset I/O and PSY3 paths.

Phase 5A Arguru-family preservation is complete. Arguru Compressor, Arguru Distortion, Arguru Goaslicer, Arguru Reverb, Arguru Synth 2f and Arguru XFilter/CrossDelay are all gated on Linux. Compressor coverage is split across `.github/workflows/phase5-arguru-compressor.yml` and `.github/workflows/phase5-arguru-compressor-state.yml`; Distortion is covered by `.github/workflows/phase5-arguru-distortion.yml`; Goaslicer by `.github/workflows/phase5-arguru-goaslicer.yml`; Reverb by `.github/workflows/phase5-arguru-reverb.yml`; Synth 2f by `.github/workflows/phase5-arguru-synth-2f.yml`; and XFilter/CrossDelay by `.github/workflows/phase5-arguru-xfilter.yml`. These gates cover native ABI/metadata, representative deterministic DSP/timing/synthesis behavior, preset/state persistence and fresh PSY3 reopen. None of these retained machines exposes an opaque `GetData` payload, so no new state format was invented.

### 5B — Pooplog family

Preserve the Jeremy Evers/Pooplog native-machine family present in the imported source, including representative synth and effect variants such as:

- [x] Pooplog FM / FM Laboratory synth family
  - [x] Build and gate the retained FM Laboratory, FM Light and FM UltraLight Linux `.so` variants.
  - [x] Freeze native identity/version/type plus complete parameter names, descriptions, ranges, flags and defaults with exact metadata hashes.
  - [x] Verify deterministic active-note rendering and live 44.1 kHz → 88.2 kHz `SequencerTick()` sample-rate transition on existing synth instances.
  - [x] Preserve historical opaque `GetData` state byte-for-byte across version-1 presets and fresh PSY3 reopen, including deterministic reserved pointer slots without changing the legacy layout or size.
- [x] Pooplog Delay
  - [x] Gate both retained full and Light builds, neutral DSP, live host-timing reinitialization, parameter persistence, presets and PSY3 reopen.
- [x] Pooplog Filter
  - [x] Gate native identity/metadata, neutral deterministic DSP, public parameter endpoints, presets and PSY3 reopen.
- [x] Pooplog Autopan
  - [x] Gate native identity/metadata, centered/depth-zero deterministic DSP, public parameter endpoints, presets and PSY3 reopen.
- [x] Pooplog Lofi Processor
  - [x] Restore the missing Linux makefile/top-level build target, fix the missing C++ math declaration without changing DSP equations, and gate neutral DSP, state and PSY3 reopen.
- [x] Pooplog Scratch
  - [x] Restore the missing Linux makefile/top-level build target and gate dry-path DSP, timing reinitialization, state and PSY3 reopen.

Phase 5B Pooplog-family preservation is complete for the nine retained source-built identities: FM Laboratory, FM Light, FM UltraLight, Delay, Delay Light, Filter, Autopan, Lofi Processor and Scratch Master. `.github/workflows/phase5-pooplog-family.yml` gates native ABI/identity/metadata, exact parameter hashes, representative deterministic DSP, live sample-rate/timing transitions, public parameter application, per-machine preset persistence, FM opaque state, one-song nine-machine PSY3 reopen and all machine-to-Master topology edges. The host category alias `pooplog-scratch-master-2:0` is not claimed because no corresponding retained source/build directory exists in the audited tree.

### 5C — broader classic Psycle ecosystem

- [x] Druttis machines
  - [x] Build and gate all seven retained source-built Druttis identities: EQ-3, FeedMe, Koruz, Phantom, Plucked String, Slicit and Sublime.
  - [x] Freeze native identity/version/type plus complete parameter names, descriptions, ranges, flags and defaults with exact metadata hashes for all seven machines.
  - [x] Preserve representative historical DSP and timing behaviour, including FeedMe first-instance wavetable initialization, Koruz/Phantom stochastic anti-denormal paths, live 44.1 → 88.2 kHz rate transitions and the native 256-sample `MAX_BUFFER_LENGTH` contract.
  - [x] Preserve Slicit's historical 16-program opaque bank as the exact 2144-byte payload (`0x7271bd63c9a7782d`) through `PutData`, version-1 preset reload and fresh PSY3 reopen.
  - [x] Verify production `PluginCatcher` / `MachineFactory` discovery, public parameter endpoints, per-machine presets, one-song seven-machine fresh PSY3 reopen and all seven machine-to-Master topology edges.
- [x] JM machines, including JAZ's JM Drum
  - [x] Build and gate the retained source-built JM Drum v2.5 native generator independently on Linux.
  - [x] Freeze native identity/version/type, four-column geometry and all 16 historical parameter names, descriptions, ranges, flags and defaults.
  - [x] Preserve deterministic drum/thump synthesis, historical `0Cxx` volume behaviour, the 256-sample Note Off release and live 44.1 → 88.2 kHz sample-rate reinitialization without rewriting synthesis equations.
  - [x] Verify production `PluginCatcher` identity `jmdrum:0`, `MachineFactory` instantiation, complete 16-parameter version-1 preset persistence, fresh PSY3 reopen and JM Drum-to-Master topology.
- [x] JME machines
  - [x] Build and gate all four retained source-built JME identities independently on Linux: Blitz 1.2.1, Blitz 1.6, GameFX 1.3.1 and GameFX 1.6.
  - [x] Freeze each historical generator identity/version/parameter geometry and complete parameter metadata with exact hashes (`0x93f6aa60b3378502`, `0x194a11c1f2c71a31`, `0x85b25fe270d3bdd0`, `0x1a597c19c5c61a60`).
  - [x] Preserve deterministic default synthesis, fresh 88.2 kHz rendering and version-specific `0C80` semantics: Blitz 1.2.1, Blitz 1.6 and GameFX 1.3.1 retain note-on amplitude scaling while GameFX 1.6 retains its later `InitEffect` path without that note-on scaling.
  - [x] Define Blitz 1.2.1 runtime state before production `Init()` by carrying forward the later Blitz lifecycle initialization only; no synthesis/filter/envelope equations are rewritten.
  - [x] Verify production catcher identities `blitz12:0`, `blitzn:0`, `gamefx13:0`, `gamefxn:0`, complete public-parameter version-1 preset persistence, one four-machine fresh PSY3 reopen and all four generator-to-Master topology edges.
- [x] Zephod machines
  - [x] Build and gate the retained source-built Zephod SuperFM (Arguru Remix) generator independently on Linux as `zephod-superfm.so`.
  - [x] Freeze native identity/version/type, two-column geometry and all 20 historical parameter names, descriptions, ranges, flags and defaults for catcher identity `zephod-superfm:0`.
  - [x] Define the envelope lifecycle so stopped/zero-level notes enter a real attack instead of a zero-coefficient anti-click deadlock, while retaining the anti-click path for active retriggers.
  - [x] Preserve deterministic FM synthesis, historical `0C80` volume scaling, the default 2414-sample smooth Note Off release and live 44.1 → 88.2 kHz envelope reconfiguration without rewriting oscillator/FM-routing equations.
  - [x] Preserve the historical `sustain < 16 => until noteoff` rule during live sample-rate changes for both VCA and modulation envelopes.
  - [x] Verify production `PluginCatcher` / `MachineFactory` discovery, complete 20-parameter version-1 preset persistence, fresh PSY3 reopen and Zephod SuperFM-to-Master topology.
- [x] Yezar machines
  - [x] Build and gate the retained source-built `yezar_freeverb` implementation independently on Linux as the historical module `arguru-freeverb.so`.
  - [x] Freeze the compatibility identity `Jezar Freeverb` / `Freeverb` / `Jezar`, version `0x0110`, effect/two-column geometry, catcher `arguru-freeverb:0`, and all five historical parameter records including the retained `Absortion` spelling.
  - [x] Preserve exact Dry=320/Wet=0 unity plus wet-only stereo comb timing at 44.1 kHz (1116 left / 1139 right) and live 88.2 kHz `SequencerTick()` network reinitialization (2232 left / 2278 right) without rewriting Freeverb comb/allpass equations.
  - [x] Verify all 5/5 public parameters with legal non-default values through version-1 preset restore and independent fresh PSY3 reopen, with zero opaque state and Jezar Freeverb-to-Master topology.
- [x] DW machines
  - [x] Build and gate all four retained source-built D. W. Aley identities independently on Linux: dw eq, dw granulizer, dw IoPan and dw Tremolo.
  - [x] Freeze native identity/version/type/geometry and complete parameter metadata with exact hashes (`0xc82fe5a084c00d43`, `0x77e34d124f74ed41`, `0xd2b2cf8908d12251`, `0x637aa08128ba99b8`).
  - [x] Preserve representative historical DSP/timing behaviour: dw eq default unity and live 44.1 → 88.2 kHz coefficient reconfiguration, Granulizer fixed-grain 10→20-sample rate scaling with random modulation disabled, IoPan default unity/full channel flip, and Tremolo Depth=0 unity plus live wall-clock LFO rate scaling.
  - [x] Verify production catcher identities `dw-eq:0`, `dw-granulizer:0`, `dw-iopan:0` and `dw-tremolo:0`, including version-1 preset restore, one four-machine fresh PSY3 reopen and all four effect-to-Master topology edges.
  - [x] Preserve DW public state without inventing opaque payloads: EQ 12/12 state slots, Granulizer 38 `MPF_STATE` slots (36 writable controls plus two derived runtime values) while freezing its 12 structural label/null ABI slots only through metadata, IoPan 4/4 and Tremolo 8/8.
- [x] STK-derived Psycle machines where licensing/provenance permits
- [ ] Remaining retained native generators/effects in the audited r12005 set
  - [x] Alk Muter
    - [x] Build and gate the retained source independently on Linux as `alk-muter.so` and freeze native identity/version/type/geometry plus the complete one-parameter metadata surface.
    - [x] Preserve exact default unity, historical click-avoiding mute/unmute timing at 44.1 kHz and the live 44.1 → 88.2 kHz `SequencerTick()` timing update.
    - [x] Remove the historical one-sample out-of-bounds mute-tail write by making `Work()` consume exactly the host-supplied sample count; canary-guarded regression coverage freezes the block boundary.
    - [x] Verify production catcher `alk-muter:0`, non-default Mute=1 version-1 preset restore, fresh PSY3 reopen and Alk Muter-to-Master topology with zero opaque state.

The Druttis, JM, JME, Zephod, Yezar, DW and STK slices of Phase 5C are complete and gated by `.github/workflows/phase5-druttis-family.yml`, `.github/workflows/phase5-jm-drum.yml`, `.github/workflows/phase5-jme-family.yml`, `.github/workflows/phase5-zephod-superfm.yml`, `.github/workflows/phase5-yezar-freeverb.yml`, `.github/workflows/phase5-dw-family.yml` and `.github/workflows/phase5-stk-family.yml`. Alk Muter is the first completed focused slice of the final remaining-native audit and is gated by `.github/workflows/phase5-alk-muter.yml`; the parent remaining-native item stays open until the rest of the retained r12005 machine set has equivalent source-derived preservation evidence.

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

- [x] CI build on the current supported Ubuntu/GCC reference platform.
- [ ] Debug and release build jobs.
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer jobs where compatible with the audio path.
- [x] Headless/library-level tests where UI testing is unnecessary.
- [x] `.psy` fixture load tests.
- [x] Save/reload round-trip tests.
- [x] Native-machine discovery and state tests.
- [x] Audio render checksum or tolerance-based regression tests where deterministic output is realistic.
- [ ] Dependency and packaging smoke tests.

Current Phase 9 evidence comes from the Linux build/runtime audit plus the Phase 4 interactive-editing, sequencer/transport, audible-sample, preset, historical-PSY2 and render/bounce gates. Phase 5 now adds real native-machine ABI/discovery, deterministic DSP/timing/synthesis, preset/state serialization and PSY3 reopen coverage through the completely gated Arguru family and the nine source-built Pooplog identities, including the three FM opaque-state variants.

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
