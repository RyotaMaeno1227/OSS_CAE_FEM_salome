# parser_multipart_manifest_v0


## Scope and release-surface status

This document is internal parser/schema support material for the `assembly_manifest_v0` workflow. It is not a user-facing release artifact.

Under the A2 release-surface policy, the parser/manifest bridge flow and helper tooling such as `build_mbd_master_from_manifest.py` are internal-only unless they are explicitly promoted by a later release decision. The final release surface should remain allowlist-based and should not include internal scripts, bridge helpers, generated bundles, golden/fallback artifacts, or PM/continuity materials by default.

## Purpose

`assembly_manifest_v0` is the human-readable assembly-level input format for text-based MBD / coupled preparation.

It is designed to sit **between**:

- multi-part parser export
- current FEM4C text directives (`MBD_*`, `COUPLED_FLEX_*`)

The manifest is the **authoritative assembly description**. Solver input files may be generated from it.

## Non-goals

- direct GUI authoring
- native coupled solver core redesign
- native binary exchange with NX Motion / Adams

## Schema

### top-level required fields

- `schema_version`
- `assembly_name`
- `units`
- `parts`
- `bodies`
- `gravity`
- `forces`
- `joints`
- `coupled_flex`

### `schema_version`
Required string.

Current value:

```json
"assembly_manifest_v0"
```

### `assembly_name`
Required string.

### `source_bdf`
Optional string. Source CAD / FE file path for provenance only.

### `units`
Required object.

Required keys:

- `length`
- `mass`
- `time`
- `force`

### `parts[]`
Required array.

Each entry describes one parser-exported part package.

Required keys per item:

- `part_id`
- `parser_package_dir`
- `mesh_file`
- `material_file`
- `boundary_file`

Optional keys per item:

- `surface_file`
- `ridgeline_file`
- `volume_file`
- `inertia_file`

### `bodies[]`
Required array.

Each entry maps one assembly body to one exported part.

Required keys per item:

- `body_id` (integer)
- `part_id` (string, must exist in `parts[]`)
- `mass` (number)
- `inertia` (number)
- `q` = `[x, y, theta]`
- `v` = `[vx, vy, omega]`

### `gravity`
Required object.

Required keys:

- `gx`
- `gy`

### `forces[]`
Required array. May be empty.

Required keys per item:

- `body_id`
- `fx`
- `fy`
- `tau`

### `joints[]`
Required array. May be empty.

Required keys common to all joint types:

- `joint_id`
- `type`
- `body_i`
- `body_j`
- `ai` = `[x, y]`
- `aj` = `[x, y]`

Supported `type` values in v0:

- `revolute`
- `distance`

Additional required field for `distance`:

- `distance`

### `coupled_flex[]`
Required array. May be empty.

Each item binds a body to a flexible FE file and its interface sets.

Required keys:

- `body_id`
- `flex_dat_path`
- `root_set`
- `tip_set`

`root_set` and `tip_set` are integer node-id arrays.

## Mapping to current FEM4C directives

### body

```text
MBD_BODY_DYN <body_id> <mass> <inertia> <x> <y> <theta> <vx> <vy> <omega>
```

### gravity

```text
MBD_GRAVITY <gx> <gy>
```

### force

```text
MBD_FORCE <body_id> <fx> <fy> <tau>
```

### revolute joint

```text
MBD_REVOLUTE <joint_id> <body_i> <body_j> <ai_x> <ai_y> <aj_x> <aj_y>
```

### distance joint

```text
MBD_DISTANCE <joint_id> <body_i> <body_j> <ai_x> <ai_y> <aj_x> <aj_y> <distance>
```

### flexible body binding

```text
COUPLED_FLEX_BODY <body_id> <flex_dat_path>
COUPLED_FLEX_ROOT_SET <body_id> <node_count> <node_1> ... <node_n>
COUPLED_FLEX_TIP_SET  <body_id> <node_count> <node_1> ... <node_n>
```

## Validation rules

- `schema_version` must be `assembly_manifest_v0`
- every `part_id` in `bodies[]` must exist in `parts[]`
- every `body_id` referenced by `forces[]`, `joints[]`, `coupled_flex[]` must exist in `bodies[]`
- `q` and `v` must each contain exactly 3 numeric values
- `ai` and `aj` must each contain exactly 2 numeric values
- `root_set` and `tip_set` must be non-empty integer arrays
- `distance` joints must define positive `distance`

## Initial workflow

1. export one or more parts from parser into per-part package directories
2. author `assembly_manifest_v0.json`
3. run `build_mbd_master_from_manifest.py`
4. feed generated text into current FEM4C MBD workflow

## Example

See:

- `examples/assembly_manifest_2link_v0.json`
