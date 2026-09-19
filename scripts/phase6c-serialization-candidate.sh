#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT="$(realpath "${1:?candidate artifact root required}")"
BUILD="$(mktemp -d)"
STAGING="$ROOT/psycle-cpp-r12005-sanitized/psycle-player/phase6c-probe-build"
[[ ! -e "$STAGING" ]] || { echo 'refusing existing probe staging directory' >&2; exit 2; }
mkdir "$STAGING"
trap 'rm -rf -- "$BUILD" "$STAGING"' EXIT
cp "$ROOT/tests/phase6c_serialization.pro" "$STAGING/probe.pro"
# Reuse the already verified/built core archives, with a separate observation
# executable. Never replace the frozen player's main or edit the engine tree.
(
    cd "$STAGING"
    qmake CONFIG-=shared CONFIG+=release "PROBE_BUILD_DIR=$BUILD" "REPO_ROOT=$ROOT" \
        -o "$BUILD/Makefile" "$STAGING/probe.pro"
)
make -C "$BUILD" -j2
python3 "$ROOT/scripts/phase6c-serialization-evidence.py" collect \
    "$ARTIFACT" "$BUILD/phase6c-serialization-probe"
python3 "$ROOT/scripts/phase6c-serialization-evidence.py" candidate "$ARTIFACT"
