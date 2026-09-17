#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

DEST="${1:-psycle-cpp-r12005-sanitized/build-systems/qmake}"
SVN_REVISION="12005"
SVN_URL="https://svn.code.sf.net/p/psycle/code/trunk/build-systems/qmake@12005"
EXPECTED_FILE_COUNT=12
EXPECTED_MANIFEST_SHA256="cb5b84840ae1380601464d5be279c6747917cf6ccb68d7b08ba450f3dfe57229"

die() {
  echo "phase6b-materialize-qmake-support: $*" >&2
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
  die "unexpected qmake support file count: $file_count"

manifest_sha="$(sha256sum "$manifest" | awk '{print $1}')"
[[ "$manifest_sha" == "$EXPECTED_MANIFEST_SHA256" ]] || \
  die "qmake support manifest identity mismatch: $manifest_sha"

# The retained qmake support files are Psycle-authored build metadata. Require
# the upstream GPL-2-or-later notice on the files that carry source comments;
# pure dependency snippets may contain only qmake expressions.
for required_notice in common.pri platform.pri platform-unix.pri boost.pri libxml++.pri zlib.pri; do
  grep -Fq 'GNU General Public License' "$DEST/$required_notice" || \
    die "missing upstream GPL notice in qmake support file: $required_notice"
done

echo "phase6b-materialize-qmake-support: PASS files=$file_count manifest=$manifest_sha"
