# Upstream Import Omissions

The PSYCLE-LINUX Phase 1 baseline is derived from SourceForge C-Psycle SVN revision `r12005`.

The upstream export contains **2,415 files**. After **66 documented whole-file omissions**, the public baseline retains **2,349 upstream paths**; two of those paths contain independently written provenance-safe replacements rather than the imported source expressions. Phase 1 also adds fourteen licensing/provenance files that were not present in the r12005 C-Psycle export:

- `cpsycle/ui/src/scintilla/License.txt`
- `cpsycle/audio/src/LADSPA-LICENSE.txt`
- `cpsycle/luascripts/psycle/SERPENT-LICENSE.txt`
- `cpsycle/audio/src/EXS24-RENOISE-LICENSE.txt`
- `cpsycle/luascripts/psycle/socket/LICENSE`
- `cpsycle/plugins/zephod_super_fm/ZEPHOD-SUPERFM-LICENSE-PROVENANCE.txt`
- `cpsycle/detail/SDCC-STRLWR-LICENSE.txt`
- `cpsycle/driver/wasapi/PORTAUDIO-LICENSE.txt`
- `cpsycle/file/src/LIBXML2-COPYRIGHT.txt`
- `cpsycle/build-systems/mswindows-installer/INNOTOOLS-DOWNLOADER-LICENSE.txt`
- `cpsycle/luascripts/psycle/MOBDEBUG-LICENSE.txt`
- `cpsycle/dsp/src/FFT-SOURCES-NOTICE.txt`
- `cpsycle/dsp/src/MODPLUG-FILTER-PROVENANCE.txt`
- `cpsycle/audio/src/SAMPULSE-PROVENANCE.txt`

The resulting canonical `cpsycle/` tree therefore contains **2,363 files**. The musl MIT permission grant is restored inside the existing `cpsycle/detail/strcasestr.h` file and therefore does not alter the file count.

Phase 1 avoids broad source modernization or refactoring. The only source-expression replacements are `luascripts/psycle/orderedtable.lua` and `container/src/qsort.c`, plus the aggregate plugin makefile edit required to stop building the omitted M3 component; all other changes are omissions or licensing/provenance metadata.

## Baseline Record

- Upstream revision: `r12005`
- Upstream identity: SourceForge SVN `r12005` plus the recorded ZIP SHA-256
- Sanitized archival import ref: `archive/cpsycle-r12005-sanitized-import`
- Canonical audited baseline tag: `cpsycle-r12005-baseline`
- Upstream ZIP SHA-256 used for the initial audit: `2f70d86e64ab8be3755cf449fa5dc757e3c005d8aecd59f3890f2d222089dabc`
- Upstream file count before exclusions: 2,415
- Raw vendor import count after the original 44 exclusions: 2,371 upstream files
- Retained upstream paths after all 66 whole-file exclusions: 2,349
- Added licensing/provenance files: 14
- Canonical public-baseline count under `cpsycle/`: 2,363 files

The exact upstream provenance anchor is SourceForge SVN `r12005` plus the recorded ZIP SHA-256. The public `archive/cpsycle-r12005-sanitized-import` ref contains only the 2,349 retained upstream paths after all 66 whole-file omissions and the two provenance-safe source replacements; it deliberately does not retain the unsanitized bootstrap tree. The canonical audited tag adds the Phase 1 licensing/provenance records described below.

## Steinberg ASIO SDK 2.3

The entire upstream directory below is intentionally omitted:

```text
driver/asiodriver/asio-2/
```

This accounts for 41 files in r12005.

The directory contains Steinberg ASIO SDK 2.3 documentation, sample code, headers and host/driver examples. The licensing agreement bundled with that SDK states that the licensed SDK itself may not be sold, licensed, given away, or distributed for use as a software development kit.

PSYCLE-LINUX therefore does not republish those files.

The Psycle-owned or otherwise separately attributed ASIO driver material located outside `asio-2/` remains in the imported source tree so the historical r12005 structure is still understandable. ASIO is Windows-specific and is not required for the native Linux port.

## Legacy VST2 SDK-Derived Headers

The following three r12005 files are also omitted:

```text
audio/src/aeffect.h
audio/src/aeffectx.h
audio/src/vstfxstore.h
```

