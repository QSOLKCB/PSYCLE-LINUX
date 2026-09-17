#!/usr/bin/env bash
set -euo pipefail

export LC_ALL=C

RECEIPT="${1:-phase6b-sanitized-manifest}"
DEST="${2:-phase6b-sanitized-source}"
SVN_REVISION="12005"
SVN_ROOT="https://svn.code.sf.net/p/psycle/code/trunk"
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
  echo "phase6b-materialize-source: $*" >&2
  exit 2
}

for command_name in svn sha256sum sort find awk cp mkdir mktemp diff wc; do
  command -v "$command_name" >/dev/null 2>&1 || \
    die "missing required command: $command_name"
done

[[ -f "$RECEIPT/retained-all.sha256" ]] || \
  die "missing retained aggregate receipt: $RECEIPT/retained-all.sha256"
[[ -f "$RECEIPT/baseline.sha256" ]] || \
  die "missing retained baseline identity: $RECEIPT/baseline.sha256"
[[ "$(cat "$RECEIPT/baseline.sha256")" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "unexpected Phase 6B receipt identity"

# The destination is caller-controlled. Require mkdir itself to create it and
# never recursively clean or reuse an existing path.
if ! mkdir -- "$DEST"; then
  die "refusing existing or invalid destination: $DEST"
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
ACTUAL="$WORK/retained-all.sha256"
: > "$ACTUAL"

for component in "${COMPONENTS[@]}"; do
  retained="$RECEIPT/retained/$component.sha256"
  [[ -f "$retained" ]] || die "missing retained component receipt: $retained"

  export_dir="$WORK/$component"
  pinned_url="$SVN_ROOT/$component@$SVN_REVISION"
  svn export --quiet --force -r "$SVN_REVISION" "$pinned_url" "$export_dir"

  while read -r expected_sha path; do
    [[ "$expected_sha" =~ ^[0-9a-f]{64}$ ]] || \
      die "invalid SHA-256 in $retained"
    [[ "$path" == ./* ]] || die "unexpected retained path: $path"

    relative="${path#./}"
    source_path="$export_dir/$relative"
    target_path="$DEST/$component/$relative"
    [[ -f "$source_path" ]] || die "retained upstream file missing: $component/$relative"

    actual_sha="$(sha256sum "$source_path" | awk '{print $1}')"
    [[ "$actual_sha" == "$expected_sha" ]] || \
      die "retained upstream hash mismatch: $component/$relative"

    mkdir -p -- "$(dirname "$target_path")"
    cp -- "$source_path" "$target_path"
    copied_sha="$(sha256sum "$target_path" | awk '{print $1}')"
    [[ "$copied_sha" == "$expected_sha" ]] || \
      die "copied-file hash mismatch: $component/$relative"

    printf '%s  %s/%s\n' "$copied_sha" "$component" "$relative" >> "$ACTUAL"
  done < "$retained"
done

sort -o "$ACTUAL" "$ACTUAL"
if ! diff -u "$RECEIPT/retained-all.sha256" "$ACTUAL"; then
  die "materialized source does not match the retained aggregate receipt"
fi

actual_count="$(find "$DEST" -type f -printf '.' | wc -c | tr -d ' ')"
[[ "$actual_count" -eq "$EXPECTED_FILE_COUNT" ]] || \
  die "unexpected materialized file count: $actual_count"

actual_baseline_sha="$(sha256sum "$ACTUAL" | awk '{print $1}')"
[[ "$actual_baseline_sha" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "materialized baseline identity mismatch: $actual_baseline_sha"

# Defense in depth: these paths must be absent even if the selector changes.
[[ ! -e "$DEST/psycle-core/src/seib/vst" ]] || die "Seib VST quarantine materialized"
[[ ! -e "$DEST/psycle-audiodrivers/src/asio" ]] || die "ASIO quarantine materialized"
[[ -f "$DEST/psycle-plugins/src/psycle/plugins/plugin.hpp" ]] || \
  die "required native plugin API header missing"
[[ "$(find "$DEST/psycle-plugins" -type f | wc -l | tr -d ' ')" -eq 1 ]] || \
  die "unexpected plugin-tree material entered first baseline"

printf '%s\n' "$actual_baseline_sha" > "$DEST/.phase6b-retained-manifest.sha256"
# The identity marker is generated metadata, not part of the 291 upstream-file count.

echo "phase6b-materialize-source: PASS upstream-files=$actual_count baseline=$actual_baseline_sha"
