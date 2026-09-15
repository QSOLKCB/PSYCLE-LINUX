# PSYCLE-LINUX

**A compatibility-first Linux revival of Psycle Modular Music Studio.**

> **Port Psycle to Linux. Do not reinvent Psycle.**

PSYCLE-LINUX exists to bring the workflow and behaviour of the original Psycle Modular Music Studio to modern Linux while preserving old songs, native machines, plugin behaviour, tracker timing, routing, presets, and the character of the application.

This project is not a ground-up DAW redesign.

## Important Architecture Clarification

Direct clarification from long-time Psycle maintainer **JosepMa / JAZ** has made the upstream lineage more precise than the project originally understood it.

The relevant upstream trees have different roles:

- **`psycle/`** — the original Psycle application: C++, Microsoft Visual Studio and MFC, plus Windows-oriented SDKs and libraries. This is the primary behavioural and UI reference, but MFC makes it unsuitable as a direct Linux implementation base.
- **`psycle-core` + `psycle-audiodrivers` + `psycle-helpers` + `psycle-player` + `psycle-plugins`** — the first C++ reimplementation. It was buildable on Debian Linux and could play most songs, but was not fully playback-compatible and provided a player rather than the complete tracker application. This is now the leading candidate engine base for a faithful Linux Psycle.
- **`cpsycle/`** — the later C reimplementation with its own UI toolkit, tracker and event/sequencer architecture. It intentionally diverged in some areas and historically relied on Psycle-built plugins where its own plugin builds were incomplete.

That distinction changes the implementation strategy.

**C-Psycle is no longer treated as the literal implementation base for the final Linux Psycle.** The extensive work already completed against `cpsycle/` remains valuable as a preservation corpus, compatibility laboratory, behavioural oracle, Linux-port reference, and regression suite.

The next implementation step is therefore **not a UI rewrite**. It is a controlled compatibility audit of the `psycle-core` family against original Psycle behaviour, using the C-Psycle regressions where they provide useful independent evidence.

## Dedication

PSYCLE-LINUX is dedicated to the memory of **Juan Antonio Arguelles Rius (Arguru)** and to the developers, musicians, testers, plugin authors, and users who built the Psycle community around the project—including the community that gathered in `#Psycle` on IRC.

## Mission

The project now has three explicit source roles:

1. **Original Psycle is the compatibility reference.** Preserve its song behaviour, workflow, machine semantics and user-facing identity.
2. **`psycle-core` is the candidate Linux engine.** Audit it first, then close demonstrated compatibility gaps instead of rebuilding the engine from scratch.
3. **C-Psycle is a tested donor and oracle.** Reuse its Linux work, tests, architectural lessons and independently validated machine behaviour where that helps restore original-Psycle compatibility.

Modernization is allowed when it solves a real Linux, reliability, security, packaging or maintainability problem without casually changing historical behaviour.

## Work Already Completed

The current repository contains an audited SourceForge **C-Psycle r12005** baseline and a substantial preservation/test program around it.

Completed work includes:

- reproducible modern-Linux build auditing;
- native X11 host/runtime smoke testing;
- ALSA, ALSA MIDI, JACK, SDL2 and Linux input-driver build work;
- tracker editing, Machine View, sequencer/transport and preset workflows;
- PSY2/PSY3 save/reload and state-persistence tests;
- WAV render → Sampler round-trip testing;
- broad native-machine preservation across Arguru, Pooplog, Druttis, JM/JME, STK and other retained machines;
- source-derived DSP/state/timing regression oracles;
- FluidSynth SF2 Player preservation;
- explicit VST2 licensing/provenance boundaries;
- startup-safety planning for future third-party plugin scanning.

None of that work is discarded by the architecture correction. It becomes the compatibility evidence used to evaluate and harden the next engine path.

## Upstream Architectural Evidence

The retained C-Psycle documentation includes the **Psycle Developer Guide — C-Version, Feb 2021 (unfinished)** at:

- [`cpsycle/doc/cpsycle-developer-guide.txt`](cpsycle/doc/cpsycle-developer-guide.txt)

It documents C-Psycle's own architecture, including the audio/UI split, platform bridge, 256-sample Psycle-plugin compatibility chunks, variable VST process intervals and the historical 64-channel native-plugin limit.

See [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md) for how that guide is used. Because the guide is explicitly unfinished, demonstrated source behaviour remains authoritative for claims about C-Psycle, while original Psycle remains the compatibility target for the final Linux product.

## Next Implementation Milestone

