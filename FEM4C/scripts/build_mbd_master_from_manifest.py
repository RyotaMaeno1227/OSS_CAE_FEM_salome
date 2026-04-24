#!/usr/bin/env python3
import runpy
import sys
from pathlib import Path


def find_workspace_root() -> Path:
    current = Path(__file__).resolve().parent
    for candidate in (current, *current.parents):
        if (candidate / "scripts").is_dir() and (candidate / "tools").is_dir():
            return candidate
    raise SystemExit("FAIL: could not determine workspace root")


WORKSPACE_ROOT = find_workspace_root()
TARGET = WORKSPACE_ROOT / "tools/builders/manifests/build_mbd_master_from_manifest.py"

sys.argv[0] = str(TARGET)
runpy.run_path(str(TARGET), run_name="__main__")
