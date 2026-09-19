#!/usr/bin/env python3
"""Derive fixture-scoped PSY3 load acceptance without rewriting process evidence.

The frozen player's playing marker follows a successful CoreSong::load return.
Psy3Filter can warn and still return true, so every warning is checked too.
This is deliberately limited to the project-authored sampler/master fixture.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

BASELINE = '00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a'
FIXTURE = 'cpsycle-psy3/phase4-first.psy'
FIXTURE_SHA256 = 'db18fbf83afd13c28ff7cf01a20ebd2e97018b4ecf77496afe936c5c20050065'
PROCEDURE = ('Derive PSY3 parse/load acceptance from the hash-bound candidate process receipt and complete log; '
             'require the exact project-authored fixture, dummy driver, ordered single load/PSY3-loader/'
             'Sampler/Master/playing markers, and no unrecognized warning, error or log line; '
             'retain the recorded process exit and timeout independently. No playback or serialization claim.')
LINE = re.compile(r'log:\s+\d+us: ([A-Z]): (main|thread-id-\d+|psycle::core::Player#\d+): (.*)')
VERSION_WARNING = 'This file is from a newer version of Psycle! This process will try to load it anyway.'
WARNINGS = {
    'psycle: player: native plugin path not configured. You can set the PSYCLE_PATH environment variable.',
    'psycle: player: ladspa plugin path not configured. You can set the LADSPA_PATH environment variable.',
    VERSION_WARNING,
}
SCHEDULER_LOCATION = ('# psycle-core # ../src/psycle/core/player.cpp:279 # '
                      'void psycle::core::Player::thread_function(std::size_t)')
SCHEDULER_WARNING = ('no permission to set thread scheduling policy and priority to realtime: '
                     'standard error: 1 0x1: Operation not permitted')
PROMPT = ('psycle: player: currently, we have no way find out when the song is finished with the sequence... '
          'for now, please press enter or ctrl+d (EOF) to stop.')


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def child(root: Path, value: str) -> Path:
    ref = PurePosixPath(value)
    if ref.is_absolute() or '..' in ref.parts or '\\' in value:
        raise ValueError('artifact path must be relative and contained')
    path = (root / value).resolve()
    path.relative_to(root.resolve())
    return path


def derive(raw_bytes: bytes, log_bytes: bytes) -> dict:
    raw = json.loads(raw_bytes)
    expected = {
        'schema_version': 1, 'phase': '6C', 'scope': 'candidate-observation',
        'evidence_role': 'candidate', 'contract': 'project-io-psy3-parse',
        'snapshot': BASELINE, 'fixture': FIXTURE, 'fixture_sha256': FIXTURE_SHA256,
        'source_revision': 'SourceForge SVN r12005',
        'executable': 'psycle-cpp-r12005-sanitized/psycle-player/++qmake/psycle-player',
        'original_psycle_observed': False, 'parity_status': 'UNKNOWN',
        'procedure': ('timeout 20s psycle-cpp-r12005-sanitized/psycle-player/++qmake/psycle-player '
                      '--output-driver dummy --input-file <fixture> </dev/null > <log> 2>&1'),
    }
    for key, value in expected.items():
        if type(raw.get(key)) is not type(value) or raw[key] != value:
            raise ValueError(f'unsupported candidate identity: {key}')
    if not re.fullmatch('[0-9a-f]{64}', raw.get('executable_sha256', '')):
        raise ValueError('invalid candidate executable identity')
    if raw.get('log') != {'path': 'candidate-psy3.log', 'sha256': sha(log_bytes)}:
        raise ValueError('candidate log identity mismatch')
    rc = raw.get('exit_code')
    if type(rc) is not int:
        raise ValueError('exit_code must be an integer')
    diagnostics, warnings, events = [], [], []
    paths = []
    markers = {
        'psycle: core: player: setting audio driver to: dummy': ('driver', 'T'),
        'psycle: core: machine factory: create machine: loading with host: 0, plugin: <sampler>': ('sampler', 'I'),
        'psycle: core: machine factory: create machine: loading with host: 0, plugin: <master>': ('master', 'I'),
        'psycle: player: playing...': ('load-returned-success', 'I'),
    }
    for number, line in enumerate(log_bytes.decode('utf-8').splitlines(), 1):
        match = LINE.fullmatch(line)
        if not match:
            if line != PROMPT:
                diagnostics.append(f'unrecognized log line {number}')
            continue
        level, thread, message = match.groups()
        if level == 'W':
            allowed = ((thread == 'main' and message in WARNINGS) or
                       (re.fullmatch(r'psycle::core::Player#\d+', thread) and
                        message in {SCHEDULER_LOCATION, SCHEDULER_WARNING}))
            if allowed:
                warnings.append({'line': number, 'message': message})
            else:
                diagnostics.append(f'unrecognized warning at line {number}: {message}')
        elif level not in {'T', 'I'}:
            diagnostics.append(f'candidate diagnostic at line {number}: {message}')
        if thread != 'main':
            continue
        for prefix, event, expected_level in (
            ('psycle: player: loading song file: ', 'load-request', 'I'),
            ('psycle: core: psy3 loader: loading psycle song fileformat version 3: ', 'psy3-loader', 'T'),
        ):
            if message.startswith(prefix):
                path = message[len(prefix):]
                paths.append(path)
                if not path.startswith('/') or not path.endswith('/' + FIXTURE) or level != expected_level:
                    diagnostics.append(f'wrong fixture load marker at line {number}')
                events.append({'marker': event, 'line': number})
        if message in markers:
            event, expected_level = markers[message]
            if level != expected_level:
                diagnostics.append(f'wrong marker severity at line {number}')
            events.append({'marker': event, 'line': number})
    if [e['marker'] for e in events] != ['driver', 'load-request', 'psy3-loader', 'sampler', 'master', 'load-returned-success']:
        diagnostics.append('missing, duplicate or out-of-order load markers')
    if len(paths) != 2 or paths[0] != paths[1]:
        diagnostics.append('load request and PSY3 loader do not identify the same fixture path')
    if (rc, raw.get('observation')) not in {(0, 'load-and-clean-exit'), (124, 'timeout')}:
        diagnostics.append('process outcome is not clean exit or recorded observation timeout')
    return {
        'schema_version': 1, 'phase': '6C', 'scope': 'candidate-parse-observation',
        'contract': raw['contract'], 'evidence_role': 'candidate',
        'snapshot': BASELINE, 'executable_sha256': raw['executable_sha256'],
        'fixture': FIXTURE, 'fixture_sha256': FIXTURE_SHA256,
        'procedure': PROCEDURE,
        'source_receipt': {'path': 'candidate-psy3.json', 'sha256': sha(raw_bytes)},
        'log': raw['log'],
        'parse_result': 'inconclusive' if diagnostics else 'accepted',
        'markers': events, 'allowed_warnings': warnings, 'diagnostics': diagnostics,
        'process_observation': raw['observation'], 'process_exit_code': rc,
        'original_psycle_observed': False, 'parity_status': 'UNKNOWN',
    }


def validate(root: Path, *, require_fixture: bool = True) -> dict:
    raw_path = root / 'candidate-psy3.json'
    raw_bytes = raw_path.read_bytes()
    raw = json.loads(raw_bytes)
    if require_fixture and sha(child(root, raw['fixture']).read_bytes()) != FIXTURE_SHA256:
        raise ValueError('fixture bytes differ from the scoped PSY3 fixture')
    result = derive(raw_bytes, child(root, raw['log']['path']).read_bytes())
    actual = json.loads((root / 'candidate-psy3-parse.json').read_bytes())
    if json.dumps(actual, sort_keys=True) != json.dumps(result, sort_keys=True):
        raise ValueError('parse receipt differs from independently rederived evidence')
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('artifact_root', type=Path)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    root = args.artifact_root
    if args.check:
        result = validate(root)
    else:
        raw_bytes = (root / 'candidate-psy3.json').read_bytes()
        raw = json.loads(raw_bytes)
        if sha(child(root, raw['fixture']).read_bytes()) != FIXTURE_SHA256:
            raise ValueError('fixture bytes differ from the scoped PSY3 fixture')
        result = derive(raw_bytes, child(root, raw['log']['path']).read_bytes())
        output = root / 'candidate-psy3-parse.json'
        with output.open('x', encoding='utf-8') as handle:
            handle.write(json.dumps(result, indent=2) + '\n')
    print(f"phase6c-candidate-parse: parse={result['parse_result']} process={result['process_observation']} exit={result['process_exit_code']}")


if __name__ == '__main__':
    main()
