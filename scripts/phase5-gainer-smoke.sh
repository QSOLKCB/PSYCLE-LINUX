#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-gainer}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/gainer/src"
PLUGIN="$CPSYCLE/plugins/build/gainer.so"
NATIVE_BIN="$OUT/phase5-gainer"
STATE_BIN="$OUT/phase5-gainer-state"
NATIVE_LOG="$OUT/phase5-gainer.log"
STATE_LOG="$OUT/phase5-gainer-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Gainer shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Gainer clean left the generated shared object behind" >&2
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
    echo "rebuilt Gainer shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_gainer_preservation.cpp" \
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
    "$ROOT/tests/phase5_gainer_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-gainer: metadata PASS version=0x0110 parameters=1 identity=ayeternal-Gainer' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer: describe PASS raw0=hard-zero default=dB max=e^4' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer: mute PASS raw0=hard-silence stereo=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer: exponential PASS raw32767=exp(-4/65535) raw32768=exp(+4/65535) max=e^4' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-gainer-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: gainer:0' "$STATE_LOG" >/dev/null
grep -F 'state: 1/1 public parameter seeded non-default, gain=0 hard-mute, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: ayeternal Gainer -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-gainer.prs" ]] || {
    echo "Gainer preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-gainer.psy" ]] || {
    echo "Gainer PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C ayeternal Gainer Preservation

- Retained `gainer` source builds independently as `gainer.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `ayeternal Gainer` / `Gainer` / `bohan`, version `0x0110`, effect type and one-column geometry: PASS
- Complete one-parameter metadata surface: PASS
- Historical gain description preserves raw-zero hard mute text and dB representation for nonzero gain: PASS
- Zero and negative host callback counts are strict no-ops: PASS
- Raw gain 0 preserves the historical hard-silence special case rather than the exponential scale's `exp(-4)` floor: PASS
- Legacy 16-bit exponential mapping preserves raw 32767 = `exp(-4/65535)`, raw 32768 = `exp(+4/65535)`, reciprocal midpoint behavior, and raw 65535 = `exp(4)`: PASS
- Production `PluginCatcher` identity `gainer:0` and `MachineFactory` instantiation: PASS
- Version-1 preset restore through an independent catcher/factory preserves the non-default hard-mute state: PASS
- Fresh PSY3 reopen preserves gain=0 and the ayeternal Gainer -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical single public parameter: PASS

The only production DSP-source change is a guard that makes zero and negative host
sample counts strict no-ops before the retained reverse sample loop. Positive-count
multiplication, exponential scaling, raw-zero mute semantics and descriptions are unchanged.
EOF

cat "$SUMMARY"
