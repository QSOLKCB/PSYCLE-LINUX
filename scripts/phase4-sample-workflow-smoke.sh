#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-sample-workflow}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-sample-workflow"
LOG="$OUT/phase4-sample-workflow.log"
SUMMARY="$OUT/summary.md"

# Build only the library boundary required by the sample/song workflow. The
# full native Linux build remains covered by the existing Phase 2/3 workflow.
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
    "$ROOT/tests/phase4_sample_workflow.c" \
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

WAV="$OUT/phase4-generated-sine.wav"
PSY="$OUT/phase4-sample-workflow.psy"

[ -s "$WAV" ] || { echo "Generated WAV fixture is missing" >&2; exit 1; }
[ -s "$PSY" ] || { echo "Generated PSY3 fixture is missing" >&2; exit 1; }
[ "$(head -c 4 "$WAV")" = "RIFF" ] || { echo "Invalid WAV header" >&2; exit 1; }
[ "$(dd if="$WAV" bs=1 skip=8 count=4 status=none)" = "WAVE" ] || { echo "Invalid WAVE signature" >&2; exit 1; }
[ "$(head -c 8 "$PSY")" = "PSY3SONG" ] || { echo "Invalid PSY3 header" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 4 Audible Sample Workflow

- Project-owned deterministic PCM generation: PASS
- WAV import through Psycle song reader: PASS
- Sample pool population: PASS
- Instrument/sample workflow construction: PASS
- Pattern trigger explicitly selects instrument 0: PASS
- Pattern trigger explicitly targets sampler machine 0: PASS
- Sequencer order 0:0 → pattern 0: PASS
- Built-in sampler creation: PASS
- Sampler → Master wiring: PASS
- PSY3 save with embedded sample: PASS
- Fresh PSY3 reload: PASS
- Embedded sample frame count / rate preservation: PASS
- Embedded PCM frame-by-frame preservation: PASS
- Playable trigger instrument/machine routing after reload: PASS
- Song BPM / LPB / metadata preservation: PASS

The WAV and PSY files are generated test artifacts only. No demo song or third-party audio asset is committed.
EOF

cat "$LOG"
cat "$SUMMARY"
