#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-render-bounce-sampler}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-render-bounce-sampler"
LOG="$OUT/phase4-render-bounce-sampler.log"
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
    "$ROOT/tests/phase4_render_bounce_sampler.c" \
    -o "$BIN" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

"$BIN" "$OUT" >"$LOG" 2>&1

SOURCE_WAV="$OUT/phase4-render-source.wav"
RENDER_WAV="$OUT/phase4-rendered-bounce.wav"
PSY="$OUT/phase4-rendered-bounce.psy"

[ -s "$SOURCE_WAV" ] || { echo "Generated source WAV is missing" >&2; exit 1; }
[ -s "$RENDER_WAV" ] || { echo "Psycle-rendered WAV is missing" >&2; exit 1; }
[ -s "$PSY" ] || { echo "Bounce PSY3 fixture is missing" >&2; exit 1; }
[ "$(head -c 8 "$PSY")" = "PSY3SONG" ] || { echo "Invalid bounce PSY3 header" >&2; exit 1; }

python3 - "$RENDER_WAV" <<'PY'
import os
import struct
import sys
import wave

path = sys.argv[1]
with wave.open(path, "rb") as wav:
    assert wav.getnchannels() == 2, wav.getnchannels()
    assert wav.getsampwidth() == 2, wav.getsampwidth()
    assert wav.getframerate() == 44100, wav.getframerate()
    frames = wav.getnframes()
    assert frames > 0
    payload = wav.readframes(frames)
    assert len(payload) == frames * 4
    samples = struct.unpack("<" + "h" * (len(payload) // 2), payload)
    assert max(abs(v) for v in samples) >= 100

assert os.path.getsize(path) == 44 + frames * 4
print(f"python-wave-validation: PASS frames={frames}")
PY

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 4 Render → WAV → Sampler

- Project-owned deterministic source WAV generation: PASS
- Source WAV import through Psycle historical WAV song path: PASS
- Player startup without optional MIDI configuration: PASS
- Real Psycle Player playback through FileOutDriver: PASS
- RIFF/WAVE header and data-size integrity: PASS
- FileOut data size derived from actual payload bytes: PASS
- Render duration bounded to one final FileOut block: PASS
- 44.1 kHz stereo 16-bit PCM render format: PASS
- Non-silent rendered PCM: PASS
- Psycle-rendered WAV re-import to built-in Sampler: PASS
- Full rendered-WAV ↔ Sampler PCM equality: PASS
- Rendered frame/channel/sample-rate preservation on import: PASS
- Bounce song PSY3 save: PASS
- Fresh PSY3 reload of bounced Sampler song: PASS
- Full bounced PCM preservation across PSY3 reload: PASS
- Phase 4 roadmap evidence recorded for sequencer/transport and render/bounce: PASS

This is the automated historical bounce-to-sampler acceptance slice. UI file-dialog
interaction and physical audio-device rendering remain separate runtime targets.
EOF

cat "$LOG"
cat "$SUMMARY"
