#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

SOURCE_ROOT="${1:-psycle-cpp-r12005-sanitized}"
RECEIPT="${2:-phase6b-sanitized-manifest}"
EXPECTED_FILE_COUNT=291
EXPECTED_BASELINE_SHA256="00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"
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

for command_name in sha256sum sort find awk diff wc tr; do
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

actual_file_count="$(find "$SOURCE_ROOT" -type f -printf '.' | wc -c | tr -d ' ')"
[[ "$actual_file_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected committed upstream file count: $actual_file_count"

actual_manifest="$(mktemp)"
trap 'rm -f "$actual_manifest"' EXIT
: > "$actual_manifest"

for component in "${COMPONENTS[@]}"; do
  component_root="$SOURCE_ROOT/$component"
  [[ -d "$component_root" ]] || die "missing committed component: $component"

  while IFS= read -r -d '' file; do
    relative="${file#"$SOURCE_ROOT/"}"
    sha="$(sha256sum "$file" | awk '{print $1}')"
    printf '%s  %s\n' "$sha" "$relative" >> "$actual_manifest"
  done < <(find "$component_root" -type f -print0 | sort -z)
done

sort -o "$actual_manifest" "$actual_manifest"
if ! diff -u "$RECEIPT/retained-all.sha256" "$actual_manifest"; then
  die "committed source differs from frozen retained manifest"
fi

actual_baseline_sha="$(sha256sum "$actual_manifest" | awk '{print $1}')"
[[ "$actual_baseline_sha" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "committed baseline identity mismatch: $actual_baseline_sha"

[[ ! -e "$SOURCE_ROOT/psycle-core/src/seib/vst" ]] || \
  die "Seib VST quarantine leaked into committed source"
[[ ! -e "$SOURCE_ROOT/psycle-audiodrivers/src/asio" ]] || \
  die "ASIO quarantine leaked into committed source"
[[ -f "$SOURCE_ROOT/psycle-plugins/src/psycle/plugins/plugin.hpp" ]] || \
  die "required native plugin API header missing"
[[ "$(find "$SOURCE_ROOT/psycle-plugins" -type f -printf '.' | wc -c | tr -d ' ')" -eq 1 ]] || \
  die "unexpected plugin-tree material entered committed baseline"

if find "$SOURCE_ROOT" -type f \( -iname '*.dll' -o -iname '*.psy' \) -print -quit | grep -q .; then
  die "restricted binary/song material entered committed source"
fi

echo "phase6b-verify-committed-source: PASS files=$actual_file_count baseline=$actual_baseline_sha"
