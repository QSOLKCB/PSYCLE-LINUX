#!/usr/bin/env bash
set -euo pipefail

# Manifest hashes are content identities, so filename ordering must not depend on
# the invoking machine's locale.
export LC_ALL=C

OUT="${1:-phase6-upstream-audit}"
SVN_REVISION="12005"
SVN_ROOT="https://svn.code.sf.net/p/psycle/code/trunk"
REFERENCE_VERSION="1.12.0"
REFERENCE_ARCH="x86"
REFERENCE_FILE="PsycleInstallerx86-1.12.0.exe"
REFERENCE_URL="https://sourceforge.net/projects/psycle/files/psycle/1.12/${REFERENCE_FILE}/download"
EXPECTED_REFERENCE_SHA256="f42c7f542011804346dd924f011684ac40fd7c62c1b25c5de72776f88ea86769"
EXPECTED_REFERENCE_SIZE="9322919"

COMPONENTS=(
  universalis
  psycle-core
  psycle-audiodrivers
  psycle-helpers
  psycle-player
  psycle-plugins
)

declare -A EXPECTED_MANIFEST_SHA256=(
  [universalis]="827586daad2efcfbc10466394670a4a0e5f208da94afb7b3bf72239d396a618e"
  [psycle-core]="eb25467bdfbdea7296fc2c8e01c802e3b2b977d95775ab363cc810309aee729b"
  [psycle-audiodrivers]="4518274595b58fa89f59ca9012198e4602bdabb32216193edc38fdbe7aef0ab1"
  [psycle-helpers]="13d05df94637cef8701fee0555bb4a9915fd082dcd531ae2b772c4a633f3f5db"
  [psycle-player]="6fd4fb58b3841b89cafd69c1c4a6c4f5c95864f1d7edb6e6f999621bfadecb6f"
  [psycle-plugins]="a8d66a18e363229ad9ff13b688149fec4682da8b177afb757c064491123de888"
)

declare -A EXPECTED_FILE_COUNT=(
  [universalis]="83"
  [psycle-core]="108"
  [psycle-audiodrivers]="36"
  [psycle-helpers]="68"
  [psycle-player]="7"
  [psycle-plugins]="634"
)

declare -A EXPECTED_LAST_CHANGED_REV=(
  [universalis]="12004"
  [psycle-core]="10901"
  [psycle-audiodrivers]="12004"
  [psycle-helpers]="12004"
  [psycle-player]="10725"
  [psycle-plugins]="12004"
)

for command_name in svn curl sha256sum find sort grep sed stat awk python3; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "phase6-upstream-audit: missing required command: $command_name" >&2
    exit 2
  fi
done

# The output path is caller-controlled, so never recursively clean or reuse it.
# Requiring mkdir itself to create the directory makes broad paths such as /tmp,
# $HOME, the checkout root, existing files, and symlinks fail safely before any
# upstream retrieval begins. A contributor who wants to rerun the audit must
# remove the prior dedicated receipt directory explicitly or choose a fresh path.
if ! mkdir -- "$OUT"; then
  echo "phase6-upstream-audit: refusing existing or invalid output path: $OUT" >&2
  echo "choose a fresh output directory; this audit never deletes caller-supplied paths" >&2
  exit 2
fi
mkdir -- "$OUT/components"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

svn_info_with_retry() {
  local pinned_url="$1"
  local output_path="$2"
  local attempt tmp_path
  tmp_path="${output_path}.tmp"

  for attempt in 1 2 3 4; do
    rm -f -- "$tmp_path"
    if svn info -r "$SVN_REVISION" "$pinned_url" > "$tmp_path"; then
      mv -- "$tmp_path" "$output_path"
      return 0
    fi
    rm -f -- "$tmp_path"
    if [ "$attempt" -lt 4 ]; then
      echo "phase6-upstream-audit: svn info transient failure for $pinned_url; retry $attempt/3" >&2
      sleep "$((attempt * 2))"
    fi
  done

  echo "phase6-upstream-audit: svn info failed after 4 attempts: $pinned_url" >&2
  return 1
}

