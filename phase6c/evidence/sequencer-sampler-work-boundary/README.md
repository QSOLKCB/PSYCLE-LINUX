# Sampler work-boundary diagnostic projection

This directory freezes a concise, hash-bound projection of Phase 6C workflow
`35752301481` at evidence head
`4cf71eba6f7b5f6ba14be6089cf37422b16b5ca6`.

The hosted candidate and original artifacts remain the source observations.
`observation.json` binds their artifact digests, the pinned original-source
identity, the four fixture/receipt hashes, the exact Windows access-violation
result, and the finalized silent release-control output.

The historical generated receipt used the diagnosis label
`voice-tick-initialization-associated-exit`. That label is preserved as a
historical field, not rewritten. The current interpretation is deliberately
narrower: the crash is associated with enabled-note startup before normal
`controller.Work()` sample processing becomes reachable. The evidence does
not establish whether `Voice::Work()` was entered, whether the fault occurred
inside `Voice::Tick()`, or whether voice selection/setup is the failing
operation.

The committed projection remains `parity_status: UNKNOWN`. It is diagnostic
evidence only and does not provide the command-bearing original waveform
required to classify delayed/retrigger timing.

`scripts/phase6c-sampler-work-boundary.py` validates this projection against
the candidate and original receipt bytes whenever the native artifact lane is
revalidated.
