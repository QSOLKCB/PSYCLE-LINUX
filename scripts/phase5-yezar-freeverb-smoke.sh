#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-yezar-freeverb}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/yezar_freeverb/src"
PLUGIN="$CPSYCLE/plugins/build/arguru-freeverb.so"
NATIVE_BIN="$OUT/phase5-yezar-freeverb"
STATE_BIN="$OUT/phase5-yezar-freeverb-state"
NATIVE_LOG="$OUT/phase5-yezar-freeverb.log"
STATE_LOG="$OUT/phase5-yezar-freeverb-state.log"
SUMMARY="$OUT/summary.md"

# Freeverb links against Psycle's retained DSP library, so build that donor
# first, then establish a clean module-specific precondition without touching
# any unrelated plugin outputs.
make -C "$CPSYCLE/dsp/src"
rm -f "$PLUGIN"

make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Jezar Freeverb shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Jezar Freeverb clean left the generated shared object behind" >&2
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
    echo "rebuilt Jezar Freeverb shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_yezar_freeverb.cpp" \
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
    "$ROOT/tests/phase5_yezar_freeverb_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-yezar-freeverb: metadata PASS parameters=5 version=0x0110 identity=Jezar-Freeverb' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-yezar-freeverb: dry-unity PASS dry=320 wet=0' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-yezar-freeverb: wet-timing PASS sr=44100 first-left=1116 first-right=1139 amplitude=0.09' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-yezar-freeverb: rate-transition PASS sr=44100->88200 advanced=257 first-left=2232 first-right=2278 network=reinitialized' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-yezar-freeverb: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-yezar-freeverb-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: arguru-freeverb:0' "$STATE_LOG" >/dev/null
grep -F 'state: 5/5 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'topology: Jezar Freeverb -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-yezar-freeverb.prs" ]] || {
    echo "Jezar Freeverb preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-yezar-freeverb.psy" ]] || {
    echo "Jezar Freeverb PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Yezar/Jezar Freeverb Preservation

- Retained `yezar_freeverb` source builds independently as historical module `arguru-freeverb.so`: PASS
- Direct clean removes only the generated Freeverb module: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Jezar Freeverb` / `Freeverb` / `Jezar`, version `0x0110`, effect type and two-column geometry: PASS
- All five public parameter metadata records, including retained `Absortion` spelling: PASS
- Dry=320 / Wet=0 exact unity behavior: PASS
- Wet-only full-width stereo comb timing at 44.1 kHz (1116 left / 1139 right): PASS
- Live 44.1 -> 88.2 kHz transition after advancing the network by 257 samples rebuilds/mutes from cursor zero and preserves scaled first-comb timing at 2232 left / 2278 right: PASS
- Production `PluginCatcher` identity `arguru-freeverb:0`: PASS
- Production `MachineFactory` instantiation: PASS
- All 5/5 public parameters seeded to legal non-default values before version-1 preset save/load: PASS
- Fresh PSY3 reopen restores all five non-default values: PASS
- Jezar Freeverb -> Master topology: PASS
- Opaque state: none; persistence remains the historical five public parameters: PASS

This gate preserves the retained public-domain Jezar Freeverb implementation and
its Psycle/Arguru module identity. Sample-rate buffer reallocation now restarts the
newly allocated/muted delay-line cursors without changing the Freeverb DSP equations.
EOF

cat "$SUMMARY"
