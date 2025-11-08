#!/usr/bin/env python3
"""Run the multi-ω bench, refresh preset metadata, and sync documentation."""

from __future__ import annotations

import argparse
import datetime as dt
import subprocess
from pathlib import Path
from typing import Iterable, List

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BENCH = REPO_ROOT / "chrono-C-all" / "tests" / "bench_coupled_constraint"
DEFAULT_OUTPUT = REPO_ROOT / "data" / "diagnostics" / "bench_coupled_constraint_multi.csv"
DEFAULT_STATS = REPO_ROOT / "data" / "diagnostics" / "kkt_backend_stats.json"
README_PATH = REPO_ROOT / "README.md"
HANDSON_PATH = REPO_ROOT / "docs" / "coupled_constraint_hands_on.md"
PRESET_PATH = REPO_ROOT / "data" / "coupled_constraint_presets.yaml"
COMPARISON_SCRIPT = REPO_ROOT / "tools" / "compare_kkt_logs.py"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--bench-path",
        default=str(DEFAULT_BENCH),
        help="Path to bench_coupled_constraint binary (default: %(default)s)",
    )
    parser.add_argument(
        "--output",
        default=str(DEFAULT_OUTPUT),
        help="Path for the multi-ω CSV (default: %(default)s)",
    )
    parser.add_argument(
        "--stats-json",
        default=str(DEFAULT_STATS),
        help="Path for the KKT backend stats JSON (default: %(default)s)",
    )
    parser.add_argument(
        "--preset-id",
        default="multi_omega_reference",
        help="Preset ID to stamp in data/coupled_constraint_presets.yaml (default: %(default)s)",
    )
    parser.add_argument(
        "--omegas",
        type=float,
        nargs="+",
        default=[0.85, 1.0, 1.15],
        help="Omega values to sweep (default: %(default)s)",
    )
    parser.add_argument(
        "--timestamp",
        help="Override timestamp (ISO-8601). Defaults to current UTC time.",
    )
    parser.add_argument(
        "--skip-bench",
        action="store_true",
        help="Skip running the bench executable (useful when only updating docs).",
    )
    parser.add_argument(
        "--refresh-report",
        action="store_true",
        help="Re-run tools/compare_kkt_logs.py after updating the CSV.",
    )
    return parser.parse_args()


def format_omega(value: float) -> str:
    text = f"{value:.3f}".rstrip("0").rstrip(".")
    return text or "0"


def run_bench(args: argparse.Namespace) -> None:
    if args.skip_bench:
        return
    cmd: List[str] = [
        args.bench_path,
        "--output",
        args.output,
        "--stats-json",
        args.stats_json,
    ]
    for omega in args.omegas:
        cmd.extend(["--omega", format_omega(omega)])
    print("[multi-ω] running", " ".join(cmd))
    subprocess.run(cmd, check=True, cwd=REPO_ROOT)


def update_last_updated_line(path: Path, timestamp: str) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    marker = "Multi-ω preset last updated:"
    updated = False
    for idx, line in enumerate(lines):
        if marker in line:
            prefix = line.split(marker, 1)[0]
            lines[idx] = f"{prefix}{marker} {timestamp}"
            updated = True
            break
    if not updated:
        raise RuntimeError(f"Missing marker '{marker}' in {path}")
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def update_command_block(path: Path, omegas: Iterable[float]) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    in_block = False
    header_idx = None
    omega_start = None
    omega_indent = None

    for idx, line in enumerate(lines):
        stripped = line.strip()
        if stripped.startswith("```"):
            # Toggle fenced code block tracking
            in_block = not in_block
            continue

        if not in_block:
            continue

        if "./chrono-C-all/tests/bench_coupled_constraint" in line:
            header_idx = idx
            omega_start = idx + 1
            continue

        if header_idx is not None and omega_indent is None and line.strip().startswith(
            "--omega "
        ):
            omega_indent = line[: len(line) - len(line.lstrip())]
            break

    if header_idx is None:
        raise RuntimeError(f"Could not locate bench command block in {path}")

    if omega_indent is None:
        # Default to two spaces deeper than the command header (keeps code block indent tidy).
        header_line = lines[header_idx]
        header_indent = header_line[: len(header_line) - len(header_line.lstrip())]
        omega_indent = f"{header_indent}  "

    # Remove existing omega lines
    idx = omega_start
    while idx < len(lines) and lines[idx].strip().startswith("--omega "):
        del lines[idx]

    insertion = [
        f"{omega_indent}--omega {format_omega(value)} \\" for value in omegas
    ]
    for offset, new_line in enumerate(insertion):
        lines.insert(omega_start + offset, new_line)

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def update_preset_timestamp(path: Path, preset_id: str, timestamp: str) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    target_index = None
    indent = ""
    for idx, line in enumerate(lines):
        if line.strip().startswith("- id:") and preset_id in line:
            target_index = idx
            indent = line[: line.index("-")]
            break
    if target_index is None:
        raise RuntimeError(f"Preset '{preset_id}' not found in {path}")

    inserted = False
    scan_idx = target_index + 1
    while scan_idx < len(lines) and not lines[scan_idx].lstrip().startswith("notes:"):
        if lines[scan_idx].lstrip().startswith("last_updated:"):
            lines[scan_idx] = f"{indent}  last_updated: {timestamp}"
            inserted = True
            break
        scan_idx += 1

    if not inserted:
        if scan_idx == len(lines):
            raise RuntimeError(f"Preset '{preset_id}' is missing a notes block")
        lines.insert(scan_idx, f"{indent}  last_updated: {timestamp}")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def refresh_report() -> None:
    subprocess.run(["python3", str(COMPARISON_SCRIPT)], check=True, cwd=REPO_ROOT)


def main() -> None:
    args = parse_args()
    timestamp = args.timestamp or dt.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")
    run_bench(args)
    update_command_block(README_PATH, args.omegas)
    update_last_updated_line(README_PATH, timestamp)
    update_last_updated_line(HANDSON_PATH, timestamp)
    update_preset_timestamp(PRESET_PATH, args.preset_id, timestamp)
    if args.refresh_report:
        refresh_report()
    print(f"Multi-ω assets updated at {timestamp}")


if __name__ == "__main__":
    main()
