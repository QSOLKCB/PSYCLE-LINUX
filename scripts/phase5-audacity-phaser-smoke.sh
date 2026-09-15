#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-audacity-phaser}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/phaser/src"
PLUGIN="$CPSYCLE/plugins/build/audacity-phaser.so"
NATIVE_BIN="$OUT/phase5-audacity-phaser"
STATE_BIN="$OUT/phase5-audacity-phaser-state"
NATIVE_LOG="$OUT/phase5-audacity-phaser.log"
STATE_LOG="$OUT/phase5-audacity-phaser-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Audacity Phaser shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Audacity Phaser clean left the generated shared object behind" >&2
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
    echo "rebuilt Audacity Phaser shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_audacity_phaser_preservation.cpp" \
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
    "$ROOT/tests/phase5_audacity_phaser_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-audacity-phaser: metadata PASS version=0x0120 parameters=6 identity=Audacity-Phaser' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: describe PASS lfo=0.4Hz phase=90deg odd-stage=2 mix=50:50 feedback=-0.25' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: zero-block PASS strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: dry PASS mix=0 exact-unity=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: stages PASS requested=3 effective=2 waveform=reference' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: stereo PASS opposed-lfo=yes finite=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: live-rate PASS 44.1->88.2k fresh-reference=yes rate-sensitive=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-phaser-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: audacity-phaser:0' "$STATE_LOG" >/dev/null
grep -F 'state: 6/6 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: Audacity Phaser -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-audacity-phaser.prs" ]] || {
    echo "Audacity Phaser preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-audacity-phaser.psy" ]] || {
    echo "Audacity Phaser PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Audacity Phaser Preservation

- Retained `phaser` source builds independently as `audacity-phaser.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Audacity Phaser` / `APhaser` / `Nasca Octavian Paul/Sartorius`, version `0x0120`, effect type and one-column geometry: PASS
- Complete six-parameter metadata surface and historical value descriptions: PASS
- Zero-length host blocks are strict no-ops instead of entering the historical decrementing `do/while` body: PASS
- Dry/Wet=0 preserves exact stereo unity: PASS
- Historical odd stage requests are coerced to the preceding even stage count in both display and DSP: PASS
- Default opposed stereo LFO phases remain finite and produce channel-separated modulation: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` LFO scaling matches a fresh 88.2 kHz instance and the oracle is rate-sensitive: PASS
- Production `PluginCatcher` identity `audacity-phaser:0` and `MachineFactory` instantiation: PASS
- All 6/6 public parameters are preserved through a version-1 preset restored with an independent catcher/factory: PASS
- Fresh PSY3 reopen restores all six non-default values and the Audacity Phaser -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical six public parameters: PASS

The retained Audacity-derived Phaser remains under its existing GPL provenance.
The preservation fix only makes empty callbacks safe; positive-length DSP equations
and the historical parameter surface are unchanged.
EOF

cat "$SUMMARY"
