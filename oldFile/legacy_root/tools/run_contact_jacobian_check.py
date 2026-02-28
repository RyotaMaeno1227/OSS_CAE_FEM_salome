#!/usr/bin/env python3
"""Run the island parallel contact test and capture the 3DOF Jacobian log."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BINARY = REPO_ROOT / "chrono-C-all" / "tests" / "test_island_parallel_contacts"
DEFAULT_LOG = REPO_ROOT / "data" / "diagnostics" / "contact_jacobian_log.csv"
DEFAULT_REPORT = REPO_ROOT / "docs" / "coupled_contact_test_notes.md"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--binary",
        default=str(DEFAULT_BINARY),
        help="Path to test_island_parallel_contacts binary (default: %(default)s)",
    )
    parser.add_argument(
        "--log",
        default=str(DEFAULT_LOG),
        help="Jacobian CSV output path (default: %(default)s)",
    )
    parser.add_argument(
        "--report",
        default=str(DEFAULT_REPORT),
        help="Markdown file updated via --jacobian-report (default: %(default)s)",
    )
    parser.add_argument(
        "--skip-report",
        action="store_true",
        help="Skip updating the Markdown status block.",
    )
    parser.add_argument(
        "--output-dir",
        help="Directory to place the Jacobian log (overrides the directory portion of --log).",
    )
    parser.add_argument(
        "--extra-args",
        nargs=argparse.REMAINDER,
        help="Additional arguments forwarded to the binary (everything after '--').",
    )
    parser.add_argument(
        "--list-presets",
        action="store_true",
        help="Show preset IDs/descriptions from data/coupled_constraint_presets.yaml and exit.",
    )
    return parser.parse_args()


def list_presets() -> None:
    preset_file = REPO_ROOT / "data" / "coupled_constraint_presets.yaml"
    if not preset_file.exists():
        print(f"Preset file not found: {preset_file}")
        return
    entries = []
    current_id = None
    current_desc = ""
    for line in preset_file.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped.startswith("- id:"):
            if current_id:
                entries.append((current_id, current_desc.strip()))
            current_id = stripped.split(":", 1)[1].strip()
            current_desc = ""
        elif stripped.startswith("description:"):
            current_desc = stripped.split(":", 1)[1].strip()
    if current_id:
        entries.append((current_id, current_desc.strip()))
    print("Available presets (id — description):")
    for preset_id, desc in entries:
        display = desc or "(no description)"
        print(f"  - {preset_id} — {display}")


def main() -> int:
    args = parse_args()
    if args.list_presets:
        list_presets()
        return 0
    log_path = Path(args.log)
    if args.output_dir:
        output_dir = Path(args.output_dir)
        output_dir.mkdir(parents=True, exist_ok=True)
        log_path = output_dir / log_path.name
    else:
        log_path.parent.mkdir(parents=True, exist_ok=True)

    cmd = [args.binary]
    cmd.extend(["--jacobian-log", str(log_path)])
    if not args.skip_report:
        cmd.extend(["--jacobian-report", args.report])
    if args.extra_args:
        cmd.extend(args.extra_args)

    subprocess.run(cmd, check=True, cwd=REPO_ROOT)
    print(f"Jacobian log written to {log_path}")
    if not args.skip_report:
        print(f"Jacobian status updated in {args.report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