Their headers identify them as based on or originating from Steinberg VST Plug-Ins SDK 2.4 material. Because legacy VST2 SDK redistribution rights are not being assumed, these files are not mirrored in the public baseline.

This omission is deliberate and does not change the Phase 1 objective. VST2 hosting is not required for the first native Linux host and is tracked as a separate preservation/licensing question in Phase 6.

## 7-Zip Prebuilt Windows Binaries

The r12005 tree also contains prebuilt 7-Zip Windows object code without the corresponding 7-Zip source tree in this repository:

```text
external-packages/7za/7z.exe
external-packages/7za/7z.dll
```

Those two binaries are omitted from the canonical public baseline rather than redistributing LGPL-covered object code without the corresponding source or an equivalent source-access mechanism.

The historical 7-Zip documentation and license files remain because they document what the upstream bundle contained:

```text
external-packages/7za/7-zip.chm
external-packages/7za/License.txt
external-packages/7za/copying.txt
external-packages/7za/readme.txt
```

The omitted binaries are not required for the native Linux port.

## Bundled `.psy` Songs Pending Redistribution Clearance

Eight r12005 song/example files exist in the pinned upstream source but are omitted from all public archival and canonical refs because the repository does not establish redistribution permission for all compositions and embedded sample material:

```text
doc/Demosong - Oldstyle.psy
doc/Demosong - Sampulsedemo.psy
doc/Example - classic sounds demo.psy
doc/Example - mixerdemo.psy
doc/Psycle Tutorial - Dynamics_NewYorkCompressor.psy
plugins/jme_blitzn/src/songs/Rm-Im_in_a_place_i_dont_belong.psy
plugins/jme_blitzn/src/songs/voskomo_-_hawkeye_loader.psy
plugins/jme_gamefxn/src/songs/example.psy
```

These omissions do **not** abandon `.psy` compatibility. Phase 4 still requires legacy song loading and round-trip testing, but canonical regression fixtures must be created from material with clear redistribution permission or restored if permission for these historical examples is established.

## MAKK M3 Plugin Source Omitted

The complete r12005 M3 component is omitted because the source identifies MAKK's original Buzz M3 machine but does not carry a sufficiently clear redistribution grant for the adapted whole-machine source:

```text
plugins/m3/m3.vcxproj
plugins/m3/m3.vcxproj.filters
plugins/m3/src/m3.cpp
plugins/m3/src/makefile
plugins/m3/src/track.cpp
plugins/m3/src/track.hpp
```

The aggregate `plugins/makefile` is narrowly adjusted to remove `m3/src` from `PLUGDIRS`.

## TinyBoxes / 404 RGB-Derived Skin Omitted

The skin readme identifies TinyBoxes as based on 404's RGB skin without recording permission to redistribute or modify that artwork. These six paths are omitted:

```text
skins/Readme.txt
skins/ksh-tinyboxes-jazzed.psv
skins/tinyboxes-jazzed/tinyboxes-jazzed.bmp
skins/tinyboxes-jazzed/tinyboxes-jazzed.psm
skins/tinyboxes-jazzed/tinyboxes-jazzedheader.bmp
skins/tinyboxes-jazzed/tinyboxes-jazzedheader.psh
```

## Provenance-Safe Source Replacements

Two r12005 paths remain present, but their imported source expressions are not redistributed:

- `luascripts/psycle/orderedtable.lua` is replaced with an independently written GPL-2.0-or-later API-compatible module.
- `container/src/qsort.c` is replaced with an independently written GPL-2.0-or-later heap-sort implementation behind the same `psy_qsort` API.

These replacements do not change the file-count arithmetic.

## Exact ASIO SDK Paths Omitted

