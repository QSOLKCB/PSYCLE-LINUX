#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

SOURCE="${1:-psycle-cpp-r12005-sanitized/build-systems/src}"
EXPECTED_FILE_COUNT=11
EXPECTED_MANIFEST_SHA256="7a9c0e434281d20dcd201a5814ee7b6a3d740fab6218ec95662da528da99d91c"

die() {
  echo "phase6b-verify-build-system-src: $*" >&2
  exit 2
}

for command_name in sha256sum find sort xargs wc tr awk; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

[[ -d "$SOURCE" ]] || die "missing build-system source tree: $SOURCE"

manifest="$(mktemp)"
trap 'rm -f "$manifest"' EXIT
(
  cd "$SOURCE"
  find . -type f -print0 | sort -z | xargs -0 -r sha256sum
) > "$manifest"

file_count="$(find "$SOURCE" -type f -printf '.' | wc -c | tr -d ' ')"
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
  grep -Fq 'GNU General Public License' "$SOURCE/$required_notice" || \
    die "missing upstream GPL notice in build-system source file: $required_notice"
done

echo "phase6b-verify-build-system-src: PASS files=$file_count manifest=$manifest_sha"
