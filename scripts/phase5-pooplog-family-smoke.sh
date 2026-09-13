#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-pooplog-family}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT" "$CPSYCLE/plugins/build"

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

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

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
    "$ROOT/tests/phase5_pooplog_family.cpp" \
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
    "$ROOT/tests/phase5_pooplog_family_state.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$ABI_BIN" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$ABI_LOG"
"$STATE_BIN" "$OUT" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$STATE_LOG"

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
- Native ABI / identity / version / parameter-table geometry for all nine binaries: PASS
- Complete parameter metadata hashes emitted/frozen by the ABI regression: PASS
- Neutral deterministic DSP for Delay, Delay Light, Filter, Autopan, Lofi and Scratch: PASS
- Sample-rate/BPM reinitialization survival for retained effects: PASS
- Deterministic active-note rendering for all three FM synth variants: PASS
- 44.1 kHz and 88.2 kHz FM synth rendering remains finite/non-silent: PASS
- Production `PluginCatcher` recognition for all nine binaries: PASS
- Production `MachineFactory` instantiation for all nine binaries: PASS
- Every exposed parameter exercised through the public scaled API: PASS
- Per-machine version-1 preset save/load and fresh-machine restore: PASS
- FM Laboratory / Light / UltraLight opaque `GetData` byte preservation: PASS
- One-song nine-machine PSY3 save and fresh reopen: PASS
- All nine machine -> Master topology edges survive reopen: PASS

The stale host category alias `pooplog-scratch-master-2:0` is not claimed as a
source-built target because no corresponding retained source/build directory is
present in the audited tree. The retained `pooplog_scratch` source builds and is
gated as `pooplog-scratch-master.so` / `pooplog-scratch-master:0`.
EOF

cat "$SUMMARY"
