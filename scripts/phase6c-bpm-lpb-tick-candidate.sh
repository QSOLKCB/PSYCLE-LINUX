#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT="$(realpath "${1:?candidate artifact root required}")"
OUT="$ARTIFACT/bpm-lpb-tick"
BUILD="$(mktemp -d)"
STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-bpm-lpb-tick-probe-build"

[[ ! -e "$OUT" ]] || { echo 'refusing stale BPM/LPB/tick artifact directory' >&2; exit 2; }
[[ ! -e "$STAGING" ]] || { echo 'refusing existing timing probe staging directory' >&2; exit 2; }
mkdir "$OUT" "$STAGING"
trap 'rm -rf -- "$BUILD" "$STAGING"' EXIT

run_logged() {
    local log="$1"
    shift
    set +e
    "$@" >"$log" 2>&1
    local status=$?
    set -e
    if (( status != 0 )); then
        cat "$log" >&2
        return "$status"
    fi
}

CPSYCLE="$ROOT/cpsycle"
make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

COMMON_CFLAGS=(
    -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration
    -I"$CPSYCLE/audio/src" -I"$CPSYCLE/driver" -I"$CPSYCLE/thread/src"
    -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" -I"$CPSYCLE/file/src"
    -I"$CPSYCLE/dsp/src" -I"$CPSYCLE/diversalis/src"
)
COMMON_LDFLAGS=(
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" -L"$CPSYCLE/container/src"
    -L"$CPSYCLE/dsp/src" -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src"
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm -lpthread -ldl
    -lstdc++ -lcontainer
)
# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

run_logged "$OUT/fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_bpm_lpb_tick_fixture.c" \
    -o "$BUILD/phase6c-bpm-lpb-tick-fixture" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/fixture-generator.log" \
    "$BUILD/phase6c-bpm-lpb-tick-fixture" "$OUT"
FIXTURE="$OUT/phase6c-bpm-lpb-tick.psy"
[[ -s "$FIXTURE" ]] || { echo 'timing fixture was not generated' >&2; exit 2; }
[[ "$(head -c 8 "$FIXTURE")" == "PSY3SONG" ]] || { echo 'timing fixture is not PSY3' >&2; exit 2; }

cp "$ROOT/tests/phase6c_bpm_lpb_tick.pro" "$STAGING/probe.pro"
set +e
(
    cd "$STAGING"
    qmake CONFIG-=shared CONFIG+=release \
        "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT" \
        -o "$BUILD/Makefile" "$STAGING/probe.pro"
) >"$OUT/probe-qmake.log" 2>&1
qmake_status=$?
set -e
if (( qmake_status != 0 )); then
    cat "$OUT/probe-qmake.log" >&2
    exit "$qmake_status"
fi

run_logged "$OUT/probe-build.log" make -C "$BUILD" -j2
run_logged "$OUT/candidate-collect.log" \
    python3 "$ROOT/scripts/phase6c-bpm-lpb-tick-evidence.py" collect \
    "$ARTIFACT" "$BUILD/phase6c-bpm-lpb-tick-probe"
run_logged "$OUT/candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-bpm-lpb-tick-evidence.py" candidate "$ARTIFACT"
