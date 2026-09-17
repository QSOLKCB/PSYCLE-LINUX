#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

SOURCE="${1:-psycle-cpp-r12005-sanitized/build-systems/qmake}"
EXPECTED_FILE_COUNT=12
EXPECTED_MANIFEST_SHA256="cb5b84840ae1380601464d5be279c6747917cf6ccb68d7b08ba450f3dfe57229"
BOOST_COMPAT_PATH="boost.pri"
BOOST_UPSTREAM_SHA256="e0857065286b4a2f6479b54d4dc5e847879e35ff05c1f9d9d2dd9d15e6b9716d"

die() {
  echo "phase6b-verify-qmake-support: $*" >&2
  exit 2
}

for command_name in sha256sum find sort wc tr awk python3; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

[[ -d "$SOURCE" ]] || die "missing qmake support tree: $SOURCE"

tmpdir="$(mktemp -d)"
manifest="$tmpdir/files.sha256"
boost_normalized="$tmpdir/boost-upstream-normalized.pri"
trap 'rm -rf "$tmpdir"' EXIT
: > "$manifest"

normalize_boost_pri() {
  local source="$1"
  local output="$2"
  python3 - "$source" "$output" <<'PY'
import pathlib
import sys

source = pathlib.Path(sys.argv[1])
output = pathlib.Path(sys.argv[2])
data = source.read_bytes()

newline = b"\r\n" if b"\r\n" in data else b"\n"
patched = (
    b"\t\telse: LIBS *= $$linkLibs(boost_thread boost_filesystem "
    b"boost_system boost_chrono boost_date_time)" + newline
)
upstream = (
    b"\t\telse: LIBS *= $$linkLibs(boost_signals boost_thread "
    b"boost_filesystem boost_system boost_chrono boost_date_time)" + newline
)

if data.count(patched) != 1:
    raise SystemExit(
        "tracked boost.pri compatibility patch must contain exactly one Linux linker line without boost_signals"
    )
if data.count(upstream) != 0:
    raise SystemExit(
        "tracked boost.pri compatibility patch unexpectedly retains the upstream Linux boost_signals linker line"
    )

data = data.replace(patched, upstream, 1)
output.write_bytes(data)
PY
}

while IFS= read -r -d '' file; do
  relative="${file#"$SOURCE/"}"
  sha="$(sha256sum "$file" | awk '{print $1}')"

  if [[ "$relative" == "$BOOST_COMPAT_PATH" ]]; then
    normalize_boost_pri "$file" "$boost_normalized"
    normalized_sha="$(sha256sum "$boost_normalized" | awk '{print $1}')"
    [[ "$normalized_sha" == "$BOOST_UPSTREAM_SHA256" ]] || \
      die "tracked boost.pri compatibility patch contains changes beyond the approved Linux boost_signals removal"
    # Preserve the frozen historical qmake-support identity by hashing the
    # exactly reverse-normalized upstream bytes into the aggregate manifest.
    sha="$normalized_sha"
  fi

  printf '%s  ./%s\n' "$sha" "$relative" >> "$manifest"
done < <(find "$SOURCE" -type f -print0 | sort -z)

file_count="$(find "$SOURCE" -type f -printf '.' | wc -c | tr -d ' ')"
[[ "$file_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected qmake support file count: $file_count"

manifest_sha="$(sha256sum "$manifest" | awk '{print $1}')"
[[ "$manifest_sha" == "$EXPECTED_MANIFEST_SHA256" ]] || \
  die "qmake support manifest identity mismatch outside approved compatibility patches: $manifest_sha"

for required_notice in common.pri platform.pri platform-unix.pri boost.pri libxml++.pri zlib.pri; do
  grep -Fq 'GNU General Public License' "$SOURCE/$required_notice" || \
    die "missing upstream GPL notice in qmake support file: $required_notice"
done

echo "phase6b-verify-qmake-support: PASS files=$file_count manifest=$manifest_sha compat-patches=1"
