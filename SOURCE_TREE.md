# C-Psycle r12005 Source Tree Map

This document is a maintainer-oriented map of the imported C-Psycle baseline under `cpsycle/`.

It describes the source as received from upstream. It is **not** a proposal to reorganize the tree.

## Build Entry Points

The main upstream build entry point is:

```text
cpsycle/makefile
```

Its default `all` target builds:

1. host;
2. plugins;
3. drivers;
4. player.

Important sub-build entry points include:

```text
cpsycle/host/makefile
cpsycle/plugins/makefile
cpsycle/driver/makefile
cpsycle/player/makefile
```

The existing Linux-oriented driver makefile targets:

- SDL2;
- ALSA;
- ALSA MIDI;
- JACK;
- Linux event joystick.

The top-level upstream README already documents Linux dependencies and a native `make` workflow. Phase 2 begins from these files rather than replacing them.

## Core Runtime and Libraries

### `audio/`

Audio engine, machine/plugin integration, song/audio structures, playback support and related interfaces.

This area also contains historical VST2-hosting integration points. Three Steinberg SDK-derived headers are intentionally absent from the public baseline; see [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md).

### `dsp/`

Shared digital-signal-processing helpers used by the host and machines.

### `container/`

General-purpose container/data-structure support used by the C codebase.

### `file/`

Filesystem, path, file and stream support.

### `thread/`

Threading/synchronization helpers.

### `script/`

Scripting support shared by host/player components.

## User Interface

### `ui/`

Cross-platform UI abstraction and implementations.

Of particular importance to the Linux port:

```text
cpsycle/ui/src/imps/x11/
```

This existing X11 implementation is the starting point for the native Linux UI. Phase 1 does not replace it with a new toolkit.

### `luaui/`

Lua-facing UI bindings/integration.

### `luascripts/`

Lua scripts and UI assets used by Psycle features, including pianoroll-related scripts, icons and other host-side scripting resources.

### `skins/`

Historical Psycle skin assets and related resources.

### `pixmaps/`

Application, installer and UI image/icon resources.

## Host and Player

### `host/`

The main Psycle application/host implementation.

The host makefile builds shared support modules and `host/src`, producing the native host executable copied by the top-level makefile to:

```text
cpsycle/psycle
```

The host tree also contains substantial historical Windows resources/project metadata. Those files are retained in the baseline but are not assumed to be relevant to a Linux build.

### `player/`

Standalone/smaller player target using the shared audio engine and support libraries.

The top-level build copies the resulting executable to:

```text
cpsycle/psyplayer
```

`psyplayer` is an important Phase 2 audit target because it may exercise core song/audio code with less UI complexity than the full host.

## Audio and Input Drivers

### `driver/alsa/`

Native ALSA audio backend.

### `driver/alsamidi/`

Native ALSA MIDI backend.

### `driver/jack/`

JACK audio backend.

### `driver/sdl2/`

SDL2 backend.

### `driver/evjoystick/`

Linux event-device joystick/input support.

### Other driver directories

The tree also retains historical Windows-oriented driver code for baseline fidelity. In particular, `driver/asiodriver/` contains Psycle-side ASIO driver integration, but the bundled Steinberg `asio-2/` SDK subtree is intentionally not mirrored because of its redistribution terms.

## Native Machines and Plugins

### `plugins/`

Large collection of Psycle native machines, generators and effects plus their build metadata.

The upstream Linux plugin makefile already enumerates a broad native-machine set.

### Arguru preservation targets

The following six source directories are present in the baseline and are explicit compatibility targets:

```text
cpsycle/plugins/arguru-compressor/
cpsycle/plugins/arguru-distortion/
cpsycle/plugins/arguru-goaslicer/
cpsycle/plugins/arguru-reverb/
cpsycle/plugins/arguru-synth-2f/
cpsycle/plugins/arguru-xfilter/
```

These are not candidates for replacement merely because they are old. Their behaviour, parameter/state handling and compatibility with historical songs will be validated later in the roadmap.

## Presets, Songs and Documentation

### `presets/`

Historical Psycle preset banks (`.prs`), including Arguru Synth 2f presets and other native-machine presets.

### `doc/`

Developer/user documentation and diagrams. The canonical public baseline contains no `.psy` example songs.

The eight historical `.psy` demo/example files found in r12005 were omitted from public archival and canonical refs because their composition/sample redistribution permissions are unresolved; their exact upstream paths are recorded in `UPSTREAM_OMISSIONS.md`. Phase 4 will create or adopt cleared redistributable `.psy` fixtures for compatibility testing.

## Vendored and External Material

### `lua54/`

Vendored Lua 5.4.0 source.

### `zlib/`

Vendored zlib 1.2.11 source and upstream contribution directories.

### `external-packages/`

Historical third-party packages/tools, notably:

- retained Windows 7-Zip compiled help plus license/readme metadata (`7z.exe` and `7z.dll` are intentionally omitted);
- STK 4.5.0 source archive.

See [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md).

M3 and TinyBoxes provenance omissions are documented in [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md); those files are not present in the public baseline.

## Build-System and Platform Metadata

### `build-systems/`

Build/packaging support, including Windows installer material.

### Visual Studio project files

The root and many subdirectories contain historical Visual Studio solution/project files (`.sln`, `.vcxproj`, `.vcproj`, `.filters`, `.dsw`, `.dsp`).

They remain useful provenance and may help distinguish Windows-only assumptions, but the Linux port does not depend on them as its primary build system.

## Tests and Small Utilities

### `uitest/`

Small UI test area from upstream.

### `detail/`

Low-level/detail helpers used by the codebase.

## Baseline Artifact Notes

Phase 1 deliberately retains old build/platform artifacts when redistribution is permitted. Examples include compiled help and historical package metadata under `external-packages/`, installer assets and the Visual Studio `resource.aps` file. The upstream 7-Zip `7z.exe` and `7z.dll` binaries are intentionally omitted; see [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md).

They are **identified**, not cleaned up, in this phase. Deletion or replacement belongs in a later, evidence-driven change if it materially helps the Linux port.

## Maintainer Rule

When touching the imported tree, ask first:

> Is this change required to make existing Psycle behaviour work correctly on Linux?

If a source move, rename, framework migration or broad cleanup is not required to answer that question, it should not be mixed into an early porting fix.
