#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-negative}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/negative/src"
PLUGIN="$CPSYCLE/plugins/build/negative.so"
NATIVE_BIN="$OUT/phase5-negative"
STATE_BIN="$OUT/phase5-negative-state"
NATIVE_LOG="$OUT/phase5-negative.log"
STATE_LOG="$OUT/phase5-negative-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Negative shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Negative clean left the generated shared object behind" >&2
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
    echo "rebuilt Negative shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" -I"$CPSYCLE/plugins/psycle" \
    "$ROOT/tests/phase5_negative_preservation.cpp" \
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
    "$ROOT/tests/phase5_negative_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-negative: metadata PASS version=0x0100 parameters=0 identity=Negative author=who-cares' "$NATIVE_LOG"
grep -Fqx 'phase5-negative: abi PASS create-delete=exact-reference-signatures' "$NATIVE_LOG"
grep -Fqx 'phase5-negative: help PASS contract=out-equals-minus-in' "$NATIVE_LOG"
grep -Fqx 'phase5-negative: inversion PASS max-block=256 stereo=exact signed-zero=yes infinity=yes bounded=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-negative: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-negative: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-negative-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: negative:0' "$STATE_LOG"
grep -Fqx 'state: 0 public parameters, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'audio: production host path exact stereo negation, bounded positive block, zero callback no-op' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: Negative -> Master' "$STATE_LOG"

[[ -s "$OUT/phase5-negative.prs" ]]
[[ -s "$OUT/phase5-negative.psy" ]]

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Negative Preservation

- Retained `negative` source builds independently as `negative.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Actual module exports the historical higher-level `plugin.hpp` ABI and exact reference-return/reference-delete lifecycle: PASS
- Historical identity `Negative` / `Negative` / `who cares`, version `0x0100`, effect type, one-column geometry and zero public parameters: PASS
- Historical help contract remains `just a Negative (out = -in)`: PASS
- Positive processing preserves exact stereo `out = -in` over Psycle's 256-sample maximum host block without crossing canary boundaries: PASS
- IEEE sign behavior for positive/negative zero and infinity remains exact unary negation: PASS
- Zero and negative direct callback counts are strict no-ops instead of walking backwards outside the host block: PASS
- Production `PluginCatcher` identity `negative:0` and `MachineFactory` instantiation: PASS
- Production machine-work path preserves exact stereo inversion, a bounded positive block and a zero-length no-op: PASS
- Version-1 zero-parameter preset restore through an independent catcher/factory: PASS
- Fresh PSY3 reopen preserves the zero-state contract and `Negative -> Master` topology edge: PASS
- Opaque state: none; the historical machine has zero public parameters and zero plugin data bytes: PASS

The only DSP-adjacent production change is a non-positive callback guard. Positive-count
processing remains the retained reverse-index loop and exact unary sign inversion.
EOF

cat "$SUMMARY"
