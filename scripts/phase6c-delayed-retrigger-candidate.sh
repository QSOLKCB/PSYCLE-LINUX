#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT="$(realpath "${1:?candidate artifact root required}")"
OUT="$ARTIFACT/delayed-retrigger"
BUILD="$(mktemp -d)"
STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-delayed-retrigger-probe-build"
RENDER_STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-sampulse-render-build"

[[ ! -e "$OUT" ]] || {
    echo 'refusing stale delayed/retrigger artifact directory' >&2
    exit 2
}
[[ ! -e "$STAGING" ]] || {
    echo 'refusing existing delayed/retrigger probe staging directory' >&2
    exit 2
}
[[ ! -e "$RENDER_STAGING" ]] || {
    echo 'refusing existing Sampulse render staging directory' >&2
    exit 2
}
mkdir "$OUT" "$STAGING" "$RENDER_STAGING"
trap 'rm -rf -- "$BUILD" "$STAGING" "$RENDER_STAGING"' EXIT

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
    "$ROOT/tests/phase6c_delayed_retrigger_fixture.c" \
    -o "$BUILD/phase6c-delayed-retrigger-fixture" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/fixture-generator.log" \
    "$BUILD/phase6c-delayed-retrigger-fixture" "$OUT"

FIXTURE="$OUT/phase6c-delayed-retrigger.psy"
[[ -s "$FIXTURE" ]] || {
    echo 'delayed/retrigger fixture was not generated' >&2
    exit 2
}
[[ "$(head -c 8 "$FIXTURE")" == "PSY3SONG" ]] || {
    echo 'delayed/retrigger fixture is not PSY3' >&2
    exit 2
}

run_logged "$OUT/execution-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_delayed_retrigger_execution_fixture.c" \
    -o "$BUILD/phase6c-delayed-retrigger-execution-fixture" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/execution-fixture-generator.log" \
    "$BUILD/phase6c-delayed-retrigger-execution-fixture" "$OUT"

EXECUTION_FIXTURE="$OUT/phase6c-delayed-retrigger-execution.psy"
[[ -s "$EXECUTION_FIXTURE" ]] || {
    echo 'delayed/retrigger execution witness was not generated' >&2
    exit 2
}
[[ "$(head -c 8 "$EXECUTION_FIXTURE")" == "PSY3SONG" ]] || {
    echo 'delayed/retrigger execution witness is not PSY3' >&2
    exit 2
}

run_logged "$OUT/execution-candidate-receipt.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-evidence.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/execution-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-evidence.py" candidate-check \
    "$ARTIFACT"

run_logged "$OUT/sampulse-execution-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_delayed_retrigger_sampulse_execution_fixture.c" \
    -o "$BUILD/phase6c-delayed-retrigger-sampulse-execution-fixture" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/sampulse-execution-fixture-generator.log" \
    "$BUILD/phase6c-delayed-retrigger-sampulse-execution-fixture" "$OUT"

SAMPULSE_FIXTURE="$OUT/phase6c-delayed-retrigger-sampulse-execution.psy"
MODERN_SAMPULSE_FIXTURE="$BUILD/phase6c-delayed-retrigger-sampulse-execution-modern.psy"
mv "$SAMPULSE_FIXTURE" "$MODERN_SAMPULSE_FIXTURE"
run_logged "$OUT/sampulse-eins-compat.log" \
    python3 "$ROOT/scripts/phase6c-sampulse-eins-compat.py" \
    "$MODERN_SAMPULSE_FIXTURE" "$SAMPULSE_FIXTURE"

[[ -s "$SAMPULSE_FIXTURE" ]] || {
    echo 'Sampulse execution witness was not generated' >&2
    exit 2
}
[[ "$(head -c 8 "$SAMPULSE_FIXTURE")" == "PSY3SONG" ]] || {
    echo 'Sampulse execution witness is not PSY3' >&2
    exit 2
}

ISOLATION_OUT="$ARTIFACT/delayed-retrigger-isolation"
mkdir "$ISOLATION_OUT"
run_logged "$OUT/render-isolation-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_delayed_retrigger_render_isolation.c" \
    -o "$BUILD/phase6c-delayed-retrigger-render-isolation" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/render-isolation-fixture-generator.log" \
    "$BUILD/phase6c-delayed-retrigger-render-isolation" "$ISOLATION_OUT"

for variant in control fd fb fa fe; do
    fixture="$ISOLATION_OUT/phase6c-delayed-retrigger-isolation-${variant}.psy"
    [[ -s "$fixture" ]] || {
        echo "render-isolation fixture was not generated: $variant" >&2
        exit 2
    }
    [[ "$(head -c 8 "$fixture")" == "PSY3SONG" ]] || {
        echo "render-isolation fixture is not PSY3: $variant" >&2
        exit 2
    }
done

run_logged "$OUT/render-isolation-candidate-receipts.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-isolation.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/render-isolation-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-isolation.py" candidate-check \
    "$ARTIFACT"

SUBSTRATE_OUT="$ARTIFACT/delayed-retrigger-substrate"
mkdir "$SUBSTRATE_OUT"
run_logged "$OUT/render-substrate-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_delayed_retrigger_render_substrate.c" \
    -o "$BUILD/phase6c-delayed-retrigger-render-substrate" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/render-substrate-fixture-generator.log" \
    "$BUILD/phase6c-delayed-retrigger-render-substrate" "$SUBSTRATE_OUT"

