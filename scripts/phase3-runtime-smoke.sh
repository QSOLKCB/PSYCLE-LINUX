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

# A failed rerun must never leave a PASS summary from an earlier invocation.
rm -f "$SUMMARY"

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
CONFIG_FILE="$STATE/config/psycle.ini"

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
export PSYCLE_RUNTIME_SMOKE=1

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

# SDL's dummy backend exercises the real SDL2 callback path without physical
# audio hardware. Require both the unbuffered device-open diagnostic and a
# marker emitted only after one complete callback invocation has returned from
# Psycle host work and filled the SDL buffer.
audio_device_started=0
audio_callback_completed=0
for _ in $(seq 1 50); do
    if grep -q 'psycle: sdl2 audio device started' "$LOG"; then
        audio_device_started=1
    fi
    if grep -q 'psycle: sdl2 audio callback completed' "$LOG"; then
        audio_callback_completed=1
    fi
    if [ "$audio_device_started" -eq 1 ] &&
            [ "$audio_callback_completed" -eq 1 ]; then
        break
    fi
    if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
        rc=0
        wait "$psycle_pid" || rc=$?
        echo "Psycle exited before SDL2 runtime evidence completed (exit $rc)." >&2
        tail -n 120 "$LOG" >&2 || true
        exit 1
    fi
    sleep 0.1
done

if [ "$audio_device_started" -ne 1 ]; then
    echo 'Psycle UI is alive, but the SDL2 dummy audio device did not start.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

if [ "$audio_callback_completed" -ne 1 ]; then
    echo 'SDL2 device opened, but no completed Psycle audio callback was observed.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

# Alt+A is Psycle's default CMD_IMM_ENABLEAUDIO shortcut. It toggles the real
# global `enableaudio` configuration property. Start from a fresh isolated
# config directory (default = enabled), inject the command, then close Psycle
# through the X11 WM_DELETE path so workspace_dispose() persists configuration.
# Requiring enableaudio=0 in the resulting psycle.ini proves an observable
# keyboard -> command dispatch -> application state mutation -> save round trip.
if [ -e "$CONFIG_FILE" ]; then
    echo "Runtime smoke expected a fresh config, but $CONFIG_FILE already exists." >&2
    exit 1
fi

if ! xdotool key --clearmodifiers --window "$window_id" alt+a; then
    echo 'xdotool could not inject Psycle Enable Audio shortcut (Alt+A).' >&2
    exit 1
fi

sleep 0.5
if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
    rc=0
    wait "$psycle_pid" || rc=$?
    echo "Psycle exited after Alt+A before the persistence check (exit $rc)." >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

if ! xdotool windowclose "$window_id"; then
    echo 'Could not request a normal X11 close for Psycle.' >&2
    exit 1
fi

clean_shutdown=0
for _ in $(seq 1 80); do
    if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
        clean_shutdown=1
        break
    fi
    sleep 0.1
done

if [ "$clean_shutdown" -ne 1 ]; then
    echo 'Psycle did not shut down after its X11 close request.' >&2
    exit 1
fi

psycle_rc=0
wait "$psycle_pid" || psycle_rc=$?
if [ "$psycle_rc" -ne 0 ]; then
    echo "Psycle returned nonzero from normal X11 shutdown: $psycle_rc" >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

if [ ! -f "$CONFIG_FILE" ]; then
    echo "Psycle shut down cleanly but did not save $CONFIG_FILE." >&2
    exit 1
fi

if ! grep -Eq '^[[:space:]]*enableaudio[[:space:]]*=[[:space:]]*(0|false)[[:space:]]*$' "$CONFIG_FILE"; then
    echo 'Alt+A was injected, but the persisted enableaudio state was not disabled.' >&2
    echo 'Saved configuration evidence:' >&2
    grep -n -i 'enableaudio' "$CONFIG_FILE" >&2 || true
    exit 1
fi

cat > "$SUMMARY" <<EOF
# PSYCLE-LINUX Phase 3 Runtime Smoke

- Native executable: PASS
- X11 window creation: PASS
- X11 event loop survival: PASS
- Dynamic SDL2 driver selection/load: PASS
- SDL2 dummy audio device open: PASS
- Completed SDL2 audio callback including Psycle host work: PASS
- Alt+A Enable Audio keyboard command dispatch: PASS
- Persisted application state mutation (\`enableaudio=0\`): PASS
- Clean X11 shutdown and configuration save: PASS
- Main window: \`${window_name:-Psycle}\`
- Audio override: \`PSYCLE_AUDIO_DRIVER=sdl2\`
- SDL backend: \`SDL_AUDIODRIVER=dummy\`
- Config evidence: \`${CONFIG_FILE}\`

This smoke test validates native X11 startup, a completed real audio-driver callback, and a keyboard command whose successful dispatch is evidenced by a persisted Psycle configuration state change after normal shutdown. It does not claim physical ALSA/JACK hardware coverage on the GitHub-hosted runner.
EOF

trap - EXIT
cat "$SUMMARY"
