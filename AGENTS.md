# AGENTS.md

AGENT_POLICY_VERSION: 1
REPOSITORY: QSOLKCB/PSYCLE-LINUX
AUDIENCE: MACHINE_ONLY
OPERATING_MODE: PRESERVATION_VIGILANT
DEFAULT_RISK_POSTURE: CONSERVATIVE
PRIMARY_GOAL: ORIGINAL_PSYCLE_COMPATIBILITY_ON_LINUX
PRIMARY_RULE: PORT_PSYCLE_TO_LINUX_DO_NOT_REINVENT_PSYCLE
HISTORY_VALUE: HIGH
PROVENANCE_SENSITIVITY: HIGH
COMPATIBILITY_SENSITIVITY: HIGH

## 1. PRECEDENCE

Agents MUST apply repository instructions in this order:

1. platform/system/security/tool constraints;
2. explicit maintainer instruction;
3. this `AGENTS.md`;
4. `ROADMAP.md`;
5. `PORTING.md`;
6. provenance/licensing documents;
7. subsystem-specific documentation;
8. existing implementation convention.

Agents MUST NOT interpret this file as authorization to violate licensing, provenance, security, or redistribution restrictions.

If instructions conflict, agents MUST preserve source/history/evidence and surface the conflict rather than silently selecting the more destructive interpretation.

## 2. REQUIRED READ SET

Before making a substantive repository change, agents MUST inspect the current versions of all relevant files from this set:

* `AGENTS.md`
* `ROADMAP.md`
* `PORTING.md`
* `README.md`
* `PROVENANCE.md`
* `LICENSING.md`
* `UPSTREAM_OMISSIONS.md`
* `THIRD_PARTY_INVENTORY.md`
* `UPSTREAM_ARCHITECTURE.md`
* `PHASE6_REFERENCE_PROVENANCE.md`
* `PHASE6_CPP_IMPORT_AUDIT.md`
* `PSYCLE_CORE_PARITY.md`

Agents MUST NOT rely on remembered phase status when the repository contains a current roadmap or current PR head.

Current repository state and current branch/PR head take precedence over stale summaries.

## 3. SOURCE ROLES

SOURCE_ROLE_ORIGINAL_PSYCLE:
ROLE: PRIMARY_BEHAVIOURAL_AND_UI_REFERENCE
AUTHORITY: HIGHEST_WHEN_VERSION_PINNED_AND_REPRODUCIBLE
DIRECT_LINUX_BASE: FALSE

SOURCE_ROLE_CPP_FAMILY:
COMPONENTS:
- universalis
- psycle-core
- psycle-audiodrivers
- psycle-helpers
- psycle-player
- psycle-plugins
ROLE: CANDIDATE_LINUX_ENGINE
AUTHORITY: IMPLEMENTATION_UNDER_TEST
ASSUME_PARITY: FALSE

SOURCE_ROLE_CPSYCLE:
PATH: cpsycle/
ROLE:
- PRESERVATION_BASELINE
- LINUX_DONOR
- REGRESSION_CORPUS
- COMPATIBILITY_ORACLE_WHERE_SEMANTICS_OVERLAP
FINAL_BEHAVIOURAL_AUTHORITY: FALSE
ASSUME_SEQUENCER_EQUIVALENCE: FALSE

Agents MUST NOT collapse these roles.

Agents MUST NOT describe C-Psycle as original Psycle.

Agents MUST NOT treat successful compilation of the C++ candidate as proof of original-Psycle compatibility.

Agents MUST NOT use C-Psycle behaviour to override reproducible version-pinned original-Psycle behaviour.

## 4. EVIDENCE HIERARCHY

When implementations, documentation, tests, or assumptions disagree, agents MUST prefer evidence in this order:

1. version-pinned original Psycle source/build plus reproducible observation;
2. trustworthy historical songs, formats, and frozen compatibility contracts tied to a known version;
3. pinned C++ candidate-family behaviour;
4. C-Psycle source and PSYCLE-LINUX regression evidence where semantics overlap;
5. retained upstream developer documentation;
6. historical branches, posts, release notes, and secondary descriptions;
7. inference.

Inference MUST NOT be promoted to historical fact.

Memory-only behavioural claims MUST NOT be recorded as parity evidence.

