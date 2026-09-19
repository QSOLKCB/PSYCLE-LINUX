# Scoped PSY3 parse/load comparison

This comparison uses PR #61's successful Phase 6C workflow run
[35364406445](https://github.com/QSOLKCB/PSYCLE-LINUX/actions/runs/35364406445),
head `118678d257afc0545f63b783098884e10a97e91e`, merged into main through
`35d819374955e20e72e1669253e43252636cd3ff`.

- Original artifact: `10556051657`, SHA-256
  `7d8d9df684324538c9d8a08959516d8002689eed681a1c10994c05e12bc5ff51`.
- Candidate artifact: `10556141223`, SHA-256
  `6b9e3dae1fd9d5137f172692ed49bad5a24ad000cd7cb9d658b9077def9aca1e`.
- Fixture SHA-256:
  `db18fbf83afd13c28ff7cf01a20ebd2e97018b4ecf77496afe936c5c20050065`.

Both artifact archive hashes, their fixture bytes, and the original-reference
receipt validator were checked before classification. The project-authored
fixture can be regenerated with `scripts/phase4-core-workflow-smoke.sh`.
The original executable remains transient and is not redistributed.

`original-psy3.json` is a field-preserving classification projection of the
original receipt. Its original procedure, warning bootstrap, stable marker,
error/harness/runtime state, executable identity, and workflow attribution are
pinned in the matrix validator. Omitted inventory and UI artifacts remain in
the hash-bound workflow archive.

`candidate-psy3.json` and `candidate-psy3.log` are unchanged bytes from the
candidate artifact. They retain `observation: timeout` and `exit_code: 124`.
`candidate-psy3-parse.json` was derived later by
`scripts/phase6c-candidate-parse.py`; it was **not** emitted by the historical run.

To reproduce the derivation after verifying and extracting the candidate ZIP:

```sh
python3 scripts/phase6c-candidate-parse.py /path/to/extracted-candidate-artifact
python3 scripts/phase6c-candidate-parse.py /path/to/extracted-candidate-artifact --check
python3 scripts/phase6c-validate-matrix.py
python3 tests/phase6c_candidate_parse.py
```

The sidecar generator refuses an existing output and checks the fixture bytes.
The matrix independently rederives the committed sidecar from the pinned raw
receipt and log. It does not require redistributing generated fixture bytes.

The source-level interpretation is confined to
`psycle-player/src/psycle/player/main.cpp`: `playing...` follows successful
`song.load(input_file_name)`. Because `psy3filter.cpp` can return true after
reporting missing chunks, the complete log is checked and all unrecognized
warnings/diagnostics prevent acceptance. Allowed warnings are the exact version
warning for this fixture, absent native/LADSPA search paths (this fixture uses
only internal Sampler/Master), and scheduler realtime-permission warnings.
No warning is erased from the sidecar.

`PASS` means both implementations accepted this identical fixture under the
stated load procedures. It does not prove equivalent song state, audible output,
transport, clean process termination, serialization, or general PSY3 support.
The next contract is serialization/round-trip: record original save/reopen and
candidate save/reload observations, bind each output's hash, and compare semantic
state separately from byte identity before classifying that row.
