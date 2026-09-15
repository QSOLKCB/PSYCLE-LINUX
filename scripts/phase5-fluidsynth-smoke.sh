#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-fluidsynth}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/fluid_synth_sf2_player/src"
PLUGIN="$CPSYCLE/plugins/build/fluidsynth.so"
SF2="${PSYCLE_TEST_SF2:-/usr/share/sounds/sf2/TimGM6mb.sf2}"
NATIVE_BIN="$OUT/phase5-fluidsynth"
STATE_BIN="$OUT/phase5-fluidsynth-state"
NATIVE_LOG="$OUT/phase5-fluidsynth.log"
STATE_LOG="$OUT/phase5-fluidsynth-state.log"
SUMMARY="$OUT/summary.md"

[[ -s "$SF2" ]] || {
    echo "FluidSynth preservation SoundFont missing: $SF2" >&2
    exit 1
}

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "FluidSynth shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "FluidSynth clean left the generated shared object behind" >&2
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
    echo "rebuilt FluidSynth shared object missing" >&2
    exit 1
}

# shellcheck disable=SC2207
FLUID_CFLAGS=($(pkg-config --cflags fluidsynth))
# shellcheck disable=SC2207
FLUID_LIBS=($(pkg-config --libs fluidsynth))

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" "${FLUID_CFLAGS[@]}" \
    "$ROOT/tests/phase5_fluidsynth_preservation.cpp" \
    -ldl "${FLUID_LIBS[@]}" -o "$NATIVE_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase5_fluidsynth_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" "$SF2" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" "$SF2" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-fluidsynth: metadata PASS version=0x0101 slots=24 state=19 labels=3 nulls=2 identity=FluidSynth' "$NATIVE_LOG"
grep -Fqx 'phase5-fluidsynth: constructor PASS defaults=24 initialized-before-Init=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-fluidsynth: state-defaults PASS bytes=5184 damp=35 gain=48 channels=64 null-putdata=safe' "$NATIVE_LOG"
grep -Fq 'phase5-fluidsynth: audio PASS sf2=TimGM6mb note=60 velocity=127 rate=44100 direct-reference=yes ' "$NATIVE_LOG"
grep -Fq 'phase5-fluidsynth: samplerate PASS live=44100->88200 direct-reference=yes ' "$NATIVE_LOG"
grep -Fqx 'phase5-fluidsynth: boundaries PASS nonpositive=noop midi-channel63=safe invalid-track-instrument=safe aux-column=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-fluidsynth: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-fluidsynth-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: fluidsynth:0' "$STATE_LOG"
grep -Fqx 'state: opaque-bytes=5184 version=3 sf2=TimGM6mb channels=64 gain=80 polyphony=64' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: FluidSynth SF2 player -> Master' "$STATE_LOG"

[[ -s "$OUT/phase5-fluidsynth.prs" ]] || {
    echo "FluidSynth preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-fluidsynth.psy" ]] || {
    echo "FluidSynth PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C FluidSynth SF2 Player Preservation

- Retained source builds independently as `fluidsynth.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Historical `FluidSynth SF2 player` / `FluidSynth`, version `0x0101`, generator / three-column identity: PASS
- Complete 24-slot ABI frozen as 19 state controls, 3 labels and 2 null separators: PASS
- Constructor initializes all 24 published parameter defaults before `Init()`: PASS
- Historical opaque `SYNPAR` v3 remains 5184 bytes with 64 channel/instrument records and SoundFont path: PASS
- Reverb Damp default is the published 35 and Global Gain default is the published 48: PASS
- TimGM6mb note-60 rendering at 44.1 kHz matches a direct FluidSynth reference instance: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` updates the actual FluidSynth sample rate and matches a direct 88.2 kHz reference: PASS
- Zero/negative callback counts are strict no-ops; high MIDI channel and invalid tracker indices remain memory-safe: PASS
- Production `PluginCatcher` / `MachineFactory` discovery and `fluidsynth:0` identity: PASS
- Full opaque state including the SF2 path persists through independent version-1 preset restore: PASS
- Fresh PSY3 reopen restores opaque state and the `FluidSynth SF2 player -> Master` topology: PASS

The SoundFont fixture is supplied by Ubuntu's `timgm6mb-soundfont` package during CI and is not committed to this repository.
EOF

cat "$SUMMARY"
