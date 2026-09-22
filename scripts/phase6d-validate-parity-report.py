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
    return re.sub(r"\\s+", " ", value.strip())


def normalize_evidence_text(value: str) -> str:
    value = value.lower().replace("`", "").replace("-", " ")
    return re.sub(r"\\s+", " ", value.strip())


def evidence_sentences(value: str) -> list[str]:
    normalized = normalize_evidence_text(value)
    return [
        sentence.strip()
        for sentence in re.split(r"[.;](?:\\s+|$)", normalized)
        if sentence.strip()
    ]


def find_evidence_sentence(
    value: str,
    required_terms: tuple[str, ...],
    context: str,
) -> str:
    matches = [
        sentence
        for sentence in evidence_sentences(value)
        if all(term in sentence for term in required_terms)
    ]
    if len(matches) != 1:
        die(
            f"{context} must contain exactly one semantic result clause with "
            f"terms {required_terms}, found {len(matches)}"
        )
    return matches[0]


def assert_positive_predicate(
    sentence: str,
    positive_pattern: str,
    negative_pattern: str,
    context: str,
) -> None:
    if re.search(negative_pattern, sentence, re.IGNORECASE):
        die(f"{context} reverses the canonical result polarity: {sentence}")
    if re.search(positive_pattern, sentence, re.IGNORECASE) is None:
        die(f"{context} lacks the required positive result predicate: {sentence}")


def delayed_evidence_signature(value: str, context: str) -> dict[str, bool]:
    early = find_evidence_sentence(
        value,
        ("no previous instrument", "missing sample"),
        context + " early controls",
    )
    assert_positive_predicate(
        early,
        r"\\b(?:survive|survives|survived|retain|retains|retained)\\b",
        r"\\b(?:(?:do|does|did)\\s+not|never|fail(?:s|ed)?\\s+to)\\s+"
        r"(?:survive|retain)\\b|\\bnot\\s+(?:survive|retain)\\b",
        context + " early controls",
    )

    enabled = find_evidence_sentence(
        value,
        ("enabled sample", "constructor default", "serialized instrument state"),
        context + " enabled-sample variants",
    )
    assert_positive_predicate(
        enabled,
        r"\\bexit(?:s|ed)?\\b",
        r"\\b(?:(?:do|does|did)\\s+not|never|fail(?:s|ed)?\\s+to)\\s+"
        r"exit\\b|\\bnot\\s+exit\\b",
        context + " enabled-sample variants",
    )
    if "0xc0000005" not in enabled or "zero byte" not in enabled:
        die(
            f"{context} enabled-sample result must bind the exit to "
            "0xC0000005 and zero-byte output"
        )

    normalized = normalize_evidence_text(value)
    serialized_not_boundary = (
        re.search(
            r"serialized instrument state\\s+(?:is|was)\\s+not\\s+"
            r"(?:the\\s+)?differentiator",
            normalized,
        )
        is not None
        or re.search(
            r"boundary\\s+(?:is|was)\\s+enabled sample voice startup\\s*,?\\s*"
            r"not\\s+serialized instrument state",
            normalized,
        )
        is not None
    )
    if not serialized_not_boundary:
        die(
            f"{context} must state that serialized instrument state is not "
            "the differentiating boundary"
        )
    if re.search(
        r"serialized instrument state\\s+(?:is|was)\\s+"
        r"(?:the\\s+)?differentiator",
        normalized,
    ):
        die(f"{context} contradicts the serialized-instrument boundary result")

    if re.search(
        r"(?:isolate|boundary).{0,160}voice::tick.{0,160}"
        r"(?:versus|vs\\.?).{0,80}(?:first\\s+)?voice::work",
        normalized,
    ) is None:
        die(
            f"{context} must preserve the next boundary as Voice::Tick "
            "initialization versus first Voice::Work"
        )

    return {
        "early_controls_survive": True,
        "enabled_sample_variants_exit": True,
        "serialized_instrument_not_boundary": True,
        "next_boundary_voice_tick_vs_work": True,
    }


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

        if row_id == "sequencer-delayed-retrigger":
            matrix_signature = delayed_evidence_signature(
                str(row.get("notes", "")),
                "sequencer-delayed-retrigger matrix notes",
            )
            report_signature = delayed_evidence_signature(
                report_rows[label][5],
                "sequencer-delayed-retrigger human projection",
            )
            if report_signature != matrix_signature:
                die(
                    "sequencer-delayed-retrigger human projection semantic "
                    "result differs from canonical matrix notes"
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
    unknown_matches = [
        int(match.group(1))
        for match in re.finditer(
            r"(?<!\d)(\d+)(?!\d)\s+contracts\s+remain\s+UNKNOWN\b",
            status_section,
            re.IGNORECASE,
        )
    ]
    if unknown_matches != [unknown_count]:
        die(
            "status summary UNKNOWN count does not match matrix inventory: "
            f"expected exactly {unknown_count}, found {unknown_matches}"
        )

    # Validate the sequence-order claim as a whole clause, not by finding one
    # favorable token. A summary such as "sequence/pattern order are scoped PASS
    # results but remain UNKNOWN" is contradictory and must fail closed.
    status_summary_lines = [
        line.strip() for line in status_section.splitlines() if line.strip()
    ]
    if not status_summary_lines:
        die("status summary is empty")
    status_summary = status_summary_lines[0].strip("*").strip()

    sequence_row = next(
        row for row in contracts if row.get("id") == "sequencer-pattern-order"
    )
    sequence_occurrences = list(
        re.finditer(r"sequence/pattern order\b", status_summary, re.IGNORECASE)
    )
    if len(sequence_occurrences) != 1:
        die(
            "status summary must contain exactly one sequence/pattern order "
            f"claim, found {len(sequence_occurrences)}"
        )

    sequence_match = sequence_occurrences[0]
    remainder = status_summary[sequence_match.end():]
    next_subject = re.search(
        r"\b(?:serialization/save capability|\d+ contracts remain)\b",
        remainder,
        re.IGNORECASE,
    )
    sequence_context_end = (
        sequence_match.end() + next_subject.start()
        if next_subject is not None
        else len(status_summary)
    )
    sequence_context = status_summary[
        sequence_match.start():sequence_context_end
    ]
    sequence_statuses = [
        match.group(1).upper()
        for match in re.finditer(
            r"\b(PASS|DIFFERENT|MISSING|UNKNOWN)\b",
            sequence_context,
            re.IGNORECASE,
        )
    ]
    expected_sequence_status = sequence_row.get("status")
    if sequence_statuses != [expected_sequence_status]:
        die(
            "status summary sequence/pattern order context is contradictory or "
            f"does not match matrix status {expected_sequence_status!r}: "
            f"found {sequence_statuses}"
        )

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
