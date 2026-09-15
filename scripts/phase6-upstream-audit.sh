#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-phase6-upstream-audit}"
SVN_REVISION="${PSYCLE_SVN_REVISION:-12005}"
SVN_ROOT="${PSYCLE_SVN_ROOT:-https://svn.code.sf.net/p/psycle/code/trunk}"
REFERENCE_VERSION="1.12.0"
REFERENCE_ARCH="x86"
REFERENCE_FILE="PsycleInstallerx86-1.12.0.exe"
REFERENCE_URL="https://sourceforge.net/projects/psycle/files/psycle/1.12/${REFERENCE_FILE}/download"
EXPECTED_REFERENCE_SHA256="${PSYCLE_REFERENCE_SHA256:-}"

COMPONENTS=(
  psycle-core
  psycle-audiodrivers
  psycle-helpers
  psycle-player
  psycle-plugins
)

for command_name in svn curl sha256sum find sort grep stat awk; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "phase6-upstream-audit: missing required command: $command_name" >&2
    exit 2
  fi
done

rm -rf "$OUT"
mkdir -p "$OUT/components"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

SUMMARY="$OUT/summary.md"
{
  echo '# Phase 6 upstream provenance receipt'
  echo
  echo "- SVN repository root: \`$SVN_ROOT\`"
  echo "- Pinned repository revision: \`r$SVN_REVISION\`"
  echo "- Primary original-Psycle reference: \`$REFERENCE_VERSION $REFERENCE_ARCH\` / \`$REFERENCE_FILE\`"
  echo '- Imported source or original-Psycle binary retained in this artifact: **no**'
  echo
  echo '## C++ reimplementation snapshot'
  echo
  echo '| Component | SVN path | Last-changed revision | Files | Manifest SHA-256 |'
  echo '| --- | --- | ---: | ---: | --- |'
} > "$SUMMARY"

for component in "${COMPONENTS[@]}"; do
  url="$SVN_ROOT/$component"
  component_out="$OUT/components/$component"
  export_dir="$WORK/$component"
  mkdir -p "$component_out"

  svn info -r "$SVN_REVISION" "$url" > "$component_out/svn-info.txt"
  svn export --quiet --force -r "$SVN_REVISION" "$url" "$export_dir"

  (
    cd "$export_dir"
    find . -type f -print0 | sort -z | xargs -0 -r sha256sum
  ) > "$component_out/files.sha256"

  manifest_sha="$(sha256sum "$component_out/files.sha256" | awk '{print $1}')"
  file_count="$(find "$export_dir" -type f -printf '.' | wc -c | tr -d ' ')"
  last_changed_rev="$(awk -F': ' '/^Last Changed Rev:/ {print $2}' "$component_out/svn-info.txt")"

  find "$export_dir" -type f \
    \( -iname 'COPYING*' -o -iname 'LICENSE*' -o -iname 'LICENCE*' -o -iname 'NOTICE*' -o -iname 'AUTHORS*' -o -iname 'README*' \) \
    -printf '%P\n' | sort > "$component_out/licensing-candidates.txt"

  find "$export_dir" -type f -printf '%P\n' \
    | grep -Ei '(^|/)(asio|vst|boost|zlib|lua|ladspa|lv2|portaudio|stk|fluidsynth|fftw|libxml|7z)(/|$)' \
    | sort -u > "$component_out/dependency-path-hints.txt" || true

  printf '| `%s` | `%s` | `%s` | %s | `%s` |\n' \
    "$component" "$url" "$last_changed_rev" "$file_count" "$manifest_sha" >> "$SUMMARY"
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

if [ -n "$EXPECTED_REFERENCE_SHA256" ] && [ "$reference_sha" != "$EXPECTED_REFERENCE_SHA256" ]; then
  echo "phase6-upstream-audit: original reference SHA-256 mismatch" >&2
  echo "expected: $EXPECTED_REFERENCE_SHA256" >&2
  echo "actual:   $reference_sha" >&2
  exit 3
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
  if [ -n "$EXPECTED_REFERENCE_SHA256" ]; then
    echo '- SHA-256 pin verification: **PASS**'
  else
    echo '- SHA-256 pin verification: **DISCOVERY RUN** — commit this observed hash before treating it as frozen.'
  fi
  echo
  echo '## Safety / redistribution boundary'
  echo
  echo '- The SourceForge trees are exported only into a temporary directory.'
  echo '- Only SVN metadata, per-file hashes, licensing-candidate paths and dependency-path hints are retained.'
  echo '- The original Psycle executable is downloaded only to compute identity and is deleted before artifact upload.'
  echo '- No upstream source tree, executable, song, plugin binary, SDK or asset is copied into the audit artifact.'
} >> "$SUMMARY"

echo "phase6-upstream-audit: PASS"
echo "receipt: $SUMMARY"
