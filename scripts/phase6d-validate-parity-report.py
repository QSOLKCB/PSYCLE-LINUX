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


def normalize_evidence_text(value: str) -> str:
    value = value.lower().replace("`", "").replace("-", " ")
    return re.sub(r"\s+", " ", value.strip())


def evidence_sentences(value: str) -> list[str]:
    normalized = normalize_evidence_text(value)
    return [
        sentence.strip()
        for sentence in re.split(r"[.;](?:\s+|$)", normalized)
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
    subject_scan = re.sub(
        r"\bno\s+(?:previous\s+instrument|active\s+voice)\b",
        "canonical control",
        sentence,
        flags=re.IGNORECASE,
    )
    subject_negation = re.search(
        r"\b(?:neither|none|no)\b",
        subject_scan,
        re.IGNORECASE,
    )
    if subject_negation is not None or re.search(
        negative_pattern, sentence, re.IGNORECASE
    ):
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
        r"\b(?:survive|survives|survived|retain|retains|retained)\b",
        r"\b(?:(?:do|does|did)\s+not|never|cannot|can't|could\s+not|"
        r"would\s+not|should\s+not|must\s+not|fail(?:s|ed)?\s+to|"
        r"(?:(?:am|is|are|was|were)\s+)?unable\s+to)\s+"
        r"(?:survive|retain)\b|\bnot\s+(?:survive|retain)\b",
        context + " early controls",
    )

    enabled = find_evidence_sentence(
        value,
        ("enabled sample", "constructor default", "serialized instrument state"),
        context + " enabled-sample variants",
    )
    assert_positive_predicate(
        enabled,
        r"\bexit(?:s|ed)?\b",
        r"\b(?:(?:do|does|did)\s+not|never|cannot|can't|could\s+not|"
        r"would\s+not|should\s+not|must\s+not|fail(?:s|ed)?\s+to|"
        r"(?:(?:am|is|are|was|were)\s+)?unable\s+to)\s+"
        r"exit\b|\bnot\s+exit\b",
        context + " enabled-sample variants",
    )
    if re.search(
        r"\b(?:non\s+zero|nonzero|not\s+zero|zero\s+or\s+more)\s+byte",
        enabled,
        re.IGNORECASE,
    ):
        die(
            f"{context} reverses the canonical zero-byte output result: "
            f"{enabled}"
        )
    if "0xc0000005" not in enabled or re.search(
        r"\bzero\s+byte\b", enabled, re.IGNORECASE
    ) is None:
        die(
            f"{context} enabled-sample result must bind the exit to "
            "0xC0000005 and zero-byte output"
        )

    normalized = normalize_evidence_text(value)
    serialized_not_boundary = (
        re.search(
            r"serialized instrument state\s+(?:is|was)\s+not\s+"
            r"(?:the\s+)?differentiator",
            normalized,
        )
        is not None
        or re.search(
            r"boundary\s+(?:is|was)\s+enabled sample voice startup\s*,?\s*"
            r"not\s+serialized instrument state",
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
        r"serialized instrument state\s+(?:is|was)\s+"
        r"(?:the\s+)?differentiator",
        normalized,
    ):
        die(f"{context} contradicts the serialized-instrument boundary result")

    work_boundary = find_evidence_sentence(
        value,
        (
            "release/no active voice",
            "four beat e df",
            "ordinary note",
        ),
        context + " Voice::Tick work-boundary result",
    )
    if re.search(
        r"\bone\s+row(?:\s+sampler\s+local)?\s+e\s+df\b",
        work_boundary,
        re.IGNORECASE,
    ) is None:
        die(
            f"{context} Voice::Tick work-boundary result lost the "
            "one-row E-DF discriminator"
        )
    assert_positive_predicate(
        work_boundary,
        r"\b(?:survive|survives|survived|retain|retains|retained)\b",
        r"\b(?:(?:do|does|did)\s+not|never|cannot|can't|could\s+not|"
        r"would\s+not|should\s+not|must\s+not|fail(?:s|ed)?\s+to|"
        r"(?:(?:am|is|are|was|were)\s+)?unable\s+to)\s+"
        r"(?:survive|retain)\b|\bnot\s+(?:survive|retain)\b",
        context + " release/no-active-voice control",
    )
    assert_positive_predicate(
        work_boundary,
        r"\bexit(?:s|ed)?\b",
        r"\b(?:(?:do|does|did)\s+not|never|cannot|can't|could\s+not|"
        r"would\s+not|should\s+not|must\s+not|fail(?:s|ed)?\s+to|"
        r"(?:(?:am|is|are|was|were)\s+)?unable\s+to)\s+"
        r"exit\b|\bnot\s+exit\b",
        context + " delayed/ordinary work-boundary fixtures",
    )
    if re.search(
        r"\b(?:non\s+zero|nonzero|not\s+zero|zero\s+or\s+more)\s+byte",
        work_boundary,
        re.IGNORECASE,
    ):
        die(
            f"{context} reverses the work-boundary zero-byte output result: "
            f"{work_boundary}"
        )
    if "0xc0000005" not in work_boundary or re.search(
        r"\bzero\s+byte\b", work_boundary, re.IGNORECASE
    ) is None:
        die(
            f"{context} work-boundary exits must bind exact 0xC0000005 "
            "and zero-byte output"
        )

    if "2.5 row" not in normalized or "one row" not in normalized:
        die(
            f"{context} must preserve the source-bound one-row versus "
            "2.5-row E-DF timing discriminator"
        )
    if re.search(
        r"\b(?:cannot|can't|does\s+not|could\s+not)\s+reach\s+"
        r"controller\.work\b",
        normalized,
        re.IGNORECASE,
    ) is None:
        die(
            f"{context} must state that the one-row delayed fixture cannot "
            "reach controller.Work"
        )
    if re.search(
        r"voice::tick\s+initialization.{0,100}"
        r"(?:before|not).{0,60}(?:first\s+)?voice::work",
        normalized,
        re.IGNORECASE,
    ) is None:
        die(
            f"{context} must preserve Voice::Tick initialization as the "
            "current pre-Voice::Work boundary"
        )
    for phrase in ("resampler", "envelope", "inside voice::tick"):
        if phrase not in normalized:
            die(
                f"{context} lost the next internal Voice::Tick boundary "
                f"phrase: {phrase}"
            )

    return {
        "early_controls_survive": True,
        "enabled_sample_variants_exit": True,
        "serialized_instrument_not_boundary": True,
        "release_control_survives": True,
        "work_reachable_variants_exit": True,
        "short_delay_excludes_controller_work": True,
        "voice_tick_initialization_boundary": True,
        "next_boundary_inside_voice_tick": True,
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