```text
driver/asiodriver/asio-2/ASIO SDK 2.3.pdf
driver/asiodriver/asio-2/Steinberg ASIO Licensing Agreement.pdf
driver/asiodriver/asio-2/asio/asio.dsw
driver/asiodriver/asio-2/asio/asio.opt
driver/asiodriver/asio-2/changes.txt
driver/asiodriver/asio-2/common/asio.cpp
driver/asiodriver/asio-2/common/asio.h
driver/asiodriver/asio-2/common/asiodrvr.cpp
driver/asiodriver/asio-2/common/asiodrvr.h
driver/asiodriver/asio-2/common/asiosys.h
driver/asiodriver/asio-2/common/combase.cpp
driver/asiodriver/asio-2/common/combase.h
driver/asiodriver/asio-2/common/debugmessage.cpp
driver/asiodriver/asio-2/common/dllentry.cpp
driver/asiodriver/asio-2/common/iasiodrv.h
driver/asiodriver/asio-2/common/register.cpp
driver/asiodriver/asio-2/common/wxdebug.h
driver/asiodriver/asio-2/driver/asiosample/asiosample.def
driver/asiodriver/asio-2/driver/asiosample/asiosample.txt
driver/asiodriver/asio-2/driver/asiosample/asiosample/asiosample.dsp
driver/asiodriver/asio-2/driver/asiosample/asiosample/asiosample.vcproj
driver/asiodriver/asio-2/driver/asiosample/asiosmpl.cpp
driver/asiodriver/asio-2/driver/asiosample/asiosmpl.h
driver/asiodriver/asio-2/driver/asiosample/macnanosecs.cpp
driver/asiodriver/asio-2/driver/asiosample/mactimer.cpp
driver/asiodriver/asio-2/driver/asiosample/makesamp.cpp
driver/asiodriver/asio-2/driver/asiosample/wintimer.cpp
driver/asiodriver/asio-2/host/ASIOConvertSamples.cpp
driver/asiodriver/asio-2/host/ASIOConvertSamples.h
driver/asiodriver/asio-2/host/asiodrivers.cpp
driver/asiodriver/asio-2/host/asiodrivers.h
driver/asiodriver/asio-2/host/ginclude.h
driver/asiodriver/asio-2/host/mac/asioshlib.cpp
driver/asiodriver/asio-2/host/mac/codefragments.cpp
driver/asiodriver/asio-2/host/mac/codefragments.hpp
driver/asiodriver/asio-2/host/pc/asiolist.cpp
driver/asiodriver/asio-2/host/pc/asiolist.h
driver/asiodriver/asio-2/host/sample/hostsample.cpp
driver/asiodriver/asio-2/host/sample/hostsample.dsp
driver/asiodriver/asio-2/host/sample/hostsample.vcproj
driver/asiodriver/asio-2/readme.txt
```

Together with the three VST2 SDK-derived headers, two 7-Zip binaries, eight `.psy` song/example files, six M3 files, and six TinyBoxes skin files documented above, these paths account for all **66 whole-file upstream omissions**.

## Licensing/Provenance Files Added During Phase 1

Fourteen files are intentionally added after the upstream omissions. They are licensing/provenance metadata only and do not change Psycle functionality.

### Scintilla

The imported Scintilla headers explicitly refer to a `License.txt` that is absent from the r12005 C-Psycle export. Phase 1 restores the corresponding Scintilla/SciTE distribution terms at:

```text
ui/src/scintilla/License.txt
```

### LADSPA

The imported `audio/src/ladspa.h` declares itself LGPL-2.1-or-later but the r12005 C-Psycle export does not place a component-local license association beside that header. Phase 1 adds:

```text
audio/src/LADSPA-LICENSE.txt
```

This file records the component, copyright holders, SPDX-equivalent license expression, the authoritative notice in `ladspa.h`, and where the complete LGPL 2.1 terms can be obtained.

### Serpent

The bundled `luascripts/psycle/serpent.lua` identifies itself as Serpent 0.272 by Paul Kulchenko under the MIT License but does not contain the complete permission grant. Phase 1 adds:

```text
luascripts/psycle/SERPENT-LICENSE.txt
```

### EXS24 For Renoise

The Psycle EXS24 loader identifies itself as derived from Matt Allan's MIT-licensed EXS24 For Renoise implementation. Phase 1 preserves the upstream copyright and permission notice at:

```text
audio/src/EXS24-RENOISE-LICENSE.txt
```

### LuaSocket

The five bundled LuaSocket modules under `luascripts/psycle/socket/` carry Diego Nehab author comments but no package license. Phase 1 adds the LuaSocket 2.0.2 distribution terms at:

```text
luascripts/psycle/socket/LICENSE
```

### MobDebug / RemDebug

```text
luascripts/psycle/MOBDEBUG-LICENSE.txt
```

### Inno Tools Downloader

```text
build-systems/mswindows-installer/INNOTOOLS-DOWNLOADER-LICENSE.txt
```

### libxml2-derived encoding code

```text
file/src/LIBXML2-COPYRIGHT.txt
```