If original Psycle versions differ, agents MUST record version-specific behaviour rather than creating an artificial universal result.

## 5. PRESERVATION INVARIANTS

Agents MUST preserve:

* upstream authorship;
* copyright headers;
* licensing notices;
* historical comments;
* historical names;
* machine identities;
* plugin identities where legally retainable;
* file-format contracts;
* state-layout contracts;
* known baseline identities;
* frozen manifests;
* omission records;
* quarantine records;
* provenance links;
* evidence artifacts and procedures;
* meaningful historical structure.

Agents MUST NOT:

* rewrite repository history for cosmetic convenience;
* force-push published history unless explicitly authorized by the maintainer;
* squash away provenance anchors merely to simplify history;
* alter frozen hashes/manifests to make validation pass;
* regenerate a frozen identity from modified source and present it as the old identity;
* delete retained historical source because it is currently unused;
* delete optional machines merely because they are not active;
* remove historical comments because they appear obsolete;
* perform mass formatting during functional work;
* combine provenance import, cleanup, refactoring, and behavioural changes into one indistinguishable operation;
* replace narrow historical code solely because a newer architecture is preferred;
* rename historical concepts without demonstrated necessity;
* silently relicense imported code.

A failing preservation/provenance check MUST be investigated.

The check MUST NOT be weakened merely to produce green CI.

## 6. PROVENANCE BOUNDARIES

Agents MUST treat provenance boundaries as executable project constraints.

Agents MUST NOT reintroduce material explicitly omitted, quarantined, or non-redistributable by current repository policy.

This includes historical SDK-derived or restricted material where repository documents establish an exclusion.

VST2 compatibility work MUST use the repository's clean-room/independently-authored compatibility strategy.

Agents MUST NOT restore omitted Steinberg SDK source expressions into the public repository.

Closed binaries, restricted SDK material, unresolved songs, or other quarantined inputs MUST NOT be committed merely because they are technically useful.

Transient reference material MUST remain transient when repository policy prohibits redistribution.

If provenance is uncertain:

ACTION: STOP_MUTATING_PROVENANCE_BOUNDARY
RESULT: PRESERVE_CURRENT_STATE
STATUS: UNKNOWN
REQUIRED_OUTPUT: IDENTIFY_UNCERTAINTY

Agents MUST NOT guess licensing provenance.

## 7. FROZEN BASELINE RULE

A frozen baseline is evidence.

Agents MUST distinguish:

* historical/frozen source identity;
* sanitized archival identity;
* candidate implementation;
* later compatibility patches.

Agents MUST NOT retroactively alter a frozen baseline to contain later fixes.

Compatibility work MUST occur as traceable changes relative to the preserved baseline.

If a baseline defect is discovered, agents MUST document the defect and repair forward unless explicit repository policy requires another method.

## 8. CHANGE STRATEGY

DEFAULT_CHANGE_STRATEGY:

1. reproduce;
2. observe;
3. identify authority;
4. isolate difference;
5. apply smallest maintainable patch;
6. add regression evidence;
7. run affected tests;
8. inspect resulting diff;
9. inspect current-head review feedback;
10. preserve unresolved uncertainty explicitly.

Agents MUST prefer patch-before-replace.

Large replacement work requires evidence that narrow repair is impractical, unsafe, legally unsuitable, or incompatible with the Linux target.

Agents MUST NOT redesign a subsystem merely because redesign is easier than understanding the historical implementation.

Agents MUST NOT modernize behaviour without a demonstrated compatibility, Linux, reliability, security, packaging, or maintainability reason.

## 9. SCOPE CONTROL

Each PR SHOULD address one primary concern.

Preferred scopes include:

* provenance/import;
* build compatibility;
* one parity gap;
* one machine family;
* one timing behaviour;
* one serialization behaviour;
* one audio/MIDI backend concern;
* one UI interaction concern;
* one plugin-containment concern;
* one clean-room ABI concern;
* one CI/evidence concern.

Agents MUST separate unrelated refactors.

Agents MUST NOT opportunistically clean adjacent historical code unless required for the scoped fix.

If unrelated defects are discovered:

ACTION: RECORD_OR_REPORT
DEFAULT: DO_NOT_EXPAND_CURRENT_PATCH

## 10. PARITY STATUS CONTRACT

Allowed compatibility states:

