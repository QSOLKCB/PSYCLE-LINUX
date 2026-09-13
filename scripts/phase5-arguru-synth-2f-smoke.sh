#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-synth-2f}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

ABI_BIN="$OUT/phase5-arguru-synth-2f"
STATE_BIN="$OUT/phase5-arguru-synth-2f-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-synth-2f.so"
ABI_LOG="$OUT/phase5-arguru-synth-2f.log"
STATE_LOG="$OUT/phase5-arguru-synth-2f-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-synth-2f/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Synth 2f shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_arguru_synth_2f.cpp" \
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
    "$ROOT/tests/phase5_arguru_synth_2f_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "$PLUGIN" 2>&1 | tee "$ABI_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

PRESET="$OUT/phase5-arguru-synth-2f.prs"
SONG="$OUT/phase5-arguru-synth-2f.psy"
[ -s "$PRESET" ] || { echo "Arguru Synth 2f preset evidence missing" >&2; exit 1; }
[ -s "$SONG" ] || { echo "Arguru Synth 2f PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Synth 2f Preservation

- Historical Synth 2f source builds as a Linux native-machine `.so`: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Generator identity/version/type metadata: PASS
- Historical 28-parameter contract: PASS
- Deterministic fixed-waveform A4 rendering at 44.1 kHz: PASS
- Minimum-release Note Off reaches silence: PASS
- `SequencerTick()` 88.2 kHz wavetable/sample-rate reinitialization keeps A4 in tune: PASS
- Production `PluginCatcher` recognition: PASS
- Production `MachineFactory` instantiation: PASS
- 28-parameter version-1 preset save/load and fresh-machine restore: PASS
- Fresh production PSY3 reopen: PASS
- Arguru Synth 2f -> Master topology preservation: PASS

The retained Arguru Synth 2f exposes no opaque `GetData` payload. Its persistent
state is the 28 historical `MPF_STATE` parameters. Active voices, oscillator
phase, envelopes, filter/LFO state, arpeggiator counters and generated wavetables
remain runtime synthesis state and are intentionally not given a new serialization
format.
EOF

cat "$SUMMARY"
