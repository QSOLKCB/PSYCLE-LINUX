#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-softsat}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/graue_softsat/src"
PLUGIN="$CPSYCLE/plugins/build/thunderpalace-softsat.so"
NATIVE_BIN="$OUT/phase5-softsat"
STATE_BIN="$OUT/phase5-softsat-state"
NATIVE_LOG="$OUT/phase5-softsat.log"
STATE_LOG="$OUT/phase5-softsat-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "SoftSat shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "SoftSat clean left the generated shared object behind" >&2
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
    echo "rebuilt SoftSat shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_softsat_preservation.cpp" \
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
    "$ROOT/tests/phase5_softsat_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-softsat: metadata PASS version=0x0100 parameters=2 identity=ThunderPalace-SoftSat' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat: describe PASS threshold=host-integer hardness=0.500/1.000' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat: default PASS threshold=32768 hardness=1024 deterministic-host-order odd-symmetric=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat: setter-order PASS hardness-alone=retains-range threshold-retweak=recomputes-range' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-softsat-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: thunderpalace-softsat:0' "$STATE_LOG" >/dev/null
grep -F 'state: 2/2 public parameters seeded non-default, threshold=12000 hardness=1536, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: ThunderPalace SoftSat -> Master' "$STATE_LOG" >/dev/null

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C ThunderPalace SoftSat Preservation

- Retained `graue_softsat` source builds independently as `thunderpalace-softsat.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `ThunderPalace SoftSat` / `SoftSat` / `Catatonic Porpoise`, version `0x0100`, effect type and three-column geometry: PASS
- Complete two-parameter metadata surface and historical Hardness display scaling: PASS
- Constructor coefficients are deterministically seeded from the published Threshold/Hardness defaults before the host's normal Threshold -> Hardness default callbacks: PASS
- Zero and negative host callback counts are strict no-ops before the retained do/while DSP loop: PASS
- Default positive/negative stereo waveshaping remains odd-symmetric and source-derived: PASS
- Historical live setter order remains intact: Hardness changes gradation without retroactively recomputing range; a subsequent Threshold tweak recomputes range from the current Hardness: PASS
- Production `PluginCatcher` identity `thunderpalace-softsat:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves both legal non-default public values: PASS
- Fresh PSY3 reopen preserves both values and the ThunderPalace SoftSat -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical two public parameters: PASS

The production DSP changes are limited to deterministic initialization of the two
private coefficients from published defaults and a guard for non-positive host
sample counts. Positive-count SoftSat waveshaping equations and live setter-order
semantics remain otherwise unchanged.
EOF

cat "$SUMMARY"
