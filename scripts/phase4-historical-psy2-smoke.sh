#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-historical-psy2}"
[[ "$OUT" = /* ]] || OUT="$ROOT/$OUT"
rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-historical-psy2"
LOG="$OUT/phase4-historical-psy2.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase4_historical_psy2.c" -o "$BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$BIN" "$OUT" >"$LOG" 2>&1
test -s "$OUT/phase4-historical-psy2.psy"
cat > "$SUMMARY" <<'EOS'
# PSYCLE-LINUX Phase 4 Historical PSY2 Compatibility

- Project-authored PSY2SONG fixture generation: PASS
- Legacy Psycle 1.66-era SongReader dispatch: PASS
- Legacy metadata/BPM/LPB conversion: PASS
- Legacy pattern-event conversion: PASS
- Legacy play-order -> modern sequence conversion: PASS
- Legacy Master -> modern Master-slot remapping: PASS
- Upstream demo/example song content reused: NO
EOS
cat "$LOG"
cat "$SUMMARY"
