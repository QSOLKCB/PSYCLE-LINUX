# Contributing to PSYCLE-LINUX

Thanks for helping bring Psycle to modern Linux.

The project has one rule above all others:

> **Port Psycle to Linux. Do not reinvent Psycle.**

For the active implementation track, that means preserving the **behaviour and workflow of original Psycle** while using the existing cross-platform reimplementations as evidence and donors rather than assuming any one of them is automatically authoritative.

Please read [PORTING.md](PORTING.md), [ROADMAP.md](ROADMAP.md), [PROVENANCE.md](PROVENANCE.md), and [LICENSING.md](LICENSING.md) before proposing large changes.

## Current Architecture and Priority

The repository now distinguishes three upstream roles:

- **original `psycle/`** — behavioural and UI reference;
- **`psycle-core` family** — candidate cross-platform/Linux engine to audit and converge;
- **`cpsycle/`** — preserved C reimplementation, Linux donor, compatibility laboratory, and regression corpus.

The current repository imports only the audited C-Psycle r12005 baseline. The `psycle-core` family and any original-Psycle reference material must be provenance-audited and pinned before being imported or used as reproducible release evidence.

### What We Need Most Now

The active priority is **Phase 6 — `psycle-core` Provenance and Parity Audit**. Useful contributions include:

