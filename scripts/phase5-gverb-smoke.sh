#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-gverb}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/gverb/src"
PLUGIN="$CPSYCLE/plugins/build/ladspa-gverb.so"
NATIVE_BIN="$OUT/phase5-gverb"
STATE_BIN="$OUT/phase5-gverb-state"
NATIVE_LOG="$OUT/phase5-gverb.log"
STATE_LOG="$OUT/phase5-gverb-state.log"
SUMMARY="$OUT/summary.md"

# GVerb's retained makefile links the project DSP library.
make -C "$CPSYCLE/dsp/src"
rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "GVerb shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "GVerb clean left the generated shared object behind" >&2
    exit 1
}

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "rebuilt GVerb shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_gverb_preservation.cpp" \
    -ldl -o "$NATIVE_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_gverb_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-gverb: metadata PASS version=0x0110 parameters=8 identity=LADSPA-GVerb' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb: describe PASS room=m revtime=s damping/bandwidth=fraction levels=integer-dB input=mono/stereo' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb: routing PASS mono=averaged-input stereo=dual-engine immediate-diffuser=0.29296875' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb: samplerate PASS first-delay-44100=L489/R461 first-delay-88200=L978/R923' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gverb-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: ladspa-gverb:0' "$STATE_LOG" >/dev/null
grep -F 'state: 8/8 public parameters seeded non-default, input=stereo, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: LADSPA GVerb -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-gverb.prs" ]] || {
    echo "GVerb preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-gverb.psy" ]] || {
    echo "GVerb PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C LADSPA GVerb Preservation

- Retained `gverb` source plus project DSP prerequisite builds as `ladspa-gverb.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `LADSPA GVerb` / `GVerb` / `Juhana Sadeharju/Steve Harris/Sartorius`, version `0x0110`, effect type and one-column geometry: PASS
- Complete eight-parameter metadata surface and historical value descriptions: PASS
- Zero and negative host callback counts are strict no-ops before the retained do/while DSP loop: PASS
- Fresh-state first-sample routing preserves mono averaged-input processing versus stereo dual-engine processing with source-derived immediate diffuser coefficient 0.29296875: PASS
- Source-derived early-reflection timing preserves first delayed outputs at L489/R461 samples at 44.1 kHz and reconstructs them at L978/R923 after a live 88.2 kHz `SequencerTick()` transition: PASS
- Production `PluginCatcher` identity `ladspa-gverb:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves all eight legal non-default public values: PASS
- Fresh PSY3 reopen preserves all eight values and the LADSPA GVerb -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical eight public parameters: PASS

The only production DSP-wrapper change is a guard for non-positive host sample counts.
Positive-count GVerb equations, mono/stereo routing, parameter scaling, engine state,
historical setter path-dependence, and sample-rate reconstruction behavior remain otherwise unchanged.
EOF

cat "$SUMMARY"
