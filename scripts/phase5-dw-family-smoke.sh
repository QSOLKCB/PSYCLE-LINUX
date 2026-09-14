#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-dw-family}"
if [[ "$OUT" != /* ]]; then OUT="$ROOT/$OUT"; fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIRS=(
  "$CPSYCLE/plugins/dw_eq/src"
  "$CPSYCLE/plugins/dw_granulizer/src"
  "$CPSYCLE/plugins/dw_iopan/src"
  "$CPSYCLE/plugins/dw_tremolo/src"
)
PLUGINS=(
  "$CPSYCLE/plugins/build/dw-eq.so"
  "$CPSYCLE/plugins/build/dw-granulizer.so"
  "$CPSYCLE/plugins/build/dw-iopan.so"
  "$CPSYCLE/plugins/build/dw-tremolo.so"
)

NATIVE_BIN="$OUT/phase5-dw-family"
STATE_BIN="$OUT/phase5-dw-family-state"
NATIVE_LOG="$OUT/phase5-dw-family.log"
STATE_LOG="$OUT/phase5-dw-family-state.log"
SUMMARY="$OUT/summary.md"

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/dsp/src"

for i in "${!PLUGIN_DIRS[@]}"; do
  rm -f "${PLUGINS[$i]}"
  make -C "${PLUGIN_DIRS[$i]}"
  [[ -s "${PLUGINS[$i]}" ]] || {
    echo "DW shared object missing after direct build: ${PLUGINS[$i]}" >&2
    exit 1
  }
  make -C "${PLUGIN_DIRS[$i]}" clean
  [[ ! -e "${PLUGINS[$i]}" ]] || {
    echo "DW direct clean left generated module behind: ${PLUGINS[$i]}" >&2
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
    echo "rebuilt DW shared object missing: $plugin" >&2
    exit 1
  }
done

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$CPSYCLE/plugins" \
  "$ROOT/tests/phase5_dw_family_preservation.cpp" \
  -ldl -o "$NATIVE_BIN"

# shellcheck disable=SC2207
LUA_CFLAGS=($(pkg-config --cflags lua))
# shellcheck disable=SC2207
LUA_LIBS=($(pkg-config --libs lua))

gcc -std=gnu11 -Wall -Wextra -Werror=implicit-function-declaration \
  -I"$CPSYCLE/audio/src" -I"$CPSYCLE/thread/src" \
  -I"$CPSYCLE/script/src" -I"$CPSYCLE/container/src" \
  -I"$CPSYCLE/file/src" -I"$CPSYCLE/dsp/src" \
  -I"$CPSYCLE/diversalis/src" "${LUA_CFLAGS[@]}" \
  "$ROOT/tests/phase5_dw_family_persistence.c" -o "$STATE_BIN" \
  -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
  -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
  -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
  -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
  -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "${PLUGINS[@]}" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "${PLUGINS[@]}" 2>&1 | tee "$STATE_LOG"

grep -F 'dw-metadata-hash[dw eq]=0xc82fe5a084c00d43' "$NATIVE_LOG" >/dev/null
grep -F 'dw-metadata-hash[dw granulizer]=0x77e34d124f74ed41' "$NATIVE_LOG" >/dev/null
grep -F 'dw-metadata-hash[dw IoPan]=0xd2b2cf8908d12251' "$NATIVE_LOG" >/dev/null
grep -F 'dw-metadata-hash[dw Tremolo]=0x637aa08128ba99b8' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: machine PASS [dw eq]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: machine PASS [dw granulizer]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: machine PASS [dw IoPan]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: machine PASS [dw Tremolo]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: EQ rate PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: Granulizer PASS fixed-grain=10@44.1k/20@88.2k random-mod=off' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: IoPan PASS default=unity flip=full-swap' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: Tremolo rate PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dw-family: PASS machines=4' "$NATIVE_LOG" >/dev/null

grep -F 'phase5-dw-family-state: seed PASS [dw eq]' "$STATE_LOG" >/dev/null
grep -F 'phase5-dw-family-state: seed PASS [dw granulizer]' "$STATE_LOG" >/dev/null
grep -F 'phase5-dw-family-state: seed PASS [dw IoPan]' "$STATE_LOG" >/dev/null
grep -F 'phase5-dw-family-state: seed PASS [dw Tremolo]' "$STATE_LOG" >/dev/null
grep -F 'phase5-dw-family-state: PASS machines=4' "$STATE_LOG" >/dev/null
grep -F 'catchers: dw-eq:0 dw-granulizer:0 dw-iopan:0 dw-tremolo:0' "$STATE_LOG" >/dev/null
grep -F 'state: EQ 12/12, Granulizer 36 writable non-default + runtime display state, IoPan 4/4, Tremolo 8/8; 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'topology: 4/4 DW effects -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-dw-family.psy" ]] || {
  echo "DW family PSY3 evidence missing" >&2
  exit 1
}
for i in 0 1 2 3; do
  [[ -s "$OUT/phase5-dw-$i.prs" ]] || {
    echo "DW preset evidence missing: phase5-dw-$i.prs" >&2
    exit 1
  }
done

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C D. W. Aley Family Preservation

- Four retained source-built DW identities audited: dw eq, dw granulizer, dw IoPan, dw Tremolo.
- All four build independently as Linux native-machine `.so` modules and clean only their own outputs: PASS
- Native ABI identity/version/type/geometry and complete parameter tables are frozen by exact hashes: PASS
  - dw eq: `0xc82fe5a084c00d43`
  - dw granulizer: `0x77e34d124f74ed41`
  - dw IoPan: `0xd2b2cf8908d12251`
  - dw Tremolo: `0x637aa08128ba99b8`
- dw eq default unity and live scaled-wrapper 44.1 -> 88.2 kHz coefficient reconfiguration: PASS
- dw granulizer deterministic fixed-grain duration scales 10 samples @44.1 kHz -> 20 @88.2 kHz with random modulation disabled: PASS
- dw IoPan default unity and historical full channel-flip matrix: PASS
- dw Tremolo Depth=0 unity and live 44.1 -> 88.2 kHz wall-clock LFO timing: PASS
- Production catcher identities: dw-eq:0, dw-granulizer:0, dw-iopan:0, dw-tremolo:0: PASS
- Public state seeds: EQ 12/12; Granulizer 36 directly writable controls plus derived runtime display state; IoPan 4/4; Tremolo 8/8: PASS
- Version-1 preset round-trip for each DW effect through a fresh MachineFactory instance: PASS
- One-song four-machine fresh PSY3 reopen and all four DW -> Master topology edges: PASS
- Opaque state: none.
EOF

cat "$SUMMARY"
