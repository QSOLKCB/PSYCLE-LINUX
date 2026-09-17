#!/usr/bin/env bash
set -euo pipefail

# The retained manifest is a content identity, so ordering must be host-independent.
export LC_ALL=C

RECEIPT="${1:-phase6-upstream-audit}"
OUT="${2:-phase6b-sanitized-manifest}"
COMPONENTS=(
  universalis
  psycle-core
  psycle-audiodrivers
  psycle-helpers
  psycle-player
  psycle-plugins
)

EXPECTED_UPSTREAM=936
EXPECTED_RETAINED=291
EXPECTED_OMITTED=645
EXPECTED_BASELINE_SHA256="00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a"

die() {
  echo "phase6b-sanitized-manifest: $*" >&2
  exit 2
}

[[ -d "$RECEIPT/components" ]] || \
  die "missing Phase 6A receipt directory: $RECEIPT/components"
if ! mkdir -- "$OUT"; then
  die "refusing existing or invalid output directory: $OUT"
fi
mkdir -- "$OUT/retained" "$OUT/omitted"

: > "$OUT/retained-all.sha256"
printf 'component\tupstream\tretained\tomitted\n' > "$OUT/counts.tsv"

classify() {
  local component="$1"
  local path="$2"

  case "$component:$path" in
    psycle-core:./src/seib/vst/*)
      echo 'quarantine-seib-vst-expression-review'
      ;;
    psycle-audiodrivers:./src/asio/*)
      echo 'quarantine-asio-expression-review'
      ;;
    psycle-plugins:./src/psycle/plugins/plugin.hpp)
      echo 'retain'
      ;;
    psycle-plugins:./closed-source/*.dll)
      echo 'exclude-closed-source-prebuilt-binary'
      ;;
    psycle-plugins:./src/psycle/plugins/jme/blitzn/songs/Rm-Im_in_a_place_i_dont_belong.psy|\
    psycle-plugins:./src/psycle/plugins/jme/blitzn/songs/voskomo_-_hawkeye_loader.psy|\
    psycle-plugins:./src/psycle/plugins/jme/gamefxn/songs/example.psy)
      echo 'exclude-unresolved-song-redistribution'
      ;;
    psycle-plugins:./src/psycle/plugins/y_midi/gmnames.h)
      echo 'exclude-steinberg-vst-sdk-derived'
      ;;
    psycle-plugins:*)
      echo 'defer-plugin-material-not-required-for-first-player-baseline'
      ;;
    *)
      echo 'retain'
      ;;
  esac
}

for component in "${COMPONENTS[@]}"; do
  manifest="$RECEIPT/components/$component/files.sha256"
  [[ -f "$manifest" ]] || die "missing frozen manifest: $manifest"

  retained="$OUT/retained/$component.sha256"
  omitted="$OUT/omitted/$component.tsv"
  : > "$retained"
  printf 'reason\tsha256\tpath\n' > "$omitted"

  upstream_count=0
  retained_count=0
  omitted_count=0

  while read -r sha path; do
    [[ "$sha" =~ ^[0-9a-f]{64}$ ]] || die "invalid sha256 in $manifest"
    [[ "$path" == ./* ]] || die "unexpected path in $manifest: $path"
    upstream_count=$((upstream_count + 1))

    reason="$(classify "$component" "$path")"
    if [[ "$reason" == retain ]]; then
      printf '%s  %s\n' "$sha" "$path" >> "$retained"
      printf '%s  %s/%s\n' \
        "$sha" "$component" "${path#./}" >> "$OUT/retained-all.sha256"
      retained_count=$((retained_count + 1))
    else
      printf '%s\t%s\t%s\n' "$reason" "$sha" "$path" >> "$omitted"
      omitted_count=$((omitted_count + 1))
    fi
  done < "$manifest"

  [[ $((retained_count + omitted_count)) -eq $upstream_count ]] || \
    die "accounting mismatch for $component"
  printf '%s\t%d\t%d\t%d\n' \
    "$component" "$upstream_count" "$retained_count" "$omitted_count" \
    >> "$OUT/counts.tsv"
done

sort -o "$OUT/retained-all.sha256" "$OUT/retained-all.sha256"
baseline_sha="$(sha256sum "$OUT/retained-all.sha256" | awk '{print $1}')"
printf '%s\n' "$baseline_sha" > "$OUT/baseline.sha256"

read -r total_upstream total_retained total_omitted < <(
  awk -F '\t' 'NR > 1 {u += $2; r += $3; o += $4} END {print u, r, o}' \
    "$OUT/counts.tsv"
)

[[ $((total_retained + total_omitted)) -eq $total_upstream ]] || \
  die 'global accounting mismatch'
[[ "$total_upstream" -eq "$EXPECTED_UPSTREAM" ]] || \
  die "unexpected frozen upstream count: $total_upstream"
[[ "$total_retained" -eq "$EXPECTED_RETAINED" ]] || \
  die "unexpected sanitized retained count: $total_retained"
[[ "$total_omitted" -eq "$EXPECTED_OMITTED" ]] || \
  die "unexpected sanitized omission/defer count: $total_omitted"
[[ "$baseline_sha" == "$EXPECTED_BASELINE_SHA256" ]] || \
  die "sanitized retained-manifest identity mismatch: $baseline_sha"

cat > "$OUT/summary.md" <<EOF
# Phase 6B sanitized C++ baseline receipt

- Frozen SourceForge observation revision: \`r12005\`
- Upstream files across six components: **$total_upstream**
- First sanitized baseline retained files: **$total_retained**
- Omitted/quarantined/deferred files: **$total_omitted**
- Retained-manifest identity (SHA-256): \`$baseline_sha\`

This receipt defines the first engine/player baseline only. Deferred plugin material is not declared non-redistributable; it is simply outside the minimum Phase 6B player-build slice and can return through separately audited preservation PRs.
EOF

echo "phase6b-sanitized-manifest: PASS baseline=$baseline_sha retained=$total_retained omitted=$total_omitted"
