# Source Provenance

PSYCLE-LINUX is a continuation and Linux-porting effort built from the existing Psycle/C-Psycle source lineage. This file records the baseline selected for the project and the rules for preserving its history.

## Selected Initial Baseline

The initial technical reference supplied for the port is:

- Project: Psycle / C-Psycle
- Upstream host: SourceForge
- Upstream tree: `https://sourceforge.net/p/psycle/code/HEAD/tree/`
- Snapshot/archive label: `r12005-trunk-cpsycle`
- Archive filename: `psycle-code-r12005-trunk-cpsycle.zip`
- SHA-256: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`

The SHA-256 identifies the exact archive used during the initial PSYCLE-LINUX documentation and source audit. Future upstream snapshots must receive their own provenance record rather than silently replacing this baseline.

## Why This Baseline

The r12005 C-Psycle tree already contains substantial cross-platform and Linux-specific work. Relevant areas include:

- `ui/src/imps/x11/`
- `driver/alsa/`
- `driver/alsamidi/`
- `driver/jack/`
- `driver/sdl2/`
- `driver/evjoystick/`
- `host/`
- `audio/`
- `player/`
- `plugins/`
- `presets/`
- `luaui/`
- `luascripts/`
- `doc/`

The top-level makefile already defines host, plugin, driver, and player targets. The Linux driver makefile includes SDL2, ALSA, ALSA MIDI, JACK, and Linux event joystick targets. The host and UI build files reference X11/Xft and related libraries.

This means PSYCLE-LINUX should begin by repairing and validating existing Linux architecture, not by throwing it away.

## Upstream Authorship

The r12005 `AUTHORS` file identifies Psycle as copyright 2000-2021 by a community of developers and plugin authors.

It identifies, among others:

- Josep Maria Antolin Segura — `jaz001`
- Johan Boule — `johan-boule`
- Stefan Nattkemper — `stefan001`
- ShadowBane — `baneofshadow`
- Juan Antonio Arguelles Rius — `arguru`
- Mats Hojlund — `cbr`
- Daniel Arena — `dubdub`
- Mark McCormack
- Marcin Kowalski — `FideLoop`
- Lukasz Langa — `kSh`
- Jeremy Evers — `pooplog`
- Martin Etnestad Johansen — `lobywang`
- James Redfern — `alkenstein`
- and additional contributors recorded in the upstream file.

The upstream `AUTHORS` file, copyright headers, and per-plugin notices are authoritative and must be preserved with imported source.

## Arguru Machines in r12005

The selected source snapshot contains source directories for the following Arguru machines:

- `plugins/arguru-compressor/`
- `plugins/arguru-distortion/`
- `plugins/arguru-goaslicer/`
- `plugins/arguru-reverb/`
- `plugins/arguru-synth-2f/`
- `plugins/arguru-xfilter/`

These are historical components of Psycle and explicit compatibility/preservation targets for PSYCLE-LINUX.

## Import Rules

When the upstream source is introduced into PSYCLE-LINUX:

1. preserve the directory structure as closely as practical in the baseline commit;
2. preserve `AUTHORS`, `COPYING`, copyright headers, README files, and component notices;
3. do not mass-format or mechanically rename source in the import commit;
4. do not claim PSYCLE-LINUX authorship over upstream code;
5. record any files intentionally omitted from the archive and why;
6. tag or otherwise identify the baseline commit before functional modifications begin;
7. keep third-party source boundaries and licensing visible;
8. record any later upstream cherry-picks or source refreshes separately.

## Historical Community

Psycle was more than its source repository. Developers, testers, musicians, plugin authors, and users shared builds, songs, machines, bug reports, and knowledge through the project's community channels, including the historical `#Psycle` IRC community.

PSYCLE-LINUX aims to preserve that history respectfully while producing a maintainable Linux port.
