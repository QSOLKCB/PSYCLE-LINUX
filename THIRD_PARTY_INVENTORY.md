# Third-Party and Historical Component Inventory

This inventory records notable bundled third-party material in the C-Psycle r12005 baseline imported under `cpsycle/`.

It is an engineering/provenance inventory, not a replacement for the license text and copyright notices shipped with each component. Component-local notices remain authoritative.

## Baseline Scope

- Upstream revision: SourceForge SVN `r12005`
- Upstream identity: SourceForge SVN `r12005` plus the recorded ZIP SHA-256
- Sanitized archival import ref: `archive/cpsycle-r12005-sanitized-import`
- Canonical audited baseline tag: `cpsycle-r12005-baseline`
- Upstream r12005 files: 2,415
- Final retained upstream files: 2,349
- Intentionally omitted upstream files: 66
- Phase 1 licensing/provenance files added: 14
- Canonical files under `cpsycle/`: 2,363

The canonical Phase 1 baseline identity is recorded in [PROVENANCE.md](PROVENANCE.md). See [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md) for the exact exclusion and reproduction policy.

## Primary Upstream License

### C-Psycle

- Location: `cpsycle/`
- Upstream notice: `cpsycle/COPYING`
- Recorded license: GNU General Public License version 2
- Attribution: `cpsycle/AUTHORS` plus per-file and per-component notices

The repository-root Apache-2.0 license applies to PSYCLE-LINUX original project material unless stated otherwise. It does not relicense imported C-Psycle code.

## Vendored Libraries and Packages

### Lua 5.4.0

- Location: `cpsycle/lua54/`
- Version recorded by upstream: 5.4.0
- License: MIT
- License/documentation location: `cpsycle/lua54/doc/readme.html`
- Status: vendored source

### zlib 1.2.11

- Location: `cpsycle/zlib/`
- Version recorded by upstream: 1.2.11
- License: zlib license
- Notice location: `cpsycle/zlib/README`
- Status: vendored source, including upstream `contrib/` material with its own notices where applicable

### Scintilla headers

- Location: `cpsycle/ui/src/scintilla/`
- Imported files: `include/Scintilla.h`, `include/SciLexer.h`, `include/Sci_Position.h`
- Copyright notices:
  - `Scintilla.h` and `SciLexer.h`: Neil Hodgson, 1998-2003
  - `Sci_Position.h`: Neil Hodgson, 2015
- License: permissive Scintilla/SciTE license
- License location: `cpsycle/ui/src/scintilla/License.txt`
- Status: third-party interface headers retained from upstream; missing license file restored during Phase 1

The r12005 C-Psycle snapshot included all three Scintilla headers but omitted the `License.txt` file they explicitly reference. `Sci_Position.h` carries its own 2015 copyright notice while referring to the same named `License.txt` distribution terms. Phase 1 restores the Scintilla/SciTE license file alongside the headers and records the mixed header provenance instead of treating all three files as 1998-2003 material.

The restored `License.txt` retains the Scintilla/SciTE license text headed `Copyright 1998-2003 by Neil Hodgson`; later Scintilla distributions that include the 2015 `Sci_Position.h` continue to use this same license text. The restoration therefore supplies the license file named by the 2015 header without rewriting either copyright notice.

### LADSPA 1.1 API header

- Location: `cpsycle/audio/src/ladspa.h`
- Component: Linux Audio Developer's Simple Plugin API Version 1.1
- Copyright notice: Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, Psycledelics Westerfeld
- License: GNU Lesser General Public License version 2.1 or later (`LGPL-2.1-or-later`)
- Component-local license association: `cpsycle/audio/src/LADSPA-LICENSE.txt`
- Status: third-party API header retained from upstream

The license declaration is retained verbatim at the top of `ladspa.h`. Phase 1 adds `LADSPA-LICENSE.txt` beside the header so recipients can unambiguously associate this component with LGPL-2.1-or-later and locate the complete LGPL 2.1 terms.

### 7-Zip historical bundle metadata

