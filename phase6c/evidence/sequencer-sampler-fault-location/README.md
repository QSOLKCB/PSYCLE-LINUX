# Phase 6C Sampler fault-location witness

This lane is an **opt-in diagnostic** that follows the qualified Sampler
work-boundary evidence from PR #76.

It targets the exact project-authored `delayed-note-short` Sampler-local
`E-DF` fixture already used by
`sequencer-sampler-work-boundary-isolation`. The ordinary Phase 6C workflow
continues to run that fixture uninstrumented. A manual workflow dispatch with
`sampler_fault_location=true` adds a second fresh-process observation after a
clean accepted load. For connector-driven evidence collection, the dedicated
PR head branch `phase-6c/sampler-fault-location-observation` is an equivalent
explicit opt-in; no other pull-request branch enables the debugger witness.

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
