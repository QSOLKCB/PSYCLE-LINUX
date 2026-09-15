# PSYCLE-LINUX

**A compatibility-first Linux revival of Psycle Modular Music Studio.**

> **Port Psycle to Linux. Do not reinvent Psycle.**

PSYCLE-LINUX exists to bring the workflow and behaviour of the original Psycle Modular Music Studio to modern Linux while preserving old songs, native machines, plugin behaviour, tracker timing, routing, presets, and the character of the application.

This project is not a ground-up DAW redesign.

## Important Architecture Clarification

Direct clarification from long-time Psycle maintainer **JosepMa / JAZ** has made the upstream lineage more precise than the project originally understood it.

The relevant upstream trees have different roles:

- **`psycle/`** — the original Psycle application: C++, Microsoft Visual Studio and MFC, plus Windows-oriented SDKs and libraries. This is the primary behavioural and UI reference, but MFC makes it unsuitable as a direct Linux implementation base.
- **`universalis` + `psycle-core` + `psycle-audiodrivers` + `psycle-helpers` + `psycle-player` + `psycle-plugins`** — the historical C++ cross-platform build-source family used by the earlier reimplementation/player path. It was buildable on Debian Linux and could play most songs, but was not fully playback-compatible and provided a player rather than the complete tracker application. This is now the leading candidate engine base for a faithful Linux Psycle.
- **`cpsycle/`** — the later C reimplementation with its own UI toolkit, tracker and event/sequencer architecture. It intentionally diverged in some areas and historically relied on Psycle-built plugins where its own plugin builds were incomplete.

That distinction changes the implementation strategy.

**C-Psycle is no longer treated as the literal implementation base for the final Linux Psycle.** The extensive work already completed against `cpsycle/` remains valuable as a preservation corpus, compatibility laboratory, behavioural oracle, Linux-port reference, and regression suite.

Phase 6A has now pinned the original-Psycle reference and the complete six-component C++ build-source snapshot. The next implementation step is **Phase 6B: materialize a provenance-safe sanitized C++ baseline and reproduce the historical Linux `psycle-player` build** before any engine convergence or Qt UI work.

## Dedication

PSYCLE-LINUX is dedicated to the memory of **Juan Antonio Arguelles Rius (Arguru)** and to the developers, musicians, testers, plugin authors, and users who built the Psycle community around the project—including the community that gathered in `#Psycle` on IRC.

## Mission

The project now has three explicit source roles:

1. **Original Psycle is the compatibility reference.** Preserve its song behaviour, workflow, machine semantics and user-facing identity.
2. **The pinned C++ build-source family is the candidate Linux engine path.** Import it conservatively, reproduce the player build, then close demonstrated compatibility gaps instead of rebuilding the engine from scratch.
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
- startup-safety planning for future third-party plugin scanning;
- Phase 6A provenance pinning of Psycle 1.12.0 x86 and the six-component r12005 C++ build-source snapshot, with an explicit sanitized-import boundary.

None of that work is discarded by the architecture correction. It becomes the compatibility evidence used to evaluate and harden the next engine path.

## Upstream Architectural Evidence

The retained C-Psycle documentation includes the **Psycle Developer Guide — C-Version, Feb 2021 (unfinished)** at:

- [`cpsycle/doc/cpsycle-developer-guide.txt`](cpsycle/doc/cpsycle-developer-guide.txt)

It documents C-Psycle's own architecture, including the audio/UI split, platform bridge, 256-sample Psycle-plugin compatibility chunks, variable VST process intervals and the historical 64-channel native-plugin limit.

See [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md) for how that guide is used. Because the guide is explicitly unfinished, demonstrated source behaviour remains authoritative for claims about C-Psycle, while original Psycle remains the compatibility target for the final Linux product.

## Next Implementation Milestone

**Phase 6A is complete.** The project has pinned:

- **Psycle 1.12.0 x86** as the primary original-Psycle behavioural reference, including its official SourceForge origin, exact size and SHA-256;
- the complete SourceForge SVN **r12005** C++ build-source set: `universalis`, `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, and `psycle-plugins`;
- the first public-import exclusions/quarantines and provenance requirements.

The active milestone is now **Phase 6B — sanitized import and historical Linux player build**:

1. materialize the exact retained-file list and omission/replacement arithmetic from the frozen six-component manifests;
2. preserve or restore complete compatible third-party permission/provenance notices for retained helper/API code;
3. create separate sanitized archival/canonical baseline identities for the C++ family;
4. import only the provenance-cleared source needed for the first engine/player build, keeping quarantined VST/ASIO/binary/song material out;
5. reproduce the historical Debian/Linux `psycle-player` build and record narrow compiler/linker/runtime blockers;
6. establish deterministic/headless playback where practical and then begin moving parity-matrix rows out of `UNKNOWN` with reproducible evidence.

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

See [PORTING.md](PORTING.md) for the project's engineering rules and [ROADMAP.md](ROADMAP.md) for the implementation plan. Those documents use the same original-Psycle → C++ candidate → C-Psycle evidence hierarchy.

## Current Repository Baseline

The currently imported and audited source baseline remains:

- Source: Psycle / C-Psycle SourceForge repository
- Revision: `r12005`
- Snapshot: `r12005-trunk-cpsycle`
- Archive SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Imported source: [`cpsycle/`](cpsycle/)

This baseline is retained as a preservation/reference asset. It is **not** being relabelled as the original Psycle implementation.

The C++ candidate family is still **not imported** into the repository. Phase 6A has already established its exact six-component r12005 identities and first licensing/provenance boundary; Phase 6B must now materialize the sanitized retained-file set before any public source import.

See [PHASE6_REFERENCE_PROVENANCE.md](PHASE6_REFERENCE_PROVENANCE.md), [PHASE6_CPP_IMPORT_AUDIT.md](PHASE6_CPP_IMPORT_AUDIT.md), [PSYCLE_CORE_PARITY.md](PSYCLE_CORE_PARITY.md), [PROVENANCE.md](PROVENANCE.md), [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md), [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md), [SOURCE_TREE.md](SOURCE_TREE.md), and [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md).

## Licensing

The PSYCLE-LINUX repository scaffolding and original project material are provided under the **Apache License 2.0** as stated in [LICENSE](LICENSE), unless a file or directory states otherwise.

Imported Psycle/C-Psycle source retains its own upstream licensing and component-specific notices. Importing upstream source does **not** relicense it under Apache-2.0.

The pending C++ import must follow the Phase 6A provenance boundary: retain cleared upstream notices, restore complete compatible third-party notices where necessary, and keep quarantined/restricted material out of the public sanitized baseline.

See [LICENSING.md](LICENSING.md) for project policy.

## Contributing

Contributions are welcome, especially from people familiar with original Psycle behaviour, old `.psy` songs, the C++ `psycle-core` lineage, tracker workflows, Linux audio, native machines, VST hosting, or the historical project.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening implementation PRs.

## Credits

Psycle exists because of a long-running community effort. PSYCLE-LINUX does not claim authorship of that history.

Special thanks to **JosepMa / JAZ** for directly clarifying the relationship between original Psycle, the C++ `psycle-core` reimplementation and the later C-Psycle reimplementation.

See [CREDITS.md](CREDITS.md) and the imported upstream [`cpsycle/AUTHORS`](cpsycle/AUTHORS) file.
