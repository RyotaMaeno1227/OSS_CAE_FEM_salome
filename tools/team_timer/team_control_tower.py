#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def main(argv: list[str] | None = None) -> int:
    argv = argv or sys.argv[1:]
    repo_root = Path(__file__).resolve().parents[2]
    env = os.environ.copy()
    env.setdefault("TEAM_TIMER_STATE_ROOT", "/tmp/highperformanceFEM_team_timer")
    cmd = [
        sys.executable,
        str(repo_root / "scripts" / "team_control_tower.py"),
        "--state-root",
        env["TEAM_TIMER_STATE_ROOT"],
        *argv,
    ]
    return subprocess.call(cmd, cwd=repo_root, env=env)


if __name__ == "__main__":
    raise SystemExit(main())
