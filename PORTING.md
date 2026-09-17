# PSYCLE-LINUX Porting Policy

## Purpose

PSYCLE-LINUX is a **compatibility and preservation project**, not a clean-sheet DAW rewrite.

The target is the behaviour and workflow of **original Psycle**. Historical reimplementations are valuable donors and test oracles, but none is automatically authoritative merely because it already builds on Linux.

The current source roles are:

- `psycle/` — original C++/MFC Psycle; primary behavioural and UI reference where it can be observed or inspected lawfully;
- `universalis` + `psycle-core` + `psycle-audiodrivers` + `psycle-helpers` + `psycle-player` + `psycle-plugins` — the pinned six-component C++ family, now imported as the sanitized candidate Linux engine/player path;
- `cpsycle/` — later C reimplementation, retained as a preservation baseline, Linux donor, regression corpus and compatibility oracle where semantics overlap.

Phase 6A pinned the reference identities and provenance boundary. Phase 6B completed the sanitized C++ import and historical Linux `psycle-player` build. The immediate implementation work is now **Phase 6C / 6D: collect versioned original-Psycle observations, keep the three-way matrix evidence-gated, and keep the human parity report synchronized with that matrix** before behavioural convergence or full UI work.

## Core Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

Every substantial change should be judged against that rule.

## Compatibility Contract

Where practical, the Linux port should preserve original Psycle's:

- `.psy` song compatibility;
- Machine View semantics and routing;
- pattern editor behaviour and tracker commands;
- timing, transport and sequence/order behaviour;
- Sampler/XMSampler behaviour;
- native machine identity, parameters, presets and state;
- plugin identity, parameters and opaque state;
- historical project loading behaviour;
- user-facing concepts, keyboard habits and terminology that are part of Psycle's workflow.

Compatibility does not require preserving crashes, undefined behaviour, insecure code, obsolete platform assumptions, or bugs that prevent the program from functioning on modern Linux.

When behaviour must change, the reason and compatibility impact should be documented and tested.

## Evidence and Authority

When implementations disagree, use evidence in this order:

1. the pinned original Psycle source/build and reproducible behaviour where legally and technically available;
2. trustworthy historical songs, documented formats and frozen compatibility contracts;
3. the pinned C++ candidate-engine family as the implementation to be measured and repaired, not assumed correct;
4. C-Psycle source and the PSYCLE-LINUX regression corpus where semantics overlap;
5. upstream developer documentation, historical branches/posts and release notes as architecture/intent evidence.

The retained C-Psycle developer guide is explicitly **C-Version, Feb 2021 (unfinished)**. It explains C-Psycle architecture and several useful compatibility constraints, but it does not make C-Psycle the final implementation authority. See `UPSTREAM_ARCHITECTURE.md`.

## Change Strategy

### 1. Preserve every imported baseline

An upstream import should remain mechanically close to the selected pinned snapshot after documented sanitization. Avoid combining source acquisition with cleanup, formatting, naming changes or architectural rewrites.

The audited C-Psycle r12005 baseline remains immutable historical evidence. The sanitized C++ family now has its own mechanically comparable baseline identity and must remain distinguishable from later compatibility patches.

### 2. Reproduce before replacing

Before replacing or substantially changing an existing subsystem:

1. build it;
2. observe the behaviour or failure;
3. compare it against the original-Psycle contract;
4. identify the narrowest cause of any difference;
5. patch the candidate implementation if practical;
6. add a regression test or reproducible fixture.

Replacement is justified when repair would be less maintainable, unsafe, legally unsuitable, or fundamentally incompatible with current Linux.

### 3. Prefer narrow compatibility patches

Good implementation changes include:

- fixing compiler errors caused by modern language/toolchain rules;
- replacing removed platform APIs with equivalent supported APIs;
- correcting pointer-width and integer-width assumptions;
- fixing Linux filesystem/path handling;
- repairing linker ordering and dependency detection;
- correcting audio/MIDI device handling;
- closing measured song/timing/sampler/mixer compatibility gaps;
- making plugin discovery, instantiation and state restoration failure-safe;
- adding an independently authored compatibility boundary where a historical SDK cannot be redistributed.

Large unrelated refactors should be separate PRs.

### 4. Keep subsystem changes reviewable

Prefer PRs scoped to one concern, for example:

- sanitized C++ provenance/import;
- player build compatibility;
- one timing or sampler parity gap;
- ALSA/JACK/MIDI integration;
- `.psy` loading or persistence;
- one native-machine compatibility family;
- Qt tracker input/focus;
- plugin discovery or process isolation;
- clean-room VST2 ABI compatibility.

A port is easier to trust when each behavioural change has a visible reason and oracle.

## C-Psycle Preservation Policy

The work already completed against `cpsycle/` is retained, not discarded.

C-Psycle provides:

- a modern-Linux build/runtime reference;
- a broad native-machine preservation corpus;
- PSY2/PSY3, preset and opaque-state regressions;
- sampler/render workflow tests;
- timing and tracker-command evidence;
- Linux driver and UI implementation donors;
- VST2 provenance research and plugin-safety lessons.

Reuse C-Psycle code or tests only when the behaviour is demonstrated equivalent to original Psycle or when the code is a platform implementation independent of C-Psycle-specific semantics. Do not import its event/sequencer behaviour into the C++ candidate merely because it is newer.

## Engine Policy

The leading engine candidate is the pinned six-component C++ family established in Phase 6A and imported through the Phase 6B sanitization boundary.

Phase 6A established:

