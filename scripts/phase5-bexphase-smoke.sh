#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-bexphase}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/bexphase/src"
PLUGIN="$CPSYCLE/plugins/build/bexphase.so"
NATIVE_BIN="$OUT/phase5-bexphase"
STATE_BIN="$OUT/phase5-bexphase-state"
NATIVE_LOG="$OUT/phase5-bexphase.log"
STATE_LOG="$OUT/phase5-bexphase-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "BexPhase shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "BexPhase clean left the generated shared object behind" >&2
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
    echo "rebuilt BexPhase shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_bexphase_preservation.cpp" \
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
    "$ROOT/tests/phase5_bexphase_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-bexphase: metadata PASS version=0x0120 parameters=6 identity=DocBexter-PhaZaR' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase: describe PASS phase=90.00 refresh=Tick-x8 mode=Single' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase: zero-block PASS strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase: default-unity PASS samples=6' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase: differentiator PASS full=zero half=0.5x' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-bexphase-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: bexphase:0' "$STATE_LOG" >/dev/null
grep -F 'state: 6/6 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F "topology: DocBexter'S PhaZaR -> Master" "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-bexphase.prs" ]] || {
    echo "BexPhase preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-bexphase.psy" ]] || {
    echo "BexPhase PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C BexPhase Preservation

- Retained `bexphase` source builds independently as `bexphase.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `DocBexter'S PhaZaR` / `BexPhase!` / `Simon Bucher`, version `0x0120`, effect type and three-column geometry: PASS
- Complete six-parameter metadata surface is frozen: PASS
- Historical value descriptions for phase, mix, refresh, mode, LFO/frequency balance and differentiator are preserved: PASS
- Zero-length host audio blocks are strict no-ops and cannot enter the historical decrementing `do/while` body: PASS
- Retained default r12005 processing is exact stereo unity while the dormant FFT/LFO analysis block remains compiled out: PASS
- Differentiator endpoint behavior is deterministic: full scale cancels the current path to exact zero; half scale produces exact 0.5x output: PASS
- Production `PluginCatcher` identity `bexphase:0` and `MachineFactory` instantiation: PASS
- All 6/6 public parameters are preserved through a version-1 preset restored with an independent catcher/factory: PASS
- Fresh PSY3 reopen restores all six non-default values and the BexPhase -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical six public parameters: PASS

The existing sanitizer-backed fixed-ring regression remains part of the same workflow
and independently proves the 24,576-sample ring cursor stays bounded even when the
refresh-derived request exceeds the physical storage window.
EOF

cat "$SUMMARY"
