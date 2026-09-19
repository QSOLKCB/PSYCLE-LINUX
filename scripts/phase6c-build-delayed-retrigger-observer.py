#!/usr/bin/env python3
"""Generate a delayed/retrigger-enabled copy of the mature Windows observer."""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path

EXPECTED_BLOB = "90df1d48db76396a0a6a580efa426643cd612eb9"


def git_blob(data: bytes) -> str:
    return hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"delayed/retrigger observer patch anchor {label!r} "
            f"count={count}, expected=1"
        )
    return text.replace(old, new, 1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    data = args.source.read_bytes()
    normalized = data.replace(b"\r\n", b"\n")
    if b"\r" in normalized:
        raise SystemExit("observer base contains unsupported carriage returns")
    actual = git_blob(normalized)
    if actual != EXPECTED_BLOB:
        raise SystemExit(
            f"delayed/retrigger observer base blob changed: "
            f"expected={EXPECTED_BLOB} actual={actual}"
        )
    text = normalized.decode("utf-8")

    text = replace_once(
        text,
        "    [switch]$ObserveSequenceOrder\n)",
        "    [switch]$ObserveSequenceOrder,\n\n"
        "    [switch]$ObserveDelayedRetrigger\n)",
        "parameter",
    )

    delayed_spec = r'''
    if ($ObserveDelayedRetrigger) {
        $fixtureSpecs += [ordered]@{
            name = "delayed-retrigger"
            candidate_receipt = "candidate-delayed-retrigger.json"
            expected_contract = "sequencer-delayed-retrigger"
            expected_song_title = "PSYCLE-LINUX Phase 6C delayed/retrigger fixture"
            load_warning_required = $true
            expected_load_warning_message = "This file is from a newer version of Psycle! This process will try to load it anyway."
        }
    }
'''
    text = replace_once(
        text,
        "\n    foreach ($spec in $fixtureSpecs) {",
        delayed_spec + "\n    foreach ($spec in $fixtureSpecs) {",
        "fixture specification",
    )

    text = replace_once(
        text,
        '            procedure = if ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        '            procedure = if ($ObserveDelayedRetrigger -and $spec.name -eq "delayed-retrigger") {\n'
        '                "$Procedure; for the sequencer-delayed-retrigger contract load the exact command fixture under pinned Psycle 1.12.0 x86 and retain the clean accepted-load/liveness evidence; command execution semantics are recorded separately from the pinned original source and are not inferred from this UI load receipt"\n'
        '            } elseif ($ObserveSequenceOrder -and $spec.name -eq "sequence-order") {',
        "procedure",
    )

    args.output.write_text(text, encoding="utf-8", newline="\n")


if __name__ == "__main__":
    main()
