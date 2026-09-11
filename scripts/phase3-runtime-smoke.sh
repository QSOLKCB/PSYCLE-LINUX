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
    for command in xvfb-run xdotool xwininfo; do
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

visible_x11_windows() {
    local root_window="$1"
    local id

    xwininfo -tree -id "$root_window" 2>/dev/null |
        awk '{
            for (i = 1; i <= NF; ++i) {
                if ($i ~ /^0x[0-9A-Fa-f]+$/) {
                    print $i
                }
            }
        }' |
        sort -u |
        while IFS= read -r id; do
            if xwininfo -id "$id" 2>/dev/null |
                    grep -q 'Map State: IsViewable'; then
                printf '%s\n' "$id"
            fi
        done |
        sort -u
}

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

# Wait for the initial X11 component mapping to settle. This gives the keyboard
# assertion a stable observable baseline rather than allowing unrelated startup
# mapping to masquerade as a response to the injected command.
before_windows="$OUT/x11-visible-before.txt"
probe_windows="$OUT/x11-visible-probe.txt"
after_windows="$OUT/x11-visible-after.txt"
visible_x11_windows "$window_id" > "$before_windows"
x11_stable=0
for _ in $(seq 1 20); do
    sleep 0.1
    visible_x11_windows "$window_id" > "$probe_windows"
    if cmp -s "$before_windows" "$probe_windows"; then
        x11_stable=1
        break
    fi
    mv "$probe_windows" "$before_windows"
done

if [ "$x11_stable" -ne 1 ]; then
    echo 'Psycle X11 component mapping did not stabilize before input testing.' >&2
    exit 1
fi

# Alt+S is Psycle's default Settings command. The Settings command selects a
# different notebook page, which maps the selected X11 component and unmaps the
# previous page. Require that externally observable viewable-window set to
# change; xdotool success plus process survival alone would not prove dispatch.
if ! xdotool key --clearmodifiers --window "$window_id" alt+s; then
    echo 'xdotool could not inject Psycle Settings shortcut (Alt+S).' >&2
    exit 1
fi

keyboard_transition=0
for _ in $(seq 1 40); do
    if ! kill -0 "$psycle_pid" >/dev/null 2>&1; then
        rc=0
        wait "$psycle_pid" || rc=$?
        echo "Psycle exited after the X11 Settings shortcut (exit $rc)." >&2
        tail -n 120 "$LOG" >&2 || true
        exit 1
    fi

    sleep 0.1
    visible_x11_windows "$window_id" > "$after_windows"
    if ! cmp -s "$before_windows" "$after_windows"; then
        keyboard_transition=1
        break
    fi
done

if [ "$keyboard_transition" -ne 1 ]; then
    echo 'Alt+S was injected, but no Settings-view X11 mapping change was observed.' >&2
    echo 'Visible X11 windows before:' >&2
    cat "$before_windows" >&2 || true
    echo 'Visible X11 windows after:' >&2
    cat "$after_windows" >&2 || true
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

cat > "$SUMMARY" <<EOF
# PSYCLE-LINUX Phase 3 Runtime Smoke

- Native executable: PASS
- X11 window creation: PASS
- X11 event loop survival: PASS
- Alt+S Settings-view keyboard dispatch with observable X11 mapping change: PASS
- Dynamic SDL2 driver selection/load: PASS
- SDL2 dummy audio device open: PASS
- Completed SDL2 audio callback including Psycle host work: PASS
- Window: \`${window_name:-Psycle}\`
- Audio override: \`PSYCLE_AUDIO_DRIVER=sdl2\`
- SDL backend: \`SDL_AUDIODRIVER=dummy\`

This smoke test validates native X11 startup, an observable Psycle keyboard-command state transition, and a completed real audio-driver callback without claiming physical ALSA/JACK hardware coverage on the GitHub-hosted runner.
EOF

cat "$SUMMARY"
