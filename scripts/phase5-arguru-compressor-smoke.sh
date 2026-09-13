#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-arguru-compressor}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

BIN="$OUT/phase5-arguru-compressor"
PLUGIN="$CPSYCLE/plugins/build/arguru-compressor.so"
LOG="$OUT/phase5-arguru-compressor.log"
SUMMARY="$OUT/summary.md"

# Build only the retained helper libraries needed by this native machine, then
# build the actual historical Arguru Compressor source with its existing makefile.
make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/plugins/arguru-compressor/src"

[ -s "$PLUGIN" ] || {
    echo "Arguru Compressor shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_arguru_compressor.cpp" \
    -ldl -o "$BIN"

"$BIN" "$PLUGIN" >"$LOG" 2>&1

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5 Arguru Compressor Preservation

- Historical Arguru Compressor source builds as a Linux native-machine `.so`: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Machine identity/version/type metadata: PASS
- Six historical parameter names/ranges/defaults: PASS
- Native-machine instantiation/destruction: PASS
- Ratio-bypass unity DSP: PASS
- Deterministic 2x input-gain DSP: PASS

This is the first Phase 5 preservation slice. It deliberately freezes retained
behaviour without rewriting the compressor equations. Preset/state and full
`.psy` save/reload coverage remain separate acceptance work before the machine
itself is marked completely preserved.
EOF

cat "$LOG"
cat "$SUMMARY"
