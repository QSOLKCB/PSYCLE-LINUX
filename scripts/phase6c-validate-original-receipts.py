#!/usr/bin/env python3
from __future__ import annotations

import pathlib
import runpy

runpy.run_path(
    str(pathlib.Path(__file__).with_name("phase6c-validate-original-receipts-v2.py")),
    run_name="__main__",
)
