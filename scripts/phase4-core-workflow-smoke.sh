#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase4-workflow}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

rm -rf "$OUT"
mkdir -p "$OUT"

BIN="$OUT/phase4-core-workflow"
LOG="$OUT/phase4-core-workflow.log"
SUMMARY="$OUT/summary.md"

# Phase 4 builds on the already-audited Phase 3 libraries. Keep this command
# deliberately close to cpsycle/player/src/makefile so it exercises the same
# static-library boundary rather than inventing a parallel test build system.
gcc \
    -std=c11 \
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
    "$ROOT/tests/phase4_core_workflow.c" \
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

for fixture in "$OUT/phase4-first.psy" "$OUT/phase4-roundtrip.psy"; do
    if [ ! -s "$fixture" ]; then
        echo "Missing generated Psycle fixture: $fixture" >&2
        exit 1
    fi
    header="$(head -c 8 "$fixture")"
    if [ "$header" != "PSY3SONG" ]; then
        echo "Unexpected .psy header in $fixture: $header" >&2
        exit 1
    fi
done

cat > "$SUMMARY" <<EOF
# PSYCLE-LINUX Phase 4 Core Workflow Smoke

- Synthetic project-owned test data generation: PASS
- Built-in sampler machine creation: PASS
- Sampler → Master wiring: PASS
- MIDI Note On → Psycle pattern-event translation: PASS
- Pattern insertion: PASS
- PSY3 save: PASS
- Fresh PSY3 load: PASS
- Song metadata round trip: PASS
- Tempo / LPB round trip: PASS
- Machine type and wire round trip: PASS
- MIDI-derived pattern data round trip: PASS
- Second save/load semantic round trip: PASS

Generated test artifacts:

- \`phase4-first.psy\`
- \`phase4-roundtrip.psy\`

These files are generated during the test and are not committed demo songs. They contain only synthetic regression data created by PSYCLE-LINUX code.
EOF

cat "$LOG"
cat "$SUMMARY"
