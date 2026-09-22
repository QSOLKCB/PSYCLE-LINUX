# Phase 6C Sampler fault-location witness

This lane is an **opt-in diagnostic** that follows the qualified Sampler
work-boundary evidence from PR #76.

It targets the exact project-authored `delayed-note-short` Sampler-local
`E-DF` fixture already used by
`sequencer-sampler-work-boundary-isolation`. The ordinary Phase 6C workflow
continues to run that fixture uninstrumented. A manual workflow dispatch with
`sampler_fault_location=true` adds a second fresh-process observation after a
clean accepted load. The temporary PR-head opt-in used to collect the first
hosted observation was removed after the evidence was frozen, so merged
`main` returns to manual opt-in only.

The optional observation:

1. requires the same pinned Psycle 1.12.0 x86 installer/executable identity;
2. reuses the exact hash-bound short `E-DF` fixture;
3. searches only for a **preinstalled x86 `cdb.exe`** on the Windows runner;
4. hashes and records the debugger identity when one is available;
5. snapshots the loaded module ranges before render;
6. attaches the debugger before the verified `Render as Wav` / `Save Wave`
   action;
7. retains the `0xC0000005` exception address and derives a module-relative
   offset when both can be observed.

No debugger is downloaded by this lane. Missing debugger tooling, attachment
failure, a missing exception address, an exit-code mismatch, or an address that
cannot be mapped to the pre-render module snapshot is **inconclusive**.

A module-relative offset is not a source-function identification. Symbols are
not required to preserve the module-offset witness, and `function_location`
remains `unresolved`. The observation cannot change
`sequencer-delayed-retrigger` from `UNKNOWN`.

The uninstrumented control and the instrumented receipt are validated together
by `scripts/phase6c-sampler-fault-location.py`. Artifact paths and hashes are
checked relative to the independently downloaded original-evidence artifact.

## Versioned hosted observation

PR #78 produced a validated hosted observation in workflow `35772562875` at
head `ed1d2ad38700021d88141e9d5310367f6e7753d9`. The exact short
`E-DF` control again exited with `0xC0000005`, while x86 CDB captured the
second-chance exception at `0x00e6c377` inside the pinned `psycle.exe`
loaded at `0x00e10000`, yielding module-relative offset **`0x5c377`**.

An earlier collection run, workflow `35770412423` at head
`d3c37b32bc709c6a77405d653cca15e704b2b453`, loaded the same executable
at `0x00520000` and captured the exception at `0x0057c377`, producing the
same **`0x5c377`** module-relative offset. That run later failed an unrelated
fresh-receipt versus immutable historical-projection check; it is retained only
as corroborating diagnostic evidence.

The stable offset across different ASLR bases localizes the failure to a
repeatable `psycle.exe` binary position. It still does **not** identify a
source function: `function_location` remains `unresolved`. The complete
workflow/artifact/receipt bindings are frozen in
[`observation.json`](observation.json).