* `PASS`
* `DIFFERENT`
* `MISSING`
* `UNKNOWN`

`UNKNOWN` is a valid and preferred result when evidence is insufficient.

Agents MUST NOT optimize for increasing the number of `PASS` rows.

For any non-`UNKNOWN` classification, agents MUST require the evidence specified by the current matrix validator and repository policy.

At minimum, classification MUST be bound to:

* a specific original-Psycle reference build/version;
* a reproducible original-reference procedure/observation;
* a specific candidate snapshot/baseline;
* a reproducible candidate procedure/observation;
* the fixture/input used.

Candidate-only evidence MUST NOT produce:

* `PASS`
* `DIFFERENT`
* `MISSING`

Candidate-only execution evidence MUST remain compatibility status `UNKNOWN`.

C-Psycle-only evidence MUST NOT produce original-Psycle parity claims.

A successful build MUST NOT produce a parity claim.

A clean program exit MUST NOT by itself produce a parity claim.

A matching output without identity/procedure binding MUST NOT produce a parity claim.

## 11. RECEIPT INTEGRITY

Machine-readable evidence MUST be self-identifying where practical.

Receipts SHOULD record:

* schema version;
* phase;
* contract identifier;
* source/reference identity;
* candidate identity;
* executable identity or hash where relevant;
* fixture path;
* fixture hash;
* procedure/command;
* exit status;
* observation;
* output/log paths;
* output/log hashes;
* parity status;
* whether original Psycle was actually observed.

Artifact paths MUST resolve relative to the artifact root when the artifact is downloaded independently.

Agents MUST NOT record a selected executable as the pinned candidate unless the selected executable has been verified as that candidate.

Override mechanisms MUST NOT silently weaken evidence identity.

Infrastructure/harness failures MUST NOT be reported as candidate behaviour.

## 12. TESTING POLICY

Tests SHOULD validate behaviour rather than implementation detail.

Priority regression domains:

* `.psy` parsing;
* PSY2/PSY3 compatibility;
* serialization;
* save/load round trips;
* sequence/pattern behaviour;
* BPM/LPB/tick timing;
* tracker commands;
* delayed/retrigger behaviour;
* Sampler;
* XMSampler;
* mixer/master/routing;
* native-machine identity;
* native-machine parameters/state;
* plugin parameter/state restoration;
* opaque-state restoration;
* MIDI;
* automation;
* WAV/sample loading;
* deterministic or tolerance-based DSP;
* render/bounce;
* missing-machine behaviour;
* historical-song playback;
* plugin crash/hang containment;
* UI input/focus behaviour when UI work begins.

Tests derived from C-Psycle MUST be marked conceptually as shared-contract tests or C-Psycle-only historical tests.

Agents MUST NOT silently convert C-Psycle-specific behaviour into an original-Psycle acceptance criterion.

## 13. CI POLICY

Agents MUST treat CI as evidence infrastructure, not ceremony.

When code affects a workflow's inputs, path filters MUST include the affected source tree.

A workflow MUST NOT remain green solely because relevant changes fail to trigger it.

Evidence-generating workflows SHOULD:

* use pinned/reproducible inputs;
* validate identities before execution;
* distinguish harness failure from behavioural result;
* upload diagnostic receipts/logs when useful;
* fail on provenance/identity violations;
* avoid presenting partial execution as parity proof.

A green workflow means only that its encoded contract passed.

Agents MUST NOT infer untested compatibility from green CI.

When fixing CI, agents MUST fix the cause where practical.

Agents MUST NOT delete, skip, weaken, or bypass a meaningful test solely to make CI green.

## 14. REVIEW POLICY

Before declaring a PR ready, agents MUST inspect the current PR head.

Agents MUST distinguish:

* current findings;
* stale findings;
* outdated threads;
* already-fixed findings;
* newly introduced regressions.

Review feedback MUST be evaluated against current code, not blindly implemented.

When a finding is valid:

1. reproduce or reason from current head;
2. patch narrowly;
3. verify;
4. reply with the concrete fix;
5. resolve only when the condition is actually addressed.

Agents MUST NOT resolve a review thread merely because the referenced lines became outdated.

Agents MUST NOT assume an automated reviewer is correct.

Agents MUST NOT dismiss a finding merely because CI is green.

## 15. BUILD POLICY

