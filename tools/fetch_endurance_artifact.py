#!/usr/bin/env python3
"""
Helper to download the latest coupled endurance artifact and suggest a repro command.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Optional, Sequence


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download coupled endurance artifacts from a workflow run and generate a repro command."
    )
    parser.add_argument(
        "run_id",
        help="GitHub Actions run ID (numeric) or run URL.",
    )
    parser.add_argument(
        "--output-dir",
        default="artifacts/coupled_endurance",
        help="Directory to store downloaded artifacts (default: %(default)s).",
    )
    parser.add_argument(
        "--artifact-prefix",
        default="coupled-endurance",
        help="Artifact name prefix (default: %(default)s). Run ID is appended automatically.",
    )
    parser.add_argument(
        "--gh-path",
        default="gh",
        help="Path to the GitHub CLI executable (default: %(default)s).",
    )
    parser.add_argument(
        "--threshold-max-condition",
        type=float,
        default=5.0e8,
        help="Threshold passed to --fail-on-max-condition for the repro command (default: %(default)s).",
    )
    parser.add_argument(
        "--threshold-rank-ratio",
        type=float,
        default=0.05,
        help="Threshold passed to --fail-on-rank-ratio for the repro command (default: %(default)s).",
    )
    parser.add_argument(
        "--summary-out",
        default="repro/latest.summary.json",
        help="Path where the repro command will write the JSON summary (default: %(default)s).",
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Interactively select a run via `gh run list` if run_id is omitted.",
    )
    parser.add_argument(
        "--max-retries",
        type=int,
        default=2,
        help="Number of retries for failed download commands (default: %(default)s).",
    )
    return parser.parse_args()


def derive_run_id(run: str) -> str:
    if run.isdigit():
        return run
    # Accept URLs like https://github.com/org/repo/actions/runs/1234567890
    parts = run.rstrip("/").split("/")
    if parts and parts[-1].isdigit():
        return parts[-1]
    raise ValueError(f"Could not extract run ID from '{run}'.")


def ensure_directory(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def run_command(cmd: list[str]) -> None:
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"Command {' '.join(cmd)} failed with exit code {exc.returncode}") from exc


def find_artifact_file(root: Path, filename: str) -> Optional[Path]:
    matches = list(root.rglob(filename))
    if not matches:
        return None
    # Prefer shortest path (closest to root)
    return min(matches, key=lambda p: len(p.parts))


def interactive_select_run(gh_path: str) -> str:
    list_cmd = [gh_path, "run", "list", "--limit", "20", "--json", "databaseId,headBranch,status,conclusion,displayTitle"]
    try:
        output = subprocess.check_output(list_cmd, text=True)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"`{' '.join(list_cmd)}` failed with exit code {exc.returncode}") from exc

    import json  # local import to avoid eager dependency when not needed

    try:
        runs = json.loads(output)
    except json.JSONDecodeError as exc:
        raise RuntimeError("Failed to parse gh run list output.") from exc

    if not runs:
        raise RuntimeError("No recent workflow runs found.")

    print("Select a workflow run:")
    for idx, run in enumerate(runs, start=1):
        ident = run.get("databaseId", "n/a")
        branch = run.get("headBranch", "unknown")
        status = run.get("status", "?")
        conclusion = run.get("conclusion") or "-"
        title = run.get("displayTitle", "")
        print(f"  {idx:2d}) #{ident} [{status}/{conclusion}] {branch} - {title}")

    while True:
        selection = input("Enter run number (or 'q' to abort): ").strip()
        if selection.lower() == "q":
            raise RuntimeError("Selection aborted by user.")
        if selection.isdigit():
            idx = int(selection)
            if 1 <= idx <= len(runs):
                return str(runs[idx - 1].get("databaseId"))
        print("Invalid selection. Please try again.")


def download_with_retries(cmd: Sequence[str], max_retries: int) -> None:
    attempt = 0
    while True:
        try:
            run_command(list(cmd))
            return
        except RuntimeError as exc:
            if attempt >= max_retries:
                raise
            attempt += 1
            print(f"Warning: {exc}. Retrying ({attempt}/{max_retries})...")


def main() -> int:
    args = parse_args()

    if args.run_id:
        try:
            run_id = derive_run_id(args.run_id)
        except ValueError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1
    elif args.interactive:
        try:
            run_id = interactive_select_run(args.gh_path)
        except RuntimeError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1
    else:
        print("Error: run_id is required unless --interactive is used.", file=sys.stderr)
        return 1

    output_dir = Path(args.output_dir).expanduser().resolve()
    ensure_directory(output_dir)

    artifact_name = f"{args.artifact_prefix}-{run_id}"
    download_target = output_dir / artifact_name
    ensure_directory(download_target)

    print(f"Downloading artifact '{artifact_name}' for run {run_id} into {download_target}")
    download_with_retries(
        [
            args.gh_path,
            "run",
            "download",
            run_id,
            "--name",
            artifact_name,
            "--dir",
            str(download_target),
        ],
        max(args.max_retries, 0),
    )

    csv_path = find_artifact_file(download_target, "latest.csv")
    summary_path = find_artifact_file(download_target, "latest.summary.json")

    if not csv_path:
        print("Warning: latest.csv not found in artifact.", file=sys.stderr)
    else:
        print(f"Located CSV at {csv_path}")

    if summary_path:
        print(f"Located summary at {summary_path}")
    else:
        print("Warning: latest.summary.json not found in artifact.", file=sys.stderr)

    repro_summary = Path(args.summary_out).expanduser()
    repro_summary.parent.mkdir(parents=True, exist_ok=True)

    repro_cmd = [
        "python",
        "tools/plot_coupled_constraint_endurance.py",
        str(csv_path) if csv_path else "path/to/latest.csv",
        "--skip-plot",
        "--summary-json",
        str(repro_summary),
        "--fail-on-max-condition",
        f"{args.threshold_max_condition}",
        "--fail-on-rank-ratio",
        f"{args.threshold_rank_ratio}",
        "--no-show",
    ]

    print("\nReproduction command:")
    print(" ".join(repro_cmd))
    print("\nRemember to install GitHub CLI and authenticate (`gh auth login`) before running this helper.")
    if not csv_path:
        print("After adjusting the CSV path, rerun the command above to reproduce the failure.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
