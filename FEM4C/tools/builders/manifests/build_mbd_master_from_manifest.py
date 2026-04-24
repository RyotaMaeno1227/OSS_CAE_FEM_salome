#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any, Dict, List, Tuple

SCHEMA_VERSION = "assembly_manifest_v0"
BODY_PROPERTIES_SCHEMA = "body_properties_v0"


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def _load_json(path: Path) -> Dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    _require(isinstance(data, dict), f"{path}: JSON root must be an object")
    return data


def _resolve_from_manifest(manifest_path: Path, rel: str) -> Path:
    p = Path(rel)
    if p.is_absolute():
        return p
    return (manifest_path.parent / p).resolve()


def _vector3_from_body(body: Dict[str, Any], q_key: str, alt_key: str, names: Tuple[str, str, str]) -> List[float]:
    if q_key in body:
        v = body[q_key]
        _require(isinstance(v, list) and len(v) == 3, f"body {body.get('body_id')}: {q_key} must be length-3 list")
        return [float(v[0]), float(v[1]), float(v[2])]
    if alt_key in body:
        v = body[alt_key]
        if isinstance(v, list):
            _require(len(v) == 3, f"body {body.get('body_id')}: {alt_key} list must be length 3")
            return [float(v[0]), float(v[1]), float(v[2])]
        _require(isinstance(v, dict), f"body {body.get('body_id')}: {alt_key} must be dict or length-3 list")
        return [float(v.get(names[0], 0.0)), float(v.get(names[1], 0.0)), float(v.get(names[2], 0.0))]
    return [0.0, 0.0, 0.0]


