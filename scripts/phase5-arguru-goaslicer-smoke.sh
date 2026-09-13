#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-goaslicer}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

ABI_BIN="$OUT/phase5-arguru-goaslicer"
STATE_BIN="$OUT/phase5-arguru-goaslicer-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-goaslicer.so"
ABI_LOG="$OUT/phase5-arguru-goaslicer.log"
STATE_LOG="$OUT/phase5-arguru-goaslicer-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-goaslicer/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Goaslicer shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_arguru_goaslicer.cpp" \
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
    "$ROOT/tests/phase5_arguru_goaslicer_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "$PLUGIN" >"$ABI_LOG" 2>&1
"$STATE_BIN" "$OUT" "$PLUGIN" >"$STATE_LOG" 2>&1

PRESET="$OUT/phase5-arguru-goaslicer.prs"
SONG="$OUT/phase5-arguru-goaslicer.psy"
[ -s "$PRESET" ] || { echo "Arguru Goaslicer preset evidence missing" >&2; exit 1; }
[ -s "$SONG" ] || { echo "Arguru Goaslicer PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Goaslicer Preservation

- Historical Goaslicer source builds as a Linux native-machine `.so`: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Machine identity/version/type metadata: PASS
- Historical `Length` and `Slope` parameter contract: PASS
- Deterministic 44.1 kHz gate/fade-down behaviour: PASS
- `SequencerTick()` gate-release/fade-up behaviour: PASS
- 88.2 kHz sample-rate scaling for Length and Slope: PASS
- Production `PluginCatcher` recognition: PASS
- Production `MachineFactory` instantiation: PASS
- Two-parameter version-1 preset save/load and fresh-machine restore: PASS
- Fresh production PSY3 reopen: PASS
- Goaslicer -> Master topology preservation: PASS

The retained Goaslicer exposes no opaque `GetData` payload. Its persistent state
is the two historical `MPF_STATE` parameters. Timer, mute/fade, and current-volume
members are runtime sequencing/DSP state and are intentionally not given a new
serialization format.
EOF

cat "$ABI_LOG"
cat "$STATE_LOG"
cat "$SUMMARY"
