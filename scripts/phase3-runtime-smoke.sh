#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/runtime-smoke}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi

mkdir -p "$OUT"
LOG="$OUT/psycle.log"
SUMMARY="$OUT/summary.md"
STATE="$OUT/state"

if [ "${PSYCLE_SMOKE_INSIDE_XVFB:-0}" != "1" ]; then
    for command in xvfb-run xdotool; do
        if ! command -v "$command" >/dev/null 2>&1; then
            echo "Missing runtime-smoke dependency: $command" >&2
            exit 2
        fi
    done

    rm -rf "$STATE"
    mkdir -p "$STATE/home" "$STATE/config"
    : > "$LOG"

    exec xvfb-run -a -s '-screen 0 1280x720x24 -nolisten tcp' \
        env PSYCLE_SMOKE_INSIDE_XVFB=1 \
            PSYCLE_SMOKE_ROOT="$ROOT" \
            PSYCLE_SMOKE_OUT="$OUT" \
            PSYCLE_SMOKE_STATE="$STATE" \
            "$0" "$OUT"
fi

ROOT="${PSYCLE_SMOKE_ROOT:-$ROOT}"
OUT="${PSYCLE_SMOKE_OUT:-$OUT}"
STATE="${PSYCLE_SMOKE_STATE:-$STATE}"
CPSYCLE="$ROOT/cpsycle"
LOG="$OUT/psycle.log"
SUMMARY="$OUT/summary.md"
PSYCLE="$CPSYCLE/psycle"

if [ ! -x "$PSYCLE" ]; then
    echo "Native host binary not found: $PSYCLE" >&2
    exit 2
fi
if [ ! -f "$CPSYCLE/driver/build/libpsysdl2.so" ]; then
    echo "SDL2 Psycle driver not found; run the Linux build first." >&2
    exit 2
fi

export HOME="$STATE/home"
export XDG_CONFIG_HOME="$STATE/config"
export PSYCLE_AUDIO_DRIVER=sdl2
export SDL_AUDIODRIVER=dummy

cd "$CPSYCLE"
"$PSYCLE" >"$LOG" 2>&1 &
psycle_pid=$!
window_id=''
window_name=''

cleanup() {
    if kill -0 "$psycle_pid" >/dev/null 2>&1; then
        kill -TERM "$psycle_pid" >/dev/null 2>&1 || true
        for _ in $(seq 1 20); do
            if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
                break
            fi
            sleep 0.1
        done
        if kill -0 "$psycle_pid" >/dev/null 2>&1; then
            kill -KILL "$psycle_pid" >/dev/null 2>&1 || true
        fi
    fi
    wait "$psycle_pid" >/dev/null 2>&1 || true
}
trap cleanup EXIT

# Give the native X11 host a bounded window in which to initialize its UI,
# scan/load runtime modules and enter the event loop. Fail immediately if the
# process exits before presenting a Psycle window.
for _ in $(seq 1 80); do
    if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
        rc=0
        wait "$psycle_pid" || rc=$?
        echo "Psycle exited before its X11 window appeared (exit $rc)." >&2
        tail -n 120 "$LOG" >&2 || true
        exit 1
    fi

    window_id="$(xdotool search --onlyvisible --name 'Psycle' 2>/dev/null | head -n 1 || true)"
    if [ -n "$window_id" ]; then
        window_name="$(xdotool getwindowname "$window_id" 2>/dev/null || true)"
        break
    fi
    sleep 0.25
done

if [ -z "$window_id" ]; then
    echo 'Psycle stayed alive but did not expose a visible X11 window.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

# Exercise the X11 input path with a harmless key event, then require the host
# to remain alive long enough to demonstrate that the event loop is running.
xdotool key --window "$window_id" Escape
sleep 2
if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
    rc=0
    wait "$psycle_pid" || rc=$?
    echo "Psycle exited after the X11 input smoke event (exit $rc)." >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

# SDL's dummy backend is a real SDL2 callback path with no physical sound card.
# The existing driver reports this only after SDL_OpenAudioDevice succeeds and
# the device is unpaused, so require that diagnostic before declaring CI audio
# runtime coverage successful.
if ! grep -q 'sdl2 audio started' "$LOG"; then
    echo 'Psycle UI is alive, but the SDL2 dummy audio device did not start.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

cat > "$SUMMARY" <<EOF
# PSYCLE-LINUX Phase 3 Runtime Smoke

- Native executable: PASS
- X11 window creation: PASS
- X11 event loop survival: PASS
- Basic keyboard event dispatch: PASS
- Dynamic SDL2 driver selection/load: PASS
- SDL2 audio callback using the dummy backend: PASS
- Window: \`${window_name:-Psycle}\`
- Audio override: \`PSYCLE_AUDIO_DRIVER=sdl2\`
- SDL backend: \`SDL_AUDIODRIVER=dummy\`

This smoke test validates native X11 startup and a real audio-driver callback path without claiming physical ALSA/JACK hardware coverage on the GitHub-hosted runner.
EOF

cat "$SUMMARY"
