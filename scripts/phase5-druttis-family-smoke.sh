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
CALLBACK_FIXTURE_OBJ="$OUT/phase5-druttis-player-callback-fixture.o"
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

# Construct the real Song -> Player -> MachineCallback chain in C. Psycle's
# legacy Player headers deliberately retain C declarations that are not C++
# clean on Linux, while the native CFxCallback ABI itself is C++.
gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
    "${COMMON_INCLUDES[@]}" -c \
    "$ROOT/tests/phase5_druttis_player_callback_fixture.c" \
    -o "$CALLBACK_FIXTURE_OBJ"

# Keep the native ABI assertion in C++ and link it to the production C fixture.
# Historical host headers are warning-heavy, so only meaningful local return
# type errors are promoted here.
g++ -std=c++17 -Wall -Wextra -Werror=return-type \
    "${COMMON_INCLUDES[@]}" \
    "$ROOT/tests/phase5_druttis_production_callback.cpp" \
    "$CALLBACK_FIXTURE_OBJ" -o "$CALLBACK_BIN" \
    "${COMMON_LIBDIRS[@]}" "${COMMON_LIBS[@]}"

# Gate the exact PluginFxCallback timing adapter used by production native
# machines. Native GetTickLength is a tracker-line duration (LPB), not the
# finer transport tick selected by song TPB. Near-integral values are snapped
# within a small floating-point tolerance while genuinely fractional durations
# still truncate. Positive sub-sample durations floor to one sample. Oversized
# finite durations are capped at INT_MAX/256 so retained signed-int consumers
# remain arithmetically safe; CrossDelay separately enforces its historical
# two-second resource ceiling before growing delay buffers. Non-finite timing
# must fall back rather than narrow. The final marker uses a real Song -> Player
# -> MachineCallback chain with the normal LPB=4 / TPB=24 split.
stdbuf -o0 -e0 "$CALLBACK_BIN" 2>&1 | tee "$CALLBACK_LOG"
EXPECTED_CALLBACK_TIMING=(
    'phase5-druttis-production-callback: line PASS sr=44100 bpm=120 lpb=4 tpb=24 samples=5512'
    'phase5-druttis-production-callback: line PASS sr=44100 bpm=35 lpb=3 tpb=24 samples=25200'
    'phase5-druttis-production-callback: line PASS sr=88200 bpm=120 lpb=4 tpb=24 samples=11025'
    'phase5-druttis-production-callback: line PASS sr=88200 bpm=137 lpb=4 tpb=24 samples=9656'
    'phase5-druttis-production-callback: line PASS sr=88200 bpm=120 lpb=8 tpb=24 samples=5512'
    'phase5-druttis-production-callback: line PASS sr=88200 bpm=120 lpb=4 tpb=48 samples=11025'
    'phase5-druttis-production-callback: integral-snap PASS samples=264'
    'phase5-druttis-production-callback: subsample-floor PASS samples=1'
    'phase5-druttis-production-callback: legacy-cap PASS raw-samples=4500000000 cap=8388607 crossdelay=134217712 bexphase=268435424 sublime=2147483392'
    'phase5-druttis-production-callback: nonfinite-fallback PASS samples=45000'
    'phase5-druttis-production-callback: actual-player PASS sr=44100 bpm=120 lpb=4 tpb=24 transport-tick=918 line=5512'
)
for marker in "${EXPECTED_CALLBACK_TIMING[@]}"; do
    grep -Fqx "$marker" "$CALLBACK_LOG" || {
        echo "Production native callback timing marker missing: $marker" >&2
        exit 1
    }
done
grep -Fqx 'phase5-druttis-production-callback: PASS tracker-line-derived native timing' "$CALLBACK_LOG" || {
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
- Production `PluginFxCallback` derives native tick length from the current tracker-line duration: PASS
- Real Song/Player callback with LPB 4 / TPB 24 distinguishes 5512-sample line from 918-sample transport tick: PASS
- 44.1 kHz / 120 BPM / LPB 4 fractional native line truncates to 5512 samples: PASS
- 44.1 kHz / 35 BPM / LPB 3 exact native line remains 25200 samples: PASS
- Multi-ULP near-integral native timing snaps to the intended integer sample count: PASS
- Positive sub-sample native timing floors to one sample, never zero: PASS
- Oversized finite native line durations cap at `INT_MAX / 256` so retained legacy signed multipliers remain representable: PASS
- CrossDelay enforces its historical two-second delay resource ceiling before buffer growth: PASS
- Non-finite native line timing is rejected before integer narrowing and uses fallback timing: PASS
- 88.2 kHz / 120 BPM / LPB 4 native line = 11025 samples: PASS
- Native line timing is independent of finer transport TPB: PASS
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