Agents MUST preserve a reproducible build path before migrating build systems.

Build-system migration MUST NOT simultaneously become:

* engine redesign;
* UI redesign;
* source-tree cleanup;
* behavioural rewrite.

New dependencies require a concrete purpose.

System packages SHOULD be preferred where practical over unnecessary bundled dependencies.

Dependency changes MUST NOT cross licensing/provenance boundaries silently.

## 16. ENGINE POLICY

The candidate engine MUST be measured before substantial rewriting.

Agents MUST NOT assume the C++ candidate is correct because it is historically related to Psycle.

Agents MUST NOT rewrite engine subsystems before the parity evidence demonstrates the relevant gap unless required for:

* buildability;
* safety;
* legality;
* platform compatibility;
* deterministic observation infrastructure.

Engine convergence changes SHOULD preserve:

* song semantics;
* timing;
* routing;
* sampler behaviour;
* native-machine state;
* plugin state;
* historical project behaviour.

## 17. C-PSYCLE POLICY

C-Psycle work is retained permanently as project evidence unless explicit maintainer policy changes.

Agents MUST NOT delete the C-Psycle preservation corpus after the C++ path becomes functional.

C-Psycle MAY provide:

* Linux implementation techniques;
* regression fixtures;
* machine behaviour evidence;
* serialization evidence;
* driver implementations;
* compatibility experiments;
* historical architectural context.

C-Psycle MUST NOT automatically supply:

* final sequencer semantics;
* final UI semantics;
* original-Psycle parity truth.

## 18. NATIVE MACHINE POLICY

Historical machine identity is compatibility-sensitive.

Agents MUST preserve names and identity unless technical or legal constraints require otherwise.

DSP changes MUST be minimized until behaviour is measurable.

Agents SHOULD separate:

* compiler fixes;
* width/portability fixes;
* undefined-behaviour fixes;
* platform fixes;
* intentional DSP changes.

Intentional sound changes MUST NOT be hidden inside portability work.

Optional retained machines MUST remain retained unless explicit evidence and policy justify removal.

## 19. PLUGIN SAFETY POLICY

Third-party plugin binaries are untrusted inputs.

Future discovery and hosting changes MUST prefer:

* incremental discovery;
* metadata caching;
* out-of-process probing;
* execution timeouts;
* crash containment;
* quarantine;
* isolated instantiation;
* isolated opaque-state restoration;
* recoverable placeholders.

A plugin MUST NOT gain the ability to crash or hang the primary application merely because:

* it exists in a scan directory;
* startup scans it;
* a song references it;
* opaque state is restored.

Compatibility does not require preservation of unsafe failure modes.

## 20. UI POLICY

Original Psycle's interaction model is the UI compatibility target.

Agents MUST NOT redesign Psycle into a generic modern DAW.

Toolkit choice does not authorize workflow redesign.

UI work SHOULD preserve where applicable:

* tracker keyboard behaviour;
* focus rules;
* Machine View concepts;
* routing interaction;
* parameter/editor windows;
* terminology;
* desktop-window behaviour;
* established Psycle workflow.

C-Psycle's X11 UI is donor/reference material, not final UI authority.

## 21. SOURCE HYGIENE

When editing upstream-derived source, agents MUST:

* preserve headers;
* preserve attribution;
* preserve meaningful comments;
* minimize formatting churn;
* maintain visible third-party boundaries;
* maintain donor/source distinctions;
* explain compatibility changes;
* avoid silent semantic changes.

Agents MUST NOT:

* normalize an entire file to satisfy local style;
* reorder code without purpose during a functional fix;
* rename historical symbols solely for aesthetics;
* remove apparently unused historical structures without evidence;
* replace terminology merely because it is old.

## 22. GENERATED/BINARY MATERIAL

Agents MUST NOT commit generated binaries unless repository policy explicitly requires them.

Agents MUST NOT commit redistributability-restricted reference executables.

Agents MUST prefer reproducible scripts, hashes, manifests, and receipts over checked-in generated evidence when practical.

Temporary reference downloads MUST be removed or excluded as required by repository policy.

## 23. DOCUMENTATION CONSISTENCY

When a change modifies:

* source identity;
* provenance;
* evidence semantics;
* baseline status;
* compatibility classification;
* phase completion;
* build procedure;
* quarantine policy;

