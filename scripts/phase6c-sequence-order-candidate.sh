#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT="$(realpath "${1:?candidate artifact root required}")"
OUT="$ARTIFACT/sequence-order"
BUILD="$(mktemp -d)"
STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-sequence-order-probe-build"

[[ ! -e "$OUT" ]] || { echo 'refusing stale sequence-order artifact directory' >&2; exit 2; }
[[ ! -e "$STAGING" ]] || { echo 'refusing existing sequence-order probe staging directory' >&2; exit 2; }
mkdir "$OUT" "$STAGING"
trap 'rm -rf -- "$BUILD" "$STAGING"' EXIT

CPSYCLE="$ROOT/cpsycle"
make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

COMMON_CFLAGS=(
    -std=gnu11
    -Wall -Wextra -Werror=implicit-function-declaration
    -I"$CPSYCLE/audio/src"
    -I"$CPSYCLE/driver"
    -I"$CPSYCLE/thread/src"
    -I"$CPSYCLE/script/src"
    -I"$CPSYCLE/container/src"
    -I"$CPSYCLE/file/src"
    -I"$CPSYCLE/dsp/src"
    -I"$CPSYCLE/diversalis/src"
)
COMMON_LDFLAGS=(
    -L"$CPSYCLE/thread/src"
    -L"$CPSYCLE/script/src"
    -L"$CPSYCLE/container/src"
    -L"$CPSYCLE/dsp/src"
    -L"$CPSYCLE/audio/src"
    -L"$CPSYCLE/file/src"
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm
    -lpthread -ldl -lstdc++ -lcontainer
)
# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}"     "$ROOT/tests/phase6c_sequence_order_fixture.c"     -o "$BUILD/phase6c-sequence-order-fixture"     "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

"$BUILD/phase6c-sequence-order-fixture" "$OUT"     >"$OUT/fixture-generator.log" 2>&1

FIXTURE="$OUT/phase6c-sequence-order.psy"
[[ -s "$FIXTURE" ]] || { echo 'sequence-order fixture was not generated' >&2; exit 2; }
[[ "$(head -c 8 "$FIXTURE")" == "PSY3SONG" ]] || {
    echo 'sequence-order fixture is not PSY3' >&2
    exit 2
}

cp "$ROOT/tests/phase6c_sequence_order.pro" "$STAGING/probe.pro"
(
    cd "$STAGING"
    qmake CONFIG-=shared CONFIG+=release         "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT"         -o "$BUILD/Makefile" "$STAGING/probe.pro"
)
make -C "$BUILD" -j2

python3 "$ROOT/scripts/phase6c-sequence-order-evidence.py" collect     "$ARTIFACT" "$BUILD/phase6c-sequence-order-probe"
python3 "$ROOT/scripts/phase6c-sequence-order-evidence.py" candidate "$ARTIFACT"
