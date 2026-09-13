#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-pooplog-family}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

ABI_BIN="$OUT/phase5-pooplog-family"
STATE_BIN="$OUT/phase5-pooplog-family-state"
ABI_LOG="$OUT/phase5-pooplog-family.log"
STATE_LOG="$OUT/phase5-pooplog-family-state.log"
SUMMARY="$OUT/summary.md"

PLUGIN_DIRS=(
    pooplog-synth
    pooplog-synth-light
    pooplog-synth-ultralight
    pooplog_delay
    pooplog_delay_light
    pooplog_filter
    pooplog_autopan
    pooplog_lofi
    pooplog_scratch
)
PLUGIN_FILES=(
    pooplog-fm-laboratory.so
    pooplog-fm-light.so
    pooplog-fm-ultralight.so
    pooplog-delay.so
    pooplog-delay-light.so
    pooplog-filter.so
    pooplog-autopan.so
    pooplog-lofi-processor.so
    pooplog-scratch-master.so
)
PLUGIN_PATHS=()

EXPECTED_HASH_LINES=(
    "pooplog-metadata-hash[FM Laboratory]=0xd2db9e08a2e098a4"
    "pooplog-metadata-hash[FM Light]=0xc955c42530dc6506"
    "pooplog-metadata-hash[FM UltraLight]=0xf9faacf032cc58f3"
    "pooplog-metadata-hash[Delay]=0xd794adcf1c57390e"
    "pooplog-metadata-hash[Delay Light]=0xdd879bae9f506e07"
    "pooplog-metadata-hash[Filter]=0xec1dd1dbd487381c"
    "pooplog-metadata-hash[Autopan]=0xc100243d6fa938a5"
    "pooplog-metadata-hash[Lofi]=0x87c4d37392e2128a"
    "pooplog-metadata-hash[Scratch]=0xd4eb1c2f76e2e4c9"
)
EXPECTED_OPAQUE_LINES=(
    "pooplog-opaque-hash[FM Laboratory]=0x2b7c633d7a33daae size=1508"
    "pooplog-opaque-hash[FM Light]=0x8f87d50fceda95e4 size=468"
    "pooplog-opaque-hash[FM UltraLight]=0xe3a47f4ea86ba4b5 size=344"
)
EFFECT_LABELS=("Delay" "Delay Light" "Filter" "Autopan" "Lofi" "Scratch")

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

# Lofi and Scratch were missing Linux makefiles before Phase 5B. Prove each new
# target can build directly from a truly absent plugins/build directory, then
# prove its direct clean removes the loadable artifact.
STANDALONE_DIRS=(pooplog_lofi pooplog_scratch)
STANDALONE_FILES=(pooplog-lofi-processor.so pooplog-scratch-master.so)
for i in "${!STANDALONE_DIRS[@]}"; do
    dir="${STANDALONE_DIRS[$i]}"
    file="${STANDALONE_FILES[$i]}"
    rm -rf "$CPSYCLE/plugins/build"
    make -C "$CPSYCLE/plugins/$dir/src"
    if [[ ! -s "$CPSYCLE/plugins/build/$file" ]]; then
        echo "Standalone Pooplog build did not create output artifact: $file" >&2
        exit 1
    fi
    make -C "$CPSYCLE/plugins/$dir/src" clean
    if [[ -e "$CPSYCLE/plugins/build/$file" ]]; then
        echo "Pooplog clean target left stale shared object: $file" >&2
        exit 1
    fi
done

# The older retained Pooplog makefiles assume the common build directory exists.
# Recreate it for the complete family build after the standalone checks above.
rm -rf "$CPSYCLE/plugins/build"
mkdir -p "$CPSYCLE/plugins/build"
for dir in "${PLUGIN_DIRS[@]}"; do
    make -C "$CPSYCLE/plugins/$dir/src"
done

for file in "${PLUGIN_FILES[@]}"; do
    path="$CPSYCLE/plugins/build/$file"
    if [[ ! -s "$path" ]]; then
        echo "Pooplog shared object was not produced: $path" >&2
        exit 1
    fi
    PLUGIN_PATHS+=("$path")
done

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_pooplog_family_preservation.cpp" \
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
    "$ROOT/tests/phase5_pooplog_family_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$ABI_LOG"