### PortAudio WASAPI-derived code

```text
driver/wasapi/PORTAUDIO-LICENSE.txt
```

### SDCC `strlwr`

```text
detail/SDCC-STRLWR-LICENSE.txt
```

### Zephod SuperFM / Arguru adaptation

```text
plugins/zephod_super_fm/ZEPHOD-SUPERFM-LICENSE-PROVENANCE.txt
```

### FFT source provenance

The mixed-origin FFT implementation in `dsp/src/fft.c` identifies Dominic Mazzoni/Audacity and Schism Tracker source lineages. Phase 1 records those origins and their GPL-2.0-or-later treatment at:

```text
dsp/src/FFT-SOURCES-NOTICE.txt
```

### ModPlug filter provenance

The IT-filter section in `dsp/src/filter.c` explicitly states `Code from Modplug`. Phase 1 records the historical ModPlug/Olivier Lapicque public-domain sound-rendering provenance at:

```text
dsp/src/MODPLUG-FILTER-PROVENANCE.txt
```

### Sampulse / XMSampler provenance

The Sampulse implementation records its Satoshi Fujiwara XMSampler lineage and historical OpenMPT IT-filter provenance at:

```text
audio/src/SAMPULSE-PROVENANCE.txt
```

### musl `strcasestr` compatibility code

The copied musl compatibility implementation in `detail/strcasestr.h` already names Rich Felker and the MIT license but omitted the permission grant after saying the license followed. Phase 1 restores the authoritative MIT permission notice directly inside that existing header; no extra file is added.

## Reproduction

A maintainer can reproduce the canonical audited Phase 1 source tree as follows.

### 1. Export the pinned upstream revision

```bash
svn export -r 12005 https://svn.code.sf.net/p/psycle/code/trunk/cpsycle cpsycle-r12005
```

At this point the tree contains **2,415 upstream files**.

### 2. Apply all 66 documented whole-file upstream omissions

Remove:

- all 41 files under `driver/asiodriver/asio-2/`;
- `audio/src/aeffect.h`;
- `audio/src/aeffectx.h`;
- `audio/src/vstfxstore.h`;
- `external-packages/7za/7z.exe`;
- `external-packages/7za/7z.dll`;
- the eight `.psy` song/example paths listed in **Bundled `.psy` Songs Pending Redistribution Clearance** above;
- all six M3 paths listed above;
- all six TinyBoxes paths listed above.

The resulting tree contains **2,349 retained upstream paths**.

### 3. Apply the provenance-safe source replacements

Replace `luascripts/psycle/orderedtable.lua` and `container/src/qsort.c` with the audited baseline implementations and remove `m3/src` from `plugins/makefile`. The file count remains **2,349**.

### 4. Restore/add the fourteen Phase 1 licensing/provenance files

Add the canonical contents of:

```text
ui/src/scintilla/License.txt
audio/src/LADSPA-LICENSE.txt
luascripts/psycle/SERPENT-LICENSE.txt
audio/src/EXS24-RENOISE-LICENSE.txt
luascripts/psycle/socket/LICENSE
luascripts/psycle/MOBDEBUG-LICENSE.txt
build-systems/mswindows-installer/INNOTOOLS-DOWNLOADER-LICENSE.txt
file/src/LIBXML2-COPYRIGHT.txt
driver/wasapi/PORTAUDIO-LICENSE.txt
detail/SDCC-STRLWR-LICENSE.txt
plugins/zephod_super_fm/ZEPHOD-SUPERFM-LICENSE-PROVENANCE.txt
dsp/src/FFT-SOURCES-NOTICE.txt
dsp/src/MODPLUG-FILTER-PROVENANCE.txt
audio/src/SAMPULSE-PROVENANCE.txt
```

Then restore the MIT permission grant in the existing `detail/strcasestr.h` comment exactly as recorded in the audited baseline.

The resulting canonical `cpsycle/` tree contains **2,363 files**.

### 5. Verify the audited tag

The upstream source is identified by SourceForge SVN `r12005` and the recorded ZIP SHA-256. The public archival comparison ref `archive/cpsycle-r12005-sanitized-import` contains the sanitized 2,349-path source state after all 66 whole-file omissions and the two provenance-safe source replacements.

The tag `cpsycle-r12005-baseline` identifies the canonical audited Phase 1 state described above, before any Linux compatibility fixes are applied.