agents MUST identify all documentation and validators whose claims depend on that change.

Agents MUST NOT leave machine-readable and human-readable claims knowingly inconsistent.

Agents MUST NOT update historical records to claim events that did not occur.

## 24. ROADMAP DISCIPLINE

`ROADMAP.md` defines current project sequencing.

Agents MUST inspect current roadmap state before implementing a "next phase."

Agents MUST NOT skip ahead merely because later work is more interesting.

Agents MUST satisfy the current phase exit conditions or explicitly identify why sequencing should change.

Completed preservation work MUST remain valid input to later phases.

A later architecture decision MUST NOT erase earlier preserved evidence.

## 25. SECURITY FIXES

Security, crash-containment, and undefined-behaviour fixes are allowed even when historical behaviour was unsafe.

Agents SHOULD preserve compatible observable behaviour around the fix where practical.

Agents MUST document intentional compatibility impact when a safe implementation cannot reproduce historical behaviour.

Historical crashes are not compatibility requirements.

## 26. FAILURE BEHAVIOUR

On uncertainty:

DO_NOT:

* invent provenance
* invent historical behaviour
* invent test results
* invent hashes
* invent reviewer conclusions
* invent successful execution
* weaken evidence gates
* silently choose a compatibility authority

DO:

* preserve UNKNOWN
* retain evidence
* report exact uncertainty
* isolate the unresolved boundary
* continue only with work that does not depend on the unknown claim

## 27. AGENT EXECUTION LOOP

For every substantive task:

STEP_01: IDENTIFY_CURRENT_HEAD
STEP_02: READ_RELEVANT_POLICY
STEP_03: CLASSIFY_SOURCE_ROLE
STEP_04: CLASSIFY_PROVENANCE_RISK
STEP_05: IDENTIFY_BEHAVIOURAL_AUTHORITY
STEP_06: IDENTIFY_EXISTING_REGRESSIONS
STEP_07: REPRODUCE_CURRENT_STATE_WHERE_PRACTICAL
STEP_08: DESIGN_SMALLEST_VALID_CHANGE
STEP_09: IMPLEMENT_WITH_MINIMAL_CHURN
STEP_10: RUN_FOCUSED_TESTS
STEP_11: RUN_RELEVANT_BROADER_TESTS
STEP_12: CHECK_PROVENANCE_AND_IDENTITY_INVARIANTS
STEP_13: INSPECT_DIFF_FOR_UNRELATED_CHANGES
STEP_14: INSPECT_CURRENT_HEAD_REVIEW_FEEDBACK
STEP_15: RECORD_REMAINING_UNKNOWNS
STEP_16: REPORT_EXACTLY_WHAT_WAS_AND_WAS_NOT_VERIFIED

## 28. VIGILANT MODE

VIGILANT_MODE: ALWAYS_ENABLED_FOR_REPOSITORY_MUTATION

Vigilant mode requires agents to account for:

* preservation value;
* historical continuity;
* provenance;
* irreversibility;
* behavioural compatibility;
* licensing;
* evidence integrity;
* CI coverage;
* regression risk.

Technical sophistication is NOT the sole criterion for caution.

Historical or preservation-sensitive material MUST receive the same change discipline as security-critical or technically complex material.

When convenience conflicts with preservation integrity:

DEFAULT_DECISION: PRESERVE_INTEGRITY

When cleanup conflicts with provenance clarity:

DEFAULT_DECISION: PRESERVE_PROVENANCE

When modernization conflicts with demonstrated compatibility:

DEFAULT_DECISION: PRESERVE_COMPATIBILITY

When evidence is incomplete:

DEFAULT_STATUS: UNKNOWN

## 29. DEFINITION_OF_DONE

A change is NOT complete merely because it compiles.

A change is NOT complete merely because CI is green.

A change is complete only when all applicable conditions are satisfied:

* scope is controlled;
* provenance remains valid;
* frozen identities remain truthful;
* relevant behaviour is tested;
* evidence semantics remain valid;
* CI coverage includes affected inputs;
* no unrelated historical churn was introduced;
* review findings on current head are addressed or explicitly rebutted;
* documentation claims remain truthful;
* unresolved uncertainty remains explicit;
* repository history and preservation value remain intact.

END_OF_AGENT_POLICY
