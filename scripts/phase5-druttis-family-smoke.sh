#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-druttis-family}"
if [[ "$OUT" != /* ]]; then OUT="$ROOT/$OUT"; fi
rm -rf "$OUT"
mkdir -p "$OUT"

ABI_BIN="$OUT/phase5-druttis-family"
STATE_BIN="$OUT/phase5-druttis-family-state"
ABI_LOG="$OUT/phase5-druttis-family.log"
STATE_LOG="$OUT/phase5-druttis-family-state.log"
SUMMARY="$OUT/summary.md"

PLUGIN_DIRS=(
    druttis_eq3
    druttis_feedme
    druttis_koruz
    druttis_phantom
    druttis_pluckedstring
    druttis_slicit
    druttis_sublime
)
PLUGIN_FILES=(
    eq3.so
    feedme.so
    koruz.so
    phantom.so
    pluckedstring.so
    slicit.so
    sublime.so
)
PLUGIN_PATHS=()

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

# Prove every retained Druttis target is independently buildable from a checkout
# with no pre-created plugins/build directory, and that direct clean removes the
# actual loadable object from that shared output directory.
for i in "${!PLUGIN_DIRS[@]}"; do
    dir="${PLUGIN_DIRS[$i]}"
    file="${PLUGIN_FILES[$i]}"
    rm -rf "$CPSYCLE/plugins/build"
    make -C "$CPSYCLE/plugins/$dir/src"
    [[ -s "$CPSYCLE/plugins/build/$file" ]] || {
        echo "Druttis standalone build did not create $file" >&2
        exit 1
    }
    make -C "$CPSYCLE/plugins/$dir/src" clean
    [[ ! -e "$CPSYCLE/plugins/build/$file" ]] || {
        echo "Druttis clean target left stale shared object: $file" >&2
        exit 1
    }
done

mkdir -p "$CPSYCLE/plugins/build"
for dir in "${PLUGIN_DIRS[@]}"; do
    make -C "$CPSYCLE/plugins/$dir/src"
done
for file in "${PLUGIN_FILES[@]}"; do
    path="$CPSYCLE/plugins/build/$file"
    [[ -s "$path" ]] || { echo "Druttis shared object missing: $path" >&2; exit 1; }
    PLUGIN_PATHS+=("$path")
done

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_druttis_family_preservation.cpp" \
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
    "$ROOT/tests/phase5_druttis_family_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

# Keep observation output unbuffered so a crash identifies the exact machine and
# stage that was executing immediately before the fault.
stdbuf -o0 -e0 "$ABI_BIN" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$ABI_LOG"
grep -Fqx "phase5-druttis-family: PASS all 7 retained source-built targets" "$ABI_LOG" || {
    echo "Druttis ABI/DSP family completion marker missing" >&2
    exit 1
}

stdbuf -o0 -e0 "$STATE_BIN" "$OUT" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$STATE_LOG"
grep -Fqx "phase5-druttis-family-state: PASS all 7 source-built Druttis machines" "$STATE_LOG" || {
    echo "Druttis production persistence completion marker missing" >&2
    exit 1
}

SONG="$OUT/phase5-druttis-family.psy"
[[ -s "$SONG" ]] || { echo "Druttis family PSY3 evidence missing" >&2; exit 1; }
for i in {0..6}; do
    preset="$OUT/phase5-druttis-$i.prs"
    [[ -s "$preset" ]] || { echo "Druttis preset evidence missing: $preset" >&2; exit 1; }
done

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Druttis Family Preservation

- Seven retained source-built Druttis `.so` targets: PASS
- Standalone build creates `plugins/build` for every Druttis target: PASS
- Direct clean removes every Druttis loadable `.so`: PASS
- Native ABI / identity / version / parameter geometry: PASS
- Complete parameter metadata hashes emitted for freezing: PASS
- Deterministic finite effect paths: PASS
- Active generator note rendering: PASS
- Live 44.1 -> 88.2 kHz generator/effect transitions where applicable: PASS
- Production PluginCatcher + MachineFactory discovery: PASS
- Every requested public parameter endpoint immediately verified: PASS
- Slicit hidden program state depends on opaque PutData restoration: PASS
- Per-machine preset save/load and fresh-machine restore: PASS
- One-song seven-machine PSY3 save and fresh reopen: PASS
- All seven machine -> Master topology edges survive reopen: PASS

This is the Druttis slice of Phase 5C only. JM/JME/Zephod/Yezar/DW and the
remaining classic ecosystem stay open until their own dedicated preservation gates pass.
EOF
cat "$SUMMARY"
