#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-moreamp-eq}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/moreamp_eq/src"
PLUGIN="$CPSYCLE/plugins/build/maeq.so"
NATIVE_BIN="$OUT/phase5-moreamp-eq"
STATE_BIN="$OUT/phase5-moreamp-eq-state"
NATIVE_LOG="$OUT/phase5-moreamp-eq.log"
STATE_LOG="$OUT/phase5-moreamp-eq-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "MoreAmp EQ shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "MoreAmp EQ clean left the generated shared object behind" >&2
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
    echo "rebuilt MoreAmp EQ shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_moreamp_eq_preservation.cpp" \
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
    "$ROOT/tests/phase5_moreamp_eq_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-moreamp-eq: metadata PASS version=0x0100 slots=36 state=35 labels=1 identity=maEQ' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: defaults PASS constructor=published-36-slot-state' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: describe-link PASS mode10=inactive-dashes mode31=all-bands link1000=32/36/40/44/40/36/32' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: preamp PASS default=unity plus6-scale=1.51571666' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: band10 PASS active=1000Hz unstarred20=ignored markers=3' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: band31 PASS active=20Hz marker0=source-derived' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: extra PASS cascade=two-pass differs=single marker0=source-derived' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: nonpositive PASS extra=on zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: samplerate PASS active1k=both 20k=44100-only history=reset markers=3' "$NATIVE_LOG"
grep -Fqx 'phase5-moreamp-eq: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-moreamp-eq-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: maeq:0' "$STATE_LOG"
grep -Fqx 'state: 35 MPF_STATE controls seeded non-default across 36 ABI slots, separators=1-header, bands=31, extra=on, link=on, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: MoreAmp EQ -> Master' "$STATE_LOG"

[[ -s "$OUT/phase5-moreamp-eq.prs" ]]
[[ -s "$OUT/phase5-moreamp-eq.psy" ]]

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C MoreAmp EQ Preservation

- Retained `moreamp_eq` source builds independently as `maeq.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`, with the deleter exercised through its exact reference signature: PASS
- Historical identity `MoreAmp EQ` / `maEQ` / `Felipe Rivera/pmisteli/Sartorius`, version `0x0100`, effect type and three-column geometry: PASS
- Complete 36-slot ABI surface: 35 state controls plus one labeled header: PASS
- Constructor storage is seeded from all 36 published defaults before `Init()` and host default callbacks: PASS
- Historical value descriptions and 10-band/31-band display behavior: PASS
- Linked 1 kHz control preserves the retained `32/36/40/44/40/36/32` interpolation across neighboring bands: PASS
- Flat default path preserves unity output and +6 dB preamp matches the retained source-derived scaling: PASS
- 10-band mode uses the starred 1 kHz control and ignores the unstarred 20 Hz control, with three source-derived response markers: PASS
- 31-band mode activates the 20 Hz control with a source-derived first-sample response marker: PASS
- Extra filtering preserves the retained second IIR cascade with a source-derived first-sample response marker: PASS
- Extra-enabled zero and negative host callback sizes are strict no-ops: PASS
- Live 44.1 kHz -> 32 kHz transition keeps 1 kHz active at both rates, disables the 20 kHz band above Nyquist, and resets pre-filled filter history as proven by three clean-history source-derived markers: PASS
- Production `PluginCatcher` identity `maeq:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves all 36 slots with all 35 state controls non-default: PASS
- Fresh PSY3 reopen preserves all stored values and the MoreAmp EQ -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical public parameter slots: PASS

Production correctness changes are limited to deterministic initialization of the published
36-slot `Vals[]` state and standalone makefile output/clean hygiene. Positive-count EQ,
preamp, link, dither, band-selection, extra-filter and sample-rate equations are unchanged.
EOF

cat "$SUMMARY"