def _validate_manifest(manifest_path: Path, data: Dict[str, Any]) -> Dict[str, Dict[str, Any]]:
    _require(data.get("schema_version") == SCHEMA_VERSION,
             f"{manifest_path}: schema_version must be '{SCHEMA_VERSION}'")

    required_top = ["schema_version", "parts", "bodies", "gravity", "forces", "joints", "coupled_flex"]
    for key in required_top:
        _require(key in data, f"{manifest_path}: missing top-level key: {key}")

    parts = data["parts"]
    bodies = data["bodies"]
    forces = data["forces"]
    joints = data["joints"]
    coupled_flex = data["coupled_flex"]

    _require(isinstance(parts, list), "parts must be a list")
    _require(isinstance(bodies, list), "bodies must be a list")
    _require(isinstance(forces, list), "forces must be a list")
    _require(isinstance(joints, list), "joints must be a list")
    _require(isinstance(coupled_flex, list), "coupled_flex must be a list")

    part_map: Dict[str, Dict[str, Any]] = {}
    for part in parts:
        for key in ["part_id", "package_dir", "mesh_path", "material_path", "body_properties_path", "boundary_path"]:
            _require(key in part, f"part missing key: {key}")
        pid = str(part["part_id"])
        _require(pid not in part_map, f"duplicate part_id: {pid}")
        part_map[pid] = part

    body_ids = set()
    ground_body_ids: List[int] = []
    forbidden_manual_keys = {
        "mass", "mass_g", "mass_kg",
        "inertia", "inertia_g_mm2", "inertia_kg_m2", "iz_com_kg_m2",
        "density", "density_kg_per_m3", "volume", "volume_m3"
    }

    for body in bodies:
        for key in ["body_id", "part_id", "body_properties_source"]:
            _require(key in body, f"body missing key: {key}")
        bid = int(body["body_id"])
        _require(bid not in body_ids, f"duplicate body_id: {bid}")
        body_ids.add(bid)

        pid = str(body["part_id"])
        _require(pid in part_map, f"body references unknown part_id: {pid}")
        _require(body["body_properties_source"] == "parser_package",
                 f"body {bid}: body_properties_source must be 'parser_package'")

        forbidden = sorted(k for k in forbidden_manual_keys if k in body)
        _require(not forbidden,
                 f"body {bid}: manual body property keys are forbidden when parser_package is mandatory: {forbidden}")

        if "mass_kg_override" in body:
            _require(float(body["mass_kg_override"]) > 0.0,
                     f"body {bid}: mass_kg_override must be > 0")
        if "iz_com_kg_m2_override" in body:
            _require(float(body["iz_com_kg_m2_override"]) > 0.0,
                     f"body {bid}: iz_com_kg_m2_override must be > 0")

        if "is_ground" in body:
            _require(isinstance(body["is_ground"], bool),
                     f"body {bid}: is_ground must be bool when provided")
            if body["is_ground"]:
                ground_body_ids.append(bid)

        _vector3_from_body(body, "q", "initial_pose", ("qx", "qy", "qtheta"))
        _vector3_from_body(body, "v", "initial_velocity", ("vx", "vy", "omega"))

    _require(len(ground_body_ids) <= 1,
             f"manifest may define at most one ground body, got {sorted(ground_body_ids)}")

    gravity = data["gravity"]
    for key in ["gx", "gy"]:
        _require(key in gravity, f"gravity missing key: {key}")

    for force in forces:
        for key in ["body_id", "fx", "fy", "tau"]:
            _require(key in force, f"force missing key: {key}")
        _require(int(force["body_id"]) in body_ids, f"force references unknown body_id: {force['body_id']}")

    for joint in joints:
        for key in ["joint_id", "type", "body_i", "body_j", "ai", "aj"]:
            _require(key in joint, f"joint missing key: {key}")
        _require(int(joint["body_i"]) in body_ids, f"joint references unknown body_i: {joint['body_i']}")
        _require(int(joint["body_j"]) in body_ids, f"joint references unknown body_j: {joint['body_j']}")
        _require(isinstance(joint["ai"], list) and len(joint["ai"]) == 2, f"joint {joint['joint_id']}: ai must have length 2")
        _require(isinstance(joint["aj"], list) and len(joint["aj"]) == 2, f"joint {joint['joint_id']}: aj must have length 2")
        _require(joint["type"] in {"revolute", "distance"},
                 f"unsupported joint type: {joint['type']}")
        if joint["type"] == "distance":
            _require("distance" in joint, f"distance joint {joint['joint_id']} missing distance")
            _require(float(joint["distance"]) > 0.0, f"distance joint {joint['joint_id']} must be positive")

    for item in coupled_flex:
        for key in ["body_id", "flex_dat_path", "root_set", "tip_set"]:
            _require(key in item, f"coupled_flex missing key: {key}")
        _require(int(item["body_id"]) in body_ids, f"coupled_flex references unknown body_id: {item['body_id']}")
        _require(isinstance(item["root_set"], list) and len(item["root_set"]) > 0,
                 f"coupled_flex body {item['body_id']}: empty root_set")
        _require(isinstance(item["tip_set"], list) and len(item["tip_set"]) > 0,
                 f"coupled_flex body {item['body_id']}: empty tip_set")

    return part_map


def _load_body_properties(manifest_path: Path, part: Dict[str, Any]) -> Tuple[Path, Dict[str, Any]]:
    body_path = _resolve_from_manifest(manifest_path, str(part["body_properties_path"]))
    _require(body_path.is_file(), f"missing body_properties.json: {body_path}")

    data = _load_json(body_path)
    _require(data.get("schema_version") == BODY_PROPERTIES_SCHEMA,
             f"{body_path}: schema_version must be '{BODY_PROPERTIES_SCHEMA}'")

    required = ["mass_kg", "iz_com_kg_m2", "inertia_reference", "part_id"]
    for key in required:
        _require(key in data, f"{body_path}: missing key: {key}")

    mass = float(data["mass_kg"])
    inertia = float(data["iz_com_kg_m2"])
    _require(mass > 0.0, f"{body_path}: mass_kg must be > 0")
    _require(inertia > 0.0, f"{body_path}: iz_com_kg_m2 must be > 0")
    _require(str(data["inertia_reference"]) == "center_of_mass",
             f"{body_path}: inertia_reference must be 'center_of_mass'")

    return body_path, data


