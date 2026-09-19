#!/usr/bin/env python3
"""Negative controls for fixture-scoped load evidence and matrix promotion."""
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EVIDENCE = ROOT / 'phase6c/evidence/project-io-psy3-parse'
spec = importlib.util.spec_from_file_location('parse', ROOT / 'scripts/phase6c-candidate-parse.py')
parse = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parse)
RAW = (EVIDENCE / 'candidate-psy3.json').read_bytes()
LOG = (EVIDENCE / 'candidate-psy3.log').read_bytes()


class ParseEvidence(unittest.TestCase):
    def derive(self, text=None, **changes):
        raw = json.loads(RAW)
        log = LOG if text is None else text.encode()
        raw['log']['sha256'] = parse.sha(log)
        raw.update(changes)
        return parse.derive(json.dumps(raw).encode(), log)

    def test_timeout_separate_from_accepted_parse(self):
        result = parse.derive(RAW, LOG)
        self.assertEqual(result['parse_result'], 'accepted')
        self.assertEqual(result['process_exit_code'], 124)
        self.assertEqual(result['process_observation'], 'timeout')
        self.assertEqual(result['parity_status'], 'UNKNOWN')
        self.assertFalse(result['original_psycle_observed'])

    def test_clean_exit_with_same_markers(self):
        self.assertEqual(self.derive(exit_code=0, observation='load-and-clean-exit')['parse_result'], 'accepted')

    def test_partial_and_duplicate_markers(self):
        lines = LOG.decode().splitlines()
        for marker in ('setting audio driver to: dummy', 'loading song file:', 'loading psycle song fileformat',
                       'plugin: <sampler>', 'plugin: <master>', 'playing...'):
            line = next(line for line in lines if marker in line)
            for text in (LOG.decode().replace(line+'\n', ''), LOG.decode()+line+'\n'):
                with self.subTest(marker=marker, text=text[-80:]):
                    self.assertEqual(self.derive(text)['parse_result'], 'inconclusive')

    def test_out_of_order_and_wrong_path(self):
        lines = LOG.decode().splitlines()
        a = next(i for i,l in enumerate(lines) if 'plugin: <sampler>' in l)
        b = next(i for i,l in enumerate(lines) if 'plugin: <master>' in l)
        lines[a], lines[b] = lines[b], lines[a]
        self.assertEqual(self.derive('\n'.join(lines))['parse_result'], 'inconclusive')
        self.assertEqual(self.derive(LOG.decode().replace('/phase4-first.psy', '/other.psy', 1))['parse_result'], 'inconclusive')

    def test_damage_warning_even_after_success_marker(self):
        for diagnostic in ('W: main: psycle: core: psy3 loader: error reading from file; chunks missing',
                           'E: main: failed to load', 'X: main: exception', 'W: main: unknown warning'):
            text = LOG.decode()+'log: 99999us: '+diagnostic+'\n'
            self.assertEqual(self.derive(text)['parse_result'], 'inconclusive')

    def test_unstructured_or_spoofed_success(self):
        self.assertEqual(self.derive(LOG.decode()+'Segmentation fault\n')['parse_result'], 'inconclusive')
        self.assertEqual(self.derive(LOG.decode().replace('I: main: psycle: player: playing...',
                                                        'I: thread-id-1: psycle: player: playing...'))['parse_result'], 'inconclusive')

    def test_signal_and_harness_failures_never_accept(self):
        for rc in (1,2,125,126,127,137,139,-11):
            with self.subTest(rc=rc):
                self.assertEqual(self.derive(exit_code=rc)['parse_result'], 'inconclusive')
        self.assertEqual(self.derive(exit_code=0)['parse_result'], 'inconclusive')
        with self.assertRaises(ValueError):
            self.derive(exit_code=False)

    def test_identity_and_hash_changes_fail(self):
        for changes in ({'fixture_sha256':'0'*64}, {'snapshot':'0'*64},
                        {'original_psycle_observed':0}, {'schema_version':True},
                        {'parity_status':'PASS'}, {'fixture':'../phase4-first.psy'}):
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                self.derive(**changes)
        with self.assertRaises(ValueError):
            parse.derive(RAW, LOG+b'\n')

    def test_live_fixture_bytes_verified(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)
            shutil.copytree(EVIDENCE,path,dirs_exist_ok=True)
            fixture=path/parse.FIXTURE
            fixture.parent.mkdir(parents=True)
            fixture.write_bytes(b'PSY3SONG wrong fixture')
            with self.assertRaises(ValueError):
                parse.validate(path)

    def test_paths_cannot_escape_artifact(self):
        for path in ('../secret', '/tmp/secret', 'nested/../../secret', 'nested\\secret'):
            with self.subTest(path=path), self.assertRaises(ValueError):
                parse.child(EVIDENCE,path)


class MatrixEvidence(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        shutil.copytree(ROOT/'phase6c',self.root/'phase6c')
        (self.root/'scripts').mkdir()
        for name in ('phase6c-validate-matrix.py','phase6c-candidate-parse.py'):
            shutil.copyfile(ROOT/'scripts'/name,self.root/'scripts'/name)

    def check(self, success):
        result=subprocess.run(['python3','scripts/phase6c-validate-matrix.py'],cwd=self.root,capture_output=True,text=True)
        self.assertEqual(result.returncode==0,success,result.stdout+result.stderr)

    def mutate(self, name, change):
        p=self.root/'phase6c/evidence/project-io-psy3-parse'/name
        data=json.loads(p.read_text())
        change(data)
        p.write_text(json.dumps(data))

    def test_archived_evidence_classifies_two_rows(self):
        self.check(True)

    def test_parse_sidecar_required(self):
        self.mutate('comparison.json',lambda d:d.pop('candidate_parse_receipt'))
        self.check(False)

    def test_forged_sidecar_rejected(self):
        self.mutate('candidate-psy3-parse.json',lambda d:d.update(process_exit_code=0))
        self.check(False)

    def test_warning_bootstrap_required(self):
        self.mutate('original-psy3.json',lambda d:d['fixture_bootstrap']['load_warning'].update(dismissed=False))
        self.check(False)

    def test_historical_process_outcome_cannot_be_rewritten(self):
        self.mutate('candidate-psy3.json',lambda d:d.update(exit_code=0,observation='load-and-clean-exit'))
        self.check(False)

    def test_comparison_binds_both_outcomes(self):
        self.mutate('comparison.json',lambda d:d.update(candidate_exit_code=0))
        self.check(False)

    def test_no_false_difference_from_timeout(self):
        p=self.root/'phase6c/compatibility-matrix.json'
        matrix=json.loads(p.read_text())
        next(r for r in matrix['contracts'] if r['id']=='project-io-psy3-parse')['status']='DIFFERENT'
        p.write_text(json.dumps(matrix))
        self.mutate('comparison.json',lambda d:d.update(verdict='DIFFERENT'))
        self.check(False)


if __name__ == '__main__':
    unittest.main()
