#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def main(argv: list[str] | None = None) -> int:
    argv = argv or sys.argv[1:]
    repo_root = Path(__file__).resolve().parents[2]
    cmd = [sys.executable, str(repo_root / "scripts" / "audit_team_sessions.py"), *argv]
    return subprocess.call(cmd, cwd=repo_root)


if __name__ == "__main__":
    raise SystemExit(main())
