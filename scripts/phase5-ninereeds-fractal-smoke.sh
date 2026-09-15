#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-ninereeds-fractal}"
if [[ "$OUT" != /* ]]; then OUT="$ROOT/$OUT"; fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/ninereeds_7900/src"
PLUGIN="$CPSYCLE/plugins/build/nrs-7900-fractal.so"
NATIVE_BIN="$OUT/phase5-ninereeds-fractal"
STATE_BIN="$OUT/phase5-ninereeds-fractal-state"
NATIVE_LOG="$OUT/phase5-ninereeds-fractal.log"
STATE_LOG="$OUT/phase5-ninereeds-fractal-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || { echo "Fractal shared object was not produced: $PLUGIN" >&2; exit 1; }
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || { echo "Fractal clean left generated module behind" >&2; exit 1; }

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || { echo "rebuilt Fractal shared object missing" >&2; exit 1; }

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_ninereeds_fractal_preservation.cpp" \
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
    "$ROOT/tests/phase5_ninereeds_fractal_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -Fqx 'phase5-ninereeds-fractal: metadata PASS version=0x0002 parameters=2 identity=Fractal-Dist' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: describe-about PASS raw=decimal samplerate-warning=retained' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: depth0 PASS identity=yes max-block=256 bounded=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: oracle PASS default-markers=8 maxeffect-depth2-markers=8 clipping=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: samplerate PASS 44100=88200 historical-not-aware=yes' "$NATIVE_LOG"
grep -Fqx 'phase5-ninereeds-fractal: PASS' "$NATIVE_LOG"

grep -Fqx 'phase5-ninereeds-fractal-state: PASS' "$STATE_LOG"
grep -Fqx 'catcher: nrs-7900-fractal:0' "$STATE_LOG"
grep -Fqx 'state: 2/2 MPF_STATE controls seeded non-default effect=65534 depth=2, 0 opaque bytes' "$STATE_LOG"
grep -Fqx 'audio: production cubic recurrence markers=8 bounded=yes zero-callback=noop' "$STATE_LOG"
grep -Fqx 'preset-factory: independent' "$STATE_LOG"
grep -Fqx 'topology: Ninereeds Fractal 7900s Port -> Master' "$STATE_LOG"
[[ -s "$OUT/phase5-ninereeds-fractal.prs" ]]
[[ -s "$OUT/phase5-ninereeds-fractal.psy" ]]

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Ninereeds Fractal 7900s Preservation

- Retained source builds independently as `nrs-7900-fractal.so`: PASS
- Direct `make clean` removes the generated module: PASS
- Historical identity/version/type/one-column geometry and exact two-parameter surface: PASS
- Historical About text retains the explicit `Not samplerate aware!` warning: PASS
- `depth=0` is exact identity over Psycle's 256-sample maximum block with canary boundaries: PASS
- Default cubic recurrence is frozen by eight source-derived stereo markers: PASS
- Maximum effect with depth two is frozen by eight repeated-map markers including hard clipping: PASS
- Zero and negative callback counts are strict no-ops: PASS
- 44.1 kHz and 88.2 kHz output remain identical, preserving the explicitly sample-rate-unaware behavior: PASS
- Production `PluginCatcher` identity `nrs-7900-fractal:0` and `MachineFactory` instantiation: PASS
- Production audio path matches source-derived recurrence markers and remains bounded: PASS
- Both public state controls persist non-default through independent version-1 preset restore: PASS
- Fresh PSY3 reopen preserves both values and the Fractal -> Master topology edge: PASS
- Opaque state: none; persistence remains the two public parameter slots: PASS

The only DSP-adjacent production change is a non-positive callback guard. Positive-count
cubic recurrence, depth iteration, clipping and intentionally sample-rate-unaware behavior are unchanged.
EOF
cat "$SUMMARY"
