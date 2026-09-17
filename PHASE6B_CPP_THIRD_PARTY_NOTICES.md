# Phase 6B C++ retained-source notices

## Scope

This record accompanies the first sanitized C++ engine/player baseline derived from SourceForge SVN `r12005`. It does not replace notice text embedded in the retained files and does not broaden the Phase 6B selection boundary.

The six retained component trees remain identified by the frozen aggregate manifest SHA-256:

`00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a`

The separately required historical build metadata is pinned from the same SVN revision with its own content identities:

- `build-systems/qmake`: 12 files, manifest SHA-256 `cb5b84840ae1380601464d5be279c6747917cf6ccb68d7b08ba450f3dfe57229`;
- `build-systems/src`: 11 files, manifest SHA-256 `7a9c0e434281d20dcd201a5814ee7b6a3d740fab6218ec95662da528da99d91c`.

## Psycle-authored retained material

The retained `universalis`, `psycle-core`, `psycle-audiodrivers`, `psycle-helpers`, `psycle-player`, qmake metadata and build-system support contain Psycle project notices identifying GPL-2-or-later terms. Those notices are preserved in the imported source rather than rewritten here.

The first baseline deliberately excludes/quarantines the nonessential provenance boundaries documented in `PHASE6_CPP_IMPORT_AUDIT.md`, including the Seib VST subtree, the Windows ASIO subtree, closed-source plugin DLLs, unresolved `.psy` songs and the Steinberg-derived `gmnames.h` file.

## Component-local third-party associations

### LADSPA API header

Path:

`psycle-core/src/psycle/core/ladspa.h`

The retained header identifies Linux Audio Developer's Simple Plugin API 1.1, credits Richard W.E. Furse, Paul Barton-Davis and Psycledelics Westerfeld, and contains the GNU Lesser General Public License version 2.1-or-later notice. The complete notice remains in the source file.

### SSE math implementation

Path:

`psycle-helpers/src/psycle/helpers/math/sse_mathfun.h`

The retained file credits Julien Pommier, describes its Cephes/Intel Approximate Math lineage and contains the complete zlib-license permission and disclaimer text. That notice must remain unaltered in source distributions.

### MT19937 / Mersenne Twister

Paths:

- `psycle-helpers/src/psycle/helpers/mersennetwister.hpp`
- `psycle-helpers/src/psycle/helpers/mersennetwister.cpp`

The retained header credits Makoto Matsumoto and Takuji Nishimura and contains the original three-condition redistribution permission and disclaimer for MT19937. The header also records the Psycle C++ adaptation and later 64-bit compatibility work. The complete original notice remains in the source; this component-local record also makes the association visible for binary/distribution documentation.

### FFT implementation lineage

Paths:

- `psycle-helpers/src/psycle/helpers/fft.cpp`
- `psycle-helpers/src/psycle/helpers/fft.hpp`

The retained implementation records two FFT lineages: the function-based implementation attributed to Dominic Mazzoni, with portions based on Don Cross, and a class-based implementation derived from Schism Tracker sources. The historical file does not present a standalone replacement license block for that lineage. Phase 6B therefore preserves the original attribution text verbatim and makes no broader relicensing claim in this notice record.

## Build-system support

The imported `build-systems/qmake` and `build-systems/src` trees are limited to the exact files needed by the historical qmake projects. Their Psycle GPL-2-or-later notices are retained verbatim. They are tracked with independent frozen manifests so adding build support cannot silently change the six-component engine/player source identity.

## Boundary rule

This notice record documents the material actually admitted to the Phase 6B baseline. It is not permission to pull additional files from the upstream plugin, VST, ASIO, song or binary trees. Any expansion of the retained source set requires a new provenance review and a new machine-checkable receipt.
