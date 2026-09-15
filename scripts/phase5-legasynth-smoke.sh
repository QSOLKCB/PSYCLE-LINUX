#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-legasynth}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/legasynth/src"
PLUGIN="$CPSYCLE/plugins/build/legasynth-303.so"
NATIVE_BIN="$OUT/phase5-legasynth"
STATE_BIN="$OUT/phase5-legasynth-state"
NATIVE_LOG="$OUT/phase5-legasynth.log"
STATE_LOG="$OUT/phase5-legasynth-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/dsp/src"
rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "LegaSynth shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "LegaSynth clean left the generated shared object behind" >&2
    exit 1
}

make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/audio/src"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "rebuilt LegaSynth shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_legasynth_preservation.cpp" \
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
    "$ROOT/tests/phase5_legasynth_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-legasynth: metadata PASS version=0x0020 slots=28 state=25 labels=3 identity=TB303' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: defaults PASS constructor=published-28-slot-state' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: describe PASS notes/cents lfo=sine/saw/square chorus=off/on delay/lfo/depth/amount' "$NATIVE_LOG"
grep -Fq 'phase5-legasynth: deterministic PASS note=48 rate=44100 rms=' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: velocity PASS command=0C80 scale=64/127' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: nonpositive PASS chorus=on zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: samplerate PASS chorus=on preroll=250ms live=44100->88200 matches=fresh-88200 differs=44100' "$NATIVE_LOG"
grep -Fqx 'phase5-legasynth: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-legasynth-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: legasynth-303:0' "$STATE_LOG"
grep -Fqx 'state: 25 MPF_STATE controls seeded non-default across 28 ABI slots, separators=3-header, chorus=on, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: LegaSynth TB303 -> Master' "$STATE_LOG"

[[ -s "$OUT/phase5-legasynth.prs" ]]
[[ -s "$OUT/phase5-legasynth.psy" ]]

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C LegaSynth TB303 Preservation

- Retained `legasynth` source builds independently as `legasynth-303.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `LegaSynth TB303` / `TB303` / `Juan Linietsky, ported by Sartorius`, version `0x0020`, generator type and two-column geometry: PASS
- Complete 28-slot ABI surface: 25 state controls plus three labeled section headers: PASS
- Constructor storage is seeded from all 28 published defaults before host default callbacks: PASS
- Representative historical descriptions for tuning, LFO waveform and chorus controls: PASS
- Fresh default instances render deterministic active TB303 output: PASS
- Pattern command `0C80` preserves the historical velocity=64/127 scaling: PASS
- Chorus-enabled zero and negative host callback sizes are strict no-ops: PASS
- Chorus-enabled live sample-rate transition after 250 ms pre-roll preserves modulation phase, matches a fresh 88.2 kHz timebase and differs from stale 44.1 kHz synthesis: PASS
- Production `PluginCatcher` identity `legasynth-303:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves all 28 stored slots with 25 non-default state controls: PASS
- Fresh PSY3 reopen preserves all stored values and the LegaSynth TB303 -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical public parameter slots: PASS

Production correctness changes are limited to deterministic published-default `Vals[]`
initialization, deterministic TB303 cutoff/resonance cache initialization, a non-positive
host-block guard before the chorus path, chorus timebase preservation across sample-rate
changes, and standalone makefile output/clean hygiene. Fixed-rate positive-count synthesis,
filter, distortion and chorus equations remain unchanged.
EOF

cat "$SUMMARY"
