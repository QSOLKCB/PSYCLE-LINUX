#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-stk-family}"
if [[ "$OUT" != /* ]]; then OUT="$ROOT/$OUT"; fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIRS=(
  "$CPSYCLE/plugins/stk-plugged/src"
  "$CPSYCLE/plugins/stk-reverbs/src"
  "$CPSYCLE/plugins/stk-shakers/src"
)
PLUGINS=(
  "$CPSYCLE/plugins/build/stk-plucked.so"
  "$CPSYCLE/plugins/build/stk-reverbs.so"
  "$CPSYCLE/plugins/build/stk-shakers.so"
)

NATIVE_BIN="$OUT/phase5-stk-family"
STATE_BIN="$OUT/phase5-stk-family-state"
NATIVE_LOG="$OUT/phase5-stk-family.log"
STATE_LOG="$OUT/phase5-stk-family-state.log"
SUMMARY="$OUT/summary.md"

# Prove each retained STK wrapper builds against the supported Linux libstk-dev
# boundary, cleans its own generated module, and rebuilds independently.
for i in "${!PLUGIN_DIRS[@]}"; do
  rm -f "${PLUGINS[$i]}"
  make -C "${PLUGIN_DIRS[$i]}"
  [[ -s "${PLUGINS[$i]}" ]] || {
    echo "STK shared object missing after direct build: ${PLUGINS[$i]}" >&2
    exit 1
  }
  make -C "${PLUGIN_DIRS[$i]}" clean
  [[ ! -e "${PLUGINS[$i]}" ]] || {
    echo "STK direct clean left generated module behind: ${PLUGINS[$i]}" >&2
    exit 1
  }
done

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
for dir in "${PLUGIN_DIRS[@]}"; do make -C "$dir"; done
for plugin in "${PLUGINS[@]}"; do
  [[ -s "$plugin" ]] || {
    echo "rebuilt STK shared object missing: $plugin" >&2
    exit 1
  }
done

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$CPSYCLE/plugins" \
  "$ROOT/tests/phase5_stk_family_preservation.cpp" \
  -ldl -lstk -o "$NATIVE_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
  -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
  -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
  -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
  -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
  "$ROOT/tests/phase5_stk_family_persistence.c" -o "$STATE_BIN" \
  -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
  -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
  -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
  -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
  -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "${PLUGINS[@]}" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "${PLUGINS[@]}" 2>&1 | tee "$STATE_LOG"

grep -F 'stk-metadata-hash[stk Plucked]=0x55e20a3f7e90f611' "$NATIVE_LOG" >/dev/null
grep -F 'stk-metadata-hash[stk Reverbs]=0xf92219f73194a3ae' "$NATIVE_LOG" >/dev/null
grep -F 'stk-metadata-hash[stk Shakers]=0x4c29aa201e9bb317' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: machine PASS [stk Plucked]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: machine PASS [stk Reverbs]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: machine PASS [stk Shakers]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: Plucked PASS idle=zero 0C00=zero Stop=zero live-rate=STK-reference' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: Reverbs PASS dry=unity algorithms=STK-reference routing=bidirectional live-rate=STK-reference' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: Shakers PASS map=48..70->STK-reference 0C00=zero Stop=zero live-rate=STK-reference' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-stk-family: PASS machines=3' "$NATIVE_LOG" >/dev/null

grep -F 'phase5-stk-family-state: seed PASS [stk Plucked] changed=5' "$STATE_LOG" >/dev/null
grep -F 'phase5-stk-family-state: seed PASS [stk Reverbs] changed=4' "$STATE_LOG" >/dev/null
grep -F 'phase5-stk-family-state: seed PASS [stk Shakers] changed=6' "$STATE_LOG" >/dev/null
grep -F 'phase5-stk-family-state: PASS machines=3' "$STATE_LOG" >/dev/null
grep -F 'catchers: stk-plucked:0 stk-reverbs:0 stk-shakers:0' "$STATE_LOG" >/dev/null
grep -F 'state: Plucked 5/5; Reverbs 4/4; Shakers 6/6; 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent PluginCatcher/MachineFactory' "$STATE_LOG" >/dev/null
grep -F 'topology: 3/3 STK wrappers -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-stk-family.psy" ]] || {
  echo "STK family PSY3 evidence missing" >&2
  exit 1
}
for i in 0 1 2; do
  [[ -s "$OUT/phase5-stk-$i.prs" ]] || {
    echo "STK preset evidence missing: phase5-stk-$i.prs" >&2
    exit 1
  }
done

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C STK-derived Family Preservation

- Three retained source-built STK wrappers audited: stk Plucked, stk Reverbs, stk Shakers.
- Linux dependency boundary: distro `libstk-dev`; retained STK 4.5.0 archive remains provenance/history evidence.
- All three build independently as Linux native-machine `.so` modules and clean only their own outputs: PASS
- Native ABI identity/version/type/geometry and complete parameter tables are frozen by exact hashes: PASS
  - stk Plucked: `0x55e20a3f7e90f611`
  - stk Reverbs: `0xf92219f73194a3ae`
  - stk Shakers: `0x4c29aa201e9bb317`
- stk Plucked idle silence, historical 0C00 mute, Stop clearing, and live 44.1 -> 88.2 kHz rendering matched against direct system-STK behavior: PASS
- stk Reverbs exact Dry/Wet=0 bypass, JCRev/NRev/PRCRev selector outputs matched against direct system-STK references, both directions of independent/mixed stereo routing, and live 44.1 -> 88.2 kHz system-STK reference rendering: PASS
- stk Shakers all historical notes 48..70 matched against the frozen old->new instrument map using seeded direct system-STK references; 0C00 mute, Stop silence, and live 44.1 -> 88.2 kHz reference rendering: PASS
- Production catcher identities: stk-plucked:0, stk-reverbs:0, stk-shakers:0: PASS
- Public state: Plucked 5/5, Reverbs 4/4, Shakers 6/6; zero opaque bytes: PASS
- Version-1 preset round-trip for each wrapper through a separate independently registered PluginCatcher/MachineFactory stack: PASS
- One-song three-machine fresh PSY3 reopen and all three wrapper -> Master topology edges: PASS
EOF

cat "$SUMMARY"
