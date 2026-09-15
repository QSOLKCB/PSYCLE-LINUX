#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-audacity-compressor}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/compressor/src"
PLUGIN="$CPSYCLE/plugins/build/audacity-compressor.so"
NATIVE_BIN="$OUT/phase5-audacity-compressor"
STATE_BIN="$OUT/phase5-audacity-compressor-state"
NATIVE_LOG="$OUT/phase5-audacity-compressor.log"
STATE_LOG="$OUT/phase5-audacity-compressor-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Audacity Compressor shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Audacity Compressor clean left the generated shared object behind" >&2
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
    echo "rebuilt Audacity Compressor shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_audacity_compressor_preservation.cpp" \
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
    "$ROOT/tests/phase5_audacity_compressor_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-audacity-compressor: metadata PASS version=0x0120 parameters=7 identity=Audacity-Compressor' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor: describe PASS ratio=2.0:1 attack=0.20s decay=1.00s methods=downward+upward' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor: downward PASS quiet=unity loud=compressed stereo=independent' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor: upward PASS energy-raised=yes stereo=symmetric' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor: live-rate PASS 44.1->88.2k fresh-reference=yes rate-sensitive=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-audacity-compressor-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: audacity-compressor:0' "$STATE_LOG" >/dev/null
grep -F 'state: 7/7 public parameters seeded non-default, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: Audacity Compressor -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-audacity-compressor.prs" ]] || {
    echo "Audacity Compressor preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-audacity-compressor.psy" ]] || {
    echo "Audacity Compressor PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Audacity Compressor Preservation

- Retained `compressor` source builds independently as `audacity-compressor.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `Audacity Compressor` / `ACompressor` / `Dominic Mazzoni/Sartorius/JosepMa`, version `0x0120`, effect type and one-column geometry: PASS
- Complete seven-parameter metadata surface and historical value descriptions: PASS
- Default downward-compressor path preserves exact sub-threshold unity, compresses a loud channel and keeps stereo state independent: PASS
- Historical peak/upward method remains finite, symmetric for equal stereo input and raises sub-full-scale signal energy: PASS
- Live 44.1 -> 88.2 kHz `SequencerTick()` reinitialization matches a fresh 88.2 kHz instance, and the oracle is measurably rate-sensitive: PASS
- Constructor defaults no longer derive attack/decay/gain from indeterminate threshold/ratio state before host initialization: PASS
- Production `PluginCatcher` identity `audacity-compressor:0` and `MachineFactory` instantiation: PASS
- All 7/7 public parameters are preserved through a version-1 preset restored with an independent catcher/factory: PASS
- Fresh PSY3 reopen restores all seven non-default values and the Audacity Compressor -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical seven public parameters: PASS

The retained Audacity-derived compressor remains under its existing GPL provenance.
The preservation fix only makes constructor-time derived state deterministic; the
host-visible compressor equations and parameter surface are unchanged.
EOF

cat "$SUMMARY"
