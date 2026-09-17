#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

DEST="${1:-psycle-cpp-r12005-sanitized/build-systems/src}"
SVN_REVISION="12005"
SVN_URL="https://svn.code.sf.net/p/psycle/code/trunk/build-systems/src@12005"
EXPECTED_FILE_COUNT=11
EXPECTED_MANIFEST_SHA256="7a9c0e434281d20dcd201a5814ee7b6a3d740fab6218ec95662da528da99d91c"

die() {
  echo "phase6b-materialize-build-system-src: $*" >&2
  exit 2
}

for command_name in svn sha256sum find sort xargs wc tr awk mkdir; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

if ! mkdir -- "$DEST"; then
  die "refusing existing or invalid destination: $DEST"
fi

svn export --quiet --force -r "$SVN_REVISION" "$SVN_URL" "$DEST"

manifest="$(mktemp)"
trap 'rm -f "$manifest"' EXIT
(
  cd "$DEST"
  find . -type f -print0 | sort -z | xargs -0 -r sha256sum
) > "$manifest"

file_count="$(find "$DEST" -type f -printf '.' | wc -c | tr -d ' ')"
[[ "$file_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected build-system source file count: $file_count"

manifest_sha="$(sha256sum "$manifest" | awk '{print $1}')"
[[ "$manifest_sha" == "$EXPECTED_MANIFEST_SHA256" ]] || \
  die "build-system source manifest identity mismatch: $manifest_sha"

for required_notice in \
    forced-include.private.hpp \
    pre-compiled.private.hpp \
    setup_feature_test_macros.private.hpp \
    setup_optimizations.private.hpp \
    setup_warnings.private.hpp; do
  grep -Fq 'GNU General Public License' "$DEST/$required_notice" || \
    die "missing upstream GPL notice in build-system source file: $required_notice"
done

echo "phase6b-materialize-build-system-src: PASS files=$file_count manifest=$manifest_sha"
