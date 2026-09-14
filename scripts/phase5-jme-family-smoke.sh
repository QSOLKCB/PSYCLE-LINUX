#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CPSYCLE="$ROOT/cpsycle"
OUT="${1:-$ROOT/phase5-jme-family}"

if [[ "$OUT" != /* ]]; then
    OUT="$ROOT/$OUT"
fi
rm -rf "$OUT"
mkdir -p "$OUT"

NATIVE_BIN="$OUT/phase5-jme-family"
STATE_BIN="$OUT/phase5-jme-family-state"
NATIVE_LOG="$OUT/phase5-jme-family.log"
STATE_LOG="$OUT/phase5-jme-family-state.log"
SUMMARY="$OUT/summary.md"

DIRS=(
    jme_blitz12
    jme_blitzn
    jme_gamefx13
    jme_gamefxn
)
PLUGINS=(
    "$CPSYCLE/plugins/build/blitz12.so"
    "$CPSYCLE/plugins/build/blitzn.so"
    "$CPSYCLE/plugins/build/gamefx13.so"
    "$CPSYCLE/plugins/build/gamefxn.so"
)

rm -rf "$CPSYCLE/plugins/build"

# Prove every retained JME target can build and clean independently.  This is
# intentionally done before helper libraries so a plugin makefile cannot rely
# on unrelated prior build side effects.
for i in "${!DIRS[@]}"; do
    dir="$CPSYCLE/plugins/${DIRS[$i]}/src"
    plugin="${PLUGINS[$i]}"
    make -C "$dir"
    [[ -s "$plugin" ]] || {
        echo "JME shared object was not produced: $plugin" >&2
        exit 1
    }
    make -C "$dir" clean
    [[ ! -e "$plugin" ]] || {
        echo "JME clean left generated shared object behind: $plugin" >&2
        exit 1
    }
done

make -C "$CPSYCLE/container/src"
make -C "$CPSYCLE/thread/src"
make -C "$CPSYCLE/script/src"
make -C "$CPSYCLE/file/src"
make -C "$CPSYCLE/dsp/src"
make -C "$CPSYCLE/audio/src"

for dir in "${DIRS[@]}"; do
    make -C "$CPSYCLE/plugins/$dir/src"
done
for plugin in "${PLUGINS[@]}"; do
    [[ -s "$plugin" ]] || {
        echo "rebuilt JME shared object missing: $plugin" >&2
        exit 1
    }
done

g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$CPSYCLE/plugins" \
    "$ROOT/tests/phase5_jme_family.cpp" \
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
    "$ROOT/tests/phase5_jme_family_persistence.c" -o "$STATE_BIN" \
    -L"$CPSYCLE/thread/src" -L"$CPSYCLE/script/src" \
    -L"$CPSYCLE/container/src" -L"$CPSYCLE/dsp/src" \
    -L"$CPSYCLE/audio/src" -L"$CPSYCLE/file/src" \
    -laudio -lthread -llilv-0 -ldsp -lscript -lfile -lm \
    -lpthread -ldl -lstdc++ -lcontainer "${LUA_LIBS[@]}"

"$NATIVE_BIN" "${PLUGINS[@]}" 2>&1 | tee "$NATIVE_LOG"
"$STATE_BIN" "$OUT" "${PLUGINS[@]}" 2>&1 | tee "$STATE_LOG"

# These are observation markers until the exact full metadata hashes are
# frozen after the first source-built run.  The final preservation branch will
# replace them with hash PASS markers before the roadmap item is checked.
grep -F 'phase5-jme-family: metadata OBSERVE [Blitz 1.2.1]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-jme-family: metadata OBSERVE [Blitz 1.6]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-jme-family: metadata OBSERVE [GameFX 1.3.1]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-jme-family: metadata OBSERVE [GameFX 1.6]' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-jme-family: PASS' "$NATIVE_LOG" >/dev/null
grep -F 'phase5-jme-family-state: PASS' "$STATE_LOG" >/dev/null

for preset in \
    phase5-jme-blitz12.prs \
    phase5-jme-blitzn.prs \
    phase5-jme-gamefx13.prs \
    phase5-jme-gamefxn.prs; do
    [[ -s "$OUT/$preset" ]] || {
        echo "JME preset evidence missing: $OUT/$preset" >&2
        exit 1
    }
done
[[ -s "$OUT/phase5-jme-family.psy" ]] || {
    echo "JME PSY3 evidence missing" >&2
    exit 1
}

cat > "$SUMMARY" <<'EOF'
# PSYCLE-LINUX Phase 5C JME Family Preservation

- Retained JME Blitz 1.2.1 builds independently as `blitz12.so`: PASS
- Retained JME Blitz 1.6 builds independently as `blitzn.so`: PASS
- Retained JME GameFX 1.3.1 builds independently as `gamefx13.so`: PASS
- Retained JME GameFX 1.6 builds independently as `gamefxn.so`: PASS
- Direct clean removes each generated JME shared object: PASS
- Historical native ABI exports (`GetInfo`, `CreateMachine`, `DeleteMachine`): PASS
- Four distinct generator identities/versions/parameter geometries: PASS
- Complete parameter metadata hashes: OBSERVATION PENDING FREEZE
- Fresh default note rendering is finite, active and deterministic: PASS
- Historical `0Cxx` tracker-volume behavior is active for all four identities: PASS
- Fresh 88.2 kHz target-rate rendering remains active: PASS
- Production `PluginCatcher` identities (`blitz12:0`, `blitzn:0`, `gamefx13:0`, `gamefxn:0`): PASS
- Production `MachineFactory` instantiation: PASS
- Seeded public parameter state and version-1 preset save/load for all four machines: PASS
- Fresh four-machine PSY3 reopen: PASS
- All four JME generator -> Master topology edges: PASS
- Opaque state: none; persistence remains public historical parameters: PASS

This gate intentionally preserves the older Blitz/GameFX identities alongside the
newer 1.6 builds.  It does not alias old songs onto the newer machines.
EOF

cat "$SUMMARY"
