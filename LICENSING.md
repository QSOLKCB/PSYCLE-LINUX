# Licensing Policy

## Repository License

The current PSYCLE-LINUX repository scaffolding and original project material are licensed under the **Apache License 2.0**, as stated in the repository's root [LICENSE](LICENSE) file, unless a file or directory states otherwise.

## Upstream Psycle / C-Psycle Licensing

The selected `r12005-trunk-cpsycle` source archive contains its own `COPYING` file with the **GNU General Public License, version 2** and contains upstream copyright and third-party attribution notices.

The Apache-2.0 license in this new repository does **not** override, replace, or relicense upstream source code.

When upstream Psycle/C-Psycle code is imported, modified, or redistributed, its applicable licensing terms and notices must remain in force.

## Third-Party Components

The r12005 source tree includes bundled or referenced third-party code and libraries. Different components may carry different licenses or attribution requirements.

Before a distributable source import or binary release, the project must:

- inventory bundled third-party components;
- preserve per-file and per-directory notices;
- identify code that is merely referenced as a system dependency versus code copied into the tree;
- avoid assuming that one top-level license applies to every historical file;
- document any component that cannot be redistributed in the intended package;
- avoid importing proprietary SDK material without a clear redistribution right.

## VST2

Legacy VST2 support must receive a separate licensing and source-provenance review before SDK files, headers, binaries, or derived material are added to PSYCLE-LINUX.

Historical compatibility is important, but it does not justify importing material whose redistribution terms are unclear.

## Future Source Layout

When the upstream source is imported, PSYCLE-LINUX should keep licensing boundaries obvious. Depending on the results of the source audit, this may include:

- preserving the upstream `COPYING` file with the imported source;
- adding a `THIRD_PARTY_NOTICES.md` or equivalent inventory;
- retaining component-specific license files in place;
- adding SPDX identifiers to newly written files where appropriate without rewriting historical headers merely for cosmetics.

## Contributions

New contributions must not silently remove or weaken existing license and attribution notices.

Contributors should clearly identify newly introduced third-party code, generated code, vendored dependencies, or copied examples in their pull requests.

## Practical Rule

If the origin or license of a file is unclear, **do not relicense it by assumption**. Preserve it, document the uncertainty, and resolve provenance before distribution.

This document records project policy and is not a substitute for legal advice.
