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
CALLBACK_BIN="$OUT/phase5-druttis-production-callback"
ABI_LOG="$OUT/phase5-druttis-family.log"
STATE_LOG="$OUT/phase5-druttis-family-state.log"
CALLBACK_LOG="$OUT/phase5-druttis-production-callback.log"
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

COMMON_INCLUDES=(
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src"
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src"
    -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src"
    -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}"
)
COMMON_LIBDIRS=(
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src"
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src"
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src"
)
COMMON_LIBS=(
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm
    -lpthread -ldl -lcontainer "${LUA_LIBS[@]}"
)

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    "${COMMON_INCLUDES[@]}" \
    "$ROOT/tests/phase5_druttis_family_persistence.c" -o "$STATE_BIN" \
    "${COMMON_LIBDIRS[@]}" "${COMMON_LIBS[@]}" -lstdc++

g++ -std=c++17 -Wall -Wextra -Werror \
    "${COMMON_INCLUDES[@]}" \
    "$ROOT/tests/phase5_druttis_production_callback.cpp" -o "$CALLBACK_BIN" \
    "${COMMON_LIBDIRS[@]}" "${COMMON_LIBS[@]}"

# Gate the exact PluginFxCallback timing adapter used by production native
# machines. These values deliberately derive 88.2 kHz timing independently,
# rather than doubling an already-truncated 44.1 kHz tick length.
stdbuf -o0 -e0 "$CALLBACK_BIN" 2>&1 | tee "$CALLBACK_LOG"
EXPECTED_CALLBACK_TIMING=(
    'phase5-druttis-production-callback: tick PASS sr=44100 bpm=120 tpb=4 samples=5512'
    'phase5-druttis-production-callback: tick PASS sr=88200 bpm=120 tpb=4 samples=11025'
    'phase5-druttis-production-callback: tick PASS sr=88200 bpm=137 tpb=4 samples=9656'
    'phase5-druttis-production-callback: tick PASS sr=88200 bpm=120 tpb=8 samples=5512'
)
for marker in "${EXPECTED_CALLBACK_TIMING[@]}"; do
    grep -Fqx "$marker" "$CALLBACK_LOG" || {
        echo "Production native callback timing marker missing: $marker" >&2
        exit 1
    }
done
grep -Fqx 'phase5-druttis-production-callback: PASS host-derived native tick timing' "$CALLBACK_LOG" || {
    echo "Production native callback timing completion marker missing" >&2
    exit 1
}

# Keep regression output unbuffered so a future fault identifies the exact
# machine/stage immediately before failure.
stdbuf -o0 -e0 "$ABI_BIN" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$ABI_LOG"

EXPECTED_METADATA=(
    'druttis-metadata-hash[EQ-3]=0x1d7768bf2bf65d17'
    'druttis-metadata-hash[FeedMe]=0x665df6f57aa34c4b'
    'druttis-metadata-hash[Koruz]=0x8c1bbabd87f42796'
    'druttis-metadata-hash[Phantom]=0xbefdde29123bea07'
    'druttis-metadata-hash[Plucked String]=0x3be7f541e23e0f3f'
    'druttis-metadata-hash[Slicit]=0x689adbb03c0fd4d3'
    'druttis-metadata-hash[Sublime]=0xbaa335eeecaf0b09'
)
for marker in "${EXPECTED_METADATA[@]}"; do
    grep -Fqx "$marker" "$ABI_LOG" || {
        echo "Frozen Druttis metadata marker missing: $marker" >&2
        exit 1
    }
done

grep -Fqx 'phase5-druttis-family: live rate/tick transition PASS [Slicit]' "$ABI_LOG" || {
    echo "Slicit 44.1 -> 88.2 kHz / 11025-sample timing marker missing" >&2
    exit 1
}
grep -Fqx 'phase5-druttis-family: deterministic active generator PASS [Plucked String]' "$ABI_LOG" || {
    echo "Plucked String deterministic generator marker missing" >&2
    exit 1
}
grep -Fqx "phase5-druttis-family: PASS all 7 retained source-built targets" "$ABI_LOG" || {
    echo "Druttis ABI/DSP family completion marker missing" >&2
    exit 1
}

stdbuf -o0 -e0 "$STATE_BIN" "$OUT" "${PLUGIN_PATHS[@]}" 2>&1 | tee "$STATE_LOG"
grep -Fqx 'druttis-opaque-hash[Slicit]=0x7271bd63c9a7782d size=2144' "$STATE_LOG" || {
    echo "Frozen Slicit opaque-state marker missing" >&2
    exit 1
}
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
- Frozen complete parameter metadata hashes for all seven machines: PASS
- Production `PluginFxCallback` derives native tick length from host timing: PASS
- 44.1 kHz / 120 BPM / TPB 4 native tick = 5512 samples: PASS
- 88.2 kHz / 120 BPM / TPB 4 native tick = 11025 samples: PASS
- Native generator observations honor the historical 256-sample `MAX_BUFFER_LENGTH`: PASS
- Sublime timing is initialized before every note trigger: PASS
- Deterministic/stochastic historical DSP paths: PASS
- Live 44.1 -> 88.2 kHz generator/effect transitions where applicable: PASS
- Production PluginCatcher + MachineFactory discovery: PASS
- Every requested public parameter endpoint immediately verified: PASS
- Slicit hidden 16-program bank survives through opaque `PutData`: PASS
- Slicit 2144-byte opaque-state hash `0x7271bd63c9a7782d`: PASS
- Per-machine preset save/load and fresh-machine restore: PASS
- One-song seven-machine PSY3 save and fresh reopen: PASS
- All seven machine -> Master topology edges survive reopen: PASS

This is the Druttis slice of Phase 5C only. JM/JME/Zephod/Yezar/DW and the
remaining classic ecosystem stay open until their own dedicated preservation gates pass.
EOF
cat "$SUMMARY"
