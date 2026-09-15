#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-audacity-wahwah}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/wahwah/src"
PLUGIN="$CPSYCLE/plugins/build/wahwah.so"
NATIVE_BIN="$OUT/phase5-audacity-wahwah"
STATE_BIN="$OUT/phase5-audacity-wahwah-state"
NATIVE_LOG="$OUT/phase5-audacity-wahwah.log"
STATE_LOG="$OUT/phase5-audacity-wahwah-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Audacity WahWah shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Audacity WahWah clean left the generated shared object behind" >&2
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
    echo "rebuilt Audacity WahWah shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_audacity_wahwah_preservation.cpp" \
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
    "$ROOT/tests/phase5_audacity_wahwah_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-audacity-wahwah: metadata PASS version=0x0120 parameters=5 identity=Audacity-WahWah' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: describe PASS lfo=1.5Hz phase=90deg depth=70 resonance=2.5 offset=331Hz' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: stereo PASS left=reference right=reference opposed=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: depth-zero PASS stereo-symmetric=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: max-offset PASS guarded-reference=absolute differs-from-1.0=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: live-rate PASS left=fresh88 right=fresh88 both-rate-sensitive=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-wahwah-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: wahwah:0' "$STATE_LOG" >/dev/null
grep -F 'state: 5/5 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: Audacity WahWah -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-audacity-wahwah.prs" ]] || {
    echo "Audacity WahWah preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-audacity-wahwah.psy" ]] || {
    echo "Audacity WahWah PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Audacity WahWah Preservation

- Retained `wahwah` source builds independently as `wahwah.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Audacity WahWah` / `WahWah` / `Nasca Octavian Paul/Sartorius`, version `0x0120`, effect type and one-column geometry: PASS
- Complete five-parameter metadata surface and historical value descriptions: PASS
- Zero and negative host callback counts are independently exercised as strict no-ops before signed sample counts can enter the retained unsigned chunk loop: PASS
- Both left and right default Wah responses match an independent source-derived opposed-LFO reference: PASS
- Depth=0 collapses modulation while preserving symmetric stereo filtering: PASS
- The public max-offset response is required to absolutely match the historical `freqofs=0.9999` reference within a tight per-sample bound, and that oracle is independently distinguishable from raw `1.0`: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` scaling matches a fresh 88.2 kHz instance independently on both channels, and both channel oracles are rate-sensitive: PASS
- Production `PluginCatcher` identity `wahwah:0` and `MachineFactory` instantiation: PASS
- All 5/5 public parameters are preserved through a version-1 preset restored with an independent catcher/factory: PASS
- Fresh PSY3 reopen restores all five non-default values and the Audacity WahWah -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical five public parameters: PASS

The retained Audacity-derived WahWah remains under its existing GPL provenance.
The preservation fix only makes non-positive callbacks explicit no-ops; positive-length
DSP equations and the historical parameter surface are unchanged.
EOF

cat "$SUMMARY"
