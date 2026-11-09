#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Early human listening harness for the historical C++ Psycle player.

This is a temporary, external GUI; it does not alter the preserved engine or
claim parity with original Psycle.  Only the Python standard library is used.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import subprocess
import tempfile
import time
import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext, ttk
from dataclasses import dataclass
from datetime import datetime, timezone

REPO = Path(__file__).resolve().parent.parent
DEFAULT_PLAYER = (
    REPO / "psycle-cpp-r12005-sanitized" / "psycle-player"
    / "++qmake" / "psycle-player"
)
DRIVERS = ("alsa", "jack", "dummy")
LOG_LIMIT = 65536


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


@dataclass(frozen=True)
class PlaybackConfig:
    player: Path
    song: Path
    driver: str = "alsa"
    device: str = ""
    wav_output: Path | None = None
    allow_plugins: bool = False

    def validate(self) -> None:
        if not self.player.is_file() or not os.access(self.player, os.X_OK):
            raise ValueError("Build psycle-player first or choose an executable player.")
        if not self.song.is_file() or self.song.suffix.lower() != ".psy":
            raise ValueError("Select an existing .psy song file.")
        if self.driver not in DRIVERS:
            raise ValueError("Unsupported output driver selection.")
        if "\x00" in self.device or "\n" in self.device:
            raise ValueError("Invalid audio-device name.")
        if self.wav_output is not None:
            if self.wav_output.suffix.lower() != ".wav":
                raise ValueError("WAV output needs a .wav file name.")
            if not self.wav_output.parent.is_dir():
                raise ValueError("WAV output directory does not exist.")
            if self.wav_output.exists():
                raise ValueError("Refusing to overwrite an existing WAV file.")

    def command(self) -> list[str]:
        self.validate()
        args = [str(self.player.resolve()), "--output-driver", self.driver]
        if self.device.strip():
            args.extend(("--output-device", self.device.strip()))
        if self.wav_output is not None:
            args.extend(("--output-file", str(self.wav_output.resolve())))
        args.extend(("--wait", "--input-file", str(self.song.resolve())))
        return args

    def environment(self) -> dict[str, str]:
        env = os.environ.copy()
        # Candidate plugin discovery/loading has not yet passed containment
        # gates. Do not opt into configured third-party plugin paths by default.
        if not self.allow_plugins:
            env.pop("PSYCLE_PATH", None)
            env.pop("LADSPA_PATH", None)
        return env


class PlayerSession:
    """One subprocess, its isolated log, and bounded nonblocking shutdown."""

    def __init__(self, config: PlaybackConfig):
        self.config = config
        self.process: subprocess.Popen[bytes] | None = None
        self.started_utc = ""
        self.ended_utc = ""
        self.stop_at: float | None = None
        self._scratch = tempfile.TemporaryDirectory(prefix="psycle-listening-")
        self.log_path = Path(self._scratch.name) / "player.log"
        self._reader = None

    def start(self) -> None:
        if self.process is not None:
            raise RuntimeError("Session already started.")
        command = self.config.command()
        with self.log_path.open("wb") as writer:
            self.process = subprocess.Popen(
                command, stdin=subprocess.PIPE, stdout=writer,
                stderr=subprocess.STDOUT, env=self.config.environment(),
            )
        self._reader = self.log_path.open("rb")
        self.started_utc = utc_now()

    def output(self) -> str:
        if self._reader is None:
            return ""
        return self._reader.read(32768).decode("utf-8", errors="replace")

    def stop(self) -> None:
        proc = self.process
        if proc is None or proc.poll() is not None or self.stop_at is not None:
            return
        self.stop_at = time.monotonic()
        try:
            if proc.stdin is not None:
                proc.stdin.write(b"\n")
                proc.stdin.flush()
                proc.stdin.close()
        except (BrokenPipeError, OSError):
            pass

    def poll(self) -> int | None:
        proc = self.process
        if proc is None:
            return None
        rc = proc.poll()
        if rc is not None:
            if not self.ended_utc:
                self.ended_utc = utc_now()
            return rc
        if self.stop_at is not None:
            elapsed = time.monotonic() - self.stop_at
            if elapsed > 5:
                proc.kill()
            elif elapsed > 3:
                proc.terminate()
        return None

    def abort(self) -> None:
        proc = self.process
        if proc is not None and proc.poll() is None:
            proc.kill()

    def log_tail(self) -> str:
        with self.log_path.open("rb") as stream:
            stream.seek(0, os.SEEK_END)
            stream.seek(max(0, stream.tell() - LOG_LIMIT))
            return stream.read().decode("utf-8", errors="replace")

    def close(self) -> None:
        self.abort()
        if self.process is not None:
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                pass
            if self.process.stdin is not None:
                self.process.stdin.close()
        if self._reader is not None:
            self._reader.close()
        self._scratch.cleanup()