for expected in "${EXPECTED_HASH_LINES[@]}"; do
    grep -Fqx "$expected" "$ABI_LOG" || {
        echo "Frozen Pooplog metadata hash was not observed: $expected" >&2
        exit 1
    }
done
for label in "${EFFECT_LABELS[@]}"; do
    grep -Fqx "phase5-pooplog-family: active sample-rate-only transition PASS [$label]" "$ABI_LOG" || {
        echo "Pooplog sample-rate-only transition evidence missing: $label" >&2
        exit 1
    }
    grep -Fqx "phase5-pooplog-family: active BPM-only transition PASS [$label]" "$ABI_LOG" || {
        echo "Pooplog BPM-only transition evidence missing: $label" >&2
        exit 1
    }
done

grep -Fqx "phase5-pooplog-family: PASS all 9 retained source-built targets" "$ABI_LOG" || {
    echo "Pooplog ABI/DSP family completion marker missing" >&2
    exit 1
}

"$STATE_BIN" "$OUT" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$STATE_LOG"
for expected in "${EXPECTED_OPAQUE_LINES[@]}"; do
    grep -Fqx "$expected" "$STATE_LOG" || {
        echo "Frozen Pooplog opaque-state contract was not observed: $expected" >&2
        exit 1
    }
done

grep -Fqx "phase5-pooplog-family-state: PASS all 9 source-built Pooplog machines" "$STATE_LOG" || {
    echo "Pooplog production persistence completion marker missing" >&2
    exit 1
}

SONG="$OUT/phase5-pooplog-family.psy"
[[ -s "$SONG" ]] || { echo "Pooplog family PSY3 evidence missing" >&2; exit 1; }
for i in {0..8}; do
    preset="$OUT/phase5-pooplog-$i.prs"
    [[ -s "$preset" ]] || { echo "Pooplog preset evidence missing: $preset" >&2; exit 1; }
done

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5B Pooplog Family Preservation

- Source-built Pooplog FM Laboratory, Light and UltraLight `.so` targets: PASS
- Source-built Pooplog Delay and Delay Light `.so` targets: PASS
- Source-built Pooplog Filter, Autopan, Lofi and Scratch `.so` targets: PASS
- Lofi and Scratch standalone direct builds create `plugins/build` and their loadable `.so` artifacts: PASS
- Lofi and Scratch direct `make clean` targets remove their loadable `.so` artifacts: PASS
- Native ABI / identity / version / parameter-table geometry for all nine binaries: PASS
- Complete parameter names/descriptions/ranges/flags/defaults frozen by nine exact metadata hashes: PASS
- Neutral deterministic DSP for Delay, Delay Light, Filter, Autopan, Lofi and Scratch: PASS
- Independent active sample-rate-only transitions for Delay, Delay Light, Filter, Autopan, Lofi and Scratch: PASS
- Independent active BPM-only transitions for Delay, Delay Light, Filter, Autopan, Lofi and Scratch: PASS
- Non-finite effect samples are rejected by the signal comparators: PASS
- Deterministic active-note rendering for all three FM synth variants: PASS
- Live 44.1 kHz -> 88.2 kHz `SequencerTick` transition preserves the note's physical-frequency estimate: PASS
- Production `PluginCatcher` recognition for all nine binaries: PASS
- Production `MachineFactory` instantiation for all nine binaries: PASS
- Every requested public parameter endpoint is immediately verified after tweak: PASS
- Non-selected FM oscillator/filter banks are seeded distinctly with inertia disabled: PASS
- Opaque-only FM bank state survives source canonicalization, fresh preset restore and fresh PSY3 reopen: PASS
- Per-machine version-1 preset save/load and fresh-machine restore: PASS
- FM Laboratory / Light / UltraLight opaque `GetData` byte preservation: PASS
- Historical FM opaque-state sizes and hidden-bank canonical payload hashes are frozen independently of the round-trip oracle: PASS
- Historical reserved pointer slots are deterministic without changing opaque-state size/layout: PASS
- One-song nine-machine PSY3 save and fresh reopen: PASS
- All nine machine -> Master topology edges survive reopen: PASS

The stale host category alias `pooplog-scratch-master-2:0` is not claimed as a
source-built target because no corresponding retained source/build directory is
present in the audited tree. The retained `pooplog_scratch` source builds and is
gated as `pooplog-scratch-master.so` / `pooplog-scratch-master:0`.
EOF

cat "$SUMMARY"