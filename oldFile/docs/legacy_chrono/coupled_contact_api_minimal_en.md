# Coupled & Contact Core API (Minimal Set, English)

This note mirrors `docs/coupled_contact_api_minimal.md` but is written in English so mixed teams can reference the same API surface. Only the bare minimum required to drive the core solver is listed here; optional helpers (logging, media generation, notifications, etc.) were removed from the repository and should be implemented per team environment if needed outside this repo.

## Phase-by-Phase API Overview

### Phase 1 – Initialisation

| Component | Function | Purpose | Reference |
|-----------|----------|---------|-----------|
| Coupled constraint | `chrono_coupled_constraint2d_init` | Register anchors, axes, ratios, spring/damper settings, initialise the equation buffer | `chrono-C-all/include/chrono_constraint2d.h:540` |
| Coupled constraint | `chrono_coupled_constraint2d_set_equation` / `chrono_coupled_constraint2d_add_equation` | Add or update up to 4 simultaneous equations | `chrono-C-all/include/chrono_constraint2d.h:570`, `:572` |
| Contact manifold | `chrono_contact_manifold2d_init/reset/set_bodies` | Prepare manifolds and bind bodies | `chrono-C-all/include/chrono_collision2d.h:39-45` |
| Contact manager | `chrono_contact_manager2d_begin_step` | Clear manifold state at the beginning of a step | `chrono-C-all/include/chrono_collision2d.h:69` |
| Island solver | `chrono_island2d_workspace_init/reset` | Allocate and reuse workspace memory for islands | `chrono-C-all/include/chrono_island2d.h:69-71` |

### Phase 2 – Solve

| Component | Function | Purpose | Reference |
|-----------|----------|---------|-----------|
| Coupled constraint | `chrono_coupled_constraint2d_prepare` | Update the local KKT block and bias terms with the current `dt` | `chrono-C-all/include/chrono_constraint2d.h:592` |
| Coupled constraint | `chrono_coupled_constraint2d_apply_warm_start` | Apply impulses from the previous frame | `...:593` |
| Coupled constraint | `chrono_coupled_constraint2d_solve_velocity` / `solve_position` | Velocity corrections and Baumgarte drift removal | `...:594-595` |
| Contact | `chrono_collision2d_detect_*` | Narrow-phase detection for circle/polygon/capsule/edge pairs | `chrono-C-all/include/chrono_collision2d.h:83-159` |
| Contact | `chrono_collision2d_resolve_*` | Build contact manifolds and compute constraint terms | same |
| Contact | `chrono_contact_manager2d_update_contact` | Persist detection results inside the manager | `chrono-C-all/include/chrono_collision2d.h:78-81` |
| Contact Jacobian | `chrono_contact2d_build_jacobian_3dof` | Emit normal/rolling/torsional rows for Coupled + Contact mixing | `chrono-C-all/include/chrono_collision2d.h:41-64` |
| Island solver | `chrono_island2d_build` | Build connected components (islands) from constraints and contacts | `chrono-C-all/include/chrono_island2d.h:73-77` |
| Island solver | `chrono_island2d_solve` | Execute per-island solves using `constraint_config` | `...:79-81` |
| Island solver | `chrono_island2d_workspace_free` | Release the workspace during shutdown | `...:70` |

### Phase 3 – Diagnostics & Monitoring

| Component | Function | Purpose | Reference |
|-----------|----------|---------|-----------|
| Coupled constraint | `chrono_coupled_constraint2d_get_diagnostics` | Expose rank, condition numbers (row-sum + spectral), min/max pivot/eigen values | `chrono-C-all/include/chrono_constraint2d.h:577` |
| Coupled constraint | `chrono_coupled_constraint2d_get_condition_warning_policy` / `set_condition_warning_policy` | Configure automatic dropping, cooldown logging, callbacks | `...:578`, `:589` |
| Contact manager | `chrono_contact_manager2d_end_step` | Finalise manifolds and recycle per-step buffers | `chrono-C-all/include/chrono_collision2d.h:70` |
| Island solver | `ChronoIsland2DSolveConfig_C` (`enable_parallel`, `constraint_config`) | Track iteration counts and parallel settings for reporting | `chrono-C-all/include/chrono_island2d.h:65` |

## Non-core API

Anything that does not affect numerical correctness (log handlers, custom notifications, media export) has been removed from this repository. When proposing new APIs, first decide whether they belong in this minimal list or should remain private to each environment.
