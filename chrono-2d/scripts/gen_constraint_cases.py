#!/usr/bin/env python3
import json
import sys
from pathlib import Path


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def main():
    root = Path(__file__).resolve().parents[1]
    constraints_json = root / "data" / "cases_constraints.json"
    contact_csv = root / "data" / "cases_contact_extended.csv"
    print(f"Constraint cases: {constraints_json}")
    print(f"Contact cases:    {contact_csv}")
    if not constraints_json.exists() or not contact_csv.exists():
        print("Missing dataset files", file=sys.stderr)
        sys.exit(1)

    data = load_json(constraints_json)
    contacts = contact_csv.read_text(encoding="utf-8").splitlines()
    print(f"Loaded {sum(len(v) for v in data.values())} constraint entries")
    print(f"Loaded {len(contacts)-1} contact entries")  # minus header


if __name__ == "__main__":
    main()