- Location: `cpsycle/external-packages/7za/`
- Retained public-baseline files: `7-zip.chm`, `License.txt`, `copying.txt`, `readme.txt`
- Upstream bundle also contained: `7z.exe`, `7z.dll`
- License notice: `cpsycle/external-packages/7za/License.txt`
- Recorded terms: LGPL-2.1-or-later for relevant 7-Zip code; `7z.dll` additionally carried the documented unRAR restriction upstream
- Status: historical Windows-era documentation/license material only; executable/object-code binaries are intentionally omitted from the public baseline

The r12005 snapshot did not bundle the corresponding 7-Zip source tree alongside `7z.exe` and `7z.dll`. Rather than redistribute LGPL-covered object code without corresponding source or an equivalent source-access mechanism, PSYCLE-LINUX omits both binaries. See [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md).

### Synthesis ToolKit (STK) 4.5.0 archive

- Location: `cpsycle/external-packages/stk-4.5.0/stk-4.5.0.tar.bz2`
- Version: 4.5.0
- Authors: Perry R. Cook and Gary P. Scavone
- License: permissive STK license included inside the upstream archive
- Status: vendored source archive; the Linux build also documents `libstk-dev` as a system dependency

The compressed STK archive is retained as part of the r12005 baseline. Phase 2 should determine whether it is actually needed when building on current Linux.

### MT19937 / Mersenne Twister implementation

- Location: `cpsycle/dsp/src/mersennetwister.c`
- Component: MT19937 Mersenne Twister pseudorandom number generator
- Original authors: Makoto Matsumoto and Takuji Nishimura
- Copyright: Copyright (C) 1997-2002, Makoto Matsumoto and Takuji Nishimura
- License: three-clause BSD-style redistribution terms embedded in `mersennetwister.c`
- Psycle modifications recorded by the source: C++ adaptation by D. W. Aley; 64-bit compatibility and thread-safety changes by Johan Boule
- Status: third-party implementation retained from upstream

The full redistribution conditions and disclaimer are preserved at the top of `mersennetwister.c`. In particular, binary redistribution requires the copyright notice, conditions, and disclaimer to be reproduced in the documentation and/or other materials supplied with the binary. Future PSYCLE-LINUX binary packaging must therefore carry this MT19937 notice in its third-party notices or equivalent accompanying documentation.

### Serpent 0.272

- Location: `cpsycle/luascripts/psycle/serpent.lua`
- Component: Serpent Lua serializer and pretty printer, version 0.272
- Author: Paul Kulchenko
- Bundled copyright notice: Copyright (c) 2012-2013 Paul Kulchenko
- License: MIT
- License location: `cpsycle/luascripts/psycle/SERPENT-LICENSE.txt`
- Status: third-party Lua module retained from upstream C-Psycle

The bundled file labels itself MIT-licensed but did not carry the permission grant. Phase 1 restores the MIT terms at the component boundary, preserving the 2012-2013 copyright period stated by the bundled version.

### SSE mathfun

- Location: `cpsycle/dsp/src/sse_mathfun.h`
- Component: SIMD SSE1/MMX/SSE2 implementations of sin, cos, exp and log
- Author: Julien Pommier
- Copyright: Copyright (C) 2007 Julien Pommier
- License: zlib license, embedded verbatim in `sse_mathfun.h`
- Consumer in this tree: `cpsycle/dsp/src/sinc-sse2.c`
- Status: third-party DSP implementation retained from upstream

This is a distinct component from the separately vendored `cpsycle/zlib/` package. Its source notice must remain intact; the embedded terms state that the origin must not be misrepresented, altered source must be marked, and the notice may not be removed from source distributions.

### musl `strcasestr` compatibility implementation

- Location: `cpsycle/detail/strcasestr.h`
- Upstream: musl / cited BlankOn musl mirror
- Copyright: Copyright © 2005-2012 Rich Felker
- License: MIT
- Status: copied compatibility implementation used for Microsoft builds

The r12005 header named the musl MIT license and copyright but omitted the permission grant after saying the license followed. Phase 1 restores the authoritative MIT permission notice directly in the header so the copied implementation carries both required notices.

