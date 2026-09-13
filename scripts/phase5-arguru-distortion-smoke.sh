#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-distortion}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

ABI_BIN="$OUT/phase5-arguru-distortion"
STATE_BIN="$OUT/phase5-arguru-distortion-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-distortion.so"
ABI_LOG="$OUT/phase5-arguru-distortion.log"
STATE_LOG="$OUT/phase5-arguru-distortion-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-distortion/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Distortion shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_arguru_distortion.cpp" \
    -ldl -o "$ABI_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_arguru_distortion_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "$PLUGIN" >"$ABI_LOG" 2>&1
"$STATE_BIN" "$OUT" "$PLUGIN" >"$STATE_LOG" 2>&1

PRESET="$OUT/phase5-arguru-distortion.prs"
SONG="$OUT/phase5-arguru-distortion.psy"
[ -s "$PRESET" ] || { echo "Arguru Distortion preset evidence missing" >&2; exit 1; }
[ -s "$SONG" ] || { echo "Arguru Distortion PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Distortion Preservation

- Historical Arguru Distortion source builds as a Linux native-machine `.so`: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Machine identity/version/effect classification: PASS
- Four historical parameter names/descriptions/ranges/defaults: PASS
- Hard-clip transfer function: PASS
- Stereo phase-inversion behaviour: PASS
- Stateful saturate-mode response: PASS
- Production `PluginCatcher` recognition and `MachineFactory` instantiation: PASS
- Four non-default `MPF_STATE` parameter values: PASS
- Version-1 preset save/load and fresh-machine preset application: PASS
- Fresh production PSY3 reopen recreates Arguru Distortion: PASS
- All four parameters survive PSY3 reload: PASS
- Arguru Distortion -> Master topology survives PSY3 reload: PASS

The retained Distortion exposes no opaque `GetData` payload. Its persistent state
is the four historical state parameters; the saturator's transient limiter state
remains runtime DSP state and is not given a new serialization format.
EOF

cat "$ABI_LOG"
cat "$STATE_LOG"
cat "$SUMMARY"
