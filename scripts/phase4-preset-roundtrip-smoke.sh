#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-preset-roundtrip}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"
BIN="$OUT/phase4-preset-roundtrip"
LOG="$OUT/phase4-preset-roundtrip.log"
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
    "$ROOT/tests/phase4_preset_roundtrip.c" -o "$BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$BIN" "$OUT" >"$LOG" 2>&1

PARAMS="$OUT/phase4-parameter-presets.prs"
STATE="$OUT/phase4-state-presets.prs"
[ -s "$PARAMS" ] || { echo "Parameter preset file missing" >&2; exit 1; }
[ -s "$STATE" ] || { echo "Opaque-state preset file missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 4 Preset Round Trip

- Version-1 preset file save: PASS
- Version-1 preset file load: PASS
- Multiple named presets: PASS
- Integer parameter preservation: PASS
- Opaque plugin-state size preservation: PASS
- Opaque plugin-state byte preservation: PASS
- Independent saved-file reload: PASS
EOF

cat "$LOG"
cat "$SUMMARY"
