# Preservation pilot: original Sampler pitch / sample-rate calculation

## Decision and current evidence state

On 19 October 2026 the maintainer deferred the existing implementation roadmap
for this bounded investigation. Its purpose is to reduce one original-binary
uncertainty before deciding the next compatibility step toward v1.0.0.

**Current result: protocol prepared; original binary inspection NOT RUN.**
The official installer was subsequently downloaded as data and its 9,322,919-byte
size and SHA-256 were verified against the pin below. Static string inspection
identifies Inno Setup 5.3.10 (Unicode). The installed `psycle.exe` has not been
extracted: no suitable static Inno extractor is currently available. No installer
or target was executed. No address,
disassembly, recovered formula, provider result or runtime parity is asserted
by this document. The broad Sampler PS1 row and scoped pitch witness remain
`UNKNOWN`. The existing [source contract](evidence/sampler-ps1/source-contract.json)
and [hosted pitch observation](evidence/sampler-ps1/pitch-hosted/observation.json)
are retained, not replaced by this plan.

## One question, one normal-playback path

For an ordinary note, with no tracker effect, no synchronized-loop mode and
zero fine/coarse tuning, does the pinned original binary calculate sample
advance from the sample's own rate divided by output rate, as the pinned
original source suggests, rather than the frozen candidate's 44,100 Hz basis?

Inspect only the calculation and the minimum caller/data flow needed to bind
its note, base-note, tuning, sample-rate and output-rate inputs. Record arithmetic
width, conversion/rounding and any fixed-point scale actually observed. Do not
assume source names or the recorded crash offset identify this function.
XMSampler, malformed inputs, crashes, exploitability, plugin execution, DRM,
licensing checks and unrelated binary functionality are outside this pilot.
Do not resume the previously stopped malformed-input audit through REA or
another route.

## Authorities and frozen inputs

1. Original Psycle 1.12.0 x86 is the behavioural authority. The installer pin in
   [reference provenance](../PHASE6_REFERENCE_PROVENANCE.md) is
   `PsycleInstallerx86-1.12.0.exe`, 9,322,919 bytes, SHA-256
   `f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769`.
   The extracted `psycle.exe` pin from the retained hosted pitch observation is
   `fdb130d2465d5b4a4acfbfe0bfb2368926380fe07a383c4774f0951591b6d6b6`.
   These are distinct artifacts: hashing the installer does not identify the
   bytes disassembled. Verify the selected executable before and after reading.
2. Pinned original source is `jpaquim/psycle` commit
   `7ac6d2c3553e2ee8dda55814d8e689919c345478`; Sampler.cpp blob
   `6cc0bd7328d01131c3d41b68f4e5d4189959e364`. Source correspondence remains
   a hypothesis until tied to the binary instruction/data flow.
3. The sanitized r12005 C++ candidate is the implementation under test, frozen
   by baseline `00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`.
   Its `psycle-core/src/psycle/core/sampler.cpp` blob is
   `9edc00fb9013fbfde0bb0977398b934265b9c463`.
4. C-Psycle remains a preserved donor and shared-contract aid. Its calculation
   cannot decide original-Psycle behaviour or substitute for either runtime side.

Never commit the installer, original executable, restricted dependencies or
unreviewed binary-derived exports. Keep transient inputs outside public evidence
roots. Review provenance/redistribution scope before publishing any disassembly;
prefer a minimal necessary excerpt, addresses, hashes and independently written
formula/limitations. Do not change historical fixtures, manifests or verdicts.

## Provider readiness and bounded static procedure

The preparation environment has GNU objdump 2.44 with advertised `pei-i386`
coverage. REA, Ghidra and Rizin were not found on PATH. Provider capability output
is not proof of successful analysis of this artifact. Nothing was installed.

