#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-crasher}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/crasher/src"
PLUGIN="$CPSYCLE/plugins/build/crasher.so"
NATIVE_BIN="$OUT/phase5-crasher"
STATE_BIN="$OUT/phase5-crasher-state"
NATIVE_LOG="$OUT/phase5-crasher.log"
STATE_LOG="$OUT/phase5-crasher-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Crasher shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Crasher clean left the generated shared object behind" >&2
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
    echo "rebuilt Crasher shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_crasher_preservation.cpp" \
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
    "$ROOT/tests/phase5_crasher_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-crasher: metadata PASS version=0x0100 parameters=0 identity=crasher' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-crasher: positive PASS stereo=inverted exception=runtime_error bounded=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-crasher: nonpositive PASS zero+negative buffers=untouched exception=runtime_error' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-crasher: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-crasher-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: crasher:0' "$STATE_LOG" >/dev/null
grep -F 'state: 0 public parameters, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: crasher -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-crasher.prs" ]] || {
    echo "Crasher preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-crasher.psy" ]] || {
    echo "Crasher PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Crasher Preservation

- Retained `crasher` source builds independently as `crasher.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `crasher` / `crasher` / `bohan`, version `0x0100`, effect type, one-column geometry and zero-parameter surface: PASS
- Positive stereo blocks retain exact sign inversion before the historical `std::runtime_error("crash on purpose!")`: PASS
- Canary guards prove positive processing remains inside the host-supplied block: PASS
- Zero and negative callback counts leave host buffers untouched and still raise the same deliberate diagnostic exception: PASS
- Production `PluginCatcher` identity `crasher:0` and `MachineFactory` instantiation: PASS
- Zero-parameter, zero-opaque-state version-1 preset save/load through an independent catcher/factory: PASS
- Fresh PSY3 reopen restores Crasher and its Crasher -> Master topology edge without inventing state: PASS

Crasher remains intentionally unsafe to execute as ordinary audio processing: its purpose is
to throw on every Work() call so host crash/error containment can be tested.  The preservation
fix only prevents malformed non-positive sample counts from walking outside the supplied audio
buffer before that intentional exception; the positive-block inversion and deliberate throw are
unchanged.
EOF

cat "$SUMMARY"
