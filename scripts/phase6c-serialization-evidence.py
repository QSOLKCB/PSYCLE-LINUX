#!/usr/bin/env python3
"""Collect and validate serialization observations, without self-promoting parity."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASELINE = '00cd95562b78303b82e17f62fff4b58622f7c0e78c0b4dd850d448082a53893a'
FIXTURE = 'cpsycle-psy3/phase4-first.psy'
FIXTURE_HASH = 'db18fbf83afd13c28ff7cf01a20ebd2e97018b4ecf77496afe936c5c20050065'
CONTRACT = 'project-io-serialization-roundtrip'
WARNING = 'This file is from a newer version of Psycle! This process will try to load it anyway.'
SOURCE_PATHS = ['tests/phase6c_serialization.cpp', 'tests/phase6c_serialization.pro'] + [
    'psycle-cpp-r12005-sanitized/psycle-core/src/psycle/core/' + p
    for p in ('song.cpp','songserializer.cpp','psy2filter.h','psy3filter.h')]


# Exact diagnostics observed for the pinned probe with PSYCLE_THREADS=1.
# Only timestamps, runtime thread IDs and the absolute artifact root may vary.
# New wording, severity, thread roles or source locations require investigation.
LOG_LINE = re.compile(r'log:\s+\d+us: ([A-Z]): (serialization-probe|thread-id-\d+): (.*)')
EXPECTED_LOG_MESSAGES = (
    ('T', r'thread-id-\d+', re.escape('# universalis # ../src/universalis/os/thread_name.cpp:56 # void universalis::os::thread_name::set_tls()')),
    ('T', r'thread-id-\d+', r'setting name for thread: id: \d+, name: serialization-probe'),
    ('T', 'serialization-probe', re.escape('# psycle-core # ../src/psycle/core/player.cpp:66 # void psycle::core::Player::start_threads()')),
    ('T', 'serialization-probe', re.escape('psycle: core: player: starting scheduler threads')),
    ('I', 'serialization-probe', re.escape('psycle: core: player: using 1 threads')),
    ('T', 'serialization-probe', re.escape('psycle: core: psy3 loader: loading psycle song fileformat version 3: ')
     + r'/(?:[^/\r\n]+/)*' + re.escape(FIXTURE)),
    ('W', 'serialization-probe', re.escape(WARNING)),
    ('I', 'serialization-probe', r'psycle: core: machine factory: create machine: loading with host: 0, plugin: <(sampler|master)>'),
    ('T', r'thread-id-\d+', re.escape('# psycle-core # ../src/psycle/core/player.cpp:452 # void psycle::core::Player::stop_threads()')),
    ('T', r'thread-id-\d+', re.escape('terminating and joining scheduler threads ...')),
    ('T', r'thread-id-\d+', re.escape('# psycle-core # ../src/psycle/core/player.cpp:454 # void psycle::core::Player::stop_threads()')),
    ('T', r'thread-id-\d+', re.escape('scheduler threads were not running')),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read(path: Path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path: Path, value) -> None:
    with path.open('x', encoding='utf-8') as f:
        f.write(json.dumps(value, indent=2)+'\n')


def child(root: Path, name: str) -> Path:
    if not isinstance(name,str):
        raise ValueError('unsafe artifact path')
    p=PurePosixPath(name)
    if p.is_absolute() or '..' in p.parts or '\\' in name:
        raise ValueError('unsafe artifact path')
    result=(root/name).resolve()
    result.relative_to(root.resolve())
    return result


def binding(root: Path, path: Path) -> dict:
    return {'path':path.relative_to(root).as_posix(),'sha256':digest(path.read_bytes())}


def bound_bytes(root: Path, mapping: dict) -> bytes:
    data=child(root,mapping['path']).read_bytes()
    if digest(data)!=mapping['sha256']:
        raise ValueError('artifact hash mismatch: '+mapping['path'])
    return data


def load_module(filename: str):
    spec=importlib.util.spec_from_file_location(filename,ROOT/'scripts'/filename)
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def outcome(raw: bytes, log: bytes, code, version: int, output: dict | None, output_bytes: bytes | None = None) -> str:
    # Only completed API calls with clean fixture loading establish save refusal.
    if type(code) is not int or code != 0:
        return 'inconclusive'
    try:
        value=json.loads(raw)
        lines=log.decode('utf-8').splitlines()
    except (ValueError, UnicodeError):
        return 'inconclusive'
    if not isinstance(value,dict):
        return 'inconclusive'
    if type(value.get('schema_version')) is not int or value.get('schema_version')!=1 or type(value.get('format_version')) is not int or value['format_version']!=version:
        return 'inconclusive'
    if value.get('load_returned') is not True or value.get('save_attempted') is not True:
        return 'inconclusive'
    if value.get('reports') != ['Load Warning: '+WARNING]:
        return 'inconclusive'
    state=value.get('state_before')
    if not isinstance(state,dict) or set(state)!={'name','author','comment','bpm','tick_speed','tracks','machine_slots'}:
        return 'inconclusive'
    if state['name']!='PSYCLE-LINUX Phase 4 synthetic fixture' or state['machine_slots']!=[0,128]:
        return 'inconclusive'
    if state!=value.get('state_after'):
        return 'inconclusive'
    for line in lines:
        if not line:
            continue
        match=LOG_LINE.fullmatch(line)
        if match:
            level,thread,message=match.groups()
            if any(level==expected_level and re.fullmatch(expected_thread,thread)
                   and re.fullmatch(expected_message,message)
                   for expected_level,expected_thread,expected_message in EXPECTED_LOG_MESSAGES):
                continue
        if version==4 and line=="SongSerializer::saveSong(): Couldn't find appropriate filter for file format version 4":
            continue
        return 'inconclusive'
    if value.get('save_returned') is False and output is None:
        return 'save-returned-false-without-output'
    if value.get('save_returned') is True and output is not None:
        # PSY4 is unregistered in this baseline. Unexpected successful PSY4
        # output needs a separate container validator before interpretation.
        header={2:b'PSY2SONG',3:b'PSY3SONG'}.get(version)
        if header and output_bytes and len(output_bytes)>len(header) and output_bytes.startswith(header):
            return 'saved-output-not-yet-reloaded'
    return 'inconclusive'


def source_hashes() -> dict:
    # Windows checkouts may use CRLF. Bind immutable Git blob bytes, matching
    # the Linux build inputs, rather than platform checkout line endings.
    return {p:digest(subprocess.check_output(['git','show','HEAD:'+p],cwd=ROOT))
            for p in SOURCE_PATHS}


def collect(root: Path, probe: Path) -> None:
    root=root.resolve(); probe=probe.resolve()
    load_module('phase6c-candidate-parse.py').validate(root)
    if not probe.is_file() or not os.access(probe,os.X_OK):
        raise ValueError('missing built observation executable')
    if source_hashes()!={p:digest((ROOT/p).read_bytes()) for p in SOURCE_PATHS}:
        raise ValueError('Linux probe inputs differ from committed source bytes')
    output_dir=root/'serialization'
    output_dir.mkdir() # Refuse stale output/results.
    receipt={
        'schema_version':1,'phase':'6C','scope':'candidate-serialization-observation',
        'contract':CONTRACT,'evidence_role':'candidate','snapshot':BASELINE,
        'fixture':FIXTURE,'fixture_sha256':FIXTURE_HASH,
        'probe_sha256':digest(probe.read_bytes()),
        'probe_sources':source_hashes(),
        'core_archive_sha256':digest((ROOT/'psycle-cpp-r12005-sanitized/psycle-core/++qmake/libpsycle-core.a').read_bytes()),
        'procedure':'Build a separate probe against the verified historical static core; in one fresh process per format version load the exact PSY3 fixture without playback using PSYCLE_THREADS=1 and call CoreSong::save(path, version) for versions 2, 3 and 4; record API returns, limited before/after state, reports, process result, logs and any new output bytes. No serializer is substituted.',
        'state_scope':'Metadata, timing scalars and occupied machine slots only; not full semantic song state or a round-trip equivalence claim.',
        'attempts':[], 'original_psycle_observed':False,'parity_status':'UNKNOWN',
    }
    for version in (2,3,4):
        path=output_dir/f'candidate-v{version}.psy'
        raw_path=output_dir/f'candidate-v{version}.json'
        log_path=output_dir/f'candidate-v{version}.log'
        env=os.environ.copy();env['PSYCLE_THREADS']='1'
        env.pop('TERM',None) # Keep the preserved logger's optional ANSI colours disabled.
        try:
            # No shell is involved: the local build's probe and each argument
            # are passed separately. Shell quoting would change the filenames.
            run=subprocess.run([str(probe),str(root/FIXTURE),str(version),str(path)],
                               input=b'',capture_output=True,timeout=20,env=env)
            code,raw,log=run.returncode,run.stdout,run.stderr
        except subprocess.TimeoutExpired as exc:
            code,raw,log=None,exc.stdout or b'',exc.stderr or b''
        raw_path.write_bytes(raw);log_path.write_bytes(log)
        output=binding(root,path) if path.is_file() else None
        receipt['attempts'].append({'format_version':version,'exit_code':code,
            'observation':outcome(raw,log,code,version,output,path.read_bytes() if output else None),
            'probe_result':binding(root,raw_path),'log':binding(root,log_path),'output':output})
    write(root/'candidate-serialization.json',receipt)


def validate_candidate(root: Path) -> dict:
    root=root.resolve()
    receipt=read(root/'candidate-serialization.json')
    expected={'schema_version':1,'phase':'6C','scope':'candidate-serialization-observation',
              'contract':CONTRACT,'evidence_role':'candidate','snapshot':BASELINE,
              'fixture':FIXTURE,'fixture_sha256':FIXTURE_HASH,
              'original_psycle_observed':False,'parity_status':'UNKNOWN'}
    for key,value in expected.items():
        if type(receipt.get(key)) is not type(value) or receipt[key]!=value:
            raise ValueError('wrong candidate identity: '+key)
    if digest(child(root,FIXTURE).read_bytes())!=FIXTURE_HASH:
        raise ValueError('wrong fixture bytes')
    for key in ('probe_sha256','core_archive_sha256'):
        if not re.fullmatch('[0-9a-f]{64}',receipt.get(key,'')):
            raise ValueError('missing executable/core identity')
    if receipt.get('probe_sources')!=source_hashes():
        raise ValueError('observation harness/source identity mismatch')
    attempts=receipt.get('attempts')
    if not isinstance(attempts,list) or len(attempts)!=3:
        raise ValueError('serialization requires three independent format attempts')
    for version,attempt in zip((2,3,4),attempts):
        if type(attempt.get('format_version')) is not int or attempt['format_version']!=version:
            raise ValueError('format attempt inventory changed')
        for key,suffix in (('probe_result','json'),('log','log')):
            if attempt[key]['path']!=f'serialization/candidate-v{version}.{suffix}':
                raise ValueError('format attempt is bound to wrong evidence')
        output=attempt.get('output')
        output_path=root/f'serialization/candidate-v{version}.psy'
        output_bytes=None
        if output is not None:
            output_bytes=bound_bytes(root,output)
            if output['path']!=output_path.relative_to(root).as_posix():
                raise ValueError('invalid output identity')
        elif output_path.exists():
            raise ValueError('unreported serialization output')
        expected_result=outcome(bound_bytes(root,attempt['probe_result']),bound_bytes(root,attempt['log']),attempt.get('exit_code'),version,output,output_bytes)
        if attempt.get('observation')!=expected_result:
            raise ValueError('serialization observation does not follow recorded API/process evidence')
    return receipt


def validate_original(candidate_root: Path, root: Path) -> dict:
    root=root.resolve();candidate_root=candidate_root.resolve()
    original=load_module('phase6c-validate-original-receipts-v2.py')
    initial=original.validate_pair('psy3','project-io-psy3-parse',candidate_root,root)
    save=read(root/'serialization-save.json')
    if save.get('schema_version')!=1 or save.get('outcome') not in {'saved','inconclusive'}:
        raise ValueError('invalid original save outcome')
    diagnostics=save.get('diagnostics')
    if not isinstance(diagnostics,list) or any(not isinstance(d,str) or not d for d in diagnostics):
        raise ValueError('invalid original save diagnostics')
    reopen=None;output=None
    if save['outcome']=='saved':
        if initial!='accepted' or diagnostics:
            raise ValueError('save requires clean accepted initial load and no diagnostics')
        initial_receipt=read(root/'original-psy3.json')
        inventory=read(child(root,initial_receipt['environment']['loaded_vc90_runtime']['path']))
        if type(save.get('process_id')) is not int or save['process_id']!=inventory['process_id']:
            raise ValueError('save action is not bound to the observed reference process')
        for field in ('command_verified','command_dispatched','dialog_verified','path_set','native_path_verified','save_invoked','dialog_closed'):
            if save.get(field) is not True:
                raise ValueError('incomplete Save As evidence: '+field)
        polls=save.get('stable_output_polls')
        if type(polls) is not int or polls<4:
            raise ValueError('save output was not stable')
        path_polls=save.get('stable_path_polls')
        if type(path_polls) is not int or path_polls<4:
            raise ValueError('save filename was not stable before invocation')
        commands=[c for c in save.get('menu_inventory',[]) if c.get('label','').split('\t')[0].replace('&','').strip() in ('Save As...','Save As','Save As…','Save as...')]
        if len(commands)!=1 or commands[0].get('enabled') is not True or commands[0].get('menu')!='File':
            raise ValueError('save menu evidence missing or ambiguous')
        controls=save.get('dialog_inventory',[])
        edits=[c for c in controls if isinstance(c,dict) and c.get('type')=='ControlType.Edit'
               and c.get('id') in ('1001','1148') and c.get('enabled') is True]
        buttons=[c for c in controls if isinstance(c,dict) and c.get('type')=='ControlType.Button'
                 and c.get('name','').replace('&','')=='Save' and c.get('enabled') is True]
        if len(edits)!=1 or len(buttons)!=1:
            raise ValueError('Save As control signature is not uniquely recorded')
        fixture=read(root/'serialization/saved-fixture.json')
        if fixture.get('fixture')!='serialization/original-saved.psy':
            raise ValueError('wrong original saved output path')
        output={'path':fixture['fixture'],'sha256':fixture['fixture_sha256']}
        if not bound_bytes(root,output).startswith(b'PSY3SONG'):
            raise ValueError('original save output is not PSY3')
        reopen=original.validate_pair('psy3-reopen',CONTRACT,root,root,saved_fixture=True)
        if read(root/'original-psy3-reopen.json')['fixture_sha256']!=output['sha256']:
            raise ValueError('reopen does not bind saved bytes')
    elif not diagnostics:
        raise ValueError('inconclusive save needs a concrete diagnostic')
    return {'save':save['outcome'],'reopen':reopen,'output':output,'parity_status':'UNKNOWN',
            'semantic_roundtrip':'not-measured','byte_identity':None if output is None else output['sha256']==FIXTURE_HASH}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode',choices=('collect','candidate','original'))
    parser.add_argument('root',type=Path)
    parser.add_argument('other',type=Path,nargs='?')
    args=parser.parse_args()
    if args.mode=='collect':
        if args.other is None: parser.error('collect requires the built probe path')
        collect(args.root,args.other)
    elif args.mode=='candidate':
        r=validate_candidate(args.root)
        print(json.dumps({'candidate_serialization':[{k:a[k] for k in ('format_version','observation','exit_code')} for a in r['attempts']],'parity_status':'UNKNOWN'}))
    else:
        if args.other is None: parser.error('original requires the original artifact root')
        print(json.dumps(validate_original(args.root,args.other)))

if __name__=='__main__':
    main()
