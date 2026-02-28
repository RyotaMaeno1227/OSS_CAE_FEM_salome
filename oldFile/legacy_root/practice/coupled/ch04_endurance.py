#!/usr/bin/env python3
"""Stub script that will wrap tools/plot_coupled_constraint_endurance.py for Hands-on Chapter 04."""

from pathlib import Path
import subprocess

DATA = Path("data/coupled_constraint_endurance.csv")
SUMMARY = Path("data/latest.endurance.json")


def run_summary() -> None:
    if not DATA.exists():
        raise SystemExit(f"missing endurance CSV: {DATA}")
    cmd = [
        "python",
        "tools/plot_coupled_constraint_endurance.py",
        str(DATA),
        "--skip-plot",
        "--summary-json",
        str(SUMMARY),
        "--no-show",
    ]
    print("[ch04_endurance] running", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main() -> None:
    run_summary()
    print("[ch04_endurance] summary JSON ready ->", SUMMARY)
    print("Attach results to docs/coupled_contact_test_notes.md when sharing evidence.")


if __name__ == "__main__":
    main()
