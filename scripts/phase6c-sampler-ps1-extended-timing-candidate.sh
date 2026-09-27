#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT="$(realpath "${1:?candidate artifact root required}")"
FIXTURE_DIR="$ARTIFACT/fixture"
BUILD="$(mktemp -d)"
STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-ps1-extended-timing-probe-build"

[[ ! -e "$FIXTURE_DIR" ]] || {
    echo 'refusing stale PS1 extended timing artifact' >&2
    exit 2
}
[[ ! -e "$STAGING" ]] || {
    echo 'refusing existing PS1 extended timing probe staging directory' >&2
    exit 2
}
mkdir -p "$FIXTURE_DIR" "$STAGING"
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

run_logged "$ARTIFACT/fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_sampler_ps1_extended_timing_fixture.c" \
    -o "$BUILD/phase6c-sampler-ps1-extended-timing-fixture" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$ARTIFACT/fixture-authoring.log" \
    "$BUILD/phase6c-sampler-ps1-extended-timing-fixture" "$FIXTURE_DIR"

for variant in delay noteoff; do
    original="$FIXTURE_DIR/phase6c-sampler-ps1-extended-${variant}-original.psy"
    [[ -s "$original" ]] || {
        echo "missing PS1 extended timing original fixture: $variant" >&2
        exit 2
    }
    [[ "$(head -c 8 "$original")" == "PSY3SONG" ]] || {
        echo "PS1 extended timing fixture is not PSY3: $variant" >&2
        exit 2
    }
done

run_logged "$ARTIFACT/fixture-compat.log" \
    python3 "$ROOT/scripts/phase6c-sampler-ps1-extended-timing-compat.py" \
    convert "$FIXTURE_DIR"
run_logged "$ARTIFACT/fixture-validation.log" \
    python3 "$ROOT/scripts/phase6c-sampler-ps1-extended-timing-compat.py" \
    check "$FIXTURE_DIR"

cp "$ROOT/tests/phase6c_sampler_ps1_extended_timing.pro" "$STAGING/probe.pro"
set +e
(
    cd "$STAGING"
    qmake CONFIG-=shared CONFIG+=release \
        "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT" \
        -o "$BUILD/Makefile" "$STAGING/probe.pro"
) >"$ARTIFACT/probe-qmake.log" 2>&1
qmake_status=$?
set -e
if (( qmake_status != 0 )); then
    cat "$ARTIFACT/probe-qmake.log" >&2
    exit "$qmake_status"
fi

run_logged "$ARTIFACT/probe-build.log" make -C "$BUILD" -j2
run_logged "$ARTIFACT/candidate-collect.log" \
    python3 "$ROOT/scripts/phase6c-sampler-ps1-extended-timing.py" collect \
    "$ARTIFACT" "$BUILD/phase6c-sampler-ps1-extended-timing-probe"
run_logged "$ARTIFACT/candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-sampler-ps1-extended-timing.py" candidate-check \
    "$ARTIFACT"

echo 'phase6c-sampler-ps1-extended-timing-candidate: PASS'
