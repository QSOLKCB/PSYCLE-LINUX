#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-haas}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/haas/src"
PLUGIN="$CPSYCLE/plugins/build/haas.so"
NATIVE_BIN="$OUT/phase5-haas"
STATE_BIN="$OUT/phase5-haas-state"
NATIVE_LOG="$OUT/phase5-haas.log"
STATE_LOG="$OUT/phase5-haas-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Haas shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Haas clean left the generated shared object behind" >&2
    exit 1
}

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "rebuilt Haas shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_haas_preservation.cpp" \
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
    "$ROOT/tests/phase5_haas_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-haas: metadata PASS version=0x0100 slots=17 state=13 labels=3 null=1 identity=Haas' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: describe PASS delta=0 delay=ms pan=0 gain=dB mix=normal/swapped/mono' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: channel-mix PASS normal=right swapped=left mono=dual mono-input=sum(L+R)' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: signed-delay PASS +6ms=R0/L264 -6ms=L0/R264 rate=44100' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: samplerate PASS direct-delta +6ms 44100=264 88200=529 stale264=zero' "$NATIVE_LOG"
grep -Fqx 'phase5-haas: PASS' "$NATIVE_LOG"
grep -Fqx 'phase5-haas-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: haas:0' "$STATE_LOG"
grep -Fqx 'state: 13 MPF_STATE controls seeded non-default across 17 ABI slots, separators=3-label+1-null, channel-mix=swapped, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: Haas stereo time delay spatial localization -> Master' "$STATE_LOG"

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Haas Preservation

- Retained `haas` source builds independently as `haas.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Haas stereo time delay spatial localization` / `Haas` / `bohan/dilvie collaboration`, version `0x0100`, effect type and one-column geometry: PASS
- Complete 17-slot ABI surface: 13 state controls, three labeled separators and one null separator: PASS
- Specialized descriptions for zero delta/pan, millisecond delays, dB gain and `normal` / `swapped` / `mono`: PASS
- Zero and negative host callback counts remain strict no-ops through the retained bounded loops: PASS
- Direct wet routing preserves mono input sum and historical normal/swapped/mono output semantics: PASS
- Signed direct Haas delta preserves ear direction at 44.1 kHz (`+6 ms = R0/L264`, `-6 ms = L0/R264`): PASS
- Live sample-rate transition reconstructs the +6 ms offset from 264 samples at 44.1 kHz to 529 samples at 88.2 kHz: PASS
- Production `PluginCatcher` identity `haas:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves all 17 stored slots with 13 non-default state controls: PASS
- Fresh PSY3 reopen preserves all stored values and the Haas -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical public parameter slots: PASS

No positive-count Haas DSP equation is changed by this slice. Production changes are
limited to standalone makefile output-directory and clean-target hardening.
EOF

cat "$SUMMARY"
