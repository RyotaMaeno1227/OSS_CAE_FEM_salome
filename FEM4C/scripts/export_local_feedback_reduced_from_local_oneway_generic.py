#!/usr/bin/env python3
import runpy
import subprocess
import sys
from pathlib import Path


def resolve_workspace_root() -> Path:
    start = Path(__file__).resolve()
    for candidate in (start.parent, *start.parents):
        if (
            (candidate / "scripts").is_dir()
            and (candidate / "docs").is_dir()
            and (candidate / "examples").is_dir()
        ):
            return candidate

    try:
        result = subprocess.run(
            ["git", "-C", str(start.parent), "rev-parse", "--show-toplevel"],
            check=True,
            capture_output=True,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError) as exc:
        raise SystemExit(f"FAIL: unable to resolve workspace root ({exc})") from exc

    git_root = Path(result.stdout.strip()).resolve()
    for candidate in (git_root / "FEM4C", git_root):
        if (
            (candidate / "scripts").is_dir()
            and (candidate / "docs").is_dir()
            and (candidate / "examples").is_dir()
        ):
            return candidate

    raise SystemExit("FAIL: unable to resolve workspace root")


def main() -> None:
    workspace_root = resolve_workspace_root()
    target = workspace_root / "tools/exporters/reduced_feedback/mbd/export_local_feedback_reduced_from_local_oneway_generic.py"
    sys.argv[0] = str(target)
    runpy.run_path(str(target), run_name="__main__")


if __name__ == "__main__":
    main()