for variant in master-only sampler-empty sample-state ordinary-note; do
    fixture="$SUBSTRATE_OUT/phase6c-delayed-retrigger-substrate-${variant}.psy"
    [[ -s "$fixture" ]] || {
        echo "render-substrate fixture was not generated: $variant" >&2
        exit 2
    }
    [[ "$(head -c 8 "$fixture")" == "PSY3SONG" ]] || {
        echo "render-substrate fixture is not PSY3: $variant" >&2
        exit 2
    }
done

run_logged "$OUT/render-substrate-candidate-receipts.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-substrate.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/render-substrate-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-render-substrate.py" candidate-check \
    "$ARTIFACT"

VOICE_OUT="$ARTIFACT/sampler-voice-startup"
mkdir "$VOICE_OUT"
run_logged "$OUT/voice-startup-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_sampler_voice_startup_fixture.c" \
    -o "$BUILD/phase6c-sampler-voice-startup" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/voice-startup-fixture-generator.log" \
    "$BUILD/phase6c-sampler-voice-startup" "$VOICE_OUT"

for variant in note-no-previous-inst note-missing-sample note-sample-default-inst note-sample-serialized-inst; do
    fixture="$VOICE_OUT/phase6c-sampler-voice-startup-${variant}.psy"
    [[ -s "$fixture" ]] || {
        echo "voice-startup fixture was not generated: $variant" >&2
        exit 2
    }
    [[ "$(head -c 8 "$fixture")" == "PSY3SONG" ]] || {
        echo "voice-startup fixture is not PSY3: $variant" >&2
        exit 2
    }
done

run_logged "$OUT/voice-startup-candidate-receipts.log" \
    python3 "$ROOT/scripts/phase6c-sampler-voice-startup.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/voice-startup-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-sampler-voice-startup.py" candidate-check \
    "$ARTIFACT"

WORK_BOUNDARY_OUT="$ARTIFACT/sampler-work-boundary"
mkdir "$WORK_BOUNDARY_OUT"
run_logged "$OUT/work-boundary-fixture-build.log" \
    gcc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}" \
    "$ROOT/tests/phase6c_sampler_work_boundary_fixture.c" \
    -o "$BUILD/phase6c-sampler-work-boundary" \
    "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"

run_logged "$OUT/work-boundary-fixture-generator.log" \
    "$BUILD/phase6c-sampler-work-boundary" "$WORK_BOUNDARY_OUT"

for variant in release-no-active-voice delayed-note-short delayed-note-long ordinary-note-short; do
    fixture="$WORK_BOUNDARY_OUT/phase6c-sampler-work-boundary-${variant}.psy"
    [[ -s "$fixture" ]] || {
        echo "work-boundary fixture was not generated: $variant" >&2
        exit 2
    }
    [[ "$(head -c 8 "$fixture")" == "PSY3SONG" ]] || {
        echo "work-boundary fixture is not PSY3: $variant" >&2
        exit 2
    }
done

run_logged "$OUT/work-boundary-candidate-receipts.log" \
    python3 "$ROOT/scripts/phase6c-sampler-work-boundary.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/work-boundary-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-sampler-work-boundary.py" candidate-check \
    "$ARTIFACT"

cp "$ROOT/tests/phase6c_delayed_retrigger.pro" "$STAGING/probe.pro"
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
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-evidence.py" collect \
    "$ARTIFACT" "$BUILD/phase6c-delayed-retrigger-probe"
run_logged "$OUT/candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-evidence.py" candidate "$ARTIFACT"

cp "$ROOT/tests/phase6c_delayed_retrigger_sampulse_render.pro" "$RENDER_STAGING/render.pro"
set +e
(
    cd "$RENDER_STAGING"
    qmake CONFIG-=shared CONFIG+=release \
        "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT" \
        -o "$BUILD/Makefile.sampulse-render" "$RENDER_STAGING/render.pro"
) >"$OUT/sampulse-render-qmake.log" 2>&1
render_qmake_status=$?
set -e
if (( render_qmake_status != 0 )); then
    cat "$OUT/sampulse-render-qmake.log" >&2
    exit "$render_qmake_status"
fi

run_logged "$OUT/sampulse-render-build.log" \
    make -C "$BUILD" -f "$BUILD/Makefile.sampulse-render" -j2

SAMPULSE_RUNTIME_OUT="$ARTIFACT/delayed-retrigger-sampulse-runtime"
mkdir "$SAMPULSE_RUNTIME_OUT"
for attempt in 1 2; do
    run_logged "$OUT/sampulse-candidate-render-${attempt}.log" \
        env PSYCLE_THREADS=1 \
        "$BUILD/phase6c-delayed-retrigger-sampulse-render" \
        "$SAMPULSE_FIXTURE" \
        "$SAMPULSE_RUNTIME_OUT/candidate-delayed-retrigger-sampulse-runtime-${attempt}.wav"
done

run_logged "$OUT/sampulse-runtime-candidate-receipt.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-same-witness.py" candidate \
    "$ARTIFACT"
run_logged "$OUT/sampulse-runtime-candidate-validation.log" \
    python3 "$ROOT/scripts/phase6c-delayed-retrigger-same-witness.py" candidate-check \
    "$ARTIFACT"