def _fmt(x: Any) -> str:
    return f"{float(x):.16g}"


def _emit_lines(manifest_path: Path, data: Dict[str, Any], part_map: Dict[str, Dict[str, Any]]) -> List[str]:
    lines: List[str] = []
    lines.append(f"# generated from {SCHEMA_VERSION}")
    lines.append(f"# manifest: {manifest_path}")

    for body in sorted(data["bodies"], key=lambda x: int(x["body_id"])):
        bid = int(body["body_id"])
        pid = str(body["part_id"])
        part = part_map[pid]
        body_prop_path, body_prop = _load_body_properties(manifest_path, part)
        mass_value = float(body.get("mass_kg_override", body_prop["mass_kg"]))
        inertia_value = float(body.get("iz_com_kg_m2_override", body_prop["iz_com_kg_m2"]))

        q = _vector3_from_body(body, "q", "initial_pose", ("qx", "qy", "qtheta"))
        v = _vector3_from_body(body, "v", "initial_velocity", ("vx", "vy", "omega"))

        lines.append(f"# body_id={bid} part_id={pid} body_properties={body_prop_path}")
        lines.append(
            "MBD_BODY_DYN "
            f"{bid} {_fmt(mass_value)} {_fmt(inertia_value)} "
            f"{_fmt(q[0])} {_fmt(q[1])} {_fmt(q[2])} "
            f"{_fmt(v[0])} {_fmt(v[1])} {_fmt(v[2])}"
        )
        if bool(body.get("is_ground", False)):
            lines.append(f"MBD_BODY_GROUND {bid}")

    g = data["gravity"]
    lines.append(f"MBD_GRAVITY {_fmt(g['gx'])} {_fmt(g['gy'])}")

    for force in sorted(data["forces"], key=lambda x: int(x["body_id"])):
        lines.append(
            "MBD_FORCE "
            f"{int(force['body_id'])} {_fmt(force['fx'])} {_fmt(force['fy'])} {_fmt(force['tau'])}"
        )

    for joint in sorted(data["joints"], key=lambda x: int(x["joint_id"])):
        ai = joint["ai"]
        aj = joint["aj"]
        if joint["type"] == "revolute":
            lines.append(
                "MBD_REVOLUTE "
                f"{int(joint['joint_id'])} {int(joint['body_i'])} {int(joint['body_j'])} "
                f"{_fmt(ai[0])} {_fmt(ai[1])} {_fmt(aj[0])} {_fmt(aj[1])}"
            )
        else:
            lines.append(
                "MBD_DISTANCE "
                f"{int(joint['joint_id'])} {int(joint['body_i'])} {int(joint['body_j'])} "
                f"{_fmt(ai[0])} {_fmt(ai[1])} {_fmt(aj[0])} {_fmt(aj[1])} {_fmt(joint['distance'])}"
            )

    for item in sorted(data["coupled_flex"], key=lambda x: int(x["body_id"])):
        root_set = " ".join(str(int(v)) for v in item["root_set"])
        tip_set = " ".join(str(int(v)) for v in item["tip_set"])
        lines.append(f"COUPLED_FLEX_BODY {int(item['body_id'])} {item['flex_dat_path']}")
        lines.append(f"COUPLED_FLEX_ROOT_SET {int(item['body_id'])} {len(item['root_set'])} {root_set}")
        lines.append(f"COUPLED_FLEX_TIP_SET {int(item['body_id'])} {len(item['tip_set'])} {tip_set}")

    return lines


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate FEM4C MBD directives from assembly_manifest_v0 using parser body_properties.json")
    ap.add_argument("manifest", type=Path, help="input assembly_manifest_v0.json")
    ap.add_argument("output", type=Path, help="output .dat path")
    args = ap.parse_args()

    data = _load_json(args.manifest)
    part_map = _validate_manifest(args.manifest, data)
    lines = _emit_lines(args.manifest, data, part_map)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(str(args.output))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
