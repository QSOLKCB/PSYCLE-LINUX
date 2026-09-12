#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-sequencer-transport}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-sequencer-transport"
LOG="$OUT/phase4-sequencer-transport.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

COMMON_CFLAGS=(
    -std=gnu11
    -Wall -Wextra -Werror=implicit-function-declaration
    -I"$CPSYCLE/audio/src"
    -I"$CPSYCLE/driver"
    -I"$CPSYCLE/thread/src"
    -I"$CPSYCLE/script/src"
    -I"$CPSYCLE/container/src"
    -I"$CPSYCLE/file/src"
    -I"$CPSYCLE/dsp/src"
    -I"$CPSYCLE/diversalis/src"
)
COMMON_LDFLAGS=(
    -L"$CPSYCLE/thread/src"
    -L"$CPSYCLE/script/src"
    -L"$CPSYCLE/container/src"
    -L"$CPSYCLE/dsp/src"
    -L"$CPSYCLE/audio/src"
    -L"$CPSYCLE/file/src"
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm
    -lpthread -ldl -lstdc++ -lcontainer
)

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase4_sequencer_transport.c" \
    -o "$BIN" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

"$BIN" "$OUT" >"$LOG" 2>&1

PSY="$OUT/phase4-sequencer-transport.psy"
[ -s "$PSY" ] || { echo "Generated sequencer fixture is missing" >&2; exit 1; }
[ "$(head -c 8 "$PSY")" = "PSY3SONG" ] || { echo "Invalid PSY3 header" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 4 Sequencer / Transport

- Sequence order insertion through SequenceInsertCommand: PASS
- Sequence insert undo / redo: PASS
- Sequence removal through SequenceRemoveCommand: PASS
- Sequence remove undo / redo / second undo: PASS
- Multi-sequence-track layout: PASS
- Pattern-length-driven order offsets: PASS
- Sequence duration calculation: PASS
- Play-selection marking / clearing: PASS
- BPM to samples timing: PASS
- LPB / beats-per-line timing: PASS
- Transport position set / deterministic windowed frame advance: PASS
- Transport start / stop: PASS
- Bounded loop range setup: PASS
- Legacy-compatible integer BPM / LPB PSY3 persistence: PASS
- Multi-track sequence PSY3 persistence: PASS
- Fresh-load transport semantics: PASS

This is a command/model timing gate. Native keyboard focus, X11 controls and
physical audio-device transport interaction remain separate runtime targets.
EOF

cat "$LOG"
cat "$SUMMARY"
