# Phase 6C serialization observation contract

The next evidence slice after PR #62 measures the pinned candidate's public save
API and original Psycle 1.12.0's Save As/fresh-reopen operation. The matrix row
`project-io-serialization-roundtrip` remains **UNKNOWN** until a versioned
comparison establishes a specific compatibility result.

## Candidate observation

`tests/phase6c_serialization.cpp` is a separately compiled, project-authored
GPL-2.0-or-later harness linked against the same verified static core used by the
historical player. No source in the frozen candidate tree is patched.

For each format version 2, 3, and 4, a fresh process:

1. Loads the exact project-authored PSY3 fixture already classified for load
   acceptance, SHA-256 `db18fbf83afd13c28ff7cf01a20ebd2e97018b4ecf77496afe936c5c20050065`.
2. Uses the dummy driver without starting playback and `PSYCLE_THREADS=1` to
   isolate serialization from the previously observed playback lifetime timeout.
3. Calls the actual `CoreSong::save(output_path, version)` API.
4. Records the load/save return values, reports, process result, log, output hash
   if a file exists, and limited before/after state.

The state slice is explicitly limited to name/author/comment, BPM, tick speed,
track count, and occupied machine slots. It is not a complete semantic state
snapshot and does not establish round-trip equivalence.

The pinned source currently registers PSY2 and PSY3 filters with save methods
that return false; the PSY4 registration is commented out. That is a source
finding, not a substitute for the runtime receipt. A recorded false return with
no output is named `save-returned-false-without-output`, not a parity verdict.
Crashes, timeouts, failed loads, or diagnostic contamination remain inconclusive.

`candidate-serialization.json` binds the exact fixture, core archive, probe
executable, harness/build source, API records, logs, and outputs. The executable
and generated build staging are removed after collection.

## Original observation

The maintained original-reference lane enables `-ObserveSerialization` only
after the normal pinned original load observations are available.

On a clean, stable PSY3 load, it:

1. Enumerates the exact PID/title-owned native File menu and requires exactly one
   enabled Save As command; re-enumerates immediately before dispatch.
2. Requires one process-owned Save As dialog, one enabled filename edit with its
   known common-dialog identifier, and one enabled Save button.
3. Sets and verifies a fresh output path, invokes Save, and records dialog
   dismissal plus a stable nonempty PSY3 output over the full polling interval.
4. Leaves the original input fixture bytes intact.
5. Terminates the initial process and launches the saved output in a fresh
   process after restoring the post-install registry baseline.
6. Runs the same complete load-marker, error, bootstrap, runtime, plugin-inventory
   and liveness checks on the reopened output.

Unknown/ambiguous UI signatures are recorded, not guessed. There is no keyboard
injection, fixed unverified command ID, or overwrite-confirmation automation.
The original executable and installer remain transient.

The saved output's input role is `original-saved-fixture`, explicitly separate
from Linux candidate evidence. The legacy `candidate_fixture` field in the
reopen observer denotes the source path only; `source_fixture_role` identifies
its actual role. The validator accepts that role exclusively for this reopen
contract and retains the existing candidate-role requirements for PSY2/PSY3.

Receipts include `serialization-save.json`, `serialization/saved-fixture.json`,
and, when saving succeeds, `original-psy3-reopen.json` with fresh inventories.
The save PID is checked against the initial reference runtime inventory.
The validator reports save/reopen observations, byte identity separately, and
`semantic_roundtrip: not-measured`. These receipts do not self-promote parity.

## Maintained execution

The Phase 6C workflow builds and runs the probe, enables the original save/reopen
observer, validates all receipts, and uploads diagnostic artifacts even when an
observation is inconclusive. The workflow includes the harness source/build
files, observer, validators, and regression tests in its trigger paths.

```sh
bash scripts/phase6c-serialization-candidate.sh phase6c-evidence
python3 scripts/phase6c-serialization-evidence.py candidate phase6c-evidence
python3 scripts/phase6c-serialization-evidence.py original candidate-artifact original-artifact
python3 tests/phase6c_serialization.py
```

The next classification must distinguish an absent serialization capability
from differences in serialized bytes and from semantic state preservation.
Neither a successful save nor an accepted reopen by itself proves the latter.
