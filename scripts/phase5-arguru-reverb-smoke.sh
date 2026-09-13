#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-reverb}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

ABI_BIN="$OUT/phase5-arguru-reverb"
STATE_BIN="$OUT/phase5-arguru-reverb-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-reverb.so"
ABI_LOG="$OUT/phase5-arguru-reverb.log"
STATE_LOG="$OUT/phase5-arguru-reverb-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-reverb/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Reverb shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_arguru_reverb.cpp" \
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
    "$ROOT/tests/phase5_arguru_reverb_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "$PLUGIN" >"$ABI_LOG" 2>&1
"$STATE_BIN" "$OUT" "$PLUGIN" >"$STATE_LOG" 2>&1

PRESET="$OUT/phase5-arguru-reverb.prs"
SONG="$OUT/phase5-arguru-reverb.psy"
[ -s "$PRESET" ] || { echo "Arguru Reverb preset evidence missing" >&2; exit 1; }
[ -s "$SONG" ] || { echo "Arguru Reverb PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Reverb Preservation

- Historical Reverb source builds as a Linux native-machine `.so`: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Machine identity/version/type metadata: PASS
- Historical eight-parameter contract: PASS
- Dry=100%, Wet=0% exact unity path: PASS
- 44.1 kHz wet-only stereo pre-delay impulse response: PASS
- `SequencerTick()` 88.2 kHz delay/cutoff reinitialization: PASS
- Production `PluginCatcher` recognition: PASS
- Production `MachineFactory` instantiation: PASS
- Eight-parameter version-1 preset save/load and fresh-machine restore: PASS
- Fresh production PSY3 reopen: PASS
- Arguru Reverb -> Master topology preservation: PASS

The retained Arguru Reverb exposes no opaque `GetData` payload. Its persistent
state is the eight historical `MPF_STATE` parameters. Comb/all-pass delay buffers,
low-pass history, and sample-rate bookkeeping remain runtime DSP state and are
intentionally not given a new serialization format.
EOF

cat "$ABI_LOG"
cat "$STATE_LOG"
cat "$SUMMARY"
