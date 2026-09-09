# PSYCLE-LINUX Porting Policy

## Purpose

PSYCLE-LINUX is a **port and preservation project**, not a clean-sheet rewrite.

The upstream C-Psycle code already contains Linux-facing architecture. Our job is to make that architecture work reliably on modern systems, repair gaps, and modernize only where necessary.

## Core Rule

> **Port Psycle to Linux. Do not reinvent Psycle.**

Every substantial change should be judged against that rule.

## Compatibility Contract

Where practical, the Linux port should preserve:

- `.psy` song compatibility;
- Machine View semantics and routing;
- pattern editor behaviour and tracker commands;
- sequencer timing and transport behaviour;
- sampler behaviour;
- native machine identity, parameters, presets, and state;
- historical project loading behaviour;
- user-facing concepts and terminology that are part of Psycle's workflow.

Compatibility does not require preserving crashes, undefined behaviour, insecure code, obsolete platform assumptions, or bugs that prevent the program from functioning on modern Linux.

When behaviour must change, the reason should be documented.

## Change Strategy

### 1. Preserve the baseline

The first upstream import should remain mechanically close to the selected r12005 snapshot. Avoid combining source import with cleanup, formatting, naming changes, or architectural rewrites.

### 2. Reproduce before replacing

Before replacing an existing subsystem:

1. build it;
2. observe the failure;
3. identify the narrowest cause;
4. patch it if practical;
5. add a regression test or reproducible fixture where possible.

Replacement is justified when repair would be less maintainable, unsafe, or fundamentally incompatible with current Linux.

### 3. Prefer narrow compatibility patches

Good early porting changes include:

- fixing compiler errors caused by modern language/toolchain rules;
- replacing removed system APIs with equivalent supported APIs;
- correcting pointer-width and integer-width assumptions;
- fixing Linux filesystem/path handling;
- repairing linker ordering and dependency detection;
- fixing X11 lifetime/event bugs;
- correcting audio-device enumeration and buffer handling;
- repairing ALSA/JACK/MIDI initialization and shutdown;
- making plugin discovery follow Linux path conventions.

Large unrelated refactors should be separate PRs.

### 4. Keep subsystem changes reviewable

Prefer PRs scoped to one concern, for example:

- build-system compatibility;
- X11 host startup;
- ALSA output;
- JACK output;
- MIDI input;
- `.psy` loading;
- one native-machine family;
- plugin discovery.

A port is easier to trust when each behavioural change has a visible reason.

## Existing Linux Architecture to Preserve First

The selected r12005 tree already contains:

- `ui/src/imps/x11/` — X11 UI implementation;
- `driver/alsa/` — ALSA audio backend;
- `driver/alsamidi/` — ALSA MIDI backend;
- `driver/jack/` — JACK backend;
- `driver/sdl2/` — SDL2 backend;
- `driver/evjoystick/` — Linux event joystick support;
- Lua UI/script components;
- Lilv/LV2-related integration points;
- `player/` / `psyplayer`;
- native plugin source and presets.

Those are the first places to repair and validate. A replacement architecture should not be introduced merely because another framework is newer.

## Build-System Policy

The existing makefiles are part of the baseline and should be made usable before any broad build-system migration.

A later move to CMake, Meson, or another system may be considered when it provides a concrete benefit such as:

- reliable dependency detection across distributions;
- reproducible builds;
- packaging support;
- test integration;
- maintainability that cannot reasonably be achieved with the existing build.

A build-system migration must not also become a source-code rewrite.

## UI Policy

The existing Linux UI path is X11-based. The first target is therefore a functional X11 Psycle host.

Wayland-native work may be evaluated later, but X11 compatibility through XWayland is acceptable for early milestones if it delivers a stable native Linux Psycle sooner.

Do not replace the Psycle interaction model merely to adopt a modern UI toolkit.

## Audio Policy

Initial priority:

1. ALSA;
2. JACK;
3. ALSA MIDI;
4. SDL2 where useful.

PipeWire compatibility should first be achieved through its established ALSA/JACK compatibility paths. A native PipeWire backend is optional and should be added only if it provides a demonstrated benefit.

## Plugin Policy

Native Psycle machines are part of the core port, not an optional plugin ecosystem.

External plugin formats should be handled in this order:

1. repair what the existing Linux code already supports;
2. establish reliable discovery and failure handling;
3. preserve song compatibility;
4. only then evaluate new formats.

LV2 should be audited early because the r12005 code already contains Lilv-related integration.

VST2 requires a separate licensing and preservation review. Do not add SDK material casually.

VST3 and CLAP are possible later additions, not prerequisites for a working Psycle Linux port.

## Native Machine Policy

Historical native machines should retain their names and identity unless a technical or legal constraint requires otherwise.

The Arguru machines in the selected baseline are explicit preservation targets:

- Compressor
- Distortion
- Goaslicer
- Reverb
- Synth 2f
- XFilter

Changes to DSP code should be minimized until baseline behaviour can be measured. Compiler fixes, undefined-behaviour fixes, and platform-width fixes should be separated from intentional DSP changes wherever possible.

## Testing Policy

Porting tests should prefer behaviour over implementation detail.

Useful regression targets include:

- application launch and clean shutdown;
- backend enumeration;
- audio initialization;
- MIDI input;
- `.psy` parsing and load;
- save/load round trips;
- native-machine discovery;
- machine-state serialization;
- deterministic or tolerance-based audio renders;
- example songs from the upstream source where redistribution terms permit.

Historical songs from community members are especially valuable when contributed with clear permission.

## Source Hygiene

When touching upstream files:

- preserve copyright and attribution headers;
- do not delete historical comments merely because they are old;
- avoid mass formatting during functional work;
- explain compatibility changes in commit messages;
- keep third-party code boundaries visible;
- do not silently change licensing notices;
- avoid adding bundled dependencies when system packages are practical.

## Definition of a Good Porting PR

A good PR answers five questions:

1. **What Psycle behaviour was broken or unavailable on Linux?**
2. **Why did the existing implementation fail?**
3. **What is the smallest maintainable fix?**
4. **How was compatibility tested?**
5. **What remains intentionally unchanged?**

If a PR cannot answer those questions, its scope may be too broad.
