#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

SOURCE_ROOT="${1:-psycle-cpp-r12005-sanitized}"
RECEIPT="${2:-phase6b-sanitized-manifest}"
EXPECTED_FILE_COUNT=291
EXPECTED_BASELINE_SHA256="00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
COMPAT_PATH="psycle-helpers/src/psycle/helpers/endiantypes.hpp"
COMPAT_UPSTREAM_SHA256="cc9c5f99924a539cba6c14ceebeb16bcece856d1479a493bba404a2b78e1db0"
COMPONENTS=(
  universalis
  psycle-core
  psycle-audiodrivers
  psycle-helpers
  psycle-player
  psycle-plugins
)

die() {
  echo "phase6b-verify-committed-source: $*" >&2
  exit 2
}

for command_name in sha256sum sort find awk diff wc tr python3; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

[[ -d "$SOURCE_ROOT" ]] || die "missing committed source root: $SOURCE_ROOT"
[[ -f "$RECEIPT/retained-all.sha256" ]] || \
  die "missing retained aggregate receipt: $RECEIPT/retained-all.sha256"
[[ -f "$RECEIPT/baseline.sha256" ]] || \
  die "missing baseline identity: $RECEIPT/baseline.sha256"
[[ "$(cat "$RECEIPT/baseline.sha256")" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "unexpected Phase 6B receipt identity"

tmpdir="$(mktemp -d)"
actual_manifest="$tmpdir/actual-manifest.sha256"
compat_normalized="$tmpdir/endiantypes-upstream-normalized.hpp"
trap 'rm -rf "$tmpdir"' EXIT
: > "$actual_manifest"
actual_file_count=0
compat_actual_sha=""

normalize_compat_header() {
  local source="$1"
  local output="$2"
  python3 - "$source" "$output" <<'PY'
import pathlib
import sys

source = pathlib.Path(sys.argv[1])
output = pathlib.Path(sys.argv[2])
data = source.read_bytes()

newline = b"\r\n" if b"\r\n" in data else b"\n"
include_original = b"#include <universalis.hpp>" + newline
include_patched = include_original + b"#include <cmath>" + newline
if data.count(include_patched) != 1:
    raise SystemExit("tracked compatibility patch must contain exactly one <cmath> include after <universalis.hpp>")
data = data.replace(include_patched, include_original, 1)

replacements = (
    (b"fMant = std::frexp(num, &expon);", b"fMant = frexp(num, &expon);", 1),
    (b"fMant = std::ldexp(fMant, expon);", b"fMant = ldexp(fMant, expon);", 1),
    (b"fMant = std::ldexp(fMant, 32);", b"fMant = ldexp(fMant, 32);", 1),
    (b"fsMant = std::floor(fMant);", b"fsMant = floor(fMant);", 2),
    (b"fMant = std::ldexp(fMant - fsMant, 32);", b"fMant = ldexp(fMant - fsMant, 32);", 1),
    (b"f  = std::ldexp(static_cast<float>(hiMant), expon-=31);", b"f  = ldexp(static_cast<float>(hiMant), expon-=31);", 1),
    (b"f += std::ldexp(static_cast<float>(loMant), expon-=32);", b"f += ldexp(static_cast<float>(loMant), expon-=32);", 1),
)
for patched, upstream, expected_count in replacements:
    actual_count = data.count(patched)
    if actual_count != expected_count:
        raise SystemExit(
            f"tracked compatibility patch expression count mismatch: {patched.decode()} expected={expected_count} actual={actual_count}"
        )
    data = data.replace(patched, upstream)

output.write_bytes(data)
PY
}

for component in "${COMPONENTS[@]}"; do
  component_root="$SOURCE_ROOT/$component"
  [[ -d "$component_root" ]] || die "missing committed component: $component"

  component_count="$(find "$component_root" -type f -printf '.' | wc -c | tr -d ' ')"
  actual_file_count=$((actual_file_count + component_count))

  while IFS= read -r -d '' file; do
    relative="${file#"$SOURCE_ROOT/"}"
    sha="$(sha256sum "$file" | awk '{print $1}')"

    if [[ "$relative" == "$COMPAT_PATH" ]]; then
      compat_actual_sha="$sha"
      normalize_compat_header "$file" "$compat_normalized"
      normalized_sha="$(sha256sum "$compat_normalized" | awk '{print $1}')"
      [[ "$normalized_sha" == "$COMPAT_UPSTREAM_SHA256" ]] || \
        die "tracked compatibility patch contains changes beyond the approved math-declaration repair"
      # Preserve the frozen upstream selection identity in the aggregate receipt.
      # The committed file is allowed to differ only by the exactly reversible
      # compatibility patch validated above.
      sha="$normalized_sha"
    fi

    printf '%s  %s\n' "$sha" "$relative" >> "$actual_manifest"
  done < <(find "$component_root" -type f -print0 | sort -z)
done

[[ "$actual_file_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected committed upstream file count: $actual_file_count"
[[ -n "$compat_actual_sha" ]] || die "tracked compatibility header is missing"

sort -o "$actual_manifest" "$actual_manifest"
if ! diff -u "$RECEIPT/retained-all.sha256" "$actual_manifest"; then
  die "committed source differs from frozen retained manifest outside approved compatibility patches"
fi

actual_baseline_sha="$(sha256sum "$actual_manifest" | awk '{print $1}')"
[[ "$actual_baseline_sha" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "committed baseline selection identity mismatch: $actual_baseline_sha"

[[ ! -e "$SOURCE_ROOT/psycle-core/src/seib/vst" ]] || \
  die "Seib VST quarantine leaked into committed source"
[[ ! -e "$SOURCE_ROOT/psycle-audiodrivers/src/asio" ]] || \
  die "ASIO quarantine leaked into committed source"
[[ -f "$SOURCE_ROOT/psycle-plugins/src/psycle/plugins/plugin.hpp" ]] || \
  die "required native plugin API header missing"
[[ "$(find "$SOURCE_ROOT/psycle-plugins" -type f -printf '.' | wc -c | tr -d ' ')" -eq 1 ]] || \
  die "unexpected plugin-tree material entered committed baseline"

for component in "${COMPONENTS[@]}"; do
  if find "$SOURCE_ROOT/$component" -type f \( -iname '*.dll' -o -iname '*.psy' \) -print -quit | grep -q .; then
    die "restricted binary/song material entered committed component source"
  fi
done

echo "phase6b-verify-committed-source: PASS files=$actual_file_count baseline=$actual_baseline_sha compat-patches=1 endiantypes-sha256=$compat_actual_sha"
