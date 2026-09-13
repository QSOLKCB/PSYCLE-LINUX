#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-compressor-state}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

BIN="$OUT/phase5-arguru-compressor-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-compressor.so"
LOG="$OUT/phase5-arguru-compressor-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-compressor/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Compressor shared object was not produced: $PLUGIN" >&2
    exit 1
}

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_arguru_compressor_state.c" -o "$BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$BIN" "$OUT" "$PLUGIN" >"$LOG" 2>&1

PRESET="$OUT/phase5-arguru-compressor.prs"
SONG="$OUT/phase5-arguru-compressor.psy"
[ -s "$PRESET" ] || { echo "Arguru Compressor preset evidence missing" >&2; exit 1; }
[ -s "$SONG" ] || { echo "Arguru Compressor PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Compressor State Round Trip

- Production `PluginCatcher` recognition of retained Linux `.so`: PASS
- Production `MachineFactory` native-plugin instantiation: PASS
- Six non-default machine parameter values: PASS
- Native-machine current-preset capture: PASS
- Version-1 preset save/load through public preset API: PASS
- Reloaded preset reapplied through production machine preset path: PASS
- PSY3 save with real Arguru Compressor machine: PASS
- Fresh `PluginCatcher` + `MachineFactory` PSY3 reload: PASS
- All six parameter values preserved across PSY3 reload: PASS
- Arguru Compressor -> Master topology preserved: PASS

Arguru Compressor's retained implementation has no opaque `GetData` payload;
its persistent state is the six historical `MPF_STATE` parameters. This gate
therefore closes the machine's preset/state and `.psy` reopen requirements
without inventing new plugin state or changing its DSP.
EOF

cat "$LOG"
cat "$SUMMARY"