The next phase is a **`psycle-core` compatibility and provenance audit**:

1. identify and pin the exact upstream revisions of `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player` and `psycle-plugins` that should be evaluated;
2. establish their licensing/provenance before importing or adapting code;
3. reproduce the historical Debian/Linux player build;
4. compare song loading, playback, timing, sampler behaviour, machine/plugin state, routing and rendering against original Psycle expectations;
5. reuse existing C-Psycle regression tests where they test shared compatibility contracts;
6. produce a concrete parity-gap matrix before writing a new tracker UI.

Only after the engine is sufficiently compatible should the project implement the full Linux tracker UI. **Qt is the leading candidate** because the original MFC UI cannot be carried directly to Linux; Qt Widgets should be evaluated first for faithful desktop behaviour, with QML remaining an option where it provides a demonstrated advantage.

## Plugin Hosting

Third-party plugins were an important part of real Psycle usage, so VST2 remains a compatibility target rather than something to remove merely because it is old.

The public repository intentionally excludes the historical Steinberg-derived:

- `aeffect.h`
- `aeffectx.h`
- `vstfxstore.h`

The plan is to independently implement only the VST2 ABI surface actually required by Psycle, without copying Steinberg SDK source expressions. The existing Psycle-owned host code and our current compatibility research remain useful donors.

Plugin discovery must also be safer than the historical all-in-process model: a bad native plugin or VST should not be able to hang or crash Psycle simply because it is present at startup **or because a song attempts to instantiate it and restore opaque state**. Planned Phase 9 work therefore includes metadata caching, incremental rescans, out-of-process probing, timeouts, crash quarantine, isolated song-load instantiation/state restoration, and recoverable placeholders for missing or failing plugins.

## Porting Principles

- **Original Psycle behaviour is the target.** Reimplementations are donors, not automatic authorities.
- **Measure before rewriting.** Build a parity matrix before changing engines or UI architecture.
- **Preserve the workflow.** Tracker editing, Machine View, routing, instruments, samples, presets and `.psy` compatibility are core identity.
- **Reuse proven work.** C-Psycle tests and Linux code should be reused where they accurately test or implement shared behaviour.
- **Patch before replace.** Prefer narrow compatibility fixes to speculative rewrites.
- **Contain plugin failure.** Optional or third-party plugins must not make startup or song loading fragile.
- **Keep provenance explicit.** Upstream authorship, licensing boundaries and donor relationships must remain auditable.

See [PORTING.md](PORTING.md) for the project's engineering rules and [ROADMAP.md](ROADMAP.md) for the implementation plan. Those documents use the same original-Psycle → `psycle-core` → C-Psycle evidence hierarchy.

## Current Repository Baseline

The currently imported and audited source baseline remains:

- Source: Psycle / C-Psycle SourceForge repository
- Revision: `r12005`
- Snapshot: `r12005-trunk-cpsycle`
- Archive SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Imported source: [`cpsycle/`](cpsycle/)

This baseline is retained as a preservation/reference asset. It is **not** being relabelled as the original Psycle implementation.

The `psycle-core` family is not yet imported into the current repository; its exact upstream source identity and licensing/provenance will be established before implementation work begins.

See [PROVENANCE.md](PROVENANCE.md), [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md), [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md), [SOURCE_TREE.md](SOURCE_TREE.md), and [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md).

## Licensing

The PSYCLE-LINUX repository scaffolding and original project material are provided under the **Apache License 2.0** as stated in [LICENSE](LICENSE), unless a file or directory states otherwise.

Imported Psycle/C-Psycle source retains its own upstream licensing and component-specific notices. Importing upstream source does **not** relicense it under Apache-2.0.

Any future `psycle-core` import must receive the same provenance and licensing audit before becoming part of the public repository.

See [LICENSING.md](LICENSING.md) for project policy.

## Contributing

Contributions are welcome, especially from people familiar with original Psycle behaviour, old `.psy` songs, `psycle-core`, tracker workflows, Linux audio, native machines, VST hosting, or the historical project.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening implementation PRs.

## Credits

Psycle exists because of a long-running community effort. PSYCLE-LINUX does not claim authorship of that history.

Special thanks to **JosepMa / JAZ** for directly clarifying the relationship between original Psycle, the C++ `psycle-core` reimplementation and the later C-Psycle reimplementation.

See [CREDITS.md](CREDITS.md) and the imported upstream [`cpsycle/AUTHORS`](cpsycle/AUTHORS) file.