REA is the proposed investigation interface, not an authority or sandbox.
The documentation reviewed is from [morluto/rea](https://github.com/morluto/rea),
with package.json at commit `e4c3d72396bf925a45724ce8b18b238b0d23d787`
reporting 6.7.0. This is a source-documentation identity, not an installed or
verified package identity. Before using REA, pin the actual package/version and
provider executable/version, confirm the intended static capability, and obtain
any required installation/configuration approval. Do not run an unpinned
`npx ...@latest setup` as part of this pilot.

The reviewed [objdump integration](https://github.com/morluto/rea/blob/e4c3d72396bf925a45724ce8b18b238b0d23d787/docs/reverse-engineering-tools.md)
retains raw provider output and before/after artifact hashes. Identical hashes
alone do not prove the exact bytes consumed if another process can mutate the
file. Use a controlled local copy with no concurrent writers, record custody,
and reject changed inputs, failed calls or truncated relevant output.

A direct installed-objdump pass is a permitted static preparation path while
REA is unavailable; label it **direct objdump**, never as REA execution.

1. Verify executable SHA-256 against the pin; record byte count and acquisition
   source/custody without exposing private paths or credentials.
2. Record tool path, executable hash, version, host architecture, arguments,
   environment relevant to decoding, exit status and complete stdout/stderr.
3. Inspect PE headers and section mapping without launching the target. Keep
   architecture, image base, section RVA/file offsets and decoding mode explicit.
4. Locate candidate normal-note arithmetic using source/constants/callers only
   as search hints. Disassemble a bounded range, then verify branch and input
   provenance. Record start/end RVA, virtual-address basis, corresponding file
   offsets, instruction bytes and output digest. Expand only enough to resolve
   the stated calculation; an unresolved call/input ends in an explicit unknown.
5. Translate observed arithmetic into an independently explained formula with
   units, assumptions and unresolved details. Compare with the frozen candidate
   source, including casts and assignment to the speed field. Preserve both
   agreeing and contradicting observations.
6. Produce an evidence index with relative output paths and SHA-256 digests,
   references to source blobs, and separate observed facts/inferences. Retain
   raw output privately if publication rights are not established.

Static processing must not execute `psycle.exe`, its installer, scripts from
an input archive, plugins, or provider debugger/capture commands. If extraction
requires new software or executing an installer, stop that step and obtain the
necessary approval; an already-extracted matching executable is sufficient.
Native Windows runtime capture is unavailable in the reviewed
[REA capture workflow](https://github.com/morluto/rea/blob/e4c3d72396bf925a45724ce8b18b238b0d23d787/docs/process-capture.md).
REA capture runs with user permissions and is not a sandbox. WSL/Linux capture
cannot be presented as native Windows observation.

## Source-derived prediction to test, not a recovered-binary result

The retained source contract says original speed uses sample-rate/output-rate;
the candidate ordinary-note expression at sampler.cpp lines 635–639 uses
`pow(2, (note + waveTune - baseC + waveFinetune/256)/12) * 2^32 * (44100/output_rate)`.
The synchronized-loop branch is separate and excluded. Validate line references
against the pinned blob, not a future moving file.

Holding the note exponent at zero gives these mathematical predictions:

| Sample rate | Output rate | Source-indicated original advance | Candidate expression advance |
| --- | --- | --- | --- |
| 44,100 | 44,100 | 1 | 1 |
| 22,050 | 44,100 | 1/2 | 1 |
| 48,000 | 48,000 | 1 | 147/160 |

Advance is sample frames per output frame, before fixed-point conversion.
These are algebraic hypotheses, not executed test vectors or audible results.
Check base-note mapping, input field meaning, resampling and rounding before
using them as test expectations. Do not invent the original instruction formula
from this table.

## Pilot exit and next decision

The static pilot may close only with all of the following reviewed:

- exact original executable identity and custody, and actual tool/provider pins;
- a bounded instruction/data-flow mapping, or a clearly documented inability to
  recover it without unsupported assumptions;
- raw evidence hashes, address/offset basis, formula, assumptions and limitations;
- source-bound candidate comparison, explicitly distinguishing executed provider
  calls, source reading and inferred predictions;
- independent review and fresh Codex review of the exact PR head, with actionable
  findings stated as minimal reproduction, expected versus actual, consequence,
  and executed versus inferred evidence;
- no historical fixture, matrix verdict, licensing boundary or prior gate weakened.

An inconclusive mapping is a valid pilot outcome, not a parity result. It must
name the next required artifact/provider/observation before further work.
A docs-only preparation PR does not satisfy the binary-analysis exit criteria.

If the mapping is useful, propose one paired **normal-note pitch** observation
using project-authored PCM and the same hash-bound fixture semantics on pinned
original native Windows and the frozen candidate. Reuse the maintained pitch
collector after checking its assumptions; preserve the previous crashed attempt.
Predeclare the pitch/duration metric and tolerance, fixture bridge differences,
output sample rate, base note, tuning, render procedure and deterministic-repeat
requirement. Actual original/candidate outputs, identity-bound receipts and a
reviewed comparison remain necessary for any scoped compatibility verdict.
Static formula agreement cannot clear the existing runtime gate.

Only then consider the smallest evidence-justified engine patch, with forward
regressions; do not rewrite or replace the engine based on this pilot. Resume or
resequence the deferred roadmap explicitly after reviewing the pilot outcome.
The private IT/MIDI observation holds and active-engine required-check governance
remain open. All [Phase 11 acceptance areas](../ROADMAP.md#phase-11--reliability-suite-and-psycle-linux-10)
still apply before a stable v1.0.0 release, including usable tracker workflow,
plugin containment, hardware, packaging and long-session reliability. No merge
or release tag is part of this pilot preparation.
