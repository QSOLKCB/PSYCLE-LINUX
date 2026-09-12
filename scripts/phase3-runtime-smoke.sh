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
    mkdir -p "$STATE/home" "$STATE/config" "$STATE/samples" "$STATE/songs"
    : > "$LOG"

    python3 - "$STATE/samples/zzzz-phase4-ui-sample.wav" <<'PYWAV'
import math
import struct
import sys
import wave
path = sys.argv[1]
rate = 44100
frames = 2205
with wave.open(path, "wb") as out:
    out.setnchannels(1)
    out.setsampwidth(2)
    out.setframerate(rate)
    pcm = bytearray()
    for i in range(frames):
        sample = int(round(12000.0 * math.sin(2.0 * math.pi * 440.0 * i / rate)))
        pcm += struct.pack("<h", sample)
    out.writeframes(pcm)
PYWAV

    cat > "$STATE/config/psycle.ini" <<EOF
[global]
enableaudio=1
[inputhandling]
savereminder=0
ft2fileexplorer=1
[directories]
songs=$STATE/songs
samples=$STATE/samples
doc=$CPSYCLE/doc
EOF

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

# Exercise the remaining Phase 4 live-UI contracts before the existing
# configuration persistence check. The seeded config keeps all state isolated,
# disables only the save-reminder prompt for this deterministic smoke, and
# points Sample Load at a directory containing exactly one generated WAV.
if ! grep -Eq '^[[:space:]]*enableaudio[[:space:]]*=[[:space:]]*1[[:space:]]*$' "$CONFIG_FILE"; then
    echo 'Runtime smoke seed did not start with enableaudio=1.' >&2
    exit 1
fi

# Shift+Enter is CMD_IMM_INFOMACHINE. Fresh songs now select Master by default,
# so this must create a real floating parameter/tool frame.
xdotool windowfocus "$window_id"
xdotool key --clearmodifiers --window "$window_id" shift+Return
editor_id=''
for _ in $(seq 1 40); do
    editor_id="$(xdotool search --onlyvisible --name '^80 :' 2>/dev/null | head -n 1 || true)"
    [ -n "$editor_id" ] && break
    sleep 0.1
done
if [ -z "$editor_id" ]; then
    echo 'Shift+Enter did not expose the selected Master parameter frame.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi
xdotool windowfocus "$editor_id"
sleep 0.1
if [ "$(xdotool getwindowfocus)" != "$editor_id" ]; then
    echo 'Machine parameter frame could not receive X11 focus.' >&2
    exit 1
fi
xdotool windowclose "$editor_id"
for _ in $(seq 1 30); do
    if ! xdotool search --onlyvisible --name '^80 :' >/dev/null 2>&1; then
        break
    fi
    sleep 0.1
done
if ! grep -q 'psycle: runtime smoke machine editor shown slot=128' "$LOG"; then
    echo 'Machine editor window appeared without reaching ParamViews show path.' >&2
    exit 1
fi

# F3 focuses Tracker Grid. Note input and Tab are handled only by focus-scoped
# tracker input handlers, so their smoke markers prove both focus and keyboard
# navigation/entry reached the real editor path.
xdotool windowfocus "$window_id"
xdotool key --clearmodifiers --window "$window_id" F3
sleep 0.25
xdotool key --clearmodifiers --window "$window_id" z
xdotool key --clearmodifiers --window "$window_id" Tab
xdotool key --clearmodifiers --window "$window_id" x
for _ in $(seq 1 30); do
    note_count="$(grep -c 'psycle: runtime smoke tracker note inserted' "$LOG" || true)"
    if [ "$note_count" -ge 2 ] && grep -q 'psycle: runtime smoke tracker column next' "$LOG"; then
        break
    fi
    sleep 0.1
done
note_count="$(grep -c 'psycle: runtime smoke tracker note inserted' "$LOG" || true)"
if [ "$note_count" -lt 2 ] || ! grep -q 'psycle: runtime smoke tracker column next' "$LOG"; then
    echo 'Tracker focus/note/navigation keyboard path did not complete.' >&2
    tail -n 120 "$LOG" >&2 || true
    exit 1
fi

# Ctrl+Shift+O is the restored CMD_IMM_LOAD_SAMPLE shortcut. FileView receives
# focus, End selects the sole WAV fixture, and Return takes the normal
# FileView -> FileSelect -> Workspace -> Sample/Instrument insertion path.
xdotool key --clearmodifiers --window "$window_id" ctrl+shift+o
sleep 0.4
xdotool key --clearmodifiers --window "$window_id" End
xdotool key --clearmodifiers --window "$window_id" Return
for _ in $(seq 1 40); do
    if grep -q 'psycle: runtime smoke sample loaded .*zzzz-phase4-ui-sample.wav' "$LOG"; then
        break
    fi
    sleep 0.1
done
if ! grep -q 'psycle: runtime smoke sample loaded .*zzzz-phase4-ui-sample.wav' "$LOG"; then
    echo 'Normal FileView sample-load workflow did not load the deterministic WAV.' >&2
    tail -n 160 "$LOG" >&2 || true
    exit 1
fi

# Alt+A is Psycle's default CMD_IMM_ENABLEAUDIO shortcut. It toggles the real
# global `enableaudio` configuration property; normal shutdown must persist it.
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
- Floating Machine parameter frame open/focus/close through Shift+Enter: PASS
- Tracker Grid focus-scoped note entry and Tab navigation: PASS
- Normal FileView WAV -> Sample/Instrument loading workflow: PASS
- Alt+A Enable Audio keyboard command dispatch: PASS
- Persisted application state mutation (\`enableaudio=0\`): PASS
- Clean X11 shutdown and configuration save: PASS
- Main window: \`${window_name:-Psycle}\`
- Audio override: \`PSYCLE_AUDIO_DRIVER=sdl2\`
- SDL backend: \`SDL_AUDIODRIVER=dummy\`
- Config evidence: \`${CONFIG_FILE}\`

This smoke test validates native X11 startup, a completed real audio-driver callback, a live Machine parameter frame, focus-scoped Tracker Grid editing/navigation, the normal FileView sample-load path, and a keyboard command whose successful dispatch is evidenced by a persisted Psycle configuration state change after normal shutdown. It does not claim physical ALSA/JACK hardware coverage on the GitHub-hosted runner.
EOF

trap - EXIT
cat "$SUMMARY"
