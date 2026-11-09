# SPDX-License-Identifier: Apache-2.0
"""Headless contract tests for the early Listening Lab controller."""
import os
from pathlib import Path
import stat
import sys
import tempfile
import time
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "ui"))
from listening_lab import PlaybackConfig, PlayerSession


class ListeningLabTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.player = self.root / "fake player"
        self.song = self.root / "song with spaces.psy"
        self.song.write_bytes(b"PSY3SONG synthetic test input")
        self.player.write_text(
            "#!" + sys.executable + "\n"
            "import os,sys\n"
            "print('FAKE_PLAYER_READY', flush=True)\n"
            "print('PSYCLE_PATH=' + repr(os.environ.get('PSYCLE_PATH')), flush=True)\n"
            "print('ARGS=' + repr(sys.argv[1:]), flush=True)\n"
            "sys.stdin.readline()\n"
            "print('FAKE_PLAYER_STOPPED', flush=True)\n",
            encoding="utf-8",
        )
        self.player.chmod(self.player.stat().st_mode | stat.S_IXUSR)

    def config(self, **kwargs):
        return PlaybackConfig(player=self.player, song=self.song, **kwargs)

    def test_argv_preserves_spaces_and_waits_for_stop(self):
        args = self.config(driver="alsa", device="plughw:0").command()
        self.assertEqual(args[0], str(self.player.resolve()))
        self.assertEqual(args[-2:], ["--input-file", str(self.song.resolve())])
        self.assertEqual(args[args.index("--output-device") + 1], "plughw:0")
        self.assertIn("--wait", args)

    def test_rejects_invalid_driver_and_non_psy_file(self):
        with self.assertRaisesRegex(ValueError, "Unsupported"):
            self.config(driver="not-a-driver").command()
        wav = self.root / "sample.wav"
        wav.touch()
        with self.assertRaisesRegex(ValueError, r"\.psy"):
            PlaybackConfig(self.player, wav).command()

    def test_refuses_overwriting_wav(self):
        wav = self.root / "existing.wav"
        wav.touch()
        with self.assertRaisesRegex(ValueError, "overwrite"):
            self.config(wav_output=wav).command()
        new = self.root / "new.wav"
        args = self.config(wav_output=new).command()
        self.assertEqual(args[args.index("--output-file") + 1], str(new.resolve()))

    def test_plugin_paths_opt_in_only(self):
        with mock.patch.dict(os.environ, {"PSYCLE_PATH": "/tmp/plugins", "LADSPA_PATH": "/tmp/ladspa"}):
            clean = self.config().environment()
            self.assertNotIn("PSYCLE_PATH", clean)
            self.assertNotIn("LADSPA_PATH", clean)
            opted = self.config(allow_plugins=True).environment()
            self.assertEqual(opted["PSYCLE_PATH"], "/tmp/plugins")
            self.assertEqual(opted["LADSPA_PATH"], "/tmp/ladspa")

    def test_fake_player_launch_and_graceful_shutdown(self):
        with mock.patch.dict(os.environ, {"PSYCLE_PATH": "/tmp/ignore"}):
            session = PlayerSession(self.config())
            try:
                session.start()
                deadline = time.monotonic() + 3
                output = ""
                while "FAKE_PLAYER_READY" not in output and time.monotonic() < deadline:
                    output += session.output()
                    time.sleep(0.02)
                self.assertIn("FAKE_PLAYER_READY", output)
                self.assertIn("PSYCLE_PATH=None", output)
                session.stop()
                while session.poll() is None and time.monotonic() < deadline:
                    time.sleep(0.02)
                self.assertEqual(session.poll(), 0)
                output += session.output()
                self.assertIn("FAKE_PLAYER_STOPPED", output)
                self.assertTrue(session.started_utc)
                self.assertTrue(session.ended_utc)
            finally:
                session.close()


if __name__ == "__main__":
    unittest.main()