### EXS24 For Renoise-derived loader logic

- Location: `cpsycle/audio/src/exs24loader.c`
- Upstream project: Matt Allan's EXS24 For Renoise
- Upstream copyright notice: Copyright (c) 2018 Matt Allan and all contributors
- License: MIT
- Preserved upstream notice: `cpsycle/audio/src/EXS24-RENOISE-LICENSE.txt`
- Status: Psycle GPL source containing logic identified by Psycle as derived from the MIT-licensed upstream implementation

The surrounding Psycle GPL declaration does not replace the upstream MIT notice. Phase 1 therefore preserves Matt Allan's copyright and permission grant alongside the derived loader.

### LuaSocket 2.0.2 Lua modules

- Location: `cpsycle/luascripts/psycle/socket/`
- Bundled modules: `headers.lua`, `http.lua`, `ltn12.lua`, `socket.lua`, `url.lua`
- Author: Diego Nehab
- Copyright: Copyright © 2004-2007 Diego Nehab
- License: MIT
- License location: `cpsycle/luascripts/psycle/socket/LICENSE`
- Upstream version reference: LuaSocket v2.0.2
- Status: third-party runtime Lua modules retained from upstream C-Psycle

The bundled `socket.lua` matches the LuaSocket 2.0.2 helper module lineage. Phase 1 restores the corresponding v2.0.2 package license so the five-module bundle has explicit distribution terms.

### MobDebug 0.5362 / RemDebug

- Location: `cpsycle/luascripts/psycle/mobdebug.lua`
- Component: MobDebug 0.5362, based on RemDebug 1.0
- Bundled attribution: Paul Kulchenko (2011-13); Kepler Project (2005)
- License: MIT-style grants for MobDebug and RemDebug
- License location: `cpsycle/luascripts/psycle/MOBDEBUG-LICENSE.txt`
- Status: third-party Lua debugger retained from upstream C-Psycle

The component-local file preserves the authoritative MobDebug and RemDebug MIT permission grants and records the bundled version's attribution separately from the upstream license text.

### Inno Tools Downloader 0.3.5

- Location: `cpsycle/build-systems/mswindows-installer/it_download.iss`
- Copyright: Sherlock Software 2008
- License: WTFPL version 2
- License location: `cpsycle/build-systems/mswindows-installer/INNOTOOLS-DOWNLOADER-LICENSE.txt`
- Status: historical Windows installer helper; not required by the Linux runtime

The upstream InnoTools-Downloader repository supplies the redistribution grant preserved beside the imported script.

### libxml2-derived encoding routines

- Location: `cpsycle/file/src/encoding.c`
- Upstream: libxml2 `encoding.c`, Daniel Veillard; original ISO Latin-1 / UTF-16 work also attributed to Martin J. Duerst by the Psycle source
- License: permissive libxml2 license
- License location: `cpsycle/file/src/LIBXML2-COPYRIGHT.txt`
- Status: copied/modified routines retained in the Psycle file layer

The component-local notice restores the libxml2 copyright and permission grant referenced by the imported source while retaining Psycle's documented `intptr_t` modification.

### PortAudio WASAPI-derived portions

- Location: `cpsycle/driver/wasapi/wasapi.cpp`
- Bundled attribution: PortAudio WASAPI implementation; Copyright (c) 2006-2010 David Viens, Dmitry Kostjuchenko
- License: PortAudio permissive license
- License location: `cpsycle/driver/wasapi/PORTAUDIO-LICENSE.txt`
- Status: copied portions in the historical Windows WASAPI backend

The PortAudio permission grant is preserved beside the WASAPI source; the surrounding Psycle license does not replace that third-party notice.

### SDCC `strlwr` implementation

- Location: `cpsycle/detail/portable.h` (`psy_strlwr`)
- Original author: Vangelis Rokas, 2004
- Upstream: SDCC string library `strlwr.c`
- License: GNU Library General Public License version 2 or later (`LGPL-2.0-or-later`)
- License boundary: `cpsycle/detail/SDCC-STRLWR-LICENSE.txt`
- Status: copied compatibility implementation retained inline

