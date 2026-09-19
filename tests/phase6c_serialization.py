#!/usr/bin/env python3
"""Synthetic negative controls; these fixtures are not compatibility evidence."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('serialization',ROOT/'scripts/phase6c-serialization-evidence.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
STATE={'name':'PSYCLE-LINUX Phase 4 synthetic fixture','author':'test','comment':'test','bpm':125,'tick_speed':4,'tracks':16,'machine_slots':[0,128]}
RAW={'schema_version':1,'load_returned':True,'save_attempted':True,'format_version':3,'save_returned':False,'state_before':STATE.copy(),'state_after':STATE.copy(),'reports':['Load Warning: '+m.WARNING]}

class Candidate(unittest.TestCase):
    def result(self,raw=None,code=0,log=b'',output=None,version=3,output_bytes=None):
        return m.outcome(json.dumps(RAW if raw is None else raw).encode(),log,code,version,output,output_bytes)
    def test_refusal_is_observation_not_parity(self):
        self.assertEqual(self.result(),'save-returned-false-without-output')
    def test_timeout_signal_and_invalid_exit(self):
        for rc in (None,-11,70,124,139,False):
            with self.subTest(rc=rc):self.assertEqual(self.result(code=rc),'inconclusive')
    def test_incomplete_load_and_save(self):
        for field in ('load_returned','save_attempted'):
            for value in (False,1,None):
                data=copy.deepcopy(RAW);data[field]=value
                self.assertEqual(self.result(data),'inconclusive')
    def test_no_false_success_from_output(self):
        self.assertEqual(self.result(output={'path':'output.psy'}),'inconclusive')
        data=copy.deepcopy(RAW);data['save_returned']=True
        self.assertEqual(self.result(data),'inconclusive')
        self.assertEqual(self.result(data,output={'path':'output.psy'},output_bytes=b'PSY3SONG synthetic'),'saved-output-not-yet-reloaded')
    def test_invalid_saved_bytes_remain_inconclusive(self):
        data=copy.deepcopy(RAW);data['save_returned']=True
        for content in (None,b'',b'PSY3SONG',b'unrelated file',b'PSY2SONG wrong format'):
            self.assertEqual(self.result(data,output={'path':'output.psy'},output_bytes=content),'inconclusive')
    def test_reports_and_state_changes_block_refusal(self):
        data=copy.deepcopy(RAW);data['reports'].append('Song Load Error: missing chunks')
        self.assertEqual(self.result(data),'inconclusive')
        data=copy.deepcopy(RAW);data['state_after']['tracks']=99
        self.assertEqual(self.result(data),'inconclusive')
    def test_diagnostic_or_corrupt_output(self):
        for log in (b'Segmentation fault\n',b'\xff',b'log: 1us: W: probe: missing chunks\n',b'log: 1us: X: probe: exception\n'):
            self.assertEqual(self.result(log=log),'inconclusive')
        self.assertEqual(m.outcome(b'not json',b'',0,3,None),'inconclusive')
    def test_default_format_absence_is_scoped(self):
        data=copy.deepcopy(RAW);data['format_version']=4
        log=b"SongSerializer::saveSong(): Couldn't find appropriate filter for file format version 4\n"
        self.assertEqual(self.result(data,version=4,log=log),'save-returned-false-without-output')
        self.assertEqual(self.result(log=log),'inconclusive')
    def test_wrong_version_and_fixture_metadata(self):
        for changes in ({'format_version':True},{'format_version':2},{'state_before':{}},{'reports':[]}):
            data=copy.deepcopy(RAW);data.update(changes)
            self.assertEqual(self.result(data),'inconclusive')
    def test_artifact_path_and_hash_integrity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); (root/'log').write_bytes(b'log')
            for value in ('../escape','/tmp/escape','a\\b'):
                with self.assertRaises(ValueError):m.child(root,value)
            with self.assertRaises(ValueError):m.bound_bytes(root,{'path':'log','sha256':'0'*64})

class Original(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name);(self.root/'serialization').mkdir()
        self.output=self.root/'serialization/original-saved.psy';self.output.write_bytes(b'PSY3SONG synthetic control')
        self.hash=m.digest(self.output.read_bytes())
        self.save={'process_id':42,'schema_version':1,'outcome':'saved','diagnostics':[],'stable_output_polls':4,'stable_path_polls':4,
                   **{k:True for k in ('command_verified','command_dispatched','dialog_verified','path_set','native_path_verified','save_invoked','dialog_closed')},
                   'menu_inventory':[{'menu':'File','label':'Save &As...','enabled':True}],
                   'dialog_inventory':[{'type':'ControlType.Edit','id':'1001','enabled':True},
                                       {'type':'ControlType.Button','name':'Save','enabled':True}]}
        self.fixture={'fixture':'serialization/original-saved.psy','fixture_sha256':self.hash}
        self.reopen={'fixture_sha256':self.hash}
        (self.root/'original-psy3.json').write_text(json.dumps({'environment':{'loaded_vc90_runtime':{'path':'runtime.json'}}}))
        (self.root/'runtime.json').write_text(json.dumps({'process_id':42}))
    def run_validation(self,initial='accepted',reopen='accepted'):
        for name,data in [('serialization-save.json',self.save),('serialization/saved-fixture.json',self.fixture),('original-psy3-reopen.json',self.reopen)]:
            (self.root/name).write_text(json.dumps(data))
        def validate(name,contract,candidate_root,original_root,**kwargs):
            if name=='psy3-reopen':
                self.assertTrue(kwargs['saved_fixture'])
                return reopen
            return initial
        with patch.object(m,'load_module',return_value=types.SimpleNamespace(validate_pair=validate)):
            return m.validate_original(self.root,self.root)
    def test_saved_and_reopened_remain_unknown_semantically(self):
        result=self.run_validation()
        self.assertEqual(result['reopen'],'accepted')
        self.assertEqual(result['parity_status'],'UNKNOWN')
        self.assertEqual(result['semantic_roundtrip'],'not-measured')
        self.assertFalse(result['byte_identity'])
    def test_unstable_save_filename_blocks_saved(self):
        for value in (None,True,0,3):
            self.save['stable_path_polls']=value
            with self.assertRaises(ValueError):self.run_validation()
    def test_observed_original_menu_spelling(self):
        self.save['menu_inventory'][0]['label']='Save &as...'
        self.assertEqual(self.run_validation()['save'],'saved')
    def test_missing_action_or_stable_output_blocks_saved(self):
        for key in ('command_verified','path_set','native_path_verified','save_invoked','dialog_closed'):
            self.save[key]=False
            with self.assertRaises(ValueError):self.run_validation()
            self.save[key]=True
        self.save['stable_output_polls']=True
        with self.assertRaises(ValueError):self.run_validation()
    def test_ambiguous_menu_and_dialog_block_saved(self):
        self.save['menu_inventory']*=2
        with self.assertRaises(ValueError):self.run_validation()
        self.save['menu_inventory']=self.save['menu_inventory'][:1]
        self.save['dialog_inventory']*=2
        with self.assertRaises(ValueError):self.run_validation()
    def test_error_and_unaccepted_initial_load_block_saved(self):
        self.save['diagnostics']=['UI failed']
        with self.assertRaises(ValueError):self.run_validation()
        self.save['diagnostics']=[]
        with self.assertRaises(ValueError):self.run_validation(initial='inconclusive')
    def test_missing_or_changed_saved_file_fails(self):
        self.output.write_bytes(b'PSY3SONG different')
        with self.assertRaises(ValueError):self.run_validation()
    def test_wrong_reopen_bytes_fail(self):
        self.reopen['fixture_sha256']='0'*64
        with self.assertRaises(ValueError):self.run_validation()
    def test_inconclusive_save_requires_diagnostics(self):
        self.save={'schema_version':1,'outcome':'inconclusive','diagnostics':['Save As unavailable']}
        self.assertIsNone(self.run_validation()['reopen'])
        self.save['diagnostics']=[]
        with self.assertRaises(ValueError):self.run_validation()

if __name__=='__main__':unittest.main()
