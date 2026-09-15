#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-dalay-delay}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

PLUGIN_DIR="$CPSYCLE/plugins/delay/src"
PLUGIN="$CPSYCLE/plugins/build/delay.so"
NATIVE_BIN="$OUT/phase5-dalay-delay"
STATE_BIN="$OUT/phase5-dalay-delay-state"
NATIVE_LOG="$OUT/phase5-dalay-delay.log"
STATE_LOG="$OUT/phase5-dalay-delay-state.log"
SUMMARY="$OUT/summary.md"

rm -f "$PLUGIN"
make -C "$PLUGIN_DIR"
[[ -s "$PLUGIN" ]] || {
    echo "Dalay Delay shared object was not produced: $PLUGIN" >&2
    exit 1
}
make -C "$PLUGIN_DIR" clean
[[ ! -e "$PLUGIN" ]] || {
    echo "Dalay Delay clean left the generated shared object behind" >&2
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
    echo "rebuilt Dalay Delay shared object missing" >&2
    exit 1
}

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_dalay_delay_preservation.cpp" \
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
    "$ROOT/tests/phase5_dalay_delay_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "$PLUGIN" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "$PLUGIN" 2>&1 | tee "$STATE_LOG"

grep -F 'phase5-dalay-delay: metadata PASS version=0x0110 parameters=7 identity=ayeternal-Dalay-Delay' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: describe PASS left=1-line right=0.5-line snap=1/8 off=1/840' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: nonpositive PASS zero+negative strict-noop' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: snap-live PASS existing=0-line future-tweak=0.125-line' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: stereo PASS left=5513 right=2757 feedback-left=32767/65535 feedback-right=-16383/65535 second-echo=yes' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: timing-sample-rate PASS left=11026 right=5513 stale-left=5513 stale-right=2757' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: timing-bpm PASS left=8821 right=4411 stale-left=11026 stale-right=5513' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: timing-tpb PASS left=4411 right=2206 stale-left=8821 stale-right=4411' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: live-timing PASS sample-rate+BPM+TPB independent' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-dalay-delay-state: PASS' "$STATE_LOG" >/dev/null
grep -F 'catcher: delay:0' "$STATE_LOG" >/dev/null
grep -F 'state: 7/7 public parameters seeded non-default, snap=839 left=101 right=201 encoded-state-idempotent, 0 opaque bytes' "$STATE_LOG" >/dev/null
grep -F 'preset-factory: independent' "$STATE_LOG" >/dev/null
grep -F 'topology: ayeternal Dalay Delay -> Master' "$STATE_LOG" >/dev/null

[[ -s "$OUT/phase5-dalay-delay.prs" ]] || {
    echo "Dalay Delay preset evidence missing" >&2
    exit 1
}
[[ -s "$OUT/phase5-dalay-delay.psy" ]] || {
    echo "Dalay Delay PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C Dalay Delay Preservation

- Retained `delay` source builds independently as `delay.so`: PASS
- Direct `make clean` removes the generated module from `cpsycle/plugins/build/`: PASS
- Native ABI exports `GetInfo` / `CreateMachine` / `DeleteMachine`: PASS
- Historical identity `ayeternal Dalay Delay` / `Dalay Delay` / `bohan`, version `0x0110`, effect type and four-column geometry: PASS
- Complete seven-parameter metadata surface and historical value descriptions: PASS
- Historical snap semantics preserve 1/8-line quantisation and the maximum `off 1 / 840 ticks (lines)` display: PASS
- Changing live `snap to` does not reinterpret or resize an already-quantized delay; the new grid applies only to a later delay tweak: PASS
- Zero and negative host callback counts are strict no-ops: PASS
- At 44.1 kHz / 120 BPM / TPB 4, snapped 1-line left and 1/2-line right delays preserve source-derived ring lengths 5513 / 2757 samples, including the retained +1 ring sample: PASS
- Distinct nontrivial left/right feedback values are verified through their measurable second echoes: PASS
- Sample-rate-only, BPM-only and TPB-only live timing transitions independently match fresh target instances and reject stale timing: PASS
- Production `PluginCatcher` identity `delay:0` and `MachineFactory` instantiation: PASS
- Preset and PSY3 restore establish saved snap before delay state and decode Dalay Delay's historical `+1` stored representation instead of replaying it as a new request: PASS
- Fine-grid snap=839 requests 100/200 save as 101/201 and restore exactly as 101/201 without reopen drift: PASS
- Fresh PSY3 reopen restores all seven non-default values and the ayeternal Dalay Delay -> Master topology edge: PASS
- Opaque state: none; persistence remains the historical seven public parameters: PASS

The preservation slice does not rewrite Dalay Delay DSP equations or its line-snap formula.
Live snap behavior remains prospective-only. Restore-specific ordering/decoding is confined to
production preset and PSY3 state restoration, where the host has an explicit restore boundary.
EOF

cat "$SUMMARY"