The original SDCC notice remains in `portable.h`; the component-local record now ships the complete GNU Library General Public License version 2 text offline and identifies the authoritative upstream lineage for future source/binary packaging.

### Zephod SuperFM / Arguru adaptation

- Location: `cpsycle/plugins/zephod_super_fm/`
- Source provenance: original Buzz SuperFM code by Zephod; remixed/adapted by Arguru
- Upstream Psycle license classification: `Open source, Public Domain` for the Arguru plugin set explicitly including Zephod SuperFM
- Provenance record: `cpsycle/plugins/zephod_super_fm/ZEPHOD-SUPERFM-LICENSE-PROVENANCE.txt`
- Status: native Psycle machine retained and buildable by the upstream plugin makefile

The provenance record preserves the upstream Psycle licensing statement beside the source without erasing Zephod's or Arguru's historical attribution.

### Thunder Palace / Graue SoftSat

- Location: `cpsycle/plugins/graue_softsat/src/softsat.cpp`
- Copyright: Copyright (c) 2005 Thunder Palace Entertainment
- Author attribution: Catatonic Porpoise (`graue@oceanbase.org`)
- License: permissive in-source grant requiring the copyright and permission notices in copies; alternatively GPL version 2 or later
- Additional provenance: source states it is based on Arguru's Distortion machine, released to the public domain
- Status: native Psycle effect retained from upstream

The complete operative permission/warranty notice is already embedded at the top of `softsat.cpp`; no separate license file is needed.

### GVerb

- Location: `cpsycle/plugins/gverb/src/`
- Copyright: Copyright (C) 1999 Juhana Sadeharju
- License: GPL version 2 or later
- Notice location: `cpsycle/plugins/gverb/src/gverb.cpp`
- Status: third-party reverb implementation retained from upstream

The complete GPL notice is embedded in the source and `cpsycle/COPYING` supplies the repository's GPL version 2 text.

### MoreAmp EQ / PCM time-domain equalizer

- Location: `cpsycle/plugins/moreamp_eq/src/moreamp_eq.cpp`
- MoreAmp copyright: Copyright (C) 2004-2005 pmisteli
- PCM time-domain equalizer copyright: Copyright (C) 2002-2006 Felipe Rivera
- License: both embedded notices specify GPL version 2 or later
- Status: third-party EQ implementations retained inside the Psycle plugin

Both upstream copyright/license notices remain embedded in `moreamp_eq.cpp`; `cpsycle/COPYING` supplies the GPL version 2 text.

### Mixed-origin FFT implementation

- Location: `cpsycle/dsp/src/fft.c`
- Recorded origins: Dominic Mazzoni/Audacity; Schism Tracker; Don Cross attribution inherited through the Mazzoni source
- License treatment for retained Mazzoni and Schism-derived portions: GPL-2.0-or-later
- Provenance record: `cpsycle/dsp/src/FFT-SOURCES-NOTICE.txt`
- Status: third-party DSP implementation retained from upstream C-Psycle

The local provenance record makes the copied source origins and license treatment explicit without changing FFT behaviour.

### ModPlug-derived IT filter

- Location: `cpsycle/dsp/src/filter.c`
- Source marker: `Code from Modplug`
- Historical upstream: Olivier Lapicque / ModPlug sound-rendering code
- Recorded redistribution status: public domain in historical libmodplug documentation
- Provenance record: `cpsycle/dsp/src/MODPLUG-FILTER-PROVENANCE.txt`
- Status: copied filter logic retained with explicit provenance

The provenance file is narrowly scoped to the ModPlug-derived filter section and makes no broader claim about later OpenMPT code.

### Sampulse / XMSampler lineage

- Location: `cpsycle/audio/src/xmsampler*.c`, `cpsycle/audio/src/xmsampler*.h`
- Psycle license: GPL-2.0-or-later, retained in the imported XMSampler source/header notices
- Upstream attribution: partially based on Satoshi Fujiwara's `XMSampler`; IT filtering attributed to OpenMPT
- Upstream author record: `cpsycle/AUTHORS` names Satoshi Fujiwara among external source authors used by Psycle
- Provenance record: `cpsycle/audio/src/SAMPULSE-PROVENANCE.txt`
- Status: core Sampulse sampler implementation retained from upstream

