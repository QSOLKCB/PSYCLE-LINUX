# PSYCLE-LINUX

**A modern Linux revival of Psycle Modular Music Studio.**

> **Port Psycle to Linux. Do not reinvent Psycle.**

PSYCLE-LINUX exists to bring Psycle's tracker workflow, modular Machine View, native instruments and effects, song format, and distinctive way of working to modern Linux systems while preserving the character of the original application.

This is a compatibility-first port and preservation effort, not a ground-up DAW redesign.

## Dedication

PSYCLE-LINUX is dedicated to the memory of **Juan Antonio Arguelles Rius (Arguru)** and to the developers, musicians, testers, plugin authors, and users who built the Psycle community around the project—including the community that gathered in `#Psycle` on IRC.

Arguru's native machines remain part of the source lineage we intend to preserve and validate, including:

- Arguru Compressor
- Arguru Distortion
- Arguru Goaslicer
- Arguru Reverb
- Arguru Synth 2f
- Arguru XFilter

## Mission

The goal is straightforward:

1. preserve an auditable upstream C-Psycle baseline;
2. make that code build cleanly on current Linux distributions;
3. restore a dependable native Linux Psycle workflow;
4. preserve compatibility with existing Psycle songs and native machines wherever technically possible;
5. modernize only where Linux compatibility, maintainability, security, packaging, or hardware support requires it.

## What We Already Have

The selected upstream baseline is the SourceForge **C-Psycle r12005 trunk snapshot**. It is not merely Windows source with a few Linux conditionals. The tree already contains substantial cross-platform work, including:

- an X11 UI implementation alongside Win32 abstractions;
- ALSA audio support;
- ALSA MIDI support;
- JACK audio support;
- SDL2 audio support;
- Linux event joystick support;
- Lua-based UI/script infrastructure;
- Lilv/LV2-related integration points;
- a standalone `psyplayer`;
- native Psycle plugins and presets;
- Psycle documentation; the pinned r12005 source identifies historical `.psy` examples, but those files are absent from all public repository refs until their redistribution permissions are established.

That existing work is the foundation. We will extend and repair it rather than replacing working architecture without a demonstrated need.

## Porting Principles

- **Compatibility before redesign.** Existing Psycle behaviour is the reference unless it is unsafe, broken, or impossible on modern Linux.
- **Preserve the workflow.** Machine View, tracker patterns, sequencer behaviour, native machines, routing, presets, and `.psy` files are core identity—not legacy clutter.
- **Patch before rewrite.** Prefer small, reviewable compatibility fixes over large framework migrations.
- **No gratuitous technology swaps.** A new toolkit, plugin API, audio layer, build system, or language must solve a real porting problem before it is adopted.
- **Modern Linux support should be additive.** PipeWire, desktop integration, packaging, VST3, CLAP, or other future work must not become an excuse to discard existing Psycle functionality.
- **Provenance matters.** Upstream authorship, copyright notices, source history, and third-party licensing must remain traceable.

See [PORTING.md](PORTING.md) for the engineering rules used by the project.

## Project Status

**Phase 2 — Modern Linux Build Audit complete.**

The audited r12005 baseline has now been exercised on Ubuntu 24.04 x86-64 with GCC 13.3 using the existing make-based architecture.

What already builds on the reference runner:

- X11/Xft UI layer;
- thread library;
- script/Lua layer;
- file library;
- Lua UI library.

The first blocking failures are documented rather than hidden: a container linkage conflict, a DSP signed/unsigned size-type mismatch, an audio player declaration mismatch, and the expected need to make legacy VST2 compilation conditional because the SDK-derived headers are intentionally absent from the public baseline.

Linux ALSA, ALSA MIDI, JACK, SDL2 and event-joystick driver source reaches linking; their remaining audit failures are downstream core-library/output-path prerequisites rather than missing Linux API headers.

See [PHASE2_BUILD_AUDIT.md](PHASE2_BUILD_AUDIT.md) for the evidence and [BUILDING.md](BUILDING.md) for reproducible development commands.

The next milestone is **Phase 3 — First Native Linux Host**: make the smallest compatibility fixes necessary to clear the documented blockers, build `psyplayer`, build the existing X11 host, then launch Psycle and validate real ALSA/JACK/MIDI behaviour.

See [ROADMAP.md](ROADMAP.md) for the milestone plan.

## Upstream Baseline

Initial source baseline:

- Source: Psycle / C-Psycle SourceForge repository
- Revision: `r12005`
- Snapshot: `r12005-trunk-cpsycle`
- Archive SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Imported source: [`cpsycle/`](cpsycle/)

See [PROVENANCE.md](PROVENANCE.md) for the exact import record, [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md) for material that was not mirrored, [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md) for bundled components and licenses, and [SOURCE_TREE.md](SOURCE_TREE.md) for the maintainer-oriented tree map.

## Licensing

The PSYCLE-LINUX repository scaffolding and original project material are provided under the **Apache License 2.0** as stated in [LICENSE](LICENSE), unless a file or directory states otherwise.

The imported C-Psycle r12005 source contains its own **GNU GPL version 2** licensing material and component-specific third-party notices. Importing upstream source does **not** relicense that source under Apache-2.0.

See [LICENSING.md](LICENSING.md) for the project policy.

## Contributing

Contributions are welcome, especially from people familiar with Psycle, tracker workflows, Linux audio, old `.psy` songs, native machines, plugin hosting, or the historical project.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening implementation PRs.

## Credits

Psycle exists because of a long-running community effort. PSYCLE-LINUX does not claim authorship of that history.

See [CREDITS.md](CREDITS.md) and the imported upstream [`cpsycle/AUTHORS`](cpsycle/AUTHORS) file.
