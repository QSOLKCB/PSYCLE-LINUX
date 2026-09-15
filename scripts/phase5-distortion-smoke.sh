#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-distortion}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/distortion/src"
PLUGIN="$CPSYCLE/plugins/build/distortion.so"
NATIVE_BIN="$OUT/phase5-distortion"
STATE_BIN="$OUT/phase5-distortion-state"
NATIVE_LOG="$OUT/phase5-distortion.log"
STATE_LOG="$OUT/phase5-distortion-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Dist! Distortion shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Dist! Distortion clean left the generated shared object behind" >&2
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
    echo "rebuilt Dist! Distortion shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_distortion_preservation.cpp" \
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
    "$ROOT/tests/phase5_distortion_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-distortion: metadata PASS version=0x0110 parameters=7 identity=ayeternal-Dist!-Distortion' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion: describe PASS symmetric=no/yes thresholds=+/-normalized gain=dB' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion: asymmetric PASS distinct-positive-negative-clamps stereo=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion: symmetric PASS positive-controls-both-polarities' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-distortion-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: distortion:0' "$STATE_LOG" >/dev/null
grep -F 'state: 7/7 public parameters seeded non-default, symmetric=1, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: ayeternal Dist! Distortion -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-distortion.prs" ]] || {
    echo "Dist! Distortion preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-distortion.psy" ]] || {
    echo "Dist! Distortion PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C ayeternal Dist! Distortion Preservation

- Retained `distortion` source builds independently as `distortion.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `ayeternal Dist! Distortion` / `Dist!ortion` / `bohan`, version `0x0110`, effect type and four-column geometry: PASS
- Complete seven-parameter metadata surface: PASS
- Historical specialized descriptions preserve symmetric mode text, normalized signed thresholds/clamps, and dB gain representation: PASS
- Zero and negative host callback counts are strict no-ops: PASS
- Asymmetric mode preserves distinct positive/negative threshold and clamp controls independently on both stereo channels: PASS
- Symmetric mode preserves the historical rule that the positive threshold/clamp controls both polarities: PASS
- Production `PluginCatcher` identity `distortion:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves all seven legal non-default public values: PASS
- Fresh PSY3 reopen preserves all seven values and the ayeternal Dist! Distortion -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical seven public parameters: PASS

The only production DSP-source change is a guard that makes zero and negative host
sample counts strict no-ops before the retained reverse sample loop. Positive-count
gain, threshold, clamp and symmetry equations are unchanged.
EOF

cat "$SUMMARY"