svn_export_with_retry() {
  local pinned_url="$1"
  local destination="$2"
  local attempt

  case "$destination" in
    "$WORK"/*) ;;
    *)
      echo "phase6-upstream-audit: refusing retry cleanup outside work root: $destination" >&2
      return 2
      ;;
  esac

  for attempt in 1 2 3 4; do
    rm -rf -- "$destination"
    if svn export --quiet --force -r "$SVN_REVISION" "$pinned_url" "$destination"; then
      return 0
    fi
    rm -rf -- "$destination"
    if [ "$attempt" -lt 4 ]; then
      echo "phase6-upstream-audit: svn export transient failure for $pinned_url; retry $attempt/3" >&2
      sleep "$((attempt * 2))"
    fi
  done

  echo "phase6-upstream-audit: svn export failed after 4 attempts: $pinned_url" >&2
  return 1
}

SUMMARY="$OUT/summary.md"
{
  echo '# Phase 6 upstream provenance receipt'
  echo
  echo "- SVN repository root: \`$SVN_ROOT\`"
  echo "- Pinned repository revision: \`r$SVN_REVISION\`"
  echo "- Primary original-Psycle reference: \`$REFERENCE_VERSION $REFERENCE_ARCH\` / \`$REFERENCE_FILE\`"
  echo '- Imported source or original-Psycle binary retained in this artifact: **no**'
  echo '- Manifest collation: `LC_ALL=C`'
  echo
  echo '## C++ reimplementation snapshot'
  echo
  echo '| Component | Last-changed revision | Files | Manifest SHA-256 | Frozen identity | Review hints |'
  echo '| --- | ---: | ---: | --- | --- | ---: |'
} > "$SUMMARY"

for component in "${COMPONENTS[@]}"; do
  url="$SVN_ROOT/$component"
  pinned_url="${url}@${SVN_REVISION}"
  component_out="$OUT/components/$component"
  export_dir="$WORK/$component"
  mkdir -p "$component_out"

  # Use an explicit peg revision as well as an operative revision so a future
  # rename/delete/replacement at HEAD cannot make the frozen historical node
  # unreachable to this audit.
  svn_info_with_retry "$pinned_url" "$component_out/svn-info.txt"
  svn_export_with_retry "$pinned_url" "$export_dir"

  (
    cd "$export_dir"
    find . -type f -print0 | sort -z | xargs -0 -r sha256sum
  ) > "$component_out/files.sha256"

  manifest_sha="$(sha256sum "$component_out/files.sha256" | awk '{print $1}')"
  file_count="$(find "$export_dir" -type f -printf '.' | wc -c | tr -d ' ')"
  last_changed_rev="$(awk -F': ' '/^Last Changed Rev:/ {print $2}' "$component_out/svn-info.txt")"

  if [ "$manifest_sha" != "${EXPECTED_MANIFEST_SHA256[$component]}" ]; then
    echo "phase6-upstream-audit: $component manifest SHA-256 mismatch" >&2
    echo "expected: ${EXPECTED_MANIFEST_SHA256[$component]}" >&2
    echo "actual:   $manifest_sha" >&2
    exit 3
  fi
  if [ "$file_count" != "${EXPECTED_FILE_COUNT[$component]}" ]; then
    echo "phase6-upstream-audit: $component file-count mismatch" >&2
    echo "expected: ${EXPECTED_FILE_COUNT[$component]}" >&2
    echo "actual:   $file_count" >&2
    exit 4
  fi
  if [ "$last_changed_rev" != "${EXPECTED_LAST_CHANGED_REV[$component]}" ]; then
    echo "phase6-upstream-audit: $component last-changed revision mismatch" >&2
    echo "expected: ${EXPECTED_LAST_CHANGED_REV[$component]}" >&2
    echo "actual:   $last_changed_rev" >&2
    exit 5
  fi

  find "$export_dir" -type f \
    \( -iname 'COPYING*' -o -iname 'LICENSE*' -o -iname 'LICENCE*' -o -iname 'NOTICE*' -o -iname 'AUTHORS*' -o -iname 'README*' \) \
    -printf '%P\n' | sort > "$component_out/licensing-candidates.txt"

  # This is deliberately a broad filename/path hint, not a classifier. Match
  # conventional names such as vsthost.cpp, asiodriver/, asio-2/, and SDK files.
  find "$export_dir" -type f -printf '%P\n' \
    | grep -Ei '(asio|vst|steinberg|boost|zlib|lua|ladspa|lv2|portaudio|stk|fluidsynth|fftw|libxml|7z)' \
    | sort -u > "$component_out/dependency-path-hints.txt" || true

  find "$export_dir" -type f -printf '%P\n' \
    | grep -Ei '\.(dll|exe|lib|a|so|psy|wav|mp3|ogg|zip|7z|obj)$' \
    | sort -u > "$component_out/redistribution-review-hints.txt" || true

  # Retain short provenance-relevant text matches rather than source files. These
  # are review hints only; they do not establish a license by themselves.
  grep -RInaI -E \
    'copyright|licen[cs]e|public domain|GNU (General|Lesser) Public|GPL|LGPL|MIT|BSD|Steinberg|ASIO SDK|VST (Plug|SDK)|Audacity|STK' \
    "$export_dir" \
    | sed "s#${export_dir}/##" \
    | head -n 1500 > "$component_out/notice-hints.txt" || true

  review_hint_count="$(wc -l < "$component_out/redistribution-review-hints.txt" | tr -d ' ')"

  printf '| `%s` | `%s` | %s | `%s` | **PASS** | %s |\n' \
    "$component" "$last_changed_rev" "$file_count" "$manifest_sha" "$review_hint_count" >> "$SUMMARY"
done

REFERENCE_DIR="$OUT/original-psycle-reference"
mkdir -p "$REFERENCE_DIR"
REFERENCE_TMP="$WORK/$REFERENCE_FILE"
FINAL_URL_FILE="$REFERENCE_DIR/final-url.txt"

curl --fail --location --retry 3 --retry-all-errors \
  --silent --show-error \
  --output "$REFERENCE_TMP" \
  --write-out '%{url_effective}\n' \
  "$REFERENCE_URL" > "$FINAL_URL_FILE"

python3 - "$REFERENCE_TMP" <<'PY'
import pathlib
import sys
path = pathlib.Path(sys.argv[1])
with path.open('rb') as handle:
    magic = handle.read(2)
if magic != b'MZ':
    raise SystemExit(f'original-Psycle reference is not a PE executable: magic={magic!r}')
PY

reference_sha="$(sha256sum "$REFERENCE_TMP" | awk '{print $1}')"
reference_size="$(stat -c '%s' "$REFERENCE_TMP")"
printf '%s  %s\n' "$reference_sha" "$REFERENCE_FILE" > "$REFERENCE_DIR/sha256.txt"
printf '%s\n' "$reference_size" > "$REFERENCE_DIR/size-bytes.txt"
printf '%s\n' "$REFERENCE_VERSION" > "$REFERENCE_DIR/version.txt"
printf '%s\n' "$REFERENCE_ARCH" > "$REFERENCE_DIR/architecture.txt"

if [ "$reference_sha" != "$EXPECTED_REFERENCE_SHA256" ]; then
  echo "phase6-upstream-audit: original reference SHA-256 mismatch" >&2
  echo "expected: $EXPECTED_REFERENCE_SHA256" >&2
  echo "actual:   $reference_sha" >&2
  exit 6
fi
if [ "$reference_size" != "$EXPECTED_REFERENCE_SIZE" ]; then
  echo "phase6-upstream-audit: original reference size mismatch" >&2
  echo "expected: $EXPECTED_REFERENCE_SIZE" >&2
  echo "actual:   $reference_size" >&2
  exit 7
fi

{
  echo
  echo '## Original Psycle reference'
  echo
  echo "- Version: \`$REFERENCE_VERSION\`"
  echo "- Primary architecture/build: \`$REFERENCE_ARCH\` / \`$REFERENCE_FILE\`"
  echo "- SourceForge origin: $REFERENCE_URL"
  echo "- Downloaded size: \`$reference_size\` bytes"
  echo "- SHA-256: \`$reference_sha\`"
  echo '- SHA-256 and size pin verification: **PASS**'
  echo
  echo '## Safety / redistribution boundary'
  echo
  echo '- The SourceForge trees are exported only into a temporary directory.'
  echo '- Only SVN metadata, per-file hashes, licensing/dependency/notice hints and redistribution-review path hints are retained.'
  echo '- The original Psycle executable is downloaded only to verify identity and is deleted before artifact upload.'
  echo '- No upstream source tree, executable, song, plugin binary, SDK or asset is copied into the audit artifact.'
} >> "$SUMMARY"

echo "phase6-upstream-audit: PASS"
echo "receipt: $SUMMARY"