The provenance record preserves the Satoshi Fujiwara lineage, the historical OpenMPT GPL-2.0-or-later filtering lineage, and the Psycle GPL terms carried by this implementation without changing sampler behaviour.

### Mrs. Brisby ZIP reader/writer

- Location: `cpsycle/file/src/zipreader.c`, `zipreader.h`, `zipwriter.c`, `zipwriter.h`
- Copyright: Copyright (c) 2007 Mrs. Brisby <mrs.brisby@nimh.org>
- License: GPL-2.0-or-later
- Notice location: retained directly in the ZIP source files
- Status: compiled into the Psycle file library

The complete component notice remains in-source and `cpsycle/COPYING` supplies GPL version 2.

### Jezar / Dreampoint Freeverb

- Location: `cpsycle/plugins/yezar_freeverb/src/`
- Original author: Jezar at Dreampoint, June 2000
- License/provenance: the retained source explicitly states `This code is public domain`
- Notice location: `cpsycle/plugins/yezar_freeverb/src/RevModel.cpp` and related upstream Freeverb source lineage
- Status: native Psycle reverb implementation retained from upstream

### Tom St Denis biquad / Robert Bristow-Johnson lineage

- Location: `cpsycle/plugins/surround/src/biquad.hpp`
- Implementation: Tom St Denis, based on Robert Bristow-Johnson's audio EQ biquad formulae
- License/provenance: the retained header explicitly places the implementation in the public domain for commercial, free, and educational use
- Status: third-party filter implementation used by the Surround plugin

The public-domain grant and original attribution remain embedded in `biquad.hpp`.

### Danny Smith `stdint.h` compatibility implementation

- Location: `cpsycle/detail/stdint.h`
- Contributor: Danny Smith
- Original date: 2000-12-02
- License/provenance: retained header states `THIS SOFTWARE IS NOT COPYRIGHTED` and offers the source for public-domain use, modification and distribution
- Status: compatibility header retained with its public-domain grant and subsequent modification notes

### Ordered-table compatibility module (Phase 1 replacement)

- Location: `cpsycle/luascripts/psycle/orderedtable.lua`
- r12005 provenance: imported path pointed to the lua-users OrderedTableSimple page but carried no author/copyright/license grant
- Phase 1 action: copied expression replaced with an independently written API-compatible implementation under GPL-2.0-or-later
- Preserved API: `new`, `hidden`, `ipairs`, `pairs`, `opairs`, `del`

### Container sort implementation (Phase 1 replacement)

- Location: `cpsycle/container/src/qsort.c`
- r12005 provenance: imported implementation described itself as based on a Kernighan & Ritchie qsort example without carrying separate source terms
- Phase 1 action: replaced with an independently written in-place heap-sort implementation under GPL-2.0-or-later
- Preserved API: `psy_qsort` and the callback interface declared by `qsort.h`

## Component-Specific Notices

### Ionic icon material

- Notice: `cpsycle/host/src/resources/LICENSE_ICONS`
- License: MIT
- Status: UI resource attribution retained from upstream

### Audacity-derived plugin material

The following plugin directories include Audacity/GPLv2 license notices:

- `cpsycle/plugins/phaser/`
- `cpsycle/plugins/compressor/`
- `cpsycle/plugins/wahwah/`

Their local `LICENSE.txt` files are retained and should remain authoritative for those components.

### JME plugin families

The following imported plugin source directories carry local GPLv2 license files:

- `cpsycle/plugins/jme_blitz12/src/`
- `cpsycle/plugins/jme_blitzn/src/`
- `cpsycle/plugins/jme_gamefx13/src/`
- `cpsycle/plugins/jme_gamefxn/src/`

## Material Not Mirrored in the Canonical Public Baseline

### Steinberg ASIO SDK 2.3

