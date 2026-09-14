#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-jm-drum}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

ABI_BIN="$OUT/phase5-jm-drum"
STATE_BIN="$OUT/phase5-jm-drum-state"
PLUGIN="$CPSYCLE/plugins/build/jmdrum.so"
ABI_LOG="$OUT/phase5-jm-drum.log"
STATE_LOG="$OUT/phase5-jm-drum-state.log"
SUMMARY="$OUT/summary.md"

# Prove the retained JM target is independently buildable from a checkout with
# no pre-created shared output directory, and that its direct clean removes the
# actual loadable native-machine object.
rm -rf "$CPSYCLE/plugins/build"
make -C "$CPSYCLE/plugins/jm_drums/src"
[[ -s "$PLUGIN" ]] || {
    echo "JM Drum standalone build did not create $PLUGIN" >&2
    exit 1
}
make -C "$CPSYCLE/plugins/jm_drums/src" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "JM Drum clean target left stale shared object: $PLUGIN" >&2
    exit 1
}

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$CPSYCLE/plugins/jm_drums/src"

[[ -s "$PLUGIN" ]] || {
    echo "JM Drum shared object was not produced: $PLUGIN" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_jm_drum.cpp" \
    -ldl -o "$ABI_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_jm_drum_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

stdbuf -o0 -e0 "$ABI_BIN" "$PLUGIN" 2>&1 | tee "$ABI_LOG"
EXPECTED_ABI=(
    'phase5-jm-drum: metadata PASS parameters=16 version=0x0250'
    'phase5-jm-drum: volume-command PASS command=0C80 scale=0.5'
    'phase5-jm-drum: noteoff PASS release=256 tail=silent'
    'phase5-jm-drum: rate-transition PASS sr=44100->88200'
    'phase5-jm-drum: PASS'
)
for marker in "${EXPECTED_ABI[@]}"; do
    grep -Fqx "$marker" "$ABI_LOG" || {
        echo "JM Drum ABI/DSP marker missing: $marker" >&2
        exit 1
    }
done
grep -Eq '^phase5-jm-drum: deterministic PASS rms=[0-9]+\.[0-9]+$' "$ABI_LOG" || {
    echo "JM Drum deterministic render marker missing" >&2
    exit 1
}

stdbuf -o0 -e0 "$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"
grep -Fqx 'phase5-jm-drum-state: PASS' "$STATE_LOG" || {
    echo "JM Drum production state completion marker missing" >&2
    exit 1
}
grep -Fqx 'state: 16 parameters, 0 opaque bytes' "$STATE_LOG" || {
    echo "JM Drum production state geometry marker missing" >&2
    exit 1
}

PRESET="$OUT/phase5-jm-drum.prs"
SONG="$OUT/phase5-jm-drum.psy"
[[ -s "$PRESET" ]] || { echo "JM Drum preset evidence missing" >&2; exit 1; }
[[ -s "$SONG" ]] || { echo "JM Drum PSY3 evidence missing" >&2; exit 1; }

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C JM Drum Preservation

- Retained JAZ JM Drum v2.5 builds independently as a Linux native-machine `.so`: PASS
- Direct JM Drum clean removes the loadable shared object: PASS
- Psycle native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Generator identity/version/type and four-column geometry: PASS
- Complete historical 16-parameter metadata contract: PASS
- Fresh default instances produce deterministic active stereo-equal drum synthesis: PASS
- Historical `0Cxx` note-volume command behavior: PASS
- Historical 256-sample Note Off release reaches silence: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` reinitialization matches a fresh target-rate instance: PASS
- Production `PluginCatcher` identity `jmdrum:0`: PASS
- Production `MachineFactory` instantiation: PASS
- Complete 16-parameter version-1 preset save/load and fresh-machine restore: PASS
- Fresh production PSY3 reopen: PASS
- JM Drum -> Master topology preservation: PASS
- Opaque plugin state: none; persistent state remains the 16 historical `MPF_STATE` parameters: PASS

This completes only the JM-machine slice of Phase 5C. JME, Zephod, Yezar, DW,
STK-derived and remaining classic native families stay open for their own gates.
EOF
cat "$SUMMARY"
