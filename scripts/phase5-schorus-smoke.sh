#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-schorus}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/schorus/src"
PLUGIN="$CPSYCLE/plugins/build/s-chorus.so"
NATIVE_BIN="$OUT/phase5-schorus"
STATE_BIN="$OUT/phase5-schorus-state"
NATIVE_LOG="$OUT/phase5-schorus.log"
STATE_LOG="$OUT/phase5-schorus-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "SChorus shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "SChorus clean left the generated shared object behind" >&2
    exit 1
}

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "rebuilt SChorus shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_schorus_preservation.cpp" \
    -ldl -o "$NATIVE_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_schorus_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-schorus: metadata PASS version=0x0100 parameters=8 state=8 identity=SChorus' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: constructor PASS defaults=8 initialized-before-Init=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: describe-about PASS dry=1 wet=0.5 fb=-0.25 delay=5ms rate=25ms/s warning=retained' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: dry PASS mix=dry-only feedback=0 max-block=256 exact-unity=yes bounded=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: oracle PASS default-feedback-trace=8 source-derived=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: delay PASS rate=44100 left-echo=44 right-ring-echo=256 wet-active=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: samplerate PASS sweep-preserved=yes live=44100->88200 left-echo=88 fresh-reference=yes stale44-differs=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-schorus: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-schorus-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: s-chorus:0' "$STATE_LOG"
grep -Fqx 'state: 8/8 MPF_STATE controls seeded non-default dry=16384 wet=24576 fbl=8192 fbr=-8192 min=2 max=7 rate=25 delayer=1025, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'audio: production feedback markers=8 bounded=yes zero-callback=noop' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: SChorus -> Master' "$STATE_LOG"

[[ -s "$OUT/phase5-schorus.prs" ]] || {
    echo "SChorus preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-schorus.psy" ]] || {
    echo "SChorus PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Sartorius SChorus Preservation

- Retained source builds independently as `s-chorus.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Historical identity `SChorus` / `SChorus` / `Sartorius`, version `0x0100`, effect type and four-column geometry: PASS
- Complete eight-parameter `MPF_STATE` surface and raw defaults are frozen: PASS
- Constructor initializes all eight public slots from published defaults before `Init()`: PASS
- Historical value descriptions and About warning are preserved: PASS
- Zero and negative callback counts are strict no-ops: PASS
- Dry-only/zero-feedback processing is exact stereo unity over the 256-sample host maximum with canary boundaries: PASS
- Historical default dry/feedback recurrence is frozen by eight source-derived markers: PASS
- Enabled wet-delay path is active with a 44-sample left echo at 44.1 kHz and 256-sample complementary right ring echo: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` preserves the physical sweep position and matches a fresh 88.2 kHz reference: PASS
- Production `PluginCatcher` identity `s-chorus:0` and `MachineFactory` instantiation: PASS
- All 8/8 state controls persist non-default through independent version-1 preset restore: PASS
- Production feedback path matches eight source-derived markers and remains block-bounded: PASS
- Fresh PSY3 reopen restores all eight values and the `SChorus -> Master` topology edge: PASS
- Opaque state: none; persistence remains the historical eight public parameters: PASS

Production changes are limited to deterministic published-default initialization,
non-positive callback safety, live-rate sweep-position conversion, and standalone
makefile hygiene. Historical positive-count delay/feedback equations are otherwise unchanged.
EOF

cat "$SUMMARY"
