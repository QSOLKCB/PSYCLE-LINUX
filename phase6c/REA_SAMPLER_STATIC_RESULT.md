# Sampler pilot: direct static observation, 19 October 2026

## Result and limits

A bounded **direct GNU objdump** pass on the exact original Psycle 1.12.0 x86
executable found a normal-note-shaped path that computes its rate factor from a
per-wave field divided by a player-global field. It does not use a 44,100 Hz
constant in that factor. The instruction/data flow strongly corresponds to the
pinned `Voice::Tick` ordinary-note branch and its sample-rate/output-rate source
expression. Function and field names remain source-correspondence inference:
the executable has no COFF symbols, and no PDB or build-to-source attestation was
used. This is a static result, **not original runtime execution or parity**.

The frozen candidate source uses the 44,100 Hz basis already documented in the
[source contract](evidence/sampler-ps1/source-contract.json). This observation
strengthens the case for the existing non-44.1-kHz paired witness; it does not
justify a compatibility verdict or an engine patch by itself. Sampler PS1 and
the scoped pitch result remain `UNKNOWN`.

REA itself was neither installed nor executed. The result is a direct-provider
preparation step under the [pilot protocol](REA_SAMPLER_PILOT.md), not a claim
that REA recovered the function or that the entire pilot's review gates closed.

Execution statements in this report describe the local static investigation.
The repository's unchanged hosted regression CI runs independently and includes
an original-Windows observer. Its activity is not evidence that this static
calculation was reached or that the pitch witness has a new runtime verdict.

## Artifact and tool custody

- Official SourceForge installer: 9,322,919 bytes, SHA-256
  `f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769`.
- Extracted `app/psycle.exe`: 5,400,576 bytes, SHA-256
  `fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6`.
  Both hashes were rechecked after inspection and stayed unchanged.