1. **Psycle 1.12.0 x86** as the primary original-Psycle behavioural reference;
2. SourceForge SVN **r12005** for `universalis`, `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, and `psycle-plugins`;
3. frozen per-component file counts, last-changed revisions, and locale-stable manifest hashes;
4. the licensing/provenance boundary and concrete exclusions/quarantines.

Phase 6B then:

1. materialized the exact retained-file list and omission/replacement arithmetic from those frozen manifests;
2. preserved/restored required compatible third-party permission/provenance notices;
3. created a separate sanitized C++ baseline identity;
4. imported only provenance-cleared source while keeping documented VST/ASIO/closed-binary/song quarantines out;
5. reproduced the historical Debian/Linux `psycle-player` build and recorded the required support inputs/blockers;
6. established deterministic candidate execution evidence for project-authored PSY2/PSY3 fixtures.

The current Phase 6C/6D rule is stricter: **do not rewrite an engine subsystem until versioned original-Psycle observations demonstrate the compatibility gap.** Candidate-only evidence remains useful, but it does not justify a Phase 7 parity fix by itself.

## Build-System Policy

Preserve a reproducible build path for each imported upstream family before considering migration.

A later move to CMake, Meson or another system may be considered when it provides a concrete benefit such as:

- reliable dependency detection across distributions;
- reproducible builds;
- packaging support;
- test integration;
- maintainability that cannot reasonably be achieved with the existing build.

A build-system migration must not also become an engine or UI rewrite.

## UI Policy

The UI compatibility target is **original Psycle's interaction model**, not C-Psycle's X11 implementation.

The C-Psycle X11/UI bridge remains useful donor and historical evidence, but it is no longer the mandatory first UI architecture for the final product.

After engine parity is strong enough, **Qt is the leading Linux UI candidate** because the original MFC UI cannot be carried directly to Linux. Start by proving Qt Widgets for tracker keyboard focus, custom painting, Machine View, parameter/editor windows and desktop-window behaviour. Evaluate QML only where it offers a demonstrated advantage.

Do not redesign Psycle into a generic modern DAW merely to adopt a newer toolkit.

## Audio and MIDI Policy

During C++ engine audit/convergence work, prefer the existing `psycle-audiodrivers` lineage where it is coherent and compatible, while using C-Psycle's ALSA/JACK/ALSA-MIDI/SDL2 work as donor/reference material where useful.

Required Linux acceptance eventually includes:

1. ALSA audio;
2. JACK / PipeWire-JACK;
3. ALSA MIDI;
4. any additional backend only when it solves a demonstrated problem.

A native PipeWire backend is optional; established ALSA/JACK compatibility paths are sufficient unless testing shows otherwise.

## Plugin Policy

Native Psycle machines and third-party plugin hosting are both historically important, but they have different trust boundaries.

For native machines:

- preserve historical identity and state contracts;
- reuse the completed C-Psycle preservation corpus as an oracle where the ABI/behaviour matches original Psycle;
- avoid adding every retained optional machine to the default runtime surface merely because source exists.

For third-party plugins:

1. restore the format/host boundary required by real songs;
2. make discovery incremental and cacheable;
3. probe untrusted binaries out of process;
4. make plugin instantiation and song-specific state restoration crash/hang-contained;
5. preserve missing/quarantined nodes as recoverable placeholders;
6. only then broaden format support.

VST2 is a deliberate compatibility target. The public repository must not casually restore omitted Steinberg SDK-derived source. Phase 9 tracks a project-authored clean-room ABI boundary, VST2 hosting, editor integration and process isolation.

VST3 and CLAP are later evaluations, not prerequisites for a faithful initial Linux Psycle.

## Native Machine Policy

Historical native machines should retain their names and identity unless a technical or legal constraint requires otherwise.

The completed C-Psycle preservation gates freeze representative metadata, DSP/timing behaviour, preset/state persistence and song reopen contracts. Those tests are evidence, but original Psycle remains the higher-level behavioural target when a reimplementation diverges.

Changes to DSP code should be minimized until behaviour can be measured. Compiler fixes, undefined-behaviour fixes and platform-width fixes should be separated from intentional sound changes wherever possible.

## Testing Policy

Porting tests should prefer behaviour over implementation detail.

Useful regression targets include:

- headless/player song playback;
- `.psy` parsing and load;
- save/load round trips;
- timing/BPM/LPB/tracker commands;
- Sampler/XMSampler output;
- mixer/master/routing semantics;
- native-machine discovery and state;
- plugin parameter/opaque-state restoration;
- deterministic or tolerance-based audio renders;
- plugin crash/hang containment;
- Qt tracker focus/input and Machine View interactions;
- historical songs contributed or redistributable with clear permission.

C-Psycle regressions should be ported only where they test a shared contract; otherwise retain them as C-Psycle historical tests.

## Source Hygiene

When touching upstream files:

- preserve copyright and attribution headers;
- do not delete historical comments merely because they are old;
- avoid mass formatting during functional work;
- explain compatibility changes in commit messages;
- keep third-party code boundaries visible;
- do not silently change licensing notices;
- avoid adding bundled dependencies when system packages are practical;
- keep separate upstream families and provenance records distinguishable;
- keep the Phase 6B sanitized C++ baseline frozen and apply later compatibility work as traceable changes relative to it.

## Definition of a Good Porting PR

A good PR answers:

1. **What original Psycle behaviour is being preserved or restored?**
2. **Which implementation is being changed, and why did it differ or fail on Linux?**
3. **What evidence establishes the expected behaviour?**
4. **What is the smallest maintainable fix?**
5. **How was compatibility tested?**
6. **What remains intentionally unchanged or deferred?**

If a PR cannot answer those questions, its scope may be too broad.
