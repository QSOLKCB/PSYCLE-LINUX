#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${1:-$ROOT/phase6c-sampler-ps1-pitch-candidate}"
BUILD="$(mktemp -d "${TMPDIR:-/tmp}/phase6c-ps1-pitch.XXXXXX")"
trap 'rm -rf -- "$BUILD"' EXIT

if [[ -e "$OUT" ]]; then
  echo "refusing existing output directory: $OUT" >&2
  exit 2
fi
mkdir -p "$OUT/fixture" "$OUT/render"

for target in container thread script file dsp audio; do
  make -C "$ROOT/cpsycle/$target/src"
done

CPSYCLE="$ROOT/cpsycle"
COMMON_CFLAGS=(
  -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration
  -I"$CPSYCLE/audio/src" -I"$CPSYCLE/driver"
  -I"$CPSYCLE/container/src" -I"$CPSYCLE/thread/src"
  -I"$CPSYCLE/script/src" -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src"
)
COMMON_LDFLAGS=(
  -L"$CPSYCLE/audio/src" -L"$CPSYCLE/thread/src"
  -L"$CPSYCLE/container/src" -L"$CPSYCLE/script/src"
  -L"$CPSYCLE/file/src" -L"$CPSYCLE/dsp/src"
  -laudio -lthread -llilv-0 -ldsp -lscript -lfile
  -lm -lpthread -ldl -lstdc++ -lcontainer
)
read -r -a LUA_CFLAGS <<<"$(pkg-config --cflags lua)"
read -r -a LUA_LIBS <<<"$(pkg-config --libs lua)"

cc "${COMMON_CFLAGS[@]}" "${LUA_CFLAGS[@]}"   "$ROOT/tests/phase6c_sampler_ps1_pitch_fixture.c"   "${COMMON_LDFLAGS[@]}" "${LUA_LIBS[@]}"   -o "$BUILD/phase6c_sampler_ps1_pitch_fixture"

"$BUILD/phase6c_sampler_ps1_pitch_fixture"   "$BUILD/phase6c-sampler-ps1-pitch-modern.psy"   >"$OUT/fixture-authoring.json"

python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch-compat.py" convert   "$BUILD/phase6c-sampler-ps1-pitch-modern.psy"   "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"
python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch-compat.py" check   "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"

PROBE_BUILD="$BUILD/render"
mkdir -p "$PROBE_BUILD"
sha256_file() { sha256sum "$1" | awk '{print $1}'; }
cat >"$PROBE_BUILD/phase6c-ps1-pitch-provenance.hpp" <<EOF
#pragma once
#define PHASE6C_RENDER_SOURCE_SHA256 "$(sha256_file "$ROOT/tests/phase6c_sampler_ps1_pitch_render.cpp")"
#define PHASE6C_RENDER_PROJECT_SHA256 "$(sha256_file "$ROOT/tests/phase6c_sampler_ps1_pitch_render.pro")"
#define PHASE6C_ENGINE_SEQUENCER_SHA256 "$(sha256_file "$ROOT/psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sequencer.cpp")"
#define PHASE6C_ENGINE_PSY3_LOADER_SHA256 "$(sha256_file "$ROOT/psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/psy3filter.cpp")"
#define PHASE6C_ENGINE_SAMPLER_SHA256 "$(sha256_file "$ROOT/psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/sampler.cpp")"
EOF

(
  cd "$ROOT/tests"
  qmake     "REPO_ROOT=$ROOT"     "PROBE_BUILD_DIR=$PROBE_BUILD"     phase6c_sampler_ps1_pitch_render.pro     -o "$PROBE_BUILD/Makefile"
)
make -C "$PROBE_BUILD" -j2

RENDER="$PROBE_BUILD/phase6c-sampler-ps1-pitch-render"
"$RENDER" --phase6c-provenance >"$OUT/renderer-provenance.json"

for n in 1 2; do
  PSYCLE_THREADS=1 "$RENDER"     "$OUT/fixture/phase6c-sampler-ps1-pitch.psy"     "$OUT/render/candidate-sampler-ps1-pitch-$n.wav"     >"$OUT/render/candidate-render-$n.json"     2>"$OUT/render/candidate-render-$n.stderr.log"
done

python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch.py" candidate "$OUT"
python3 "$ROOT/scripts/phase6c-sampler-ps1-pitch.py" candidate-check "$OUT"

echo "phase6c-sampler-ps1-pitch-candidate: PASS"