- The maintainer approved static extraction. Debian trixie's official amd64
  `innoextract_1.9-0.1+b1_amd64.deb` was downloaded over HTTPS and checked
  against the [Debian package SHA-256](https://packages.debian.org/trixie/amd64/innoextract/download):
  `01894eb3ff9b6ab1ff6d12010459fafb5edb9de69c4cc73eb921952825657dcc`.
  The package was unpacked into a private local tool directory, without package
  maintainer scripts or system-wide installation. Executable SHA-256:
  `03516c95e430069ea619d093478b5d9f5492e0781a54b8dd046a395636bc272c`;
  observed version: innoextract 1.9.
- Provider: installed GNU objdump (GNU Binutils for Debian) 2.44, SHA-256
  `96afb8521834982d0e711b5d6e9785252bf82129fb0106b93a6bd54d83fae11f`,
  on Debian 13 amd64. Invoked path `/usr/bin/objdump`, resolved path
  `/usr/bin/x86_64-linux-gnu-objdump`. The five recorded calls were repeated
  explicitly through that path with `LC_ALL=C`, before/after provider and input
  hashes, complete stdout/stderr capture and exit status. All stderr streams were
  empty; stdout hashes reproduced exactly. Other inherited environment values
  were not retained, so this is not claimed to be a hermetic run.
  No provider plugins, debugger, target or installer were run.

The extractor listed the archive and extracted only `app/psycle.exe` as data.
The Windows installer, target, bundled DLLs and plugins were never executed.
Inputs and raw disassembly remain outside the repository; this document
publishes independently written analysis and hashes, not binary-derived code.
The accompanying [receipt](evidence/sampler-ps1/static-pitch/observation.json)
is an index/projection, not a substitute for regenerating and reviewing the raw
output. Its raw output digests include the original invocation's input path;
use recorded command/input spelling to reproduce those exact text digests.

## Address map and observed instructions

The PE header identifies `pei-i386`, preferred image base `0x00400000`.
`.text` starts at RVA `0x1000`, file offset `0x400`. For the code addresses below,
file offset = RVA - `0xC00`. These are preferred-image addresses, not runtime
ASLR addresses.

| Region | Preferred VA range, end exclusive | RVA start | File offset start | Purpose |
| --- | --- | --- | --- | --- |
| Entry context | 0x004B3340–0x004B33DB | 0xB3340 | 0xB2740 | Argument layout, instrument selection, empty-command-vector exit |
| Ordinary path context | 0x004B3808–0x004B3933 | 0xB3808 | 0xB2C08 | Note gate, synchronized-loop alternative, ordinary speed and resampler handoff |
| Ordinary arithmetic | 0x004B388C–0x004B3905 | 0xB388C | 0xB2C8C | Input loads, power/rate product and speed-field stores |
| Caller context | 0x004B3ED0–0x004B3F2B | 0xB3ED0 | 0xB32D0 | Six arguments, including selected voice, note entry and base-note value |
| Power import thunk | 0x00588528–0x0058852E | 0x188528 | 0x187928 | Jump through import-table address 0x005A756C to MSVCR90 `_CIpow` |
| Conversion helper | 0x00588586–0x005885FB | 0x188586 | 0x187986 | x87 integer conversion with correction and EDX:EAX result |

Observed static flow:

1. The entry at `0x4B3340` obtains an instrument index from byte 1 of its
   note-entry argument, and selects an instrument pointer. An empty command
   vector branches from `0x4B33D5` to `0x4B3808`.
2. At `0x4B3808`, note byte 0 must be below `0x78`, and a local flag must be
   zero, to reach the note-initialization path. At `0x4B385A`, an instrument byte
   selects a length/time branch or the ordinary branch at `0x4B388C`.
3. The ordinary branch loads unsigned note byte 0, signed 16-bit fields at
   wave-pointer offsets `0x48` and `0x4A`, and subtracts its base-note argument.
   The fine field is multiplied by binary32 `0.009999999776482582`; the summed
   exponent is multiplied by binary32 `0.0833333358168602`.
4. The power call at `0x4B38C5` reaches the `_CIpow` import thunk. Its base
   constant is binary64 2. The value at wave offset `0x44` is loaded as an integer;
   a negative signed interpretation is corrected by adding 2^32, consistent
   with an unsigned 32-bit field. It is divided by a signed integer at offset
   `0xCC0` of the object addressed through global `0x99AA78`.
5. The resulting power/rate product is stored as binary64 for the later
   resampler call and multiplied by binary64 2^32. A conversion helper at
   `0x588586` returns EDX:EAX, stored at voice offsets `0x14`/`0x10`.
   The helper's exact behaviour across all inputs/FP modes is not established.

Constant identities read directly from `.rdata`:

| VA | File offset | Type | Decoded value |
| --- | --- | --- | --- |
| 0x005CA5E8 | 0x1C99E8 | binary32 | 0.009999999776482582 |
| 0x005CA598 | 0x1C9998 | binary32 | 0.0833333358168602 |
| 0x005CA498 | 0x1C9898 | binary64 | 2 |
| 0x005CA388 | 0x1C9788 | binary32 | 4294967296 |
| 0x005CA2C0 | 0x1C96C0 | binary64 | 4294967296 |

## Formula and source correspondence

Ignoring intermediate x87 precision and final integer conversion, the observed
ordinary-path arithmetic is:

    q = pow(2, (note_u8 + tune_i16 - base_i32 + fine_i16 * c01) * c12)
        * wave_rate_u32 / output_rate_i32
    stored_speed = conversion_helper(q * 2^32)

Here `c01` and `c12` are the binary32 constants above, not exact real 1/100 and
1/12. The source-label interpretation of `wave_rate`/`output_rate` is supported
by the matching unsigned field, adjacent tune fields, instrument/voice/caller
structure, and the pinned source below; it is not debug-symbol proof.

Original source commit `7ac6d2c3553e2ee8dda55814d8e689919c345478`:

- `Sampler.cpp`, blob `6cc0bd7328d01131c3d41b68f4e5d4189959e364`, lines
  288–417: `Voice::Tick`, command-vector handling, note gate, loop alternative,
  ordinary sample-rate/output-rate expression and speed/resampler assignment.
- `XMInstrument.hpp`, blob `67bc801d4df35a05176e3372c3824ff3067d09fd`, lines
  193–198 and 217–237: signed 16-bit tuning getters and unsigned 32-bit sample
  rate, with rate/tune/fine members adjacent in that order. Exact compiled layout
  is inferred from the executable, not established solely by header ordering.
- `Player.hpp`, blob `51bf11c360c52437e421dd39a00ff6ea23507e58`, lines 111 and
  161–163: integer sample-rate getter and adjacent row/rate fields. Their exact
  binary offsets remain source-correspondence inference.

The frozen candidate `sampler.cpp` blob
`9edc00fb9013fbfde0bb0977398b934265b9c463`, lines 635–640, instead uses
`44100/output_rate` and a different fine-tune unit (`/256`). Only zero tuning
and the non-loop normal-note case are selected here. No general fine-tune
compatibility result is asserted. C-Psycle is not the deciding authority.

Remaining unknowns include runtime reachability in the failed hosted witness,
actual field values supplied by loading, base-note state, imported math-runtime
behaviour, x87 control/precision mode, final rounding and resampler output.
Neither this mapping nor proximity to any historical crash address identifies
or resolves a crash. No code from the target has been emulated or executed.

## Reproduction and next acceptance boundary

Obtain the official pinned installer locally; verify size/SHA-256. With the
verified extractor, use:

    innoextract --extract --include app/psycle.exe --output-dir extracted PsycleInstallerx86-1.12.0.exe

Verify the extracted executable hash before and after analysis. Record provider
version/hash, arguments, stdout/stderr, exit status and host. For each bounded
region in the address table, substitute its start/end values:

    LC_ALL=C /usr/bin/objdump -d -M intel --start-address=0x4b3808 --stop-address=0x4b3933 /tmp/psycle-static-pilot/extracted/app/psycle.exe

Also inspect headers/imports with `objdump -f -h` and `objdump -p`. Check actual
PE section mapping before translating addresses, and read constants at the
listed file offsets. Raw searches for imported power-call references were
only location hints; inference rests on the bounded decoded branch/data flow.
Some exploratory windows began mid-instruction and were discarded as evidence;
the recorded final regions start at verified decoded boundaries.

Independent exact-head review must assess these claims and their retained raw
outputs before static pilot closeout. Fresh exact-head Codex review is also
required. Then the next evidence boundary is still a paired original
native-Windows/candidate **normal-note, non-44.1-kHz** render with verified input
loading, identities, repeatability and predeclared pitch/duration tolerances.
Preserve the old failed observation. No matrix change, engine patch, merge or
release tag follows automatically from this static result.
