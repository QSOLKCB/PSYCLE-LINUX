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

Phase 6B is complete: the provenance-cleared SourceForge SVN r12005 C++ candidate has been imported as `psycle-cpp-r12005-sanitized/`, frozen by a reproducible baseline identity, and its historical Linux `psycle-player` build is maintained in CI. Phase 6C is collecting evidence against the pinned Psycle 1.12.0 x86 reference, while Phase 6D keeps the human parity report mechanically synchronized with the canonical compatibility matrix. The current 20-contract matrix contains three scoped `PASS` results, one scoped serialization/save `MISSING` result, one scoped BPM/LPB/tick `DIFFERENT` result, and 15 `UNKNOWN` contracts. Those two confirmed gaps justify exactly two narrow Phase 7 convergence backlog items; broader engine changes remain evidence-gated.

## Dedication

PSYCLE-LINUX is dedicated to the memory of **Juan Antonio Arguelles Rius (Arguru)** and to the developers, musicians, testers, plugin authors, and users who built the Psycle community around the project—including the community that gathered in `#Psycle` on IRC.

## Mission

The project now has three explicit source roles:

1. **Original Psycle is the compatibility reference.** Preserve its song behaviour, workflow, machine semantics and user-facing identity.
2. **The pinned C++ build-source family is the candidate Linux engine path.** Measure the sanitized imported candidate against original Psycle, then close demonstrated compatibility gaps instead of rebuilding the engine from scratch.
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
- Phase 6A provenance pinning of Psycle 1.12.0 x86 and the six-component r12005 C++ build-source snapshot;
- Phase 6B sanitized C++ import, frozen baseline identity, historical qmake build reproduction, and maintained `psycle-player` CI;
- Phase 6C machine-readable three-way compatibility matrix, strict evidence gates, and reusable PSY2/PSY3 candidate receipts;
- Phase 6D machine validation that the human-readable parity report remains aligned with the canonical matrix.

None of that work is discarded by the architecture correction. It becomes the compatibility evidence used to evaluate and harden the candidate engine path.

## Upstream Architectural Evidence

The retained C-Psycle documentation includes the **Psycle Developer Guide — C-Version, Feb 2021 (unfinished)** at:

- [`cpsycle/doc/cpsycle-developer-guide.txt`](cpsycle/doc/cpsycle-developer-guide.txt)

It documents C-Psycle's own architecture, including the audio/UI split, platform bridge, 256-sample Psycle-plugin compatibility chunks, variable VST process intervals and the historical 64-channel native-plugin limit.

See [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md) for how that guide is used. Because the guide is explicitly unfinished, demonstrated source behaviour remains authoritative for claims about C-Psycle, while original Psycle remains the compatibility target for the final Linux product.

## Next Implementation Milestone

**Phase 6B is complete.** The sanitized C++ candidate is imported and buildable. The active evidence work is now **Phase 6C / 6D — compatibility observations and parity reporting**:

1. preserve the frozen Psycle 1.12.0 x86 and sanitized r12005 candidate identities;
2. collect versioned original-Psycle observations in the accepted environment for the same reproducible fixtures/procedures used by the candidate;
3. keep every compatibility row `UNKNOWN` until both observation sides and a versioned comparison verdict exist;
4. expand the evidence set from PSY2/PSY3 into timing, tracker commands, routing, Sampler/XMSampler, native state, WAV/sample handling, render/bounce, recovery, and legally usable historical songs;
5. keep `PSYCLE_CORE_PARITY.md` mechanically synchronized with the canonical matrix;
6. preserve the frozen two-item Phase 7 backlog derived from the confirmed serialization/save `MISSING` and BPM/LPB/tick `DIFFERENT` results;
7. continue the delayed/retrigger diagnostic lane without promoting it from `UNKNOWN` until a like-for-like original runtime output and candidate comparison exist.

The current classified scope is three `PASS` contracts (PSY2 parse/load, PSY3 parse/load, and single-sequence order), one serialization/save `MISSING` contract, and one BPM/LPB/tick `DIFFERENT` contract; 15 contracts remain `UNKNOWN`. The first Phase 7 backlog items are therefore the exact-fixture PSY3 save capability and a distinct legacy LPB/TPB timing model. The delayed/retrigger Sampler diagnostics remain evidence work: the one-row delayed witness excludes normal `controller.Work` sample processing as a prerequisite for the observed crash, but voice selection/setup, `Voice::Tick` initialization, and pre-`controller.Work` `Voice::Work` entry remain unresolved.

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

The original audited C-Psycle preservation baseline remains:

- Source: Psycle / C-Psycle SourceForge repository
- Revision: `r12005`
- Snapshot: `r12005-trunk-cpsycle`
- Archive SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Imported source: [`cpsycle/`](cpsycle/)

This baseline is retained as a preservation/reference asset. It is **not** being relabelled as the original Psycle implementation.

The C++ candidate family is now imported separately under [`psycle-cpp-r12005-sanitized/`](psycle-cpp-r12005-sanitized/) using the Phase 6A/6B provenance boundary. Its frozen Phase 6B baseline SHA-256 is `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`. The original Psycle 1.12.0 executable remains a non-redistributed behavioural reference.

See [PHASE6_REFERENCE_PROVENANCE.md](PHASE6_REFERENCE_PROVENANCE.md), [PHASE6_CPP_IMPORT_AUDIT.md](PHASE6_CPP_IMPORT_AUDIT.md), [PSYCLE_CORE_PARITY.md](PSYCLE_CORE_PARITY.md), [PROVENANCE.md](PROVENANCE.md), [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md), [THIRD_PARTY_INVENTORY.md](THIRD_PARTY_INVENTORY.md), [SOURCE_TREE.md](SOURCE_TREE.md), and [UPSTREAM_ARCHITECTURE.md](UPSTREAM_ARCHITECTURE.md).

## Licensing

The PSYCLE-LINUX repository scaffolding and original project material are provided under the **Apache License 2.0** as stated in [LICENSE](LICENSE), unless a file or directory states otherwise.

Imported Psycle/C-Psycle source retains its own upstream licensing and component-specific notices. Importing upstream source does **not** relicense it under Apache-2.0.

The sanitized C++ candidate remains governed by the Phase 6A/6B provenance boundary: retained upstream notices stay intact, compatible third-party notices are preserved/restored where required, and quarantined/restricted material remains outside the public baseline.

See [LICENSING.md](LICENSING.md) for project policy.

## Contributing

Contributions are welcome, especially from people familiar with original Psycle behaviour, old `.psy` songs, the C++ `psycle-core` lineage, tracker workflows, Linux audio, native machines, VST hosting, or the historical project.

Please read [CONTRIBUTING.md](CONTRIBUTING.md) before opening implementation PRs.

## Credits

Psycle exists because of a long-running community effort. PSYCLE-LINUX does not claim authorship of that history.

Special thanks to **JosepMa / JAZ** for directly clarifying the relationship between original Psycle, the C++ `psycle-core` reimplementation and the later C-Psycle reimplementation.

See [CREDITS.md](CREDITS.md) and the imported upstream [`cpsycle/AUTHORS`](cpsycle/AUTHORS) file.
