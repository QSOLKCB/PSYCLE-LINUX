#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-alk-muter}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/alk_muter/src"
PLUGIN="$CPSYCLE/plugins/build/alk-muter.so"
NATIVE_BIN="$OUT/phase5-alk-muter"
STATE_BIN="$OUT/phase5-alk-muter-state"
NATIVE_LOG="$OUT/phase5-alk-muter.log"
STATE_LOG="$OUT/phase5-alk-muter-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Alk Muter shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Alk Muter clean left the generated shared object behind" >&2
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
    echo "rebuilt Alk Muter shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_alk_muter_preservation.cpp" \
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
    "$ROOT/tests/phase5_alk_muter_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-alk-muter: metadata PASS version=0x0120 parameters=1 identity=Alk-Muter' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: describe PASS off/on' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: unity PASS mute=off' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: mute-44k PASS bounded=yes first-zero=44' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: unmute PASS floor-to-unity' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: live-rate PASS 44.1->88.2k first-zero=22' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-alk-muter-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: alk-muter:0' "$STATE_LOG" >/dev/null
grep -F 'state: 1/1 public parameter seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'topology: Alk Muter -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-alk-muter.prs" ]] || {
    echo "Alk Muter preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-alk-muter.psy" ]] || {
    echo "Alk Muter PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Alk Muter Preservation

- Retained `alk_muter` source builds independently as `alk-muter.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Alk Muter` / `Muter` / `Alk`, version `0x0120`, effect type and one-column geometry: PASS
- Complete one-parameter metadata surface (`Mute`, 0..1, default off, `MPF_STATE`): PASS
- Historical `DescribeValue` strings `off` / `on`: PASS
- Default unmuted DSP remains exact stereo unity: PASS
- 44.1 kHz click-avoiding mute ramp reaches exact zero after the historical 44 nonzero samples: PASS
- The mute transition is strictly bounded to the host-supplied sample count; the historical stale post-decrement path can no longer write one sample beyond the block when the fade completes before the block ends: PASS
- Unmute ramp returns monotonically from the muted floor to exact unity: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` timing update preserves the retained 22-nonzero-sample 88.2 kHz fade behavior: PASS
- Production `PluginCatcher` identity `alk-muter:0` and `MachineFactory` instantiation: PASS
- Version-1 preset round-trip restores the legal non-default Mute=1 state into a fresh machine: PASS
- Fresh PSY3 reopen restores Mute=1 and the Alk Muter -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical one public parameter: PASS
EOF

cat "$SUMMARY"