The SourceForge r12005 snapshot includes `driver/asiodriver/asio-2/`. The bundled Steinberg ASIO SDK 2.3 licensing agreement states that the SDK itself may not be given away or distributed as a software development kit.

PSYCLE-LINUX therefore does **not** mirror that SDK directory. The surrounding Psycle ASIO driver source outside the SDK directory remains in the baseline for provenance, but ASIO is Windows-specific and is not required for the Linux port.

### Legacy Steinberg VST2 SDK-derived headers

Three r12005 files identify themselves as based on or originating from the Steinberg VST Plug-Ins SDK 2.4:

- `audio/src/aeffect.h`
- `audio/src/aeffectx.h`
- `audio/src/vstfxstore.h`

They are omitted from the public PSYCLE-LINUX baseline pending a clear redistribution basis. Legacy VST2 hosting is not required for the initial Linux port and is already tracked separately in the roadmap.

### 7-Zip prebuilt binaries

The following r12005 files are omitted because the upstream C-Psycle bundle does not include their corresponding 7-Zip source tree or another same-place source-access mechanism:

- `external-packages/7za/7z.exe`
- `external-packages/7za/7z.dll`

See [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md) for exact paths, counts, rationale, and reproduction steps.

### Bundled `.psy` song/example files

Eight r12005 `.psy` files with unresolved composition/sample redistribution provenance are **not** mirrored in the canonical Phase 1 public baseline or any public archival ref. Their upstream paths remain documented in [UPSTREAM_OMISSIONS.md](UPSTREAM_OMISSIONS.md), and the exact upstream revision remains independently identifiable via SourceForge r12005 plus the recorded ZIP SHA-256.

This is a redistribution-boundary decision, not a compatibility decision: `.psy` loading remains a core project goal, using cleared or newly created regression fixtures.

### MAKK M3 Buzz-derived machine

The six r12005 files under `cpsycle/plugins/m3/` are omitted from public refs. The imported source identifies MAKK's original Buzz M3 machine and contains an informal request not to copy the whole machine, but the baseline does not contain a sufficiently clear redistribution grant for the adapted source. The Linux aggregate plugin makefile is adjusted only to stop referencing the omitted directory.

### TinyBoxes / 404 RGB-derived skin

The r12005 `skins/Readme.txt` states that TinyBoxes is based on 404's RGB skin, but no permission record for redistribution/modification of the underlying artwork is present. The readme, `ksh-tinyboxes-jazzed.psv`, and four files under `skins/tinyboxes-jazzed/` are omitted from all public refs.

## Generated, Binary, and Build-Only Material

The r12005 tree also contains historical artifacts that are retained for baseline fidelity but should not automatically become Linux dependencies.

Notable examples:

- `cpsycle/host/src/resources/resource.aps` - Visual Studio resource-editor generated/binary artifact;
- `cpsycle/external-packages/7za/7-zip.chm` - Windows compiled help retained as historical documentation;
- `cpsycle/zlib/contrib/dotzlib/DotZLib.chm` - compiled Windows help;
- Visual Studio `.sln`, `.vcxproj`, `.vcproj`, `.filters`, `.dsw`, `.dsp`, `.mak` and related project files - historical/build-platform metadata;
- installer resources under `cpsycle/build-systems/mswindows-installer/` - Windows packaging assets;
- `.prs` files under `presets/` - Psycle preset banks;
- image, icon, skin and splash resources - runtime/UI assets, not source generators.

The two prebuilt 7-Zip binaries previously present in the raw vendor import are specifically **not** retained in the canonical Phase 1 baseline; their omission is licensing-compliance cleanup, not generic removal of old Windows files.

## Review Rule

Before deleting, replacing, upgrading, or relicensing any bundled component:

1. identify why it exists in r12005;
2. locate its authoritative local notice;
3. determine whether the Linux build actually uses it;
4. preserve compatibility where the component affects songs, machines, presets, or serialization;
5. keep licensing changes separate from unrelated porting fixes.

The Phase 1 objective is traceability first, cleanup later.