- identifying a reproducible, version-pinned **original Psycle reference** and documenting its provenance, hash/version identity, runtime environment, and observation method;
- identifying coherent SourceForge revisions for `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, and `psycle-plugins`;
- licensing/provenance review for those sources before import;
- reproducing the historical Debian/Linux `psycle-player` build;
- modern GCC/Clang compatibility fixes required by that build, once the source is pinned;
- deterministic parity fixtures for `.psy` loading, timing, sampler behaviour, mixer/routing, plugin state, MIDI, and rendering;
- adapting the existing C-Psycle regression corpus where it tests a genuinely shared compatibility contract;
- establishing active-track GitHub Actions CI so `psycle-core` build/parity regressions gate later work;
- legally redistributable historical songs, samples, presets, and reference fixtures with clear provenance;
- historical documentation and reproducible behaviour reports that identify the exact Psycle version/build observed.

Later roadmap phases also welcome focused work when their prerequisites are met:

- **Phase 7:** evidence-driven engine parity fixes;
- **Phase 8:** a faithful Linux tracker UI, with Qt Widgets currently the leading toolkit candidate;
- **Phase 9:** clean-room VST2 interoperability, independent real-plugin validation, and crash/hang-contained plugin discovery/song loading;
- **Phase 10:** real ALSA/JACK/PipeWire-JACK/MIDI acceptance and packaging.

C-Psycle fixes are still welcome when they preserve the existing regression corpus, correct a demonstrable compatibility oracle, or provide reusable Linux evidence, but they are no longer the default implementation direction for the final application.

## Before Opening a Pull Request

For functional changes, include:

1. the subsystem and upstream tree being changed (`psycle-core`, C-Psycle donor code, Qt host, plugin host, etc.);
2. the exact source revision/commit being tested;
3. the Linux distribution and version used;
4. CPU architecture;
5. compiler and version;
6. relevant audio/MIDI/plugin environment;
7. a concise description of the observed failure or parity gap;
8. the evidence establishing expected original-Psycle behaviour, including the exact reference version/build when applicable;
9. the reason for the chosen fix;
10. commands used to build/test locally and in CI;
11. what Psycle behaviour was intentionally preserved;
12. any known compatibility difference that remains.

For machine or DSP changes, also state whether the change alters audible behaviour or only fixes compilation/platform correctness.

For historical executable/reference evidence, do not upload or redistribute binaries unless the repository has a clear legal basis to do so. Record origin, version, checksum, environment, and observation procedure instead when redistribution is not established.

## Keep PRs Narrow

Good examples for the active track:

- `phase6: pin psycle-core donor revisions and provenance`
- `player: fix GCC build without changing playback semantics`
- `sampler: add original-Psycle timing parity fixture`
- `songio: preserve machine state on legacy .psy reload`
- `ci: run psycle-core parity smoke on Ubuntu`
- `qt: add tracker keyboard-focus proof after engine parity`
- `vst2: add ABI layout assertions and independent plugin fixture`

Poor proposals include:

- importing an unpinned upstream tree and immediately modifying it;
- starting the full Qt tracker before the engine parity audit identifies the required contracts;
- replacing the engine instead of measuring the existing `psycle-core` implementation;
- replacing the build system and refactoring every directory in the same PR;
- modernizing all C/C++ at once;
- adding plugin formats before the host lifecycle and failure-containment boundaries are testable;
- claiming parity from a test where both sides are compiled against the same project-authored ABI definitions.

Large work is welcome when necessary, but the necessity and prerequisites should be demonstrated first.

## Do Not Mix Cleanup with Compatibility Fixes

Avoid mass formatting, broad renames, comment rewrites, or stylistic churn in the same PR as a compatibility fix.

Small diffs make it much easier to determine whether a change altered Psycle behaviour.

## Upstream Attribution and Provenance

Do not remove or rewrite upstream authorship, copyright, license, or attribution notices unless correcting them against authoritative upstream evidence.

When introducing third-party or historical code, binaries, assets, or reference material, identify:

- source/origin;
- author/project;
- exact version, revision, tag, commit, or build identity;
- checksum when practical;
- license and redistribution basis;
- whether the material is modified;
- whether it is committed to the repository or used only as an external observation/reference;
- the runtime/build environment needed to reproduce the observation.

See [LICENSING.md](LICENSING.md) and [PROVENANCE.md](PROVENANCE.md).

## Bug Reports and Parity Reports

A useful report should include, where relevant:

```text
Subsystem/upstream tree:
Source commit/revision:
Reference Psycle version/build:
Reference origin/hash:
Reference OS/runtime:
Linux distribution:
Kernel:
Architecture:
Desktop/session:
Compiler:
Audio backend:
MIDI backend:
Plugin format/plugin identity:
Song or machine involved:
Steps to reproduce:
Expected behaviour:
Actual behaviour:
Console output/backtrace:
Generated receipt/render/hash:
```

Do not upload copyrighted commercial samples, plugins, presets, songs, or proprietary SDK material unless you have permission to redistribute them.

## Compatibility Evidence

The best evidence is reproducible and version-specific:

- a pinned original-Psycle source revision or identified executable build observed in a documented environment;
- a minimal `.psy` fixture;
- a tiny generated sample;
- a deterministic machine graph;
- an audio render with documented tolerance;
- a crash backtrace;
- before/after build output;
- a state/preset round-trip receipt;
- comparison against a specifically identified historical Psycle version.

A statement such as “Psycle did X” is not enough for a parity gate when different releases may behave differently. Record which version/build supplied the observation.

Where exact output cannot be reproduced across architectures, document the expected tolerance or behavioural invariant.

## CI Is Part of the Compatibility Contract

New active-track regressions must not remain local-only tests.

Once the `psycle-core` family is imported, PRs that change the active engine must run the relevant build and parity tests in GitHub Actions. As later phases become active, their critical tests must join the merge-gating suite as well, including:

- `psycle-core` build/player/parity tests;
- Qt host build and headless interaction smoke where practical;
- plugin ABI, independent real-plugin processing, scanner timeout/crash, song-load containment, and state-restore regressions.

A regression test that is not executed by the maintained CI path does not protect the project from future merges.

## Native Machines

Treat historical native machines carefully. Their behaviour is part of old Psycle songs.

For DSP changes:

- prefer platform/compiler fixes that leave equations unchanged;
- isolate intentional DSP corrections in separate commits;
- test preset/state serialization;
- test song reload;
- state clearly if output changes audibly;
- distinguish original-Psycle behaviour from C-Psycle-only behaviour when the implementations differ.

The completed C-Psycle machine-preservation corpus remains valuable evidence, but final compatibility decisions must follow the evidence hierarchy in [ROADMAP.md](ROADMAP.md).

## UI Contributions

Qt is **not** prohibited by the project policy. It is the leading planned toolkit for the future Linux tracker because the original MFC UI is not directly portable.

However, the roadmap deliberately schedules the full tracker UI after the engine parity audit and convergence work. Early Qt work should therefore be limited to proofs or infrastructure that do not invent engine behaviour the audit has not established.

Preserve Psycle's interaction model—tracker keyboard workflow, Machine View, parameter editing, instruments/samples, mixer, and compact desktop-workstation character—rather than redesigning it as a generic modern DAW.

## Plugin Contributions

VST2 remains a historical compatibility target, but proprietary Steinberg SDK material must not be casually reintroduced.

Phase 9 requires both:

- project-authored ABI fixtures/assertions; and
- at least one **independently maintained free/open Linux VST2 plugin** built or obtained from a pinned, provenance-documented source that does not use PSYCLE-LINUX's compatibility headers.

The independent plugin test must exercise real audio processing in addition to applicable MIDI/parameter/state/song-reopen behaviour. Scanner and song-load tests must also prove crash/hang containment.

## Commit Messages

Prefer concise messages describing the subsystem and reason, for example:

```text
phase6: pin original Psycle reference build
core: fix 64-bit playback counter without changing timing
songio: preserve machine state on legacy .psy reload
ci: gate psycle-core timing parity
qt: prove tracker focus routing
vst2: validate host against independent Linux plugin
```

## Review Standard

A change is ready when reviewers can understand:

- what was broken or different;
- which exact reference establishes expected behaviour;
- why the patch is appropriately scoped;
- how it was tested locally and in CI;
- whether compatibility changed;
- whether provenance/licensing boundaries remain clear.

The aim is not to make old code look new. The aim is to make **original Psycle behaviour work reliably on Linux** and keep it working.
