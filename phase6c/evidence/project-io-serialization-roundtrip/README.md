# Serialization capability comparison

This directory versions the first compatibility classification for
`project-io-serialization-roundtrip`.

The verdict is **MISSING**, scoped narrowly to the save capability required by
the tested original-Psycle operation:

- final PR #63 workflow run `35439069516` observed pinned Psycle 1.12.0 x86
  save the exact PSY3 fixture to a fresh PSY3 output and accept that saved file
  in a fresh process;
- the same run observed the pinned candidate probe call
  `CoreSong::save(path, version)` for versions 2, 3 and 4; every call returned
  false, produced no output and exited 0;
- version 3 is the directly comparable PSY3 save request. Versions 2 and 4 are
  retained as surrounding capability evidence.

`candidate-serialization.json` is committed byte-for-byte from artifact
`10582559462` (SHA-256
`sha256:441854e0595636b5cb12b45bf8711a07f27d4fdacbca2c2b0468be89282427f8`).

`serialization-save.json` is a field-preserving classification projection of
artifact `10582734339` (SHA-256
`sha256:d29a7be85441a4c73df80861d0a4686b73b1d76613f2ddfead5690ac3e9daba5`).
It binds the raw initial-load, Save As, saved-fixture and fresh-reopen receipts
by SHA-256. The original executable and generated PSY3 output are not
redistributed here.

The saved original output differs byte-for-byte from the input. Complete
semantic round-trip state preservation was **not measured** and is not implied
by the MISSING verdict. The classification establishes only that the candidate
baseline lacks the required serialization/save capability for this tested
operation.
