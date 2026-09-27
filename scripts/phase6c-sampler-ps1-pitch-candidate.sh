#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$(realpath -m "${1:?candidate output directory required}")"
BUILD="$(mktemp -d)"
RENDER_STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-ps1-pitch-render-build"

[[ ! -e "$OUT" ]] || {
    echo "refusing existing candidate output directory: $OUT" >&2
    exit 2
}
[[ ! -e "$RENDER_STAGING" ]] || {
    echo "refusing existing PS1 pitch render staging directory" >&2
    exit 2
}
mkdir -p "$OUT/fixture" "$OUT/render" "$RENDER_STAGING"
trap 'rm -rf -- "$BUILD" "$RENDER_STAGING"' EXIT

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
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile
    -lm -lpthread -ldl -lstdc++ -lcontainer
)
read -r -a LUA_CFLAGS <<<"$(pkg-config --cflags lua)"
read -r -a LUA_LIBS <<<"$(pkg-config --libs lua)"

run_logged "$OUT/fixture-build.log"     cc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}"     "$ROOT/tests/phase6c_sampler_ps1_pitch_fixture.c"     "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"     -o "$BUILD/phase6c_sampler_ps1_pitch_fixture"

run_logged "$OUT/fixture-authoring.log"     "$BUILD/phase6c_sampler_ps1_pitch_fixture"     "$BUILD/phase6c-sampler-ps1-pitch-modern.psy"

run_logged "$OUT/fixture-compat.log"     python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch-compat.py" convert     "$BUILD/phase6c-sampler-ps1-pitch-modern.psy"     "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"
run_logged "$OUT/fixture-validation.log"     python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch-compat.py" check     "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"

git_blob_sha256() {
    git -C "$ROOT" cat-file blob "HEAD:$1" | sha256sum | awk '{print $1}'
}

cat >"$BUILD/phase6c-ps1-pitch-provenance.hpp" <<EOF
#pragma once
#define PHASE6C_RENDER_SOURCE_SHA256 "$(git_blob_sha256 tests/phase6c_sampler_ps1_pitch_render.cpp)"
#define PHASE6C_RENDER_PROJECT_SHA256 "$(git_blob_sha256 tests/phase6c_sampler_ps1_pitch_render.pro)"
#define PHASE6C_ENGINE_SEQUENCER_SHA256 "$(git_blob_sha256 psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp)"
#define PHASE6C_ENGINE_PSY3_LOADER_SHA256 "$(git_blob_sha256 psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp)"
#define PHASE6C_ENGINE_SAMPLER_SHA256 "$(git_blob_sha256 psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp)"
EOF

cp "$ROOT/tests/phase6c_sampler_ps1_pitch_render.pro" "$RENDER_STAGING/render.pro"
set +e
(
    cd "$RENDER_STAGING"
    qmake CONFIG-=shared CONFIG+=release         "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT"         -o "$BUILD/Makefile.ps1-pitch" "$RENDER_STAGING/render.pro"
) >"$OUT/render-qmake.log" 2>&1
qmake_status=$?
set -e
if (( qmake_status != 0 )); then
    cat "$OUT/render-qmake.log" >&2
    exit "$qmake_status"
fi
run_logged "$OUT/render-build.log"     make -C "$BUILD" -f "$BUILD/Makefile.ps1-pitch" -j2

RENDER="$BUILD/phase6c-sampler-ps1-pitch-render"
run_logged "$OUT/renderer-provenance.json"     "$RENDER" --phase6c-provenance

for n in 1 2; do
    run_logged "$OUT/render/candidate-render-$n.log"         env PSYCLE_THREADS=1 "$RENDER"         "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"         "$OUT/render/candidate-sampler-ps1-pitch-$n.wav"
done

run_logged "$OUT/candidate-collect.log"     python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch.py" candidate "$OUT"
run_logged "$OUT/candidate-validation.log"     python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch.py" candidate-check "$OUT"

echo "phase6c-sampler-ps1-pitch-candidate: PASS"
