#!/usr/bin/env python3
"""Validate that PSYCLE_CORE_PARITY.md is a faithful projection of Phase 6C.

This validator does not create compatibility evidence. It only prevents the
human-readable parity report from drifting away from the canonical machine
matrix and from dropping the report sections required by the Phase 6D contract.
"""

from __future__ import annotations

import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MATRIX_PATH = ROOT / "phase6c" / "compatibility-matrix.json"
REPORT_PATH = ROOT / "PSYCLE_CORE_PARITY.md"

EXPECTED_LABELS = {
    "project-io-psy2-parse": "PSY2 parsing",
    "project-io-psy3-parse": "PSY3 parsing",
    "project-io-serialization-roundtrip": "Serialization / round-trip",
    "project-io-malformed-files": "Malformed-file behaviour",
    "sequencer-pattern-order": "Sequence / pattern order",
    "sequencer-bpm-lpb-tick": "BPM / LPB / tick timing",
    "sequencer-delayed-retrigger": "Delayed / retrigger / extended commands",
    "sampler-ps1": "Sampler PS1",
    "sampler-xmsampler": "XMSampler / Sampulse-related playback",
    "routing-mixer-master": "Mixer / Master / routing / mute / bypass",
    "native-abi-identity": "Native-machine ABI / identity",
    "native-parameter-state": "Native-machine defaults / tweak / state",
    "plugin-opaque-state": "Plugin opaque-state persistence",
    "plugin-rescan-cache": "Native rescan / cache",
    "midi-routing": "MIDI routing",
    "automation-tweak": "Automation / tweak commands",
    "sample-wav-loading": "WAV / sample loading",
    "render-bounce": "Offline render / bounce",
    "missing-machine-recovery": "Missing-machine recovery",
    "historical-song-playback": "Historical song playback",
}
ALLOWED_STATUS = {"PASS", "DIFFERENT", "MISSING", "UNKNOWN"}
REQUIRED_REPORT_SECTIONS = (
    "## Engine compatibility matrix",
    "## UI-only gaps",
    "## Existing C-Psycle evidence classification",
    "### Likely portable compatibility contracts",
    "### Requires original-Psycle confirmation first",
    "### C-Psycle-only historical evidence",
    "## Current implementation backlog",
)


def die(message: str) -> None:
    raise SystemExit(f"phase6d-validate-parity-report: {message}")


def normalize_cell(value: str) -> str:
    return re.sub(r"\s+", " ", value.strip())


def extract_matrix_table(report: str) -> dict[str, list[str]]:
    start_marker = "## Engine compatibility matrix"
    end_marker = "## UI-only gaps"
    try:
        section = report.split(start_marker, 1)[1].split(end_marker, 1)[0]
    except IndexError:
        die("missing engine compatibility matrix section boundaries")

    rows: dict[str, list[str]] = {}
    for raw_line in section.splitlines():
        line = raw_line.strip()
        if not (line.startswith("|") and line.endswith("|")):
            continue

        cells = [normalize_cell(cell) for cell in line[1:-1].split("|")]
        if len(cells) != 6:
            continue
        if cells[0] == "Subsystem / contract" or all(
            set(cell) <= {"-", ":", " "} for cell in cells
        ):
            continue

        label = cells[0]
        if label in rows:
            die(f"duplicate report row: {label}")
        rows[label] = cells

    return rows


def main() -> int:
    try:
        matrix = json.loads(MATRIX_PATH.read_text(encoding="utf-8"))
    except FileNotFoundError:
        die(f"missing matrix: {MATRIX_PATH.relative_to(ROOT)}")
    except json.JSONDecodeError as exc:
        die(f"invalid matrix JSON: {exc}")

    try:
        report = REPORT_PATH.read_text(encoding="utf-8")
    except FileNotFoundError:
        die(f"missing report: {REPORT_PATH.relative_to(ROOT)}")

    for heading in REQUIRED_REPORT_SECTIONS:
        if heading not in report:
            die(f"missing required report section: {heading}")

    if matrix.get("schema_version") != 1 or matrix.get("phase") != "6C":
        die("unexpected matrix schema_version/phase")

    contracts = matrix.get("contracts")
    if not isinstance(contracts, list):
        die("matrix contracts must be an array")

    ids = {row.get("id") for row in contracts if isinstance(row, dict)}
    if ids != set(EXPECTED_LABELS):
        missing = sorted(set(EXPECTED_LABELS) - ids)
        extra = sorted(ids - set(EXPECTED_LABELS))
        die(f"contract inventory drift: missing={missing} extra={extra}")

    report_rows = extract_matrix_table(report)
    expected_report_labels = set(EXPECTED_LABELS.values())
    actual_report_labels = set(report_rows)
    if actual_report_labels != expected_report_labels:
        missing = sorted(expected_report_labels - actual_report_labels)
        extra = sorted(actual_report_labels - expected_report_labels)
        die(f"report inventory drift: missing={missing} extra={extra}")

    classified = 0
    for row in contracts:
        assert isinstance(row, dict)
        row_id = row["id"]
        status = row.get("status")
        if status not in ALLOWED_STATUS:
            die(f"{row_id} has invalid matrix status: {status!r}")

        label = EXPECTED_LABELS[row_id]
        report_status = report_rows[label][4]
        if report_status != status:
            die(
                f"{row_id} report status {report_status!r} does not match "
                f"matrix status {status!r}"
            )

        if status != "UNKNOWN":
            classified += 1
            original = row.get("original")
            candidate = row.get("candidate")
            comparison = row.get("comparison")
            if not isinstance(original, dict) or not isinstance(candidate, dict):
                die(f"{row_id} classified row lacks observation mappings")
            for side, mapping in (("original", original), ("candidate", candidate)):
                receipt = mapping.get("observation")
                if not isinstance(receipt, str) or not receipt.strip():
                    die(f"{row_id} classified row lacks {side} receipt reference")
            if not isinstance(comparison, str) or not comparison.strip():
                die(f"{row_id} classified row lacks comparison receipt reference")

    # Keep the prose status summary synchronized with the matrix inventory, not
    # only the table cells. This prevents a newly classified row from leaving a
    # stale UNKNOWN count or omitting the classification from the overview.
    try:
        status_section = report.split("## Status", 1)[1].split("## Evidence rule", 1)[0]
    except IndexError:
        die("missing status/evidence-rule section boundaries")
    unknown_count = len(contracts) - classified
    if f"{unknown_count} contracts remain UNKNOWN" not in status_section:
        die(
            "status summary UNKNOWN count does not match matrix inventory: "
            f"expected {unknown_count}"
        )
    sequence_row = next(
        row for row in contracts if row.get("id") == "sequencer-pattern-order"
    )
    if (
        sequence_row.get("status") == "PASS"
        and "sequence/pattern order" not in status_section.lower()
    ):
        die("status summary omits classified sequence/pattern order PASS")

    # Phase 6D must remain evidence-driven even while every compatibility row is UNKNOWN.
    backlog_section = report.split("## Current implementation backlog", 1)[1]
    required_backlog_language = (
        "confirmed `DIFFERENT` / `MISSING` results",
        "Original Psycle remains the compatibility target",
    )
    for phrase in required_backlog_language:
        if phrase not in backlog_section:
            die(f"implementation backlog lost evidence gate phrase: {phrase}")

    print(
        "phase6d-validate-parity-report: PASS "
        f"contracts={len(contracts)} classified={classified} "
        f"unknown={len(contracts) - classified}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