class ListeningLab:
    def __init__(self, root: tk.Tk, player: Path, song: Path | None = None):
        self.root = root
        self.root.title("PSYCLE-LINUX — Listening Lab (candidate, not parity)")
        self.root.minsize(760, 640)
        self.player_var = tk.StringVar(value=str(player))
        self.song_var = tk.StringVar(value=str(song) if song else "")
        self.driver_var = tk.StringVar(value="alsa")
        self.device_var = tk.StringVar()
        self.record_var = tk.BooleanVar(value=False)
        self.wav_var = tk.StringVar()
        self.plugins_var = tk.BooleanVar(value=False)
        self.issue_var = tk.StringVar(value="Sounds correct")
        self.status_var = tk.StringVar(value="Ready — lower system volume before Play.")
        self.session: PlayerSession | None = None
        self._finished_status = ""
        self._build()
        self.root.protocol("WM_DELETE_WINDOW", self._close)
        self.root.after(150, self._tick)

    def _build(self) -> None:
        outer = ttk.Frame(self.root, padding=12)
        outer.pack(fill="both", expand=True)
        outer.columnconfigure(1, weight=1)

        def file_row(row: int, title: str, var: tk.StringVar, action):
            ttk.Label(outer, text=title).grid(row=row, column=0, sticky="w", pady=3)
            ttk.Entry(outer, textvariable=var).grid(row=row, column=1, sticky="ew", padx=5)
            ttk.Button(outer, text="Browse…", command=action).grid(row=row, column=2)

        file_row(0, "Song (.psy)", self.song_var, self._browse_song)
        file_row(1, "C++ player", self.player_var, self._browse_player)
        ttk.Label(outer, text="Audio driver").grid(row=2, column=0, sticky="w")
        ttk.Combobox(outer, values=DRIVERS, textvariable=self.driver_var, state="readonly", width=18).grid(
            row=2, column=1, sticky="w", pady=3)
        ttk.Label(outer, text="Device (optional)").grid(row=3, column=0, sticky="w")
        ttk.Entry(outer, textvariable=self.device_var).grid(row=3, column=1, sticky="ew", padx=5, pady=3)
        ttk.Checkbutton(outer, text="Also render a WAV", variable=self.record_var).grid(
            row=4, column=0, sticky="w")
        ttk.Entry(outer, textvariable=self.wav_var).grid(row=4, column=1, sticky="ew", padx=5)
        ttk.Button(outer, text="WAV path…", command=self._browse_wav).grid(row=4, column=2)
        ttk.Checkbutton(
            outer, text="Allow configured third-party plugin paths (unsafe / optional)",
            variable=self.plugins_var,
        ).grid(row=5, column=0, columnspan=3, sticky="w", pady=4)
        ttk.Label(
            outer,
            text="Start at low system volume. 'dummy' is silent. No plugins are scanned from configured paths unless opted in.",
            wraplength=700,
        ).grid(row=6, column=0, columnspan=3, sticky="w", pady=3)

        controls = ttk.Frame(outer)
        controls.grid(row=7, column=0, columnspan=3, sticky="ew", pady=7)
        self.play_button = ttk.Button(controls, text="▶ Play song", command=self._play)
        self.play_button.pack(side="left", padx=(0, 6))
        self.stop_button = ttk.Button(controls, text="■ Stop", command=self._stop, state="disabled")
        self.stop_button.pack(side="left")
        ttk.Button(controls, text="Save listening report…", command=self._save_report).pack(side="right")
        ttk.Label(outer, textvariable=self.status_var, wraplength=700).grid(
            row=8, column=0, columnspan=3, sticky="w", pady=3)
        ttk.Label(outer, text="Player diagnostics (not a parity verdict)").grid(
            row=9, column=0, columnspan=3, sticky="w")
        self.log = scrolledtext.ScrolledText(outer, height=11, wrap="word", state="disabled")
        self.log.grid(row=10, column=0, columnspan=3, sticky="nsew", pady=4)
        outer.rowconfigure(10, weight=1)
        ttk.Label(outer, text="What did your ears hear?").grid(row=11, column=0, sticky="w", pady=4)
        ttk.Combobox(
            outer, textvariable=self.issue_var, state="readonly",
            values=("Sounds correct", "Silent", "Wrong pitch", "Timing / rhythm",
                    "Crackle / clipping", "Dropouts / stutter", "Stereo / balance", "Other"),
        ).grid(row=11, column=1, sticky="w")
        ttk.Label(outer, text="Notes / steps / timestamps").grid(
            row=12, column=0, columnspan=3, sticky="w", pady=3)
        self.notes = scrolledtext.ScrolledText(outer, height=5, wrap="word")
        self.notes.grid(row=13, column=0, columnspan=3, sticky="nsew")

    def _browse_song(self) -> None:
        path = filedialog.askopenfilename(filetypes=[("Psycle songs", "*.psy"), ("All files", "*")])
        if path:
            self.song_var.set(path)

    def _browse_player(self) -> None:
        path = filedialog.askopenfilename(title="Select compiled historical psycle-player")
        if path:
            self.player_var.set(path)

    def _browse_wav(self) -> None:
        path = filedialog.asksaveasfilename(defaultextension=".wav", filetypes=[("Wave audio", "*.wav")])
        if path:
            self.wav_var.set(path)
            self.record_var.set(True)

    def _append(self, message: str) -> None:
        self.log.configure(state="normal")
        self.log.insert("end", message)
        # Bound GUI memory independently of the session's full temporary log.
        if float(self.log.index("end-1c").split(".")[0]) > 2000:
            self.log.delete("1.0", "500.0")
        self.log.see("end")
        self.log.configure(state="disabled")

    def _play(self) -> None:
        if self.session is not None and self.session.process is not None and self.session.poll() is None:
            return
        cfg = PlaybackConfig(
            player=Path(self.player_var.get()).expanduser(),
            song=Path(self.song_var.get()).expanduser(),
            driver=self.driver_var.get(),
            device=self.device_var.get(),
            wav_output=Path(self.wav_var.get()).expanduser() if self.record_var.get() else None,
            allow_plugins=self.plugins_var.get(),
        )
        try:
            cfg.validate()
            if self.session is not None:
                self.session.close()
                self.session = None
            candidate = PlayerSession(cfg)
            try:
                candidate.start()
            except (OSError, subprocess.SubprocessError):
                candidate.close()
                raise
            self.session = candidate
        except (ValueError, OSError, subprocess.SubprocessError) as error:
            messagebox.showerror("Cannot start playback", str(error))
            return
        self._finished_status = ""
        self._append("\n--- Candidate playback session started ---\n")
        self.status_var.set("Player started. A successful start does not prove that audio is audible.")
        self.play_button.configure(state="disabled")
        self.stop_button.configure(state="normal")

    def _stop(self) -> None:
        if self.session:
            self.session.stop()
            self.status_var.set("Stopping (waiting for player to finalize; bounded timeout)…")
            self.stop_button.configure(state="disabled")

    def _tick(self) -> None:
        if self.session is not None:
            content = self.session.output()
            if content:
                self._append(content)
            rc = self.session.poll()
            if rc is not None and not self._finished_status:
                self._append(self.session.output())
                self._finished_status = f"Player exited with code {rc}"
                self.status_var.set(self._finished_status + " — listen, classify, then save report.")
                self.play_button.configure(state="normal")
                self.stop_button.configure(state="disabled")
        self.root.after(150, self._tick)

    def _save_report(self) -> None:
        session = self.session
        if session is None:
            messagebox.showinfo("No session", "Play a song before saving a report.")
            return
        target = filedialog.asksaveasfilename(
            title="Save listening evidence", defaultextension=".json",
            filetypes=[("JSON", "*.json")],
        )
        if not target:
            return
        cfg = session.config
        try:
            report = {
                "schema_version": 1,
                "scope": "human-listening-candidate-only",
                "original_psycle_observed": False,
                "parity_status": "UNKNOWN",
                "started_utc": session.started_utc,
                "ended_utc": session.ended_utc or None,
                "saved_utc": utc_now(),
                "song": str(cfg.song.resolve()),
                "song_sha256": sha256(cfg.song),
                "candidate_player": str(cfg.player.resolve()),
                "candidate_player_sha256": sha256(cfg.player),
                "driver": cfg.driver,
                "device": cfg.device,
                "wav_output": str(cfg.wav_output.resolve()) if cfg.wav_output else None,
                "wav_sha256": (
                    sha256(cfg.wav_output)
                    if cfg.wav_output and session.poll() is not None and cfg.wav_output.is_file()
                    else None
                ),
                "configured_plugin_paths_enabled": cfg.allow_plugins,
                "player_exit_code": session.process.poll() if session.process else None,
                "user_observation": self.issue_var.get(),
                "user_notes": self.notes.get("1.0", "end-1c"),
                "log_tail": session.log_tail(),
            }
            path = Path(target)
            with tempfile.NamedTemporaryFile(
                mode="w", encoding="utf-8", dir=path.parent,
                prefix=".psycle-report-", suffix=".tmp", delete=False,
            ) as stream:
                tmp = Path(stream.name)
                json.dump(report, stream, indent=2, ensure_ascii=False)
                stream.write("\n")
            try:
                os.replace(tmp, path)
            finally:
                tmp.unlink(missing_ok=True)
            self.status_var.set(f"Listening report saved: {path}")
        except (OSError, ValueError) as error:
            messagebox.showerror("Cannot save report", str(error))

    def _close(self) -> None:
        if self.session is not None:
            self.session.close()
        self.root.destroy()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--player", type=Path, default=Path(os.environ.get("PSYCLE_PLAYER_BIN", DEFAULT_PLAYER)))
    parser.add_argument("--song", type=Path)
    parser.add_argument("--smoke-test", action="store_true", help="Construct widgets then exit (CI/Xvfb)")
    args = parser.parse_args()
    try:
        root = tk.Tk()
    except tk.TclError as error:
        print(f"Listening Lab: Tk desktop initialization failed: {error}", file=sys.stderr)
        if "display" in str(error).lower() or not os.environ.get("DISPLAY"):
            print(
                "Tkinter is installed, but this shell cannot reach an X11/XWayland "
                "display. Open a fresh terminal inside the graphical desktop "
                "(outside tmux or SSH) and try again.",
                file=sys.stderr,
            )
            for key in ("DISPLAY", "WAYLAND_DISPLAY", "XDG_SESSION_TYPE", "TMUX"):
                print(f"  {key}={os.environ.get(key) or '<unset>'}", file=sys.stderr)
            print(
                "On GNOME Wayland, Tk typically uses XWayland. "
                "Do not guess DISPLAY=:0; use your actual session environment.",
                file=sys.stderr,
            )
        return 2
    ListeningLab(root, args.player, args.song)
    if args.smoke_test:
        root.after(200, root.destroy)
    root.mainloop()
    return 0


if __name__ == "__main__":
    sys.exit(main())
