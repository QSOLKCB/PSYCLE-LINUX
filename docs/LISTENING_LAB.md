# PSYCLE-LINUX Listening Lab — early human-ear feedback

Status: **experimental GUI for the preserved C++ candidate player**, not the
full tracker UI or a certification of original Psycle 1.12.0 compatibility.

This small Python/Tk desktop application makes the already-maintained
historical psycle-player executable accessible to human listening tests.
It does **not** change the original engine, import any unlicensed songs,
claim a working Machine View, or modify the frozen evidence matrix.

The maintainer explicitly requested an early audible feedback tool on
9 November 2026, taking a narrowly scoped UI-test exception to the
deferred roadmap. The full Qt Widgets tracker UI remains deferred; this
temporary wrapper is not its architectural replacement.

## Launch on Ubuntu

Requires Python 3, Tk (Ubuntu package python3-tk), a desktop session,
and the previously maintained C++ psycle-player build.

~~~sh
sudo apt-get install python3-tk
# From PSYCLE-LINUX repository root:
python3 ui/listening_lab.py
~~~

No PyPI packages, virtual environments, containers, or Node/npm are needed.

If the player has not been built, reproduce the **Phase 6B historical-player**
build from .github/workflows/phase6b-player-build.yml (Ubuntu 24.04 reference).
The workflow pins the upstream support sources and historical plugin interface,
installs Qt/qmake and C++ library dependencies, and builds the C++ binary at:

~~~text
psycle-cpp-r12005-sanitized/psycle-player/++qmake/psycle-player
~~~

**Do not bypass the staging hashes or copy unknown SDK files to make the
build pass.** The GUI refuses to launch when that player is missing. A
different locally built candidate path may be supplied explicitly:

~~~sh
python3 ui/listening_lab.py \
  --player /path/to/psycle-player \
  --song /path/to/your-song.psy
~~~

The --player option is intentionally explicit so reports can identify the
actual executable bytes; selecting an executable does not assert that it is
the frozen Phase 6B build. The GUI hashes both song and player in its report.

## Listening procedure

1. **Lower system output volume** before starting. There is no in-app gain
   limiter; headphones and speakers are driven by the selected audio backend.
2. Load a .psy song you own or are permitted to test. Use project-generated
   synthetic fixtures where practical. The public repository deliberately
   excludes historical demo songs with unresolved redistribution permissions.
3. Select alsa (typical desktop), jack (if available), or dummy (silent
   diagnostic control). Optional device names are forwarded literally as
   separate arguments, with no shell expansion. Driver availability depends on
   the compiled player, and the historical player may fall back to
   dummy after reporting an unavailable output driver: **audio silence is
   not proof that the song has no sound**.
4. Click **Play song** and listen. The historical CLI loops until stopped;
   the GUI sends Enter to its --wait stdin to stop and finalize.
   A bounded terminate/kill fallback prevents a hung child from running
   indefinitely. **Stop normally before closing or interrupting WAV export**
   so the output file has a chance to finalize.
5. Optionally select a fresh, non-existing .wav output destination before
   pressing Play. The GUI refuses to overwrite an existing WAV path.
6. Classify the listening result, describe the audible fault and reproduction
   steps, and **Save listening report** as JSON. The report includes SHA-256
   of song and player, runtime driver/device, the observed process exit code,
   notes and a bounded tail of diagnostics. Reports may contain local file
   paths and personal notes: review before publishing.

Configured third-party native/LADSPA plugin paths are **disabled by default**.
Enabling them is a deliberate local opt-in, not a safety certification.
The historical player is not plugin-sandboxed; do not load untrusted songs
or plugins as though they were isolated.

### First three suggested ear tests

| Test | Procedure | Tell us |
| --- | --- | --- |
| A — Is sound present? | Try alsa, then compare dummy | Audible/silent, device name, output log |
| B — Pitch | Listen to a known single-note or familiar sample passage | Correct, too high/low, unstable, octave error |
| C — Timing | Play a short repetitive pattern and stop after several loops | Steady, early/late note, drift, dropout, tick irregularity |

If there is a problem, include a timestamp or pattern position if possible,
the expected sound, the actual sound, and whether it reproduces twice.

## Evidence boundaries

This is **candidate-only auditory evidence**. Every report deliberately declares
original_psycle_observed=false and parity_status=UNKNOWN.
A human listening impression is invaluable for finding defects but does not
by itself justify changing the Phase 6C compatibility matrix.

Current known limitations:

- No full tracker editor, Machine View, sample keyboard, or instrument UI.
- No automatic song-end detection (the preserved CLI loops).
- No level meter, live waveform, speaker calibration, or automatic audio analysis.
- No isolation of arbitrary third-party plugins.
- No claim that the candidate matches original Psycle's pitch/timing semantics.
- The historical player must be built separately; GUI CI does not rebuild it.

## Run focused tests

~~~sh
python3 -m unittest discover -s tests -p 'test_listening_lab.py' -v
python3 -m py_compile ui/listening_lab.py
# In a Linux CI/Xvfb environment:
xvfb-run -a python3 ui/listening_lab.py --smoke-test
~~~

The headless subprocess test uses a clearly labeled **fake player**. It checks
command construction, spaces in paths, safe default plugin-path handling, and
graceful stdin shutdown. It does not pretend to render or audibly verify Psycle.
