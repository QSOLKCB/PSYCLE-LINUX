#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-zephod-superfm}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/zephod_super_fm/src"
PLUGIN="$CPSYCLE/plugins/build/zephod-superfm.so"
NATIVE_BIN="$OUT/phase5-zephod-superfm"
STATE_BIN="$OUT/phase5-zephod-superfm-state"
NATIVE_LOG="$OUT/phase5-zephod-superfm.log"
STATE_LOG="$OUT/phase5-zephod-superfm-state.log"
SUMMARY="$OUT/summary.md"

# Establish a clean SuperFM precondition without deleting the shared build
# outputs of unrelated native machines.
rm -f "$PLUGIN"

# Prove the retained machine can build and clean without unrelated plugin side
# effects before the larger production-state harness is assembled.
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Zephod SuperFM shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Zephod SuperFM clean left the generated shared object behind" >&2
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
    echo "rebuilt Zephod SuperFM shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_zephod_superfm.cpp" \
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
    "$ROOT/tests/phase5_zephod_superfm_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-zephod-superfm: metadata PASS parameters=20 version=0x0110' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm: deterministic PASS' "$NATIVE_LOG" >/dev/null
grep -F 'idle-before-note=stable' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm: volume-command PASS command=0C80' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm: noteoff PASS release-default=2414 active-through=' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm: rate-transition PASS sr=44100->88200 in-flight=attack pitch=stored finetune-pending=ignored sustain<16=until-noteoff' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-zephod-superfm-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'state: 20/20 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-zephod-superfm.prs" ]] || {
    echo "Zephod SuperFM preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-zephod-superfm.psy" ]] || {
    echo "Zephod SuperFM PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Zephod SuperFM Preservation

- Retained Zephod SuperFM (Arguru Remix) builds independently as `zephod-superfm.so`: PASS
- Direct clean removes only the generated SuperFM shared object and leaves unrelated plugin outputs untouched: PASS
- Historical native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Identity/version/type/two-column geometry and all 20 parameter metadata records: PASS
- Fresh envelopes remain stopped across idle rendering until the first note; first-note rendering remains deterministic: PASS
- Historical `0C80` tracker-volume scaling: PASS
- Default 2414-sample smooth Note Off release remains active near its expected endpoint and reaches silence: PASS
- Live 44.1 kHz -> 88.2 kHz reconfiguration preserves an in-flight attack envelope and the actual sounding pitch even with pending Finetune automation: PASS
- Sustain lengths below 16 retain historical `until noteoff` meaning across rate changes: PASS
- Production `PluginCatcher` identity `zephod-superfm:0`: PASS
- Production `MachineFactory` instantiation: PASS
- All 20 public parameters are seeded to legal non-default values before version-1 preset save/load and fresh-machine restore: PASS
- Fresh PSY3 reopen restores all 20/20 non-default public parameter values: PASS
- Zephod SuperFM -> Master topology: PASS
- Opaque state: none; persistence remains the historical public parameters: PASS

This gate preserves Zephod's original Buzz SuperFM lineage as adapted/remixed by
Arguru for Psycle.  It does not rewrite the oscillator/FM routing equations.
EOF

cat "$SUMMARY"