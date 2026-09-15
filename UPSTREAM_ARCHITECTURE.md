# Upstream C-Psycle Architectural Reference

## Source

The audited r12005 tree already contains an upstream text rendering of the C-Psycle developer guide:

- `cpsycle/doc/cpsycle-developer-guide.txt`
- title: **Psycle Developer Guide**
- edition: **C-Version, Feb 2021 (unfinished)**

The imported Visual Studio 2019 solution also records `doc/cpsycle-developer-guide.docx` as a documentation item. PSYCLE-LINUX therefore treats the retained text file as a primary upstream architectural reference for C-Psycle intent and terminology.

This guide is **historical evidence, not a replacement specification**. It identifies itself as unfinished. When prose, source code and observed r12005 behavior disagree, the pinned r12005 source plus compatibility evidence take precedence. The guide is used to explain intent, architecture and compatibility constraints that the code alone may not make obvious.

## What the guide establishes

### Compatibility and portability were explicit C-Psycle goals

The guide describes C-Psycle as a C-language variant intended to remain compatible with MFC-Psycle and share as many features as possible. It also describes cross-platform portability as a step-by-step separation of Win32-specific code from platform-independent code, with the UI and audio core already structured around that separation.

This directly supports PSYCLE-LINUX's primary rule:

> **Port Psycle to Linux. Do not reinvent Psycle.**

The port should repair and validate the existing architecture before considering replacement subsystems.

### Audio engine and UI are separate subsystems

The guide divides Psycle into two principal systems:

- the audio engine;
- the UI.

DSP, container and file helpers support those systems, while audio and event drivers are loaded by the audio engine at runtime. This matches the retained source layout and supports keeping drivers and platform implementations behind explicit boundaries.

### Workspace is the host integration point

The guide describes `Workspace` as connecting the host UI and player and holding references to:

- `psy_audio_MachineCallback`;
- `PsycleConfig`;
- `ViewHistory`;
- `psy_audio_Player`;
- `psy_audio_MachineFactory`;
- `psy_audio_PluginCatcher`;
- `psy_audio_Song`.

That model is important to the production compatibility tests in PSYCLE-LINUX: native-machine discovery, factory construction, song persistence and playback should be exercised through these real host-side objects rather than only through isolated plugin code.

### The 256-sample work size is a compatibility contract

The guide explicitly explains that the player splits driver requests into `psy_audio_MAX_STREAM_SIZE` chunks of **256 samples** to maintain compatibility with Psycle plugins. It further splits work at tracker-line boundaries so machines can receive line-end/new-line notifications and `SeqTick()` events in the historical order.

PSYCLE-LINUX therefore treats the 256-sample native-machine work boundary as historical compatibility behavior rather than an arbitrary implementation detail. Preservation tests may exercise smaller blocks, but must not casually remove or redefine this contract.

### C-Psycle deliberately bridges tracker timing into a sequencer model

The guide documents the transition from a line/tick-oriented tracker engine to a beat-position/event sequencer. It explains line-boundary splitting, event timestamps, retrigger/delay event generation and sampler tick timers as compatibility machinery.

This is important when evaluating timing changes: modernizing the event engine does not justify discarding tracker-era timing semantics that native machines and old songs depend on.

### VST processing must tolerate variable block sizes

The guide notes that tracker-line splitting can produce unequal processing intervals for VSTs and states that a VST plugin should handle arbitrary sample counts correctly.

For Phase 6 plugin hosting, this is an upstream compatibility requirement: VST2 restoration must preserve Psycle's variable-block host behavior rather than forcing the tracker/sequencer to provide equal-sized VST blocks.

### Native-machine polyphony is constrained to 64 physical channels

The guide documents the 64-channel native-plugin limit and the use of `LogicalChannel` mapping when multiple sequence tracks would otherwise collide on the same physical plugin channels.

PSYCLE-LINUX should preserve this limit where it is part of the native ABI/behavior instead of expanding it silently in a way that could change voice allocation or old-song behavior.

### The UI was intentionally built around a platform bridge

The guide describes the UI as a bridge with host-side components delegating to implementation-side objects, with Win32 being the developed implementation at the time. That is strong upstream evidence that a Unix/Linux implementation was expected to fit behind the existing UI abstraction rather than require a wholesale host redesign.

### The 2021 build state explains the inherited Linux gap

The guide records that Visual Studio 2019 was the primary build, that plugins were not then built as part of that Windows C-Psycle setup and had to be taken from an MFC-Psycle release, and that the GCC build was "currently out of date".

This historical state helps explain why r12005 already contains substantial cross-platform structure while still requiring extensive Linux build and native-machine preservation work.

## Roadmap consequences

PSYCLE-LINUX uses this upstream guide as supporting evidence for the following project choices:

1. preserve the existing C-Psycle host/audio/UI architecture before considering rewrites;
2. retain the 256-sample native-machine compatibility boundary;
3. preserve tracker-line/`SeqTick()` timing semantics while validating the sequencer implementation;
4. preserve the native 64-channel model unless a separately versioned compatibility design proves a safe extension;
5. exercise `MachineFactory`, `PluginCatcher`, `Song` and real host persistence paths in compatibility tests;
6. require restored VST hosting to accept Psycle's variable process-block sizes;
7. keep plugin discovery/loading failures recoverable so optional or third-party plugins cannot prevent the host from starting;
8. treat the existing platform/UI bridge as the preferred path for Linux integration.

## Authority rule

When using this document in reviews or future preservation work:

1. **pinned r12005 source and observed behavior** are the executable compatibility reference;
2. **the C-Psycle developer guide** is primary historical architecture/intent evidence;
3. older branches, posts and release material are supporting historical donors;
4. later PSYCLE-LINUX behavior changes require explicit tests and rationale rather than being inferred solely from unfinished prose.
