#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-interactive-editing}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-interactive-editing"
LOG="$OUT/phase4-interactive-editing.log"
SUMMARY="$OUT/summary.md"

# Exercise the same core library boundary used by the other Phase 4 workflow
# regressions. The production Machine View and Tracker Grid call into these
# Machines/UndoRedo/InsertCommand paths; X11 rendering itself remains covered
# by the native runtime smoke.
make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

gcc \
    -std=gnu11 \
    -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" \
    -I"$CPSYCLE/driver" \
    -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" \
    -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" \
    -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" \
    $(pkg-config --cflags lua) \
    "$ROOT/tests/phase4_interactive_editing.c" \
    -o "$BIN" \
    -L"$CPSYCLE/thread/src" \
    -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" \
    -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" \
    -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    $(pkg-config --libs lua) \
    -lpthread -ldl -lstdc++ -lcontainer

"$BIN" "$OUT" >"$LOG" 2>&1

PSY="$OUT/phase4-interactive-editing.psy"
[ -s "$PSY" ] || { echo "Generated editing fixture is missing" >&2; exit 1; }
[ "$(head -c 8 "$PSY")" = "PSY3SONG" ] || { echo "Invalid PSY3 header" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 4 Interactive Editing

- Built-in Sampler creation through Machines command path: PASS
- Built-in Mixer creation through Machines command path: PASS
- Sampler → Mixer → Master wiring: PASS
- Wire undo / redo: PASS
- Machine delete with automatic rewire: PASS
- Machine-delete undo and topology restoration: PASS
- Machine View edit-name persistence: PASS
- Machine View position persistence: PASS
- Mute persistence: PASS
- Bypass persistence: PASS
- Panning persistence: PASS
- Machine parameter surface access: PASS
- Tracker note + command insertion through InsertCommand: PASS
- Multi-track/effect-column edit: PASS
- Tracker edit undo / redo: PASS
- PSY3 save and fresh reload: PASS
- Edited machine topology/state survives reload: PASS
- Edited tracker data survives reload: PASS

This is a command/model compatibility gate. Native X11 rendering and event-loop
behaviour remain covered separately by the Phase 3 runtime smoke.
EOF

cat "$LOG"
cat "$SUMMARY"
