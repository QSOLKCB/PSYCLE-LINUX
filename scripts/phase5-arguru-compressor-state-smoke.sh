#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-compressor-state}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

BIN="$OUT/phase5-arguru-compressor-state"
PLUGIN="$CPSYCLE/plugins/build/arguru-compressor.so"
LOG="$OUT/phase5-arguru-compressor-state.log"
SUMMARY="$OUT/summary.md"

# Build Psycle's production native-machine wrapper/song I/O plus the retained
# Arguru Compressor shared object. This intentionally exercises the real host
# serialization path instead of a test-only plugin-state format.
make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/arguru-compressor/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Compressor shared object was not produced: $PLUGIN" >&2
    exit 1
}

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_arguru_compressor_state.c" -o "$BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$BIN" "$OUT" "$PLUGIN" >"$LOG" 2>&1

PRESET="$OUT/arguru-compressor-state.prs"
FIRST_SONG="$OUT/arguru-compressor-first.psy"
ROUNDTRIP_SONG="$OUT/arguru-compressor-roundtrip.psy"
for artifact in "$PRESET" "$FIRST_SONG" "$ROUNDTRIP_SONG"; do
    [ -s "$artifact" ] || {
        echo "Arguru Compressor preservation artifact missing: $artifact" >&2
        exit 1
    }
done

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Compressor State Preservation

- Production `PluginCatcher` registration and `MachineFactory` creation: PASS
- Six non-default machine parameter values through the host wrapper: PASS
- Captured machine preset through `currentpreset`: PASS
- Version-1 preset file save/load: PASS
- Fresh native-machine restore from the saved preset: PASS
- Explicit zero-byte opaque-state contract: PASS
- PSY3 save with a real Arguru Compressor machine: PASS
- Fresh PSY3 load recreates the native machine rather than a Dummy: PASS
- All six parameter values survive first load: PASS
- Compressor-to-Master wire survives first load: PASS
- Independent second PSY3 save/reload: PASS
- All six parameter values and routing survive the second reload: PASS

Together with the Phase 5 Arguru Compressor ABI/metadata/DSP baseline, this
closes the preservation acceptance loop for this machine without altering its
historical DSP implementation.
EOF

cat "$LOG"
cat "$SUMMARY"
