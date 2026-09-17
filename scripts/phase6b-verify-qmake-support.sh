#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

SOURCE="${1:-psycle-cpp-r12005-sanitized/build-systems/qmake}"
EXPECTED_FILE_COUNT=12
EXPECTED_MANIFEST_SHA256="cb5b84840ae1380601464d5be279c6747917cf6ccb68d7b08ba450f3dfe57229"

die() {
  echo "phase6b-verify-qmake-support: $*" >&2
  exit 2
}

for command_name in sha256sum find sort xargs wc tr awk; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

[[ -d "$SOURCE" ]] || die "missing qmake support tree: $SOURCE"

manifest="$(mktemp)"
trap 'rm -f "$manifest"' EXIT
(
  cd "$SOURCE"
  find . -type f -print0 | sort -z | xargs -0 -r sha256sum
) > "$manifest"

file_count="$(find "$SOURCE" -type f -printf '.' | wc -c | tr -d ' ')"
[[ "$file_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected qmake support file count: $file_count"

manifest_sha="$(sha256sum "$manifest" | awk '{print $1}')"
[[ "$manifest_sha" == "$EXPECTED_MANIFEST_SHA256" ]] || \
  die "qmake support manifest identity mismatch: $manifest_sha"

for required_notice in common.pri platform.pri platform-unix.pri boost.pri libxml++.pri zlib.pri; do
  grep -Fq 'GNU General Public License' "$SOURCE/$required_notice" || \
    die "missing upstream GPL notice in qmake support file: $required_notice"
done

echo "phase6b-verify-qmake-support: PASS files=$file_count manifest=$manifest_sha"
