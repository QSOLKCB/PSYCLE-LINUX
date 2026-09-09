# Contributing to PSYCLE-LINUX

Thanks for helping bring Psycle to modern Linux.

The project has one rule above all others:

> **Port Psycle to Linux. Do not reinvent Psycle.**

Please read [PORTING.md](PORTING.md), [ROADMAP.md](ROADMAP.md), [PROVENANCE.md](PROVENANCE.md), and [LICENSING.md](LICENSING.md) before proposing large changes.

## What We Need Most

Useful contributions include:

- modern GCC/Clang compatibility fixes;
- Linux build fixes;
- X11 UI fixes;
- ALSA, JACK, ALSA MIDI, and SDL2 backend fixes;
- x86-64 portability fixes;
- `.psy` loading/saving regression tests;
- native-machine build and behaviour fixes;
- plugin discovery and LV2/Lilv work;
- Linux filesystem/path integration;
- deterministic test songs and audio fixtures that can legally be redistributed;
- packaging and CI once the baseline build is stable;
- historical documentation and reproducible bug reports.

## Before Opening a Pull Request

For functional changes, include:

1. the Linux distribution and version used;
2. CPU architecture;
3. compiler and version;
4. relevant audio/MIDI backend;
5. a concise description of the upstream failure;
6. the reason for the chosen fix;
7. commands used to build/test;
8. what Psycle behaviour was intentionally preserved;
9. any known compatibility difference that remains.

For machine or DSP changes, also state whether the change alters audible behaviour or only fixes compilation/platform correctness.

## Keep PRs Narrow

Good examples:

- `build: fix Xft detection on Ubuntu`
- `alsa: repair device enumeration on x86-64`
- `x11: fix host shutdown event handling`
- `goaslicer: fix compiler error without changing DSP`
- `songio: add regression fixture for legacy .psy load`

Poor early-port examples:

- "rewrite all UI code in a new toolkit";
- "replace the build system and refactor every directory";
- "modernize all C and C++ at once";
- "rename all old types and files";
- "add three new plugin APIs before the host produces audio."

Large work is welcome when necessary, but the necessity should be demonstrated first.

## Do Not Mix Cleanup with Porting Fixes

Avoid mass formatting, broad renames, comment rewrites, or stylistic churn in the same PR as a compatibility fix.

Small diffs make it much easier to determine whether a change altered Psycle behaviour.

## Upstream Attribution

Do not remove or rewrite upstream authorship, copyright, license, or attribution notices unless correcting them against authoritative upstream evidence.

When introducing third-party code or assets, identify:

- source;
- author/project;
- version or revision;
- license;
- whether the material is modified;
- whether redistribution is permitted.

See [LICENSING.md](LICENSING.md).

## Bug Reports

A useful bug report should include, where relevant:

```text
Distribution:
Kernel:
Architecture:
Desktop/session: X11 / Wayland / XWayland
Compiler:
Audio backend: ALSA / JACK / SDL2 / other
MIDI backend:
Psycle build/commit:
Song or machine involved:
Steps to reproduce:
Expected behaviour:
Actual behaviour:
Console output/backtrace:
```

Do not upload copyrighted commercial samples, plugins, presets, or songs unless you have permission to redistribute them.

## Compatibility Evidence

The best evidence is reproducible:

- a minimal `.psy` fixture;
- a tiny generated sample;
- a deterministic machine graph;
- an audio render with documented tolerance;
- a crash backtrace;
- before/after build output;
- comparison against a known historical Psycle version.

Where exact output cannot be reproduced across architectures, document the expected tolerance or behavioural invariant.

## Native Machines

Treat historical native machines carefully. Their behaviour is part of old Psycle songs.

For DSP changes:

- prefer platform/compiler fixes that leave equations unchanged;
- isolate intentional DSP corrections in separate commits;
- test preset/state serialization;
- test song reload;
- state clearly if output changes audibly.

The Arguru machines are explicit preservation targets and should not be casually redesigned or renamed.

## Commit Messages

Prefer concise messages describing the subsystem and reason, for example:

```text
x11: fix 64-bit window handle conversion
jack: handle missing server without aborting host
songio: preserve machine state on legacy .psy reload
goaslicer: fix modern C++ compile failure
```

## Review Standard

A change is ready when reviewers can understand:

- what was broken;
- why it was broken;
- why the patch is appropriately scoped;
- how it was tested;
- whether compatibility changed.

The aim is not to make old code look new. The aim is to make **Psycle work on Linux** and keep it working.
