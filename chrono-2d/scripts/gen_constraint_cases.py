#!/usr/bin/env python3
import argparse
import csv
import json
import sys
from pathlib import Path


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))

def write_json(path: Path, data):
    path.write_text(json.dumps(data, indent=2), encoding="utf-8")

def parse_scale_list(raw: str):
    scales = []
    for part in raw.split(","):
        part = part.strip()
        if not part:
            continue
        scales.append(float(part))
    return scales

def scale_anchor_list(anchor, scale):
    return [anchor[0] * scale, anchor[1] * scale]

def generate_constraint_sweep(data, scales):
    sweep = {}
    for key, cases in data.items():
        sweep_cases = []
        for case in cases:
            for scale in scales:
                new_case = dict(case)
                name = new_case.get("name", "case")
                new_case["name"] = f"{name}_s{scale:g}"
                if "anchor_a" in new_case:
                    new_case["anchor_a"] = scale_anchor_list(new_case["anchor_a"], scale)
                if "anchor_b" in new_case:
                    new_case["anchor_b"] = scale_anchor_list(new_case["anchor_b"], scale)
                sweep_cases.append(new_case)
        sweep[key] = sweep_cases
    return sweep

def load_contact_csv(path: Path):
    rows = []
    with path.open(newline="", encoding="utf-8") as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            rows.append(row)
    return rows

def write_contact_csv(path: Path, rows, fieldnames):
    with path.open("w", newline="", encoding="utf-8") as fp:
        writer = csv.DictWriter(fp, fieldnames=fieldnames)
        writer.writeheader()
        for row in rows:
            writer.writerow(row)

def generate_contact_sweep(rows, scales):
    sweep_rows = []
    for row in rows:
        sweep_rows.append(row)
        for scale in scales:
            new_row = dict(row)
            new_row["name"] = f"{row['name']}_s{scale:g}"
            new_row["vn"] = f"{float(row['vn']) * scale:.6f}"
            new_row["vt"] = f"{float(row['vt']) * scale:.6f}"
            sweep_rows.append(new_row)
    return sweep_rows


def main():
    parser = argparse.ArgumentParser(description="Generate constraint/contact sweeps for chrono-2d datasets.")
    parser.add_argument("--output-dir", default=None, help="directory to write generated datasets")
    parser.add_argument("--sweep-scales", default="0.5,1.0,2.0", help="comma-separated scale factors")
    parser.add_argument("--emit-constraint-sweep", action="store_true", help="write scaled constraint JSON")
    parser.add_argument("--emit-contact-sweep", action="store_true", help="write scaled contact CSV")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    constraints_json = root / "data" / "cases_constraints.json"
    contact_csv = root / "data" / "cases_contact_extended.csv"
    print(f"Constraint cases: {constraints_json}")
    print(f"Contact cases:    {contact_csv}")
    if not constraints_json.exists() or not contact_csv.exists():
        print("Missing dataset files", file=sys.stderr)
        sys.exit(1)

    data = load_json(constraints_json)
    contacts = load_contact_csv(contact_csv)
    print(f"Loaded {sum(len(v) for v in data.values())} constraint entries")
    print(f"Loaded {len(contacts)} contact entries")

    if not args.output_dir:
        return 0

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    scales = parse_scale_list(args.sweep_scales)

    if args.emit_constraint_sweep:
        sweep = generate_constraint_sweep(data, scales)
        out_path = output_dir / "cases_constraints_sweep.json"
        write_json(out_path, sweep)
        print(f"Wrote constraint sweep: {out_path}")

    if args.emit_contact_sweep:
        sweep_rows = generate_contact_sweep(contacts, scales)
        out_path = output_dir / "cases_contact_sweep.csv"
        fieldnames = list(contacts[0].keys()) if contacts else ["name", "vn", "vt", "mu_s", "mu_d", "stick"]
        write_contact_csv(out_path, sweep_rows, fieldnames)
        print(f"Wrote contact sweep: {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
