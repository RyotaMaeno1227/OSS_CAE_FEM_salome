#include "../include/chrono_constraint2d.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

static double length(const double v[2]) {
    return sqrt(v[0] * v[0] + v[1] * v[1]);
}

static double dot(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

static double cross(const double a[2], const double b[2]) {
    return a[0] * b[1] - a[1] * b[0];
}

static void scale(const double v[2], double s, double out[2]) {
    out[0] = v[0] * s;
    out[1] = v[1] * s;
}

static void add(const double a[2], const double b[2], double out[2]) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
}

static void sub(const double a[2], const double b[2], double out[2]) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
}

static void normalize(double v[2]) {
    double len = length(v);
    if (len > 1e-12) {
        v[0] /= len;
        v[1] /= len;
    }
}

static void rotate_angle(double angle, const double v[2], double out[2]) {
    double c = cos(angle);
    double s = sin(angle);
    out[0] = c * v[0] - s * v[1];
    out[1] = s * v[0] + c * v[1];
}

static int invert2x2(const double *m, double *out) {
    double det = m[0] * m[3] - m[1] * m[2];
    if (fabs(det) < 1e-12) {
        return 0;
    }
    double inv_det = 1.0 / det;
    out[0] = m[3] * inv_det;
    out[1] = -m[1] * inv_det;
    out[2] = -m[2] * inv_det;
    out[3] = m[0] * inv_det;
    return 1;
}

static void chrono_distance_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_distance_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_distance_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_distance_constraint2d_solve_position_impl(void *constraint_ptr);
static void chrono_distance_angle_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_distance_angle_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_distance_angle_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_distance_angle_constraint2d_solve_position_impl(void *constraint_ptr);
static void chrono_coupled_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_coupled_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_coupled_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_coupled_constraint2d_solve_position_impl(void *constraint_ptr);

static void chrono_revolute_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_revolute_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_revolute_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_revolute_constraint2d_solve_position_impl(void *constraint_ptr);

static void chrono_prismatic_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_prismatic_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_prismatic_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_prismatic_constraint2d_solve_position_impl(void *constraint_ptr);

static void chrono_spring_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_spring_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_spring_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_spring_constraint2d_solve_position_impl(void *constraint_ptr);

static void chrono_planar_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_planar_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_planar_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_planar_constraint2d_solve_position_impl(void *constraint_ptr);

static void chrono_gear_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_gear_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_gear_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_gear_constraint2d_solve_position_impl(void *constraint_ptr);

static const ChronoConstraint2DOps_C chrono_distance_constraint2d_ops = {
    chrono_distance_constraint2d_prepare_impl,
    chrono_distance_constraint2d_apply_warm_start_impl,
    chrono_distance_constraint2d_solve_velocity_impl,
    chrono_distance_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_distance_angle_constraint2d_ops = {
    chrono_distance_angle_constraint2d_prepare_impl,
    chrono_distance_angle_constraint2d_apply_warm_start_impl,
    chrono_distance_angle_constraint2d_solve_velocity_impl,
    chrono_distance_angle_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_coupled_constraint2d_ops = {
    chrono_coupled_constraint2d_prepare_impl,
    chrono_coupled_constraint2d_apply_warm_start_impl,
    chrono_coupled_constraint2d_solve_velocity_impl,
    chrono_coupled_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_revolute_constraint2d_ops = {
    chrono_revolute_constraint2d_prepare_impl,
    chrono_revolute_constraint2d_apply_warm_start_impl,
    chrono_revolute_constraint2d_solve_velocity_impl,
    chrono_revolute_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_prismatic_constraint2d_ops = {
    chrono_prismatic_constraint2d_prepare_impl,
    chrono_prismatic_constraint2d_apply_warm_start_impl,
    chrono_prismatic_constraint2d_solve_velocity_impl,
    chrono_prismatic_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_spring_constraint2d_ops = {
    chrono_spring_constraint2d_prepare_impl,
    chrono_spring_constraint2d_apply_warm_start_impl,
    chrono_spring_constraint2d_solve_velocity_impl,
    chrono_spring_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_planar_constraint2d_ops = {
    chrono_planar_constraint2d_prepare_impl,
    chrono_planar_constraint2d_apply_warm_start_impl,
    chrono_planar_constraint2d_solve_velocity_impl,
    chrono_planar_constraint2d_solve_position_impl
};

static const ChronoConstraint2DOps_C chrono_gear_constraint2d_ops = {
    chrono_gear_constraint2d_prepare_impl,
    chrono_gear_constraint2d_apply_warm_start_impl,
    chrono_gear_constraint2d_solve_velocity_impl,
    chrono_gear_constraint2d_solve_position_impl
};

static void apply_impulse(ChronoBody2D_C *body, const double impulse[2], const double r[2]) {
    if (!body || body->is_static) {
        return;
    }
    body->linear_velocity[0] += impulse[0] * body->inverse_mass;
    body->linear_velocity[1] += impulse[1] * body->inverse_mass;
    double angular_impulse = cross(r, impulse);
    body->angular_velocity += angular_impulse * body->inverse_inertia;
}

typedef struct ChronoConstraintBodyMapEntry {
    ChronoBody2D_C *key;
    int value;
} ChronoConstraintBodyMapEntry;

void chrono_constraint2d_workspace_init(ChronoConstraint2DBatchWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    memset(workspace, 0, sizeof(*workspace));
}

void chrono_constraint2d_workspace_reset(ChronoConstraint2DBatchWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    for (size_t i = 0; i < workspace->island_ids_capacity; ++i) {
        workspace->island_ids[i] = 0;
    }
    for (size_t i = 0; i < workspace->island_sizes_capacity; ++i) {
        workspace->island_sizes[i] = 0;
    }
    for (size_t i = 0; i < workspace->island_offsets_capacity; ++i) {
        workspace->island_offsets[i] = 0;
    }
    for (size_t i = 0; i < workspace->ordered_indices_capacity; ++i) {
        workspace->ordered_indices[i] = 0;
    }
    for (size_t i = 0; i < workspace->constraint_buffer_capacity; ++i) {
        workspace->constraint_buffer[i] = NULL;
    }
}

void chrono_constraint2d_workspace_free(ChronoConstraint2DBatchWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    free(workspace->island_ids);
    free(workspace->island_sizes);
    free(workspace->island_offsets);
    free(workspace->ordered_indices);
    free(workspace->constraint_buffer);
    chrono_constraint2d_workspace_init(workspace);
}

static size_t chrono_constraint2d_hash_body(const ChronoBody2D_C *body) {
    uintptr_t ptr = (uintptr_t)body;
    return (ptr >> 4) ^ (ptr >> 9);
}

static int chrono_constraint2d_find_root(int *parent, int idx) {
    while (parent[idx] != idx) {
        parent[idx] = parent[parent[idx]];
        idx = parent[idx];
    }
    return idx;
}

static void chrono_constraint2d_union(int *parent, int *rank, int a, int b) {
    if (a < 0 || b < 0) {
        return;
    }
    int root_a = chrono_constraint2d_find_root(parent, a);
    int root_b = chrono_constraint2d_find_root(parent, b);
    if (root_a == root_b) {
        return;
    }
    if (rank[root_a] < rank[root_b]) {
        parent[root_a] = root_b;
    } else if (rank[root_a] > rank[root_b]) {
        parent[root_b] = root_a;
    } else {
        parent[root_b] = root_a;
        rank[root_a] += 1;
    }
}

static int chrono_constraint2d_body_lookup_or_add(ChronoConstraintBodyMapEntry *map,
                                                  size_t capacity_mask,
                                                  ChronoBody2D_C **body_nodes,
                                                  int *parent,
                                                  int *rank,
                                                  size_t *unique_bodies,
                                                  ChronoBody2D_C *body) {
    if (!body) {
        return -1;
    }
    size_t mask = capacity_mask;
    size_t idx = chrono_constraint2d_hash_body(body) & mask;
    while (1) {
        ChronoConstraintBodyMapEntry *entry = &map[idx];
        if (!entry->key) {
            int new_index = (int)(*unique_bodies);
            entry->key = body;
            entry->value = new_index;
            body_nodes[*unique_bodies] = body;
            parent[*unique_bodies] = new_index;
            rank[*unique_bodies] = 0;
            (*unique_bodies)++;
            return new_index;
        }
        if (entry->key == body) {
            return entry->value;
        }
        idx = (idx + 1) & mask;
    }
}

size_t chrono_constraint2d_build_islands(ChronoConstraint2DBase_C **constraints,
                                         size_t count,
                                         int *island_ids) {
    if (!constraints || count == 0) {
        return 0;
    }

    size_t max_bodies = count * 2;
    if (max_bodies == 0) {
        return 0;
    }

    size_t map_capacity = 1;
    while (map_capacity < max_bodies * 2) {
        map_capacity <<= 1;
    }

    ChronoConstraintBodyMapEntry *map = (ChronoConstraintBodyMapEntry *)calloc(map_capacity, sizeof(ChronoConstraintBodyMapEntry));
    ChronoBody2D_C **body_nodes = (ChronoBody2D_C **)malloc(max_bodies * sizeof(ChronoBody2D_C *));
    int *parent = (int *)malloc(max_bodies * sizeof(int));
    int *rank = (int *)malloc(max_bodies * sizeof(int));
    int *constraint_body_indices = (int *)malloc(count * 2 * sizeof(int));

    if (!map || !body_nodes || !parent || !rank || !constraint_body_indices) {
        free(map);
        free(body_nodes);
        free(parent);
        free(rank);
        free(constraint_body_indices);
        return 0;
    }

    size_t unique_bodies = 0;

    for (size_t i = 0; i < count; ++i) {
        ChronoConstraint2DBase_C *constraint = constraints[i];
        ChronoBody2D_C *body_a = constraint ? constraint->body_a : NULL;
        ChronoBody2D_C *body_b = constraint ? constraint->body_b : NULL;

        int idx_a = chrono_constraint2d_body_lookup_or_add(map,
                                                           map_capacity - 1,
                                                           body_nodes,
                                                           parent,
                                                           rank,
                                                           &unique_bodies,
                                                           body_a);
        int idx_b = chrono_constraint2d_body_lookup_or_add(map,
                                                           map_capacity - 1,
                                                           body_nodes,
                                                           parent,
                                                           rank,
                                                           &unique_bodies,
                                                           body_b);

        constraint_body_indices[2 * i] = idx_a;
        constraint_body_indices[2 * i + 1] = idx_b;

        if (idx_a >= 0 && idx_b >= 0) {
            chrono_constraint2d_union(parent, rank, idx_a, idx_b);
        }
    }

    int *root_to_island = NULL;
    if (unique_bodies > 0) {
        root_to_island = (int *)malloc(unique_bodies * sizeof(int));
        if (root_to_island) {
            for (size_t i = 0; i < unique_bodies; ++i) {
                root_to_island[i] = -1;
            }
        }
    }

    size_t island_count = 0;
    for (size_t i = 0; i < count; ++i) {
        int idx_a = constraint_body_indices[2 * i];
        int idx_b = constraint_body_indices[2 * i + 1];
        int first_idx = idx_a >= 0 ? idx_a : idx_b;
        int island_id = -1;
        if (first_idx >= 0 && root_to_island) {
            int root = chrono_constraint2d_find_root(parent, first_idx);
            if (root_to_island[root] < 0) {
                root_to_island[root] = (int)island_count;
                island_count++;
            }
            island_id = root_to_island[root];
        } else {
            island_id = (int)island_count;
            island_count++;
        }
        if (island_ids) {
            island_ids[i] = island_id;
        }
    }

    free(root_to_island);
    free(map);
    free(body_nodes);
    free(parent);
    free(rank);
    free(constraint_body_indices);

    return island_count;
}

void chrono_distance_constraint2d_init(ChronoDistanceConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2],
                                       double rest_length) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_distance_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    constraint->base.accumulated_impulse = 0.0;
    constraint->base.effective_mass = 0.0;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    constraint->rest_length = rest_length;
    constraint->baumgarte_beta = 0.2;
    constraint->softness_linear = 0.0;
    constraint->softness_angular = 0.0;
    constraint->slop = 0.001;
    constraint->max_correction = 0.2;
    constraint->bias = 0.0;
    constraint->spring_stiffness = 0.0;
    constraint->spring_damping = 0.0;
    constraint->spring_deflection = 0.0;
    constraint->cached_dt = 0.0;
    constraint->last_spring_force = 0.0;
    constraint->last_impulse = 0.0;
    constraint->accumulated_penetration = 0.0;
}

void chrono_distance_constraint2d_set_baumgarte(ChronoDistanceConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_distance_constraint2d_set_softness(ChronoDistanceConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness_linear = softness;
    constraint->softness_angular = softness;
}

void chrono_distance_constraint2d_set_softness_linear(ChronoDistanceConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness_linear = softness;
}

void chrono_distance_constraint2d_set_softness_angular(ChronoDistanceConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness_angular = softness;
}

void chrono_distance_constraint2d_set_slop(ChronoDistanceConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_distance_constraint2d_set_max_correction(ChronoDistanceConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

void chrono_distance_constraint2d_set_spring(ChronoDistanceConstraint2D_C *constraint,
                                             double stiffness,
                                             double damping) {
    if (!constraint) {
        return;
    }
    constraint->spring_stiffness = (stiffness > 0.0) ? stiffness : 0.0;
    constraint->spring_damping = (damping > 0.0) ? damping : 0.0;
}

void chrono_distance_angle_constraint2d_init(ChronoDistanceAngleConstraint2D_C *constraint,
                                             ChronoBody2D_C *body_a,
                                             ChronoBody2D_C *body_b,
                                             const double local_anchor_a[2],
                                             const double local_anchor_b[2],
                                             double rest_distance,
                                             double rest_angle,
                                             const double axis_local[2]) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_distance_angle_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    constraint->rest_distance = rest_distance;
    constraint->rest_angle = rest_angle;
    if (axis_local) {
        constraint->axis_local[0] = axis_local[0];
        constraint->axis_local[1] = axis_local[1];
    } else {
        constraint->axis_local[0] = 1.0;
        constraint->axis_local[1] = 0.0;
    }
    constraint->baumgarte_distance = 0.35;
    constraint->baumgarte_angle = 0.3;
    constraint->softness_linear = 0.0;
    constraint->softness_angle = 0.0;
    constraint->slop = 1e-4;
    constraint->max_correction_distance = 0.1;
    constraint->max_correction_angle = 0.2;
    constraint->spring_distance_stiffness = 0.0;
    constraint->spring_distance_damping = 0.0;
    constraint->spring_angle_stiffness = 0.0;
    constraint->spring_angle_damping = 0.0;
    constraint->cached_dt = 0.0;
    constraint->last_distance_impulse = 0.0;
    constraint->last_angle_impulse = 0.0;
    constraint->last_distance_force = 0.0;
    constraint->last_angle_force = 0.0;
    constraint->accumulated_distance_impulse = 0.0;
    constraint->accumulated_angle_impulse = 0.0;
}

void chrono_distance_angle_constraint2d_set_rest_distance(ChronoDistanceAngleConstraint2D_C *constraint,
                                                          double rest_distance) {
    if (!constraint) {
        return;
    }
    constraint->rest_distance = rest_distance;
}

void chrono_distance_angle_constraint2d_set_rest_angle(ChronoDistanceAngleConstraint2D_C *constraint,
                                                       double rest_angle) {
    if (!constraint) {
        return;
    }
    constraint->rest_angle = rest_angle;
}

void chrono_distance_angle_constraint2d_set_baumgarte(ChronoDistanceAngleConstraint2D_C *constraint,
                                                      double beta_distance,
                                                      double beta_angle) {
    if (!constraint) {
        return;
    }
    if (beta_distance < 0.0) {
        beta_distance = 0.0;
    }
    if (beta_distance > 1.0) {
        beta_distance = 1.0;
    }
    if (beta_angle < 0.0) {
        beta_angle = 0.0;
    }
    if (beta_angle > 1.0) {
        beta_angle = 1.0;
    }
    constraint->baumgarte_distance = beta_distance;
    constraint->baumgarte_angle = beta_angle;
}

void chrono_distance_angle_constraint2d_set_slop(ChronoDistanceAngleConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_distance_angle_constraint2d_set_max_correction(ChronoDistanceAngleConstraint2D_C *constraint,
                                                           double max_distance,
                                                           double max_angle) {
    if (!constraint) {
        return;
    }
    if (max_distance < 0.0) {
        max_distance = 0.0;
    }
    if (max_angle < 0.0) {
        max_angle = 0.0;
    }
    constraint->max_correction_distance = max_distance;
    constraint->max_correction_angle = max_angle;
}

void chrono_distance_angle_constraint2d_set_softness_linear(ChronoDistanceAngleConstraint2D_C *constraint,
                                                            double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness_linear = softness;
}

void chrono_distance_angle_constraint2d_set_softness_angle(ChronoDistanceAngleConstraint2D_C *constraint,
                                                           double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness_angle = softness;
}

void chrono_distance_angle_constraint2d_set_distance_spring(ChronoDistanceAngleConstraint2D_C *constraint,
                                                            double stiffness,
                                                            double damping) {
    if (!constraint) {
        return;
    }
    constraint->spring_distance_stiffness = (stiffness > 0.0) ? stiffness : 0.0;
    constraint->spring_distance_damping = (damping > 0.0) ? damping : 0.0;
}

void chrono_distance_angle_constraint2d_set_angle_spring(ChronoDistanceAngleConstraint2D_C *constraint,
                                                         double stiffness,
                                                         double damping) {
    if (!constraint) {
        return;
    }
    constraint->spring_angle_stiffness = (stiffness > 0.0) ? stiffness : 0.0;
    constraint->spring_angle_damping = (damping > 0.0) ? damping : 0.0;
}

void chrono_coupled_constraint2d_init(ChronoCoupledConstraint2D_C *constraint,
                                      ChronoBody2D_C *body_a,
                                      ChronoBody2D_C *body_b,
                                      const double local_anchor_a[2],
                                      const double local_anchor_b[2],
                                      const double axis_local[2],
                                      double rest_distance,
                                      double rest_angle,
                                      double ratio_distance,
                                      double ratio_angle,
                                      double target_offset) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_coupled_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    if (axis_local) {
        constraint->axis_local[0] = axis_local[0];
        constraint->axis_local[1] = axis_local[1];
    } else {
        constraint->axis_local[0] = 1.0;
        constraint->axis_local[1] = 0.0;
    }
    constraint->rest_distance = rest_distance;
    constraint->rest_angle = rest_angle;
    constraint->ratio_distance = ratio_distance;
    constraint->ratio_angle = ratio_angle;
    constraint->target_offset = target_offset;
    constraint->softness = 0.0;
    constraint->baumgarte = 0.35;
    constraint->slop = 5e-4;
    constraint->max_correction = 0.1;
    constraint->cached_dt = 0.0;
    constraint->effective_mass = 0.0;
    constraint->bias = 0.0;
    constraint->accumulated_impulse = 0.0;
    constraint->last_impulse = 0.0;
    constraint->base.accumulated_impulse = 0.0;
    constraint->base.effective_mass = 0.0;
}

void chrono_coupled_constraint2d_set_rest_distance(ChronoCoupledConstraint2D_C *constraint, double rest_distance) {
    if (!constraint) {
        return;
    }
    constraint->rest_distance = rest_distance;
}

void chrono_coupled_constraint2d_set_rest_angle(ChronoCoupledConstraint2D_C *constraint, double rest_angle) {
    if (!constraint) {
        return;
    }
    constraint->rest_angle = rest_angle;
}

void chrono_coupled_constraint2d_set_ratios(ChronoCoupledConstraint2D_C *constraint,
                                            double ratio_distance,
                                            double ratio_angle) {
    if (!constraint) {
        return;
    }
    constraint->ratio_distance = ratio_distance;
    constraint->ratio_angle = ratio_angle;
}

void chrono_coupled_constraint2d_set_target_offset(ChronoCoupledConstraint2D_C *constraint, double offset) {
    if (!constraint) {
        return;
    }
    constraint->target_offset = offset;
}

void chrono_coupled_constraint2d_set_baumgarte(ChronoCoupledConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte = beta;
}

void chrono_coupled_constraint2d_set_softness(ChronoCoupledConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_coupled_constraint2d_set_slop(ChronoCoupledConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_coupled_constraint2d_set_max_correction(ChronoCoupledConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

static void chrono_distance_angle_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoDistanceAngleConstraint2D_C *constraint = (ChronoDistanceAngleConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    constraint->cached_dt = dt;

    double axis_local[2] = {constraint->axis_local[0], constraint->axis_local[1]};
    if (body_a) {
        rotate_angle(body_a->angle, axis_local, constraint->normal);
    } else {
        constraint->normal[0] = axis_local[0];
        constraint->normal[1] = axis_local[1];
    }
    normalize(constraint->normal);

    double world_a[2];
    double world_b[2];

    if (body_a) {
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        constraint->ra[0] = world_a[0] - body_a->position[0];
        constraint->ra[1] = world_a[1] - body_a->position[1];
    } else {
        world_a[0] = constraint->local_anchor_a[0];
        world_a[1] = constraint->local_anchor_a[1];
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
    }

    if (body_b) {
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        constraint->rb[0] = world_b[0] - body_b->position[0];
        constraint->rb[1] = world_b[1] - body_b->position[1];
    } else {
        world_b[0] = constraint->local_anchor_b[0];
        world_b[1] = constraint->local_anchor_b[1];
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    double pa[2];
    double pb[2];
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    } else {
        pa[0] = world_a[0];
        pa[1] = world_a[1];
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    } else {
        pb[0] = world_b[0];
        pb[1] = world_b[1];
    }

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist > 1e-9) {
        constraint->normal[0] = delta[0] / dist;
        constraint->normal[1] = delta[1] / dist;
    }

    double ra_cross_n = cross(constraint->ra, constraint->normal);
    double rb_cross_n = cross(constraint->rb, constraint->normal);

    double inv_mass_linear = 0.0;
    double inv_mass_angular = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_linear += body_a->inverse_mass;
        inv_mass_angular += ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_linear += body_b->inverse_mass;
        inv_mass_angular += rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    double inv_mass_distance = inv_mass_linear + inv_mass_angular;
    double denom_distance = inv_mass_distance + constraint->softness_linear;
    constraint->mass_distance = (denom_distance > 0.0) ? 1.0 / denom_distance : 0.0;

    double inv_mass_angle = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_angle += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_angle += body_b->inverse_inertia;
    }
    double denom_angle = inv_mass_angle + constraint->softness_angle;
    constraint->mass_angle = (denom_angle > 0.0) ? 1.0 / denom_angle : 0.0;

    double C_distance = dist - constraint->rest_distance;
    double distance_error = fabs(C_distance) > constraint->slop ? C_distance - constraint->slop * (C_distance > 0 ? 1 : -1) : 0.0;
    if (constraint->baumgarte_distance > 0.0) {
        constraint->bias_distance = -constraint->baumgarte_distance / dt * distance_error;
    } else {
        constraint->bias_distance = 0.0;
    }
    constraint->spring_distance_deflection = C_distance;

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;
    double angle_error = (angle_b - angle_a) - constraint->rest_angle;
    if (constraint->baumgarte_angle > 0.0) {
        constraint->bias_angle = -constraint->baumgarte_angle / dt * angle_error;
    } else {
        constraint->bias_angle = 0.0;
    }
    constraint->spring_angle_deflection = angle_error;

    constraint->last_distance_force = 0.0;
    constraint->last_angle_force = 0.0;
    constraint->last_distance_impulse = 0.0;
    constraint->last_angle_impulse = 0.0;
}

static void chrono_distance_angle_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoDistanceAngleConstraint2D_C *constraint = (ChronoDistanceAngleConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double impulse_vec[2] = {
        constraint->normal[0] * constraint->accumulated_distance_impulse,
        constraint->normal[1] * constraint->accumulated_distance_impulse
    };
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    double lambda_angle = constraint->accumulated_angle_impulse;
    if (lambda_angle != 0.0) {
        if (body_a && !body_a->is_static) {
            body_a->angular_velocity -= lambda_angle * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angular_velocity += lambda_angle * body_b->inverse_inertia;
        }
    }
}

static void chrono_distance_angle_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoDistanceAngleConstraint2D_C *constraint = (ChronoDistanceAngleConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
    double Cdot_distance = dot(dv, constraint->normal);

    if (constraint->mass_distance > 0.0) {
        double gamma = constraint->softness_linear;
        double lambda = -(Cdot_distance + constraint->bias_distance + gamma * constraint->accumulated_distance_impulse) * constraint->mass_distance;
        constraint->accumulated_distance_impulse += lambda;

        double impulse_vec[2] = {
            constraint->normal[0] * lambda,
            constraint->normal[1] * lambda
        };
        apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
        apply_impulse(body_b, impulse_vec, constraint->rb);
        constraint->last_distance_impulse = lambda;
        if (constraint->cached_dt > 0.0) {
            constraint->last_distance_force = lambda / constraint->cached_dt;
        }
    }

    double omega_a = (body_a && !body_a->is_static) ? body_a->angular_velocity : 0.0;
    double omega_b = (body_b && !body_b->is_static) ? body_b->angular_velocity : 0.0;
    double rel_ang_vel = omega_b - omega_a;

    if (constraint->mass_angle > 0.0) {
        double gamma_angle = constraint->softness_angle;
        double lambda_angle = -(rel_ang_vel + constraint->bias_angle + gamma_angle * constraint->accumulated_angle_impulse) * constraint->mass_angle;
        constraint->accumulated_angle_impulse += lambda_angle;

        if (body_a && !body_a->is_static) {
            body_a->angular_velocity -= lambda_angle * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angular_velocity += lambda_angle * body_b->inverse_inertia;
        }
        constraint->last_angle_impulse = lambda_angle;
        if (constraint->cached_dt > 0.0) {
            constraint->last_angle_force = lambda_angle / constraint->cached_dt;
        }
    }

    if (constraint->cached_dt > 0.0) {
        if (constraint->spring_distance_stiffness > 0.0) {
            double force = -constraint->spring_distance_stiffness * constraint->spring_distance_deflection -
                           constraint->spring_distance_damping * Cdot_distance;
            double lambda = force * constraint->cached_dt;
            double impulse_vec[2] = {
                constraint->normal[0] * lambda,
                constraint->normal[1] * lambda
            };
            apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            apply_impulse(body_b, impulse_vec, constraint->rb);
            constraint->last_distance_force += force;
        }

        if (constraint->spring_angle_stiffness > 0.0) {
            double torque = -constraint->spring_angle_stiffness * constraint->spring_angle_deflection -
                            constraint->spring_angle_damping * rel_ang_vel;
            double lambda = torque * constraint->cached_dt;
            if (body_a && !body_a->is_static) {
                body_a->angular_velocity -= lambda * body_a->inverse_inertia;
            }
            if (body_b && !body_b->is_static) {
                body_b->angular_velocity += lambda * body_b->inverse_inertia;
            }
            constraint->last_angle_force += torque;
        }
    }
}

static void chrono_distance_angle_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoDistanceAngleConstraint2D_C *constraint = (ChronoDistanceAngleConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double pa[2];
    double pb[2];
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    } else {
        pa[0] = constraint->local_anchor_a[0];
        pa[1] = constraint->local_anchor_a[1];
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    } else {
        pb[0] = constraint->local_anchor_b[0];
        pb[1] = constraint->local_anchor_b[1];
    }

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    double normal[2];
    if (dist > 1e-9) {
        normal[0] = delta[0] / dist;
        normal[1] = delta[1] / dist;
    } else {
        normal[0] = constraint->normal[0];
        normal[1] = constraint->normal[1];
    }

    double C_distance = dist - constraint->rest_distance;
    double distance_error = fabs(C_distance) > constraint->slop ? C_distance - constraint->slop * (C_distance > 0 ? 1 : -1) : 0.0;
    double correction = -constraint->baumgarte_distance * distance_error;
    if (correction > constraint->max_correction_distance) {
        correction = constraint->max_correction_distance;
    } else if (correction < -constraint->max_correction_distance) {
        correction = -constraint->max_correction_distance;
    }

    double ra_cross_n = cross(constraint->ra, normal);
    double rb_cross_n = cross(constraint->rb, normal);

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }
    if (inv_mass > 0.0) {
        double lambda = correction / inv_mass;
        double impulse_vec[2] = {normal[0] * lambda, normal[1] * lambda};
        apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
        apply_impulse(body_b, impulse_vec, constraint->rb);
        if (body_a && !body_a->is_static) {
            body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
            body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
            body_a->angle -= ra_cross_n * lambda * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
            body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
            body_b->angle += rb_cross_n * lambda * body_b->inverse_inertia;
        }
    }

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;
    double angle_error = (angle_b - angle_a) - constraint->rest_angle;
    double angle_correction = -constraint->baumgarte_angle * angle_error;
    if (angle_correction > constraint->max_correction_angle) {
        angle_correction = constraint->max_correction_angle;
    } else if (angle_correction < -constraint->max_correction_angle) {
        angle_correction = -constraint->max_correction_angle;
    }

    double inv_inertia_sum = 0.0;
    if (body_a && !body_a->is_static) {
        inv_inertia_sum += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_inertia_sum += body_b->inverse_inertia;
    }
    if (inv_inertia_sum > 0.0) {
        double lambda_angle = angle_correction / inv_inertia_sum;
        if (body_a && !body_a->is_static) {
            body_a->angle -= lambda_angle * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angle += lambda_angle * body_b->inverse_inertia;
        }
    }
}

static void chrono_coupled_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoCoupledConstraint2D_C *constraint = (ChronoCoupledConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    constraint->cached_dt = dt;

    double axis_local[2] = {constraint->axis_local[0], constraint->axis_local[1]};
    if (body_a) {
        rotate_angle(body_a->angle, axis_local, constraint->normal);
    } else {
        constraint->normal[0] = axis_local[0];
        constraint->normal[1] = axis_local[1];
    }
    normalize(constraint->normal);

    double world_a[2];
    double world_b[2];

    if (body_a) {
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        constraint->ra[0] = world_a[0] - body_a->position[0];
        constraint->ra[1] = world_a[1] - body_a->position[1];
    } else {
        world_a[0] = constraint->local_anchor_a[0];
        world_a[1] = constraint->local_anchor_a[1];
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
    }

    if (body_b) {
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        constraint->rb[0] = world_b[0] - body_b->position[0];
        constraint->rb[1] = world_b[1] - body_b->position[1];
    } else {
        world_b[0] = constraint->local_anchor_b[0];
        world_b[1] = constraint->local_anchor_b[1];
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    double pa[2];
    double pb[2];
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    } else {
        pa[0] = world_a[0];
        pa[1] = world_a[1];
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    } else {
        pb[0] = world_b[0];
        pb[1] = world_b[1];
    }

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist > 1e-9) {
        constraint->normal[0] = delta[0] / dist;
        constraint->normal[1] = delta[1] / dist;
    }

    double ra_cross_n = cross(constraint->ra, constraint->normal);
    double rb_cross_n = cross(constraint->rb, constraint->normal);

    double inv_mass_linear = 0.0;
    double inv_mass_angular = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_linear += body_a->inverse_mass;
        inv_mass_angular += ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_linear += body_b->inverse_mass;
        inv_mass_angular += rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    double inv_mass_distance = inv_mass_linear + inv_mass_angular;

    double inv_mass_angle = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_angle += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_angle += body_b->inverse_inertia;
    }

    double k = constraint->ratio_distance * constraint->ratio_distance * inv_mass_distance +
               constraint->ratio_angle * constraint->ratio_angle * inv_mass_angle;
    double denom = k + constraint->softness;
    constraint->effective_mass = (denom > 0.0) ? 1.0 / denom : 0.0;

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;

    double C_distance = dist - constraint->rest_distance;
    double C_angle = (angle_b - angle_a) - constraint->rest_angle;
    double C = constraint->ratio_distance * C_distance +
               constraint->ratio_angle * C_angle -
               constraint->target_offset;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;
    if (constraint->baumgarte > 0.0) {
        constraint->bias = -constraint->baumgarte / dt * error;
    } else {
        constraint->bias = 0.0;
    }
    constraint->last_impulse = 0.0;
    constraint->base.effective_mass = constraint->effective_mass;
    constraint->base.accumulated_impulse = constraint->accumulated_impulse;
}

static void chrono_coupled_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoCoupledConstraint2D_C *constraint = (ChronoCoupledConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double lambda = constraint->accumulated_impulse;
    double linear_impulse = constraint->ratio_distance * lambda;
    double torque_impulse = constraint->ratio_angle * lambda;

    double impulse_vec[2] = {
        constraint->normal[0] * linear_impulse,
        constraint->normal[1] * linear_impulse
    };
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    if (body_a && !body_a->is_static) {
        body_a->angular_velocity -= torque_impulse * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->angular_velocity += torque_impulse * body_b->inverse_inertia;
    }
    constraint->base.accumulated_impulse = constraint->accumulated_impulse;
}

static void chrono_coupled_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoCoupledConstraint2D_C *constraint = (ChronoCoupledConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
    double Cdot_distance = dot(dv, constraint->normal);
    double omega_a = (body_a && !body_a->is_static) ? body_a->angular_velocity : 0.0;
    double omega_b = (body_b && !body_b->is_static) ? body_b->angular_velocity : 0.0;
    double Cdot_angle = omega_b - omega_a;

    double Cdot = constraint->ratio_distance * Cdot_distance +
                  constraint->ratio_angle * Cdot_angle;

    if (constraint->effective_mass == 0.0) {
        return;
    }

    double lambda = -(Cdot + constraint->bias + constraint->softness * constraint->accumulated_impulse) * constraint->effective_mass;
    constraint->accumulated_impulse += lambda;
    constraint->last_impulse = lambda;
    constraint->base.accumulated_impulse = constraint->accumulated_impulse;

    double linear_impulse = constraint->ratio_distance * lambda;
    double torque_impulse = constraint->ratio_angle * lambda;

    double impulse_vec[2] = {
        constraint->normal[0] * linear_impulse,
        constraint->normal[1] * linear_impulse
    };
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    if (body_a && !body_a->is_static) {
        body_a->angular_velocity -= torque_impulse * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->angular_velocity += torque_impulse * body_b->inverse_inertia;
    }
}

static void chrono_coupled_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoCoupledConstraint2D_C *constraint = (ChronoCoupledConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double pa[2];
    double pb[2];
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    } else {
        pa[0] = constraint->local_anchor_a[0];
        pa[1] = constraint->local_anchor_a[1];
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    } else {
        pb[0] = constraint->local_anchor_b[0];
        pb[1] = constraint->local_anchor_b[1];
    }

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    double normal[2];
    if (dist > 1e-9) {
        normal[0] = delta[0] / dist;
        normal[1] = delta[1] / dist;
    } else {
        normal[0] = constraint->normal[0];
        normal[1] = constraint->normal[1];
    }

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;

    double C_distance = dist - constraint->rest_distance;
    double C_angle = (angle_b - angle_a) - constraint->rest_angle;
    double C = constraint->ratio_distance * C_distance +
               constraint->ratio_angle * C_angle -
               constraint->target_offset;

    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;
    double correction = -constraint->baumgarte * error;
    if (correction > constraint->max_correction) {
        correction = constraint->max_correction;
    } else if (correction < -constraint->max_correction) {
        correction = -constraint->max_correction;
    }

    double ra_cross_n = cross(constraint->ra, normal);
    double rb_cross_n = cross(constraint->rb, normal);

    double inv_mass_linear = 0.0;
    double inv_mass_angular = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_linear += body_a->inverse_mass;
        inv_mass_angular += ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_linear += body_b->inverse_mass;
        inv_mass_angular += rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }
    double inv_mass_distance = inv_mass_linear + inv_mass_angular;

    double inv_mass_angle = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_angle += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_angle += body_b->inverse_inertia;
    }

    double denom = constraint->ratio_distance * constraint->ratio_distance * inv_mass_distance +
                   constraint->ratio_angle * constraint->ratio_angle * inv_mass_angle;

    if (denom == 0.0) {
        return;
    }

    double lambda = correction / denom;
    double linear_impulse = constraint->ratio_distance * lambda;
    double torque_impulse = constraint->ratio_angle * lambda;

    double impulse_vec[2] = {normal[0] * linear_impulse, normal[1] * linear_impulse};
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    if (body_a && !body_a->is_static) {
        body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
        body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
        body_a->angle -= ra_cross_n * linear_impulse * body_a->inverse_inertia;
        body_a->angle -= torque_impulse * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
        body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
        body_b->angle += rb_cross_n * linear_impulse * body_b->inverse_inertia;
        body_b->angle += torque_impulse * body_b->inverse_inertia;
    }
}

static void chrono_distance_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoDistanceConstraint2D_C *constraint = (ChronoDistanceConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    constraint->cached_dt = dt;

    chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, constraint->ra);
    sub(constraint->ra, body_a->position, constraint->ra);
    chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, constraint->rb);
    sub(constraint->rb, body_b->position, constraint->rb);

    double pa[2];
    add(body_a->position, constraint->ra, pa);
    double pb[2];
    add(body_b->position, constraint->rb, pb);

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist > 1e-9) {
        constraint->normal[0] = delta[0] / dist;
        constraint->normal[1] = delta[1] / dist;
    } else {
        constraint->normal[0] = 1.0;
        constraint->normal[1] = 0.0;
        dist = 0.0;
    }

    double ra_cross_n = cross(constraint->ra, constraint->normal);
    double rb_cross_n = cross(constraint->rb, constraint->normal);

    double inv_mass_linear = 0.0;
    double inv_mass_angular = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass_linear += body_a->inverse_mass;
        inv_mass_angular += ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass_linear += body_b->inverse_mass;
        inv_mass_angular += rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    double inv_mass = inv_mass_linear + inv_mass_angular;
    if (inv_mass > 0.0) {
        constraint->base.effective_mass = 1.0 / inv_mass;
    } else {
        constraint->base.effective_mass = 0.0;
    }

    double C = dist - constraint->rest_length;
    constraint->spring_deflection = C;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;

    double beta = constraint->baumgarte_beta;
    double bias = 0.0;
    if (beta > 0.0) {
        bias = -beta / dt * error;
    }
    constraint->bias = bias;

    double compliance_linear = constraint->softness_linear;
    double compliance_angular = constraint->softness_angular * (ra_cross_n * ra_cross_n + rb_cross_n * rb_cross_n);
    double denom = inv_mass + compliance_linear + compliance_angular;
    if (denom > 0.0) {
        constraint->base.effective_mass = 1.0 / denom;
    }

    constraint->accumulated_penetration = error;
    constraint->last_spring_force = 0.0;
    constraint->last_impulse = 0.0;
}

static void chrono_distance_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoDistanceConstraint2D_C *constraint = (ChronoDistanceConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    double impulse_vec[2];
    scale(constraint->normal, constraint->base.accumulated_impulse, impulse_vec);
    apply_impulse(constraint->base.body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(constraint->base.body_b, impulse_vec, constraint->rb);
}

static void chrono_distance_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoDistanceConstraint2D_C *constraint = (ChronoDistanceConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
  	double Cdot = dot(dv, constraint->normal);

    double gamma = constraint->softness_linear;
    double gamma_ang = constraint->softness_angular;
    double compli = gamma + gamma_ang;
    if (gamma <= 0.0 && gamma_ang > 0.0) {
        gamma = gamma_ang;
    } else if (gamma > 0.0 && gamma_ang > 0.0) {
        gamma = compli;
    }
    double denom = constraint->base.effective_mass;
    if (denom == 0.0) {
        return;
    }
    double lambda = -(Cdot + constraint->bias + gamma * constraint->base.accumulated_impulse) * denom;
    constraint->base.accumulated_impulse += lambda;

    double impulse_vec[2];
    scale(constraint->normal, lambda, impulse_vec);
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);
    constraint->last_impulse = lambda;

    double stiffness = constraint->spring_stiffness;
    double damping = constraint->spring_damping;
    if ((stiffness > 0.0 || damping > 0.0) && constraint->cached_dt > 0.0) {
        double va_post[2] = {0.0, 0.0};
        double vb_post[2] = {0.0, 0.0};

        if (body_a && !body_a->is_static) {
            va_post[0] = body_a->linear_velocity[0];
            va_post[1] = body_a->linear_velocity[1];
            double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
            double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
            va_post[0] += cross_ra_x;
            va_post[1] += cross_ra_y;
        }
        if (body_b && !body_b->is_static) {
            vb_post[0] = body_b->linear_velocity[0];
            vb_post[1] = body_b->linear_velocity[1];
            double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
            double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
            vb_post[0] += cross_rb_x;
            vb_post[1] += cross_rb_y;
        }

        double dv_post[2];
        sub(vb_post, va_post, dv_post);
        double rel_vel = dot(dv_post, constraint->normal);
        double force = -stiffness * constraint->spring_deflection - damping * rel_vel;
        double lambda_spring = force * constraint->cached_dt;
        if (lambda_spring != 0.0) {
            double impulse_spring[2] = {
                constraint->normal[0] * lambda_spring,
                constraint->normal[1] * lambda_spring
            };
            apply_impulse(body_a, (double[2]){-impulse_spring[0], -impulse_spring[1]}, constraint->ra);
            apply_impulse(body_b, impulse_spring, constraint->rb);
        }
        constraint->last_spring_force = force;
    } else {
        constraint->last_spring_force = 0.0;
    }
}

static void chrono_distance_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoDistanceConstraint2D_C *constraint = (ChronoDistanceConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double pa[2];
    add(body_a->position, constraint->ra, pa);
    double pb[2];
    add(body_b->position, constraint->rb, pb);

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist < 1e-9) {
        return;
    }
    double n[2] = {delta[0] / dist, delta[1] / dist};
    double C = dist - constraint->rest_length;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;
    double correction = -constraint->baumgarte_beta * error;
    if (correction > constraint->max_correction) {
        correction = constraint->max_correction;
    } else if (correction < -constraint->max_correction) {
        correction = -constraint->max_correction;
    }

    double ra_cross_n = cross(constraint->ra, n);
    double rb_cross_n = cross(constraint->rb, n);

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }
    if (inv_mass == 0.0) {
        return;
    }
    double lambda = correction / inv_mass;

    double impulse_vec[2];
    scale(n, lambda, impulse_vec);
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    if (body_a && !body_a->is_static) {
        body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
        body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
        body_a->angle -= ra_cross_n * lambda * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
        body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
        body_b->angle += rb_cross_n * lambda * body_b->inverse_inertia;
    }
}

void chrono_revolute_constraint2d_init(ChronoRevoluteConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2]) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_revolute_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    constraint->softness = 0.0;
    constraint->baumgarte_beta = 0.2;
    constraint->slop = 1e-3;
    constraint->max_correction = 0.2;
    constraint->motor_enable = 0;
    constraint->motor_mode = CHRONO_REVOLUTE_MOTOR_VELOCITY;
    constraint->motor_speed = 0.0;
    constraint->motor_max_torque = 0.0;
    constraint->motor_position_target = 0.0;
    constraint->motor_position_gain = 0.0;
    constraint->motor_position_damping = 0.0;
    constraint->motor_mass = 0.0;
    constraint->motor_accumulated_impulse = 0.0;
    constraint->last_motor_torque = 0.0;
}

void chrono_revolute_constraint2d_set_baumgarte(ChronoRevoluteConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_revolute_constraint2d_set_softness(ChronoRevoluteConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_revolute_constraint2d_set_slop(ChronoRevoluteConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_revolute_constraint2d_set_max_correction(ChronoRevoluteConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

void chrono_revolute_constraint2d_enable_motor(ChronoRevoluteConstraint2D_C *constraint,
                                              int enable,
                                              double speed,
                                              double max_torque) {
    if (!constraint) {
        return;
    }
    constraint->motor_enable = enable ? 1 : 0;
    constraint->motor_mode = CHRONO_REVOLUTE_MOTOR_VELOCITY;
    constraint->motor_speed = speed;
    constraint->motor_max_torque = (max_torque >= 0.0) ? max_torque : 0.0;
    if (!constraint->motor_enable) {
        constraint->motor_accumulated_impulse = 0.0;
    }
}

void chrono_revolute_constraint2d_set_motor_position_target(ChronoRevoluteConstraint2D_C *constraint,
                                                            double target_angle,
                                                            double proportional_gain,
                                                            double damping_gain) {
    if (!constraint) {
        return;
    }
    constraint->motor_enable = 1;
    constraint->motor_mode = CHRONO_REVOLUTE_MOTOR_POSITION;
    constraint->motor_position_target = target_angle;
    constraint->motor_position_gain = (proportional_gain > 0.0) ? proportional_gain : 0.0;
    constraint->motor_position_damping = (damping_gain > 0.0) ? damping_gain : 0.0;
}

static void chrono_revolute_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoRevoluteConstraint2D_C *constraint = (ChronoRevoluteConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    constraint->cached_dt = dt;

    if (body_a) {
        double world_a[2];
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        sub(world_a, body_a->position, constraint->ra);
    } else {
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
    }
    if (body_b) {
        double world_b[2];
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        sub(world_b, body_b->position, constraint->rb);
    } else {
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    double pa[2] = {0.0, 0.0};
    double pb[2] = {0.0, 0.0};
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    }

    double C[2];
    sub(pb, pa, C);

    double inv_mass_a = (body_a && !body_a->is_static) ? body_a->inverse_mass : 0.0;
    double inv_inertia_a = (body_a && !body_a->is_static) ? body_a->inverse_inertia : 0.0;
    double inv_mass_b = (body_b && !body_b->is_static) ? body_b->inverse_mass : 0.0;
    double inv_inertia_b = (body_b && !body_b->is_static) ? body_b->inverse_inertia : 0.0;

    double k00 = inv_mass_a + inv_mass_b;
    double k11 = inv_mass_a + inv_mass_b;
    double k01 = 0.0;

    if (inv_inertia_a > 0.0) {
        k00 += inv_inertia_a * constraint->ra[1] * constraint->ra[1];
        k01 -= inv_inertia_a * constraint->ra[0] * constraint->ra[1];
        k11 += inv_inertia_a * constraint->ra[0] * constraint->ra[0];
    }
    if (inv_inertia_b > 0.0) {
        k00 += inv_inertia_b * constraint->rb[1] * constraint->rb[1];
        k01 -= inv_inertia_b * constraint->rb[0] * constraint->rb[1];
        k11 += inv_inertia_b * constraint->rb[0] * constraint->rb[0];
    }

    double k[2][2] = {
        {k00, k01},
        {k01, k11}
    };

    if (!invert2x2(&k[0][0], &constraint->position_mass[0][0])) {
        memset(constraint->position_mass, 0, sizeof(constraint->position_mass));
    }

    double vel_k[2][2] = {
        {k00 + constraint->softness, k01},
        {k01, k11 + constraint->softness}
    };
    if (!invert2x2(&vel_k[0][0], &constraint->effective_mass[0][0])) {
        memset(constraint->effective_mass, 0, sizeof(constraint->effective_mass));
    }

    double error_len = length(C);
    if (error_len > constraint->slop) {
        double scale_factor = 1.0;
        if (error_len > 0.0) {
            scale_factor = (error_len - constraint->slop) / error_len;
        }
        constraint->bias[0] = -constraint->baumgarte_beta / dt * C[0] * scale_factor;
        constraint->bias[1] = -constraint->baumgarte_beta / dt * C[1] * scale_factor;
    } else {
        constraint->bias[0] = 0.0;
        constraint->bias[1] = 0.0;
    }

    double inv_inertia_sum = 0.0;
    if (body_a && !body_a->is_static) {
        inv_inertia_sum += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_inertia_sum += body_b->inverse_inertia;
    }
    constraint->motor_mass = (inv_inertia_sum > 0.0) ? 1.0 / inv_inertia_sum : 0.0;
    constraint->last_motor_torque = 0.0;
}

static void chrono_revolute_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoRevoluteConstraint2D_C *constraint = (ChronoRevoluteConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double impulse[2] = {
        constraint->accumulated_impulse[0],
        constraint->accumulated_impulse[1]
    };

    if (body_a) {
        double impulse_a[2] = {-impulse[0], -impulse[1]};
        apply_impulse(body_a, impulse_a, constraint->ra);
    }
    if (body_b) {
        apply_impulse(body_b, impulse, constraint->rb);
    }

    if (constraint->motor_enable && constraint->motor_mass > 0.0) {
        double motor_lambda = constraint->motor_accumulated_impulse;
        if (body_a && !body_a->is_static) {
            body_a->angular_velocity -= motor_lambda * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angular_velocity += motor_lambda * body_b->inverse_inertia;
        }
    }
    constraint->last_motor_torque = 0.0;
}

static void chrono_revolute_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoRevoluteConstraint2D_C *constraint = (ChronoRevoluteConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);

    double gamma = constraint->softness;
    double c[2] = {
        dv[0] + constraint->bias[0] + gamma * constraint->accumulated_impulse[0],
        dv[1] + constraint->bias[1] + gamma * constraint->accumulated_impulse[1]
    };

    double lambda[2] = {0.0, 0.0};
    lambda[0] = -(constraint->effective_mass[0][0] * c[0] + constraint->effective_mass[0][1] * c[1]);
    lambda[1] = -(constraint->effective_mass[1][0] * c[0] + constraint->effective_mass[1][1] * c[1]);

    constraint->accumulated_impulse[0] += lambda[0];
    constraint->accumulated_impulse[1] += lambda[1];

    if (body_a) {
        double impulse_a[2] = {-lambda[0], -lambda[1]};
        apply_impulse(body_a, impulse_a, constraint->ra);
    }
    if (body_b) {
        apply_impulse(body_b, lambda, constraint->rb);
    }

    double omega_a = (body_a && !body_a->is_static) ? body_a->angular_velocity : 0.0;
    double omega_b = (body_b && !body_b->is_static) ? body_b->angular_velocity : 0.0;
    double rel_ang_vel = omega_b - omega_a;

    double target_speed = constraint->motor_speed;
    if (constraint->motor_enable && constraint->motor_mode == CHRONO_REVOLUTE_MOTOR_POSITION) {
        double angle_a = body_a ? body_a->angle : 0.0;
        double angle_b = body_b ? body_b->angle : 0.0;
        double angle_error = (angle_b - angle_a) - constraint->motor_position_target;
        target_speed = constraint->motor_position_gain * (-angle_error) - constraint->motor_position_damping * rel_ang_vel;
    }

    if (constraint->motor_enable && constraint->motor_mass > 0.0 && constraint->motor_max_torque > 0.0) {
        double Cmotor = rel_ang_vel - target_speed;
        double lambda_motor = -Cmotor * constraint->motor_mass;
        double max_impulse = constraint->motor_max_torque * constraint->cached_dt;
        double prev_impulse = constraint->motor_accumulated_impulse;
        double new_impulse = prev_impulse + lambda_motor;
        if (new_impulse > max_impulse) {
            new_impulse = max_impulse;
        } else if (new_impulse < -max_impulse) {
            new_impulse = -max_impulse;
        }
        lambda_motor = new_impulse - prev_impulse;
        constraint->motor_accumulated_impulse = new_impulse;
        if (body_a && !body_a->is_static) {
            body_a->angular_velocity -= lambda_motor * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angular_velocity += lambda_motor * body_b->inverse_inertia;
        }
        if (constraint->cached_dt > 0.0) {
            constraint->last_motor_torque = lambda_motor / constraint->cached_dt;
        }
    } else {
        constraint->last_motor_torque = 0.0;
    }
}

static void chrono_revolute_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoRevoluteConstraint2D_C *constraint = (ChronoRevoluteConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double pa[2] = {0.0, 0.0};
    double pb[2] = {0.0, 0.0};
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    }

    double C[2];
    sub(pb, pa, C);

    double error_len = length(C);
    double linear_error[2] = {0.0, 0.0};
    if (error_len > constraint->slop) {
        double scale_factor = 1.0;
        if (error_len > 0.0) {
            scale_factor = (error_len - constraint->slop) / error_len;
        }
        linear_error[0] = C[0] * scale_factor;
        linear_error[1] = C[1] * scale_factor;
    }

    double correction[2] = {-linear_error[0], -linear_error[1]};
    double correction_len = length(correction);
    if (correction_len > constraint->max_correction && correction_len > 0.0) {
        double scale = constraint->max_correction / correction_len;
        correction[0] *= scale;
        correction[1] *= scale;
    }

    double lambda[2] = {0.0, 0.0};
    lambda[0] = constraint->position_mass[0][0] * correction[0] + constraint->position_mass[0][1] * correction[1];
    lambda[1] = constraint->position_mass[1][0] * correction[0] + constraint->position_mass[1][1] * correction[1];

    if (body_a && !body_a->is_static) {
        body_a->position[0] += -lambda[0] * body_a->inverse_mass;
        body_a->position[1] += -lambda[1] * body_a->inverse_mass;
        double angular = cross(constraint->ra, (double[2]){lambda[0], lambda[1]});
        body_a->angle -= angular * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->position[0] += lambda[0] * body_b->inverse_mass;
        body_b->position[1] += lambda[1] * body_b->inverse_mass;
        double angular = cross(constraint->rb, (double[2]){lambda[0], lambda[1]});
        body_b->angle += angular * body_b->inverse_inertia;
    }
}

static void chrono_prismatic_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoPrismaticConstraint2D_C *constraint = (ChronoPrismaticConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    constraint->cached_dt = dt;
    constraint->last_motor_force = 0.0;
    constraint->last_limit_force = 0.0;
    constraint->last_limit_spring_force = 0.0;

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    if (body_a) {
        double world_a[2];
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        sub(world_a, body_a->position, constraint->ra);
        rotate_angle(body_a->angle, constraint->local_axis_a, constraint->axis_world);
    } else {
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
        constraint->axis_world[0] = constraint->local_axis_a[0];
        constraint->axis_world[1] = constraint->local_axis_a[1];
    }

    if (body_b) {
        double world_b[2];
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        sub(world_b, body_b->position, constraint->rb);
    } else {
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    if (fabs(constraint->axis_world[0]) < 1e-9 && fabs(constraint->axis_world[1]) < 1e-9) {
        constraint->axis_world[0] = 1.0;
        constraint->axis_world[1] = 0.0;
    }
    normalize(constraint->axis_world);
    constraint->normal_world[0] = -constraint->axis_world[1];
    constraint->normal_world[1] = constraint->axis_world[0];
    normalize(constraint->normal_world);

    double pa[2] = {0.0, 0.0};
    double pb[2] = {0.0, 0.0};
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    }

    double delta[2];
    sub(pb, pa, delta);
    constraint->translation = dot(delta, constraint->axis_world);
    double C = dot(delta, constraint->normal_world);
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1.0 : -1.0) : 0.0;

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        double ra_cross_n = cross(constraint->ra, constraint->normal_world);
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        double rb_cross_n = cross(constraint->rb, constraint->normal_world);
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    constraint->base.effective_mass = (inv_mass > 0.0) ? 1.0 / inv_mass : 0.0;
    if (constraint->softness > 0.0) {
        constraint->base.effective_mass = (inv_mass + constraint->softness) > 0.0
            ? 1.0 / (inv_mass + constraint->softness)
            : 0.0;
    }

    double beta = constraint->baumgarte_beta;
    constraint->bias = (beta > 0.0) ? -beta / dt * error : 0.0;

    double inv_mass_axis = 0.0;
    if (body_a && !body_a->is_static) {
        double ra_cross_axis = cross(constraint->ra, constraint->axis_world);
        inv_mass_axis += body_a->inverse_mass + ra_cross_axis * ra_cross_axis * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        double rb_cross_axis = cross(constraint->rb, constraint->axis_world);
        inv_mass_axis += body_b->inverse_mass + rb_cross_axis * rb_cross_axis * body_b->inverse_inertia;
    }
    constraint->motor_mass = (inv_mass_axis > 0.0) ? 1.0 / inv_mass_axis : 0.0;

    constraint->limit_state = 0;
    constraint->limit_bias = 0.0;
    if (constraint->enable_limit) {
        if (constraint->translation < constraint->limit_lower) {
            constraint->limit_state = -1;
        } else if (constraint->translation > constraint->limit_upper) {
            constraint->limit_state = 1;
        }
        if (constraint->limit_state != 0) {
            double limit_error = 0.0;
            if (constraint->limit_state < 0) {
                limit_error = constraint->translation - constraint->limit_lower;
            } else {
                limit_error = constraint->translation - constraint->limit_upper;
            }
            double beta_limit = constraint->baumgarte_beta;
            if (beta_limit > 0.0) {
                constraint->limit_bias = -beta_limit / dt * limit_error;
            }
        } else {
            constraint->limit_accumulated_impulse = 0.0;
        }
    } else {
        constraint->limit_accumulated_impulse = 0.0;
    }

    if (!constraint->enable_motor || constraint->motor_max_force <= 0.0) {
        constraint->accumulated_motor_impulse = 0.0;
    }
}

static void chrono_prismatic_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoPrismaticConstraint2D_C *constraint = (ChronoPrismaticConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    double impulse_vec[2] = {
        constraint->normal_world[0] * constraint->base.accumulated_impulse,
        constraint->normal_world[1] * constraint->base.accumulated_impulse
    };
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;
    if (body_a) {
        apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    }
    if (body_b) {
        apply_impulse(body_b, impulse_vec, constraint->rb);
    }

    if (constraint->enable_motor && constraint->motor_max_force > 0.0 && constraint->motor_mass > 0.0) {
        double motor_vec[2] = {
            constraint->axis_world[0] * constraint->accumulated_motor_impulse,
            constraint->axis_world[1] * constraint->accumulated_motor_impulse
        };
        if (body_a) {
            apply_impulse(body_a, (double[2]){-motor_vec[0], -motor_vec[1]}, constraint->ra);
        }
        if (body_b) {
            apply_impulse(body_b, motor_vec, constraint->rb);
        }
    }

    if (constraint->enable_limit && constraint->limit_state != 0 && constraint->motor_mass > 0.0) {
        double limit_vec[2] = {
            constraint->axis_world[0] * constraint->limit_accumulated_impulse,
            constraint->axis_world[1] * constraint->limit_accumulated_impulse
        };
        if (body_a) {
            apply_impulse(body_a, (double[2]){-limit_vec[0], -limit_vec[1]}, constraint->ra);
        }
        if (body_b) {
            apply_impulse(body_b, limit_vec, constraint->rb);
        }
    }
    constraint->last_motor_force = 0.0;
    constraint->last_limit_force = 0.0;
    constraint->last_limit_spring_force = 0.0;
}

static void chrono_prismatic_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoPrismaticConstraint2D_C *constraint = (ChronoPrismaticConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};
    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
    double Cdot = dot(dv, constraint->normal_world);
    double axis_vel = dot(dv, constraint->axis_world);
    double gamma = constraint->softness;
    double denom = constraint->base.effective_mass;
    if (denom == 0.0) {
        return;
    }
    double lambda = -(Cdot + constraint->bias + gamma * constraint->base.accumulated_impulse) * denom;
    constraint->base.accumulated_impulse += lambda;

    double impulse_vec[2] = {
        constraint->normal_world[0] * lambda,
        constraint->normal_world[1] * lambda
    };

    if (body_a) {
        apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    }
    if (body_b) {
        apply_impulse(body_b, impulse_vec, constraint->rb);
    }

    double motor_target_speed = constraint->motor_speed;
    if (constraint->enable_motor && constraint->motor_mode == CHRONO_PRISMATIC_MOTOR_POSITION) {
        double position_error = constraint->motor_position_target - constraint->translation;
        motor_target_speed = constraint->motor_position_gain * position_error - constraint->motor_position_damping * axis_vel;
    }

    constraint->motor_speed = motor_target_speed;

    if (constraint->enable_motor && constraint->motor_mass > 0.0 && constraint->motor_max_force > 0.0) {
        double Cmotor = axis_vel - motor_target_speed;
        double lambda_motor = -Cmotor * constraint->motor_mass;
        double max_impulse = constraint->motor_max_force * constraint->cached_dt;
        double prev_impulse = constraint->accumulated_motor_impulse;
        double new_impulse = prev_impulse + lambda_motor;
        if (new_impulse > max_impulse) {
            new_impulse = max_impulse;
        } else if (new_impulse < -max_impulse) {
            new_impulse = -max_impulse;
        }
        lambda_motor = new_impulse - prev_impulse;
        constraint->accumulated_motor_impulse = new_impulse;
        double motor_vec[2] = {
            constraint->axis_world[0] * lambda_motor,
            constraint->axis_world[1] * lambda_motor
        };
        if (body_a) {
            apply_impulse(body_a, (double[2]){-motor_vec[0], -motor_vec[1]}, constraint->ra);
        }
        if (body_b) {
            apply_impulse(body_b, motor_vec, constraint->rb);
        }
        if (constraint->cached_dt > 0.0) {
            constraint->last_motor_force = lambda_motor / constraint->cached_dt;
        }
    } else {
        constraint->last_motor_force = 0.0;
    }

    if (constraint->enable_limit && constraint->limit_state != 0 && constraint->motor_mass > 0.0) {
        double gamma_limit = constraint->softness;
        double denom_limit = constraint->motor_mass;
        double lambda_limit = -(axis_vel + constraint->limit_bias + gamma_limit * constraint->limit_accumulated_impulse) * denom_limit;
        double old_impulse = constraint->limit_accumulated_impulse;
        double new_impulse = old_impulse + lambda_limit;
        if (constraint->limit_state < 0) {
            if (new_impulse < 0.0) {
                new_impulse = 0.0;
            }
        } else {
            if (new_impulse > 0.0) {
                new_impulse = 0.0;
            }
        }
        lambda_limit = new_impulse - old_impulse;
        constraint->limit_accumulated_impulse = new_impulse;

        double limit_vec[2] = {
            constraint->axis_world[0] * lambda_limit,
            constraint->axis_world[1] * lambda_limit
        };
        if (body_a) {
            apply_impulse(body_a, (double[2]){-limit_vec[0], -limit_vec[1]}, constraint->ra);
        }
        if (body_b) {
            apply_impulse(body_b, limit_vec, constraint->rb);
        }
        if (constraint->cached_dt > 0.0) {
            constraint->last_limit_force = lambda_limit / constraint->cached_dt;
        }
    }
    else {
        constraint->last_limit_force = 0.0;
    }

    if (constraint->enable_limit && constraint->limit_spring_stiffness > 0.0 && constraint->motor_mass > 0.0) {
        double deflection = 0.0;
        if (constraint->translation < constraint->limit_lower) {
            deflection = constraint->translation - constraint->limit_lower;
        } else if (constraint->translation > constraint->limit_upper) {
            deflection = constraint->translation - constraint->limit_upper;
        }
        if (deflection != 0.0) {
            double force = -constraint->limit_spring_stiffness * deflection - constraint->limit_spring_damping * axis_vel;
            double lambda_spring = force * constraint->cached_dt;
            double spring_vec[2] = {
                constraint->axis_world[0] * lambda_spring,
                constraint->axis_world[1] * lambda_spring
            };
            if (body_a) {
                apply_impulse(body_a, (double[2]){-spring_vec[0], -spring_vec[1]}, constraint->ra);
            }
            if (body_b) {
                apply_impulse(body_b, spring_vec, constraint->rb);
            }
            if (constraint->cached_dt > 0.0) {
                constraint->last_limit_spring_force = lambda_spring / constraint->cached_dt;
            }
        }
        else {
            constraint->last_limit_spring_force = 0.0;
        }
    } else {
        constraint->last_limit_spring_force = 0.0;
    }
}

static void chrono_prismatic_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoPrismaticConstraint2D_C *constraint = (ChronoPrismaticConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double axis_world[2] = {constraint->local_axis_a[0], constraint->local_axis_a[1]};
    if (body_a) {
        rotate_angle(body_a->angle, constraint->local_axis_a, axis_world);
    }
    normalize(axis_world);
    double normal[2] = {-axis_world[1], axis_world[0]};
    normalize(normal);

    double ra_local[2] = {0.0, 0.0};
    double rb_local[2] = {0.0, 0.0};
    if (body_a) {
        double world_a[2];
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        sub(world_a, body_a->position, ra_local);
    }
    if (body_b) {
        double world_b[2];
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        sub(world_b, body_b->position, rb_local);
    }

    double pa[2] = {0.0, 0.0};
    double pb[2] = {0.0, 0.0};
    if (body_a) {
        add(body_a->position, ra_local, pa);
    }
    if (body_b) {
        add(body_b->position, rb_local, pb);
    }

    double delta[2];
    sub(pb, pa, delta);
    double C = dot(delta, normal);
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1.0 : -1.0) : 0.0;

    double correction = -error;
    double max_corr = constraint->max_correction;
    if (correction > max_corr) {
        correction = max_corr;
    } else if (correction < -max_corr) {
        correction = -max_corr;
    }

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        double ra_cross_n = cross(ra_local, normal);
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        double rb_cross_n = cross(rb_local, normal);
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }
    if (inv_mass == 0.0) {
        return;
    }
    double lambda = correction / inv_mass;
    double impulse_vec[2] = {normal[0] * lambda, normal[1] * lambda};

    if (body_a && !body_a->is_static) {
        body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
        body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
        double angular = cross(ra_local, impulse_vec);
        body_a->angle -= angular * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
        body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
        double angular = cross(rb_local, impulse_vec);
        body_b->angle += angular * body_b->inverse_inertia;
    }

    double translation = dot(delta, axis_world);
    double limit_error = 0.0;
    if (constraint->enable_limit) {
        if (translation < constraint->limit_lower) {
            limit_error = translation - constraint->limit_lower;
        } else if (translation > constraint->limit_upper) {
            limit_error = translation - constraint->limit_upper;
        }
    }

    if (limit_error != 0.0) {
        double direction = (limit_error < 0.0) ? 1.0 : -1.0;
        double correction_mag = fmin(fabs(limit_error), constraint->max_correction);
        double correction = correction_mag * direction;

        double ra_cross_axis = cross(ra_local, axis_world);
        double rb_cross_axis = cross(rb_local, axis_world);
        double inv_mass_axis = 0.0;
        if (body_a && !body_a->is_static) {
            inv_mass_axis += body_a->inverse_mass + ra_cross_axis * ra_cross_axis * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            inv_mass_axis += body_b->inverse_mass + rb_cross_axis * rb_cross_axis * body_b->inverse_inertia;
        }
        if (inv_mass_axis > 0.0) {
            double lambda_limit = correction / inv_mass_axis;
            double limit_vec[2] = {axis_world[0] * lambda_limit, axis_world[1] * lambda_limit};

            if (body_a && !body_a->is_static) {
                body_a->position[0] -= limit_vec[0] * body_a->inverse_mass;
                body_a->position[1] -= limit_vec[1] * body_a->inverse_mass;
                double angular = cross(ra_local, limit_vec);
                body_a->angle -= angular * body_a->inverse_inertia;
            }
            if (body_b && !body_b->is_static) {
                body_b->position[0] += limit_vec[0] * body_b->inverse_mass;
                body_b->position[1] += limit_vec[1] * body_b->inverse_mass;
                double angular = cross(rb_local, limit_vec);
                body_b->angle += angular * body_b->inverse_inertia;
            }
        }
    }
}

static double planar_get_translation(const ChronoPlanarConstraint2D_C *constraint,
                                     const double delta[2],
                                     int axis) {
    return delta[0] * constraint->axis_world[axis][0] +
           delta[1] * constraint->axis_world[axis][1];
}

static void chrono_planar_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoPlanarConstraint2D_C *constraint = (ChronoPlanarConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    constraint->cached_dt = dt;

    double local_axis[2] = {constraint->local_axis_a[0], constraint->local_axis_a[1]};
    normalize(local_axis);
    if (body_a) {
        rotate_angle(body_a->angle, local_axis, constraint->axis_world[CHRONO_PLANAR_AXIS_X]);
    } else {
        constraint->axis_world[CHRONO_PLANAR_AXIS_X][0] = local_axis[0];
        constraint->axis_world[CHRONO_PLANAR_AXIS_X][1] = local_axis[1];
    }
    normalize(constraint->axis_world[CHRONO_PLANAR_AXIS_X]);
    constraint->axis_world[CHRONO_PLANAR_AXIS_Y][0] = -constraint->axis_world[CHRONO_PLANAR_AXIS_X][1];
    constraint->axis_world[CHRONO_PLANAR_AXIS_Y][1] = constraint->axis_world[CHRONO_PLANAR_AXIS_X][0];

    double world_a[2] = {0.0, 0.0};
    double world_b[2] = {0.0, 0.0};

    if (body_a) {
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        constraint->ra[0] = world_a[0] - body_a->position[0];
        constraint->ra[1] = world_a[1] - body_a->position[1];
    } else {
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
    }
    if (body_b) {
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        constraint->rb[0] = world_b[0] - body_b->position[0];
        constraint->rb[1] = world_b[1] - body_b->position[1];
    } else {
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    double pa[2];
    double pb[2];
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    } else {
        pa[0] = world_a[0];
        pa[1] = world_a[1];
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    } else {
        pb[0] = world_b[0];
        pb[1] = world_b[1];
    }

    double delta[2];
    sub(pb, pa, delta);

    for (int axis = 0; axis < CHRONO_PLANAR_AXIS_COUNT; ++axis) {
        const double *axis_vec = constraint->axis_world[axis];
        double ra_cross = cross(constraint->ra, axis_vec);
        double rb_cross = cross(constraint->rb, axis_vec);

        double inv_mass = 0.0;
        if (body_a && !body_a->is_static) {
            inv_mass += body_a->inverse_mass + ra_cross * ra_cross * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            inv_mass += body_b->inverse_mass + rb_cross * rb_cross * body_b->inverse_inertia;
        }
        if (constraint->softness > 0.0) {
            inv_mass += constraint->softness;
        }
        constraint->mass[axis] = (inv_mass > 0.0) ? 1.0 / inv_mass : 0.0;

        constraint->translation[axis] = planar_get_translation(constraint, delta, axis);
        constraint->limit_bias[axis] = 0.0;
        constraint->limit_deflection[axis] = 0.0;
        constraint->last_motor_force[axis] = 0.0;
        constraint->last_limit_force[axis] = 0.0;
        constraint->last_limit_spring_force[axis] = 0.0;

        if (!constraint->enable_motor[axis]) {
            constraint->motor_accumulated_impulse[axis] = 0.0;
        }

        if (constraint->enable_limit[axis]) {
            if (constraint->translation[axis] < constraint->limit_lower[axis]) {
                constraint->limit_state[axis] = -1;
                double error = constraint->translation[axis] - constraint->limit_lower[axis];
                constraint->limit_deflection[axis] = error;
                if (constraint->baumgarte_beta > 0.0) {
                    constraint->limit_bias[axis] = -constraint->baumgarte_beta / dt * error;
                }
            } else if (constraint->translation[axis] > constraint->limit_upper[axis]) {
                constraint->limit_state[axis] = 1;
                double error = constraint->translation[axis] - constraint->limit_upper[axis];
                constraint->limit_deflection[axis] = error;
                if (constraint->baumgarte_beta > 0.0) {
                    constraint->limit_bias[axis] = -constraint->baumgarte_beta / dt * error;
                }
            } else {
                constraint->limit_state[axis] = 0;
                constraint->limit_accumulated_impulse[axis] = 0.0;
            }
        } else {
            constraint->limit_state[axis] = 0;
            constraint->limit_accumulated_impulse[axis] = 0.0;
        }
    }

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;
    double angle_error = (angle_b - angle_a) - constraint->orientation_target;

    double inv_inertia_sum = 0.0;
    if (body_a && !body_a->is_static) {
        inv_inertia_sum += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_inertia_sum += body_b->inverse_inertia;
    }
    if (constraint->softness > 0.0) {
        inv_inertia_sum += constraint->softness;
    }
    constraint->orientation_mass = (inv_inertia_sum > 0.0) ? 1.0 / inv_inertia_sum : 0.0;
    if (constraint->baumgarte_beta > 0.0) {
        constraint->orientation_bias = -constraint->baumgarte_beta / dt * angle_error;
    } else {
        constraint->orientation_bias = 0.0;
    }
}

static void chrono_planar_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoPlanarConstraint2D_C *constraint = (ChronoPlanarConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    for (int axis = 0; axis < CHRONO_PLANAR_AXIS_COUNT; ++axis) {
        const double *axis_vec = constraint->axis_world[axis];

        double impulse_limit = constraint->limit_accumulated_impulse[axis];
        if (impulse_limit != 0.0) {
            double impulse_vec[2] = {axis_vec[0] * impulse_limit, axis_vec[1] * impulse_limit};
            if (body_a && !body_a->is_static) {
                apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            }
            if (body_b && !body_b->is_static) {
                apply_impulse(body_b, impulse_vec, constraint->rb);
            }
        }

        double impulse_motor = constraint->motor_accumulated_impulse[axis];
        if (impulse_motor != 0.0) {
            double impulse_vec[2] = {axis_vec[0] * impulse_motor, axis_vec[1] * impulse_motor};
            if (body_a && !body_a->is_static) {
                apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            }
            if (body_b && !body_b->is_static) {
                apply_impulse(body_b, impulse_vec, constraint->rb);
            }
        }

        constraint->last_motor_force[axis] = 0.0;
        constraint->last_limit_force[axis] = 0.0;
        constraint->last_limit_spring_force[axis] = 0.0;
    }

    if (constraint->orientation_mass > 0.0) {
        double lambda = constraint->orientation_accumulated_impulse;
        if (lambda != 0.0) {
            if (body_a && !body_a->is_static) {
                body_a->angular_velocity -= lambda * body_a->inverse_inertia;
            }
            if (body_b && !body_b->is_static) {
                body_b->angular_velocity += lambda * body_b->inverse_inertia;
            }
        }
    }
}

static void chrono_planar_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoPlanarConstraint2D_C *constraint = (ChronoPlanarConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);

    double omega_a = (body_a && !body_a->is_static) ? body_a->angular_velocity : 0.0;
    double omega_b = (body_b && !body_b->is_static) ? body_b->angular_velocity : 0.0;
    double rel_ang_vel = omega_b - omega_a;

    if (constraint->orientation_mass > 0.0) {
        double Cdot = rel_ang_vel;
        double lambda = -(Cdot + constraint->orientation_bias + constraint->softness * constraint->orientation_accumulated_impulse) * constraint->orientation_mass;
        constraint->orientation_accumulated_impulse += lambda;
        if (body_a && !body_a->is_static) {
            body_a->angular_velocity -= lambda * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angular_velocity += lambda * body_b->inverse_inertia;
        }
    }

    for (int axis = 0; axis < CHRONO_PLANAR_AXIS_COUNT; ++axis) {
        if (constraint->mass[axis] == 0.0) {
            constraint->last_motor_force[axis] = 0.0;
            constraint->last_limit_force[axis] = 0.0;
            constraint->last_limit_spring_force[axis] = 0.0;
            continue;
        }

        const double *axis_vec = constraint->axis_world[axis];
        double dv_axis = dot(dv, axis_vec);

        double target_speed = constraint->motor_speed[axis];
        if (constraint->enable_motor[axis] && constraint->motor_mode[axis] == CHRONO_PLANAR_MOTOR_POSITION) {
            double position_error = constraint->motor_position_target[axis] - constraint->translation[axis];
            target_speed = constraint->motor_position_gain[axis] * position_error - constraint->motor_position_damping[axis] * dv_axis;
        }

        if (constraint->enable_motor[axis] && constraint->motor_max_force[axis] > 0.0) {
            double Cmotor = dv_axis - target_speed;
            double lambda_motor = -Cmotor * constraint->mass[axis];
            double max_impulse = constraint->motor_max_force[axis] * constraint->cached_dt;
            double prev_impulse = constraint->motor_accumulated_impulse[axis];
            double new_impulse = prev_impulse + lambda_motor;
            if (new_impulse > max_impulse) {
                new_impulse = max_impulse;
            } else if (new_impulse < -max_impulse) {
                new_impulse = -max_impulse;
            }
            lambda_motor = new_impulse - prev_impulse;
            constraint->motor_accumulated_impulse[axis] = new_impulse;

            double impulse_vec[2] = {axis_vec[0] * lambda_motor, axis_vec[1] * lambda_motor};
            if (body_a && !body_a->is_static) {
                apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            }
            if (body_b && !body_b->is_static) {
                apply_impulse(body_b, impulse_vec, constraint->rb);
            }
            if (constraint->cached_dt > 0.0) {
                constraint->last_motor_force[axis] = lambda_motor / constraint->cached_dt;
            }
        } else {
            constraint->last_motor_force[axis] = 0.0;
        }

        if (constraint->enable_limit[axis] && constraint->limit_state[axis] != 0) {
            double lambda_limit = -(dv_axis + constraint->limit_bias[axis] + constraint->softness * constraint->limit_accumulated_impulse[axis]) * constraint->mass[axis];
            double old_impulse = constraint->limit_accumulated_impulse[axis];
            double new_impulse = old_impulse + lambda_limit;
            if (constraint->limit_state[axis] < 0) {
                if (new_impulse < 0.0) {
                    new_impulse = 0.0;
                }
            } else {
                if (new_impulse > 0.0) {
                    new_impulse = 0.0;
                }
            }
            lambda_limit = new_impulse - old_impulse;
            constraint->limit_accumulated_impulse[axis] = new_impulse;

            double impulse_vec[2] = {axis_vec[0] * lambda_limit, axis_vec[1] * lambda_limit};
            if (body_a && !body_a->is_static) {
                apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            }
            if (body_b && !body_b->is_static) {
                apply_impulse(body_b, impulse_vec, constraint->rb);
            }
            if (constraint->cached_dt > 0.0) {
                constraint->last_limit_force[axis] = lambda_limit / constraint->cached_dt;
            }
        } else {
            constraint->last_limit_force[axis] = 0.0;
        }

        if (constraint->limit_spring_stiffness[axis] > 0.0 && constraint->limit_deflection[axis] != 0.0) {
            double force = -constraint->limit_spring_stiffness[axis] * constraint->limit_deflection[axis] -
                           constraint->limit_spring_damping[axis] * dv_axis;
            double lambda_spring = force * constraint->cached_dt;
            double impulse_vec[2] = {axis_vec[0] * lambda_spring, axis_vec[1] * lambda_spring};
            if (body_a && !body_a->is_static) {
                apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
            }
            if (body_b && !body_b->is_static) {
                apply_impulse(body_b, impulse_vec, constraint->rb);
            }
            if (constraint->cached_dt > 0.0) {
                constraint->last_limit_spring_force[axis] = lambda_spring / constraint->cached_dt;
            }
        } else {
            constraint->last_limit_spring_force[axis] = 0.0;
        }
    }
}

static void chrono_planar_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoPlanarConstraint2D_C *constraint = (ChronoPlanarConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double inv_inertia_sum = 0.0;
    if (body_a && !body_a->is_static) {
        inv_inertia_sum += body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_inertia_sum += body_b->inverse_inertia;
    }

    if (inv_inertia_sum > 0.0) {
        double angle_a = body_a ? body_a->angle : 0.0;
        double angle_b = body_b ? body_b->angle : 0.0;
        double angle_error = (angle_b - angle_a) - constraint->orientation_target;
        double correction = -angle_error;
        double max_corr = constraint->max_correction;
        if (correction > max_corr) {
            correction = max_corr;
        } else if (correction < -max_corr) {
            correction = -max_corr;
        }
        double lambda = correction / inv_inertia_sum;
        if (body_a && !body_a->is_static) {
            body_a->angle -= lambda * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->angle += lambda * body_b->inverse_inertia;
        }
    }

    for (int axis = 0; axis < CHRONO_PLANAR_AXIS_COUNT; ++axis) {
        if (!constraint->enable_limit[axis] || constraint->limit_state[axis] == 0) {
            continue;
        }

        const double *axis_vec = constraint->axis_world[axis];
        double ra_cross = cross(constraint->ra, axis_vec);
        double rb_cross = cross(constraint->rb, axis_vec);
        double inv_mass = 0.0;
        if (body_a && !body_a->is_static) {
            inv_mass += body_a->inverse_mass + ra_cross * ra_cross * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            inv_mass += body_b->inverse_mass + rb_cross * rb_cross * body_b->inverse_inertia;
        }
        if (inv_mass == 0.0) {
            continue;
        }

        double correction = -constraint->limit_deflection[axis];
        double max_corr = constraint->max_correction;
        if (correction > max_corr) {
            correction = max_corr;
        } else if (correction < -max_corr) {
            correction = -max_corr;
        }

        double lambda = correction / inv_mass;
        double impulse_vec[2] = {axis_vec[0] * lambda, axis_vec[1] * lambda};

        if (body_a && !body_a->is_static) {
            body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
            body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
            body_a->angle -= ra_cross * lambda * body_a->inverse_inertia;
        }
        if (body_b && !body_b->is_static) {
            body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
            body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
            body_b->angle += rb_cross * lambda * body_b->inverse_inertia;
        }
    }

}

static void chrono_spring_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoSpringConstraint2D_C *constraint = (ChronoSpringConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }
    constraint->cached_dt = dt;
    constraint->velocity_applied = 0;

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    if (body_a) {
        double world_a[2];
        chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, world_a);
        sub(world_a, body_a->position, constraint->ra);
    } else {
        constraint->ra[0] = 0.0;
        constraint->ra[1] = 0.0;
    }
    if (body_b) {
        double world_b[2];
        chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, world_b);
        sub(world_b, body_b->position, constraint->rb);
    } else {
        constraint->rb[0] = 0.0;
        constraint->rb[1] = 0.0;
    }

    double pa[2] = {0.0, 0.0};
    double pb[2] = {0.0, 0.0};
    if (body_a) {
        add(body_a->position, constraint->ra, pa);
    }
    if (body_b) {
        add(body_b->position, constraint->rb, pb);
    }
    sub(pb, pa, constraint->direction);
    constraint->current_length = length(constraint->direction);
    if (constraint->current_length > 1e-12) {
        constraint->direction[0] /= constraint->current_length;
        constraint->direction[1] /= constraint->current_length;
    } else {
        constraint->direction[0] = 0.0;
        constraint->direction[1] = 0.0;
    }
}

static void chrono_spring_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    (void)constraint_ptr;
}

static void chrono_spring_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoSpringConstraint2D_C *constraint = (ChronoSpringConstraint2D_C *)constraint_ptr;
    if (!constraint || constraint->velocity_applied) {
        return;
    }
    if (constraint->cached_dt <= 0.0) {
        return;
    }
    if (constraint->current_length <= 1e-12) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};
    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra_x = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra_x;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb_x = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb_x;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
    double rel_vel = dot(dv, constraint->direction);
    double deflection = constraint->current_length - constraint->rest_length;

    double stiffness = constraint->stiffness;
    double damping = constraint->damping;
    if (stiffness == 0.0 && damping == 0.0) {
        constraint->velocity_applied = 1;
        return;
    }

    double force = -stiffness * deflection - damping * rel_vel;
    double impulse = force * constraint->cached_dt;
    double impulse_vec[2] = {
        constraint->direction[0] * impulse,
        constraint->direction[1] * impulse
    };

    if (body_a) {
        apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    }
    if (body_b) {
        apply_impulse(body_b, impulse_vec, constraint->rb);
    }

    constraint->velocity_applied = 1;
}

static void chrono_spring_constraint2d_solve_position_impl(void *constraint_ptr) {
    (void)constraint_ptr;
}

static void chrono_gear_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoGearConstraint2D_C *constraint = (ChronoGearConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

    double inv_inertia_a = (body_a && !body_a->is_static) ? body_a->inverse_inertia : 0.0;
    double inv_inertia_b = (body_b && !body_b->is_static) ? body_b->inverse_inertia : 0.0;
    double ratio = constraint->ratio;

    double mass = inv_inertia_a + ratio * ratio * inv_inertia_b;
    if (constraint->softness > 0.0) {
        mass += constraint->softness;
    }
    constraint->motor_mass = (mass > 0.0) ? 1.0 / mass : 0.0;

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;
    double C = angle_a + ratio * angle_b + constraint->phase;

    double beta = constraint->baumgarte_beta;
    constraint->bias = (beta > 0.0) ? -beta / dt * C : 0.0;
}

static void chrono_gear_constraint2d_apply_warm_start_impl(void *constraint_ptr) {
    ChronoGearConstraint2D_C *constraint = (ChronoGearConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;
    double ratio = constraint->ratio;
    double lambda = constraint->accumulated_impulse;

    if (body_a && !body_a->is_static) {
        body_a->angular_velocity -= lambda * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->angular_velocity -= lambda * ratio * body_b->inverse_inertia;
    }
}

static void chrono_gear_constraint2d_solve_velocity_impl(void *constraint_ptr) {
    ChronoGearConstraint2D_C *constraint = (ChronoGearConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;
    double ratio = constraint->ratio;

    double omega_a = (body_a && !body_a->is_static) ? body_a->angular_velocity : 0.0;
    double omega_b = (body_b && !body_b->is_static) ? body_b->angular_velocity : 0.0;
    double Cdot = omega_a + ratio * omega_b;

    double gamma = constraint->softness;
    double lambda = -(Cdot + constraint->bias + gamma * constraint->accumulated_impulse) * constraint->motor_mass;
    constraint->accumulated_impulse += lambda;

    if (body_a && !body_a->is_static) {
        body_a->angular_velocity -= lambda * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->angular_velocity -= lambda * ratio * body_b->inverse_inertia;
    }
}

static void chrono_gear_constraint2d_solve_position_impl(void *constraint_ptr) {
    ChronoGearConstraint2D_C *constraint = (ChronoGearConstraint2D_C *)constraint_ptr;
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;
    double ratio = constraint->ratio;

    double angle_a = body_a ? body_a->angle : 0.0;
    double angle_b = body_b ? body_b->angle : 0.0;
    double C = angle_a + ratio * angle_b + constraint->phase;

    double inv_inertia_a = (body_a && !body_a->is_static) ? body_a->inverse_inertia : 0.0;
    double inv_inertia_b = (body_b && !body_b->is_static) ? body_b->inverse_inertia : 0.0;
    double mass = inv_inertia_a + ratio * ratio * inv_inertia_b;
    if (mass == 0.0) {
        return;
    }
    double lambda = -constraint->baumgarte_beta * C / mass;

    if (body_a && !body_a->is_static) {
        body_a->angle += -lambda * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->angle += -lambda * ratio * body_b->inverse_inertia;
    }
}

void chrono_constraint2d_prepare(ChronoConstraint2DBase_C *constraint, double dt) {
    if (!constraint || !constraint->ops || !constraint->ops->prepare) {
        return;
    }
    constraint->ops->prepare((void *)constraint, dt);
}

void chrono_constraint2d_apply_warm_start(ChronoConstraint2DBase_C *constraint) {
    if (!constraint || !constraint->ops || !constraint->ops->apply_warm_start) {
        return;
    }
    constraint->ops->apply_warm_start((void *)constraint);
}

void chrono_constraint2d_solve_velocity(ChronoConstraint2DBase_C *constraint) {
    if (!constraint || !constraint->ops || !constraint->ops->solve_velocity) {
        return;
    }
    constraint->ops->solve_velocity((void *)constraint);
}

void chrono_constraint2d_solve_position(ChronoConstraint2DBase_C *constraint) {
    if (!constraint || !constraint->ops || !constraint->ops->solve_position) {
        return;
    }
    constraint->ops->solve_position((void *)constraint);
}

void chrono_constraint2d_batch_solve(ChronoConstraint2DBase_C **constraints,
                                     size_t count,
                                     double dt,
                                     const ChronoConstraint2DBatchConfig_C *config,
                                     ChronoConstraint2DBatchWorkspace_C *workspace) {
    if (!constraints || count == 0) {
        return;
    }

    ChronoConstraint2DBatchConfig_C cfg = {0};
    if (config) {
        cfg = *config;
    }
    if (cfg.velocity_iterations <= 0) {
        cfg.velocity_iterations = 10;
    }
    if (cfg.position_iterations <= 0) {
        cfg.position_iterations = 3;
    }

    int use_parallel =
#ifdef _OPENMP
        (cfg.enable_parallel && count > 1);
#else
        0;
#endif

    int *island_ids = NULL;
    size_t *island_sizes = NULL;
    size_t *island_offsets = NULL;
    size_t *ordered_indices = NULL;
    int external_workspace = 0;
    size_t island_count = 0;

    if (use_parallel) {
        if (workspace && workspace->island_ids_capacity >= count) {
            island_ids = workspace->island_ids;
            external_workspace = 1;
        } else if (workspace) {
            int *new_ids = (int *)realloc(workspace->island_ids, count * sizeof(int));
            if (!new_ids) {
                use_parallel = 0;
            } else {
                workspace->island_ids = new_ids;
                workspace->island_ids_capacity = count;
                island_ids = workspace->island_ids;
                external_workspace = 1;
            }
        } else {
            island_ids = (int *)malloc(count * sizeof(int));
            if (!island_ids) {
                use_parallel = 0;
            }
        }
    }

    if (use_parallel) {
        island_count = chrono_constraint2d_build_islands(constraints, count, island_ids);
        if (island_count <= 1) {
            use_parallel = 0;
        }
    }

    if (use_parallel) {
        size_t need_island = island_count;
        size_t need_constraints = count;

        if (workspace) {
            if (workspace->island_sizes_capacity < need_island) {
                size_t *new_sizes = (size_t *)realloc(workspace->island_sizes, sizeof(size_t) * need_island);
                if (!new_sizes) {
                    use_parallel = 0;
                } else {
                    workspace->island_sizes = new_sizes;
                    workspace->island_sizes_capacity = need_island;
                }
            }
            if (workspace->island_offsets_capacity < need_island) {
                size_t *new_offsets = (size_t *)realloc(workspace->island_offsets, sizeof(size_t) * need_island);
                if (!new_offsets) {
                    use_parallel = 0;
                } else {
                    workspace->island_offsets = new_offsets;
                    workspace->island_offsets_capacity = need_island;
                }
            }
            if (workspace->ordered_indices_capacity < need_constraints) {
                size_t *new_ordered = (size_t *)realloc(workspace->ordered_indices, sizeof(size_t) * need_constraints);
                if (!new_ordered) {
                    use_parallel = 0;
                } else {
                    workspace->ordered_indices = new_ordered;
                    workspace->ordered_indices_capacity = need_constraints;
                }
            }
        } else {
            island_sizes = (size_t *)calloc(need_island, sizeof(size_t));
            island_offsets = (size_t *)malloc(need_island * sizeof(size_t));
            ordered_indices = (size_t *)malloc(need_constraints * sizeof(size_t));
            size_t *cursor = (size_t *)calloc(need_island, sizeof(size_t));
            if (!island_sizes || !island_offsets || !ordered_indices || !cursor) {
                free(island_sizes);
                free(island_offsets);
                free(ordered_indices);
                free(cursor);
                use_parallel = 0;
            } else {
                for (size_t i = 0; i < count; ++i) {
                    int island = island_ids[i];
                    if (island >= 0) {
                        island_sizes[island]++;
                    }
                }
                size_t offset = 0;
                for (size_t island = 0; island < island_count; ++island) {
                    island_offsets[island] = offset;
                    offset += island_sizes[island];
                }
                for (size_t i = 0; i < count; ++i) {
                    int island = island_ids[i];
                    if (island >= 0) {
                        size_t pos = island_offsets[island] + cursor[island];
                        ordered_indices[pos] = i;
                        cursor[island] += 1;
                    }
                }
                free(cursor);
            }
        }
    }

    if (use_parallel && workspace) {
        if (!island_sizes) {
            island_sizes = workspace->island_sizes;
            memset(island_sizes, 0, sizeof(size_t) * workspace->island_sizes_capacity);
            for (size_t i = 0; i < count; ++i) {
                int island = island_ids[i];
                if (island >= 0) {
                    island_sizes[island]++;
                }
            }
        }
        if (!island_offsets) {
            island_offsets = workspace->island_offsets;
            size_t offset = 0;
            for (size_t island = 0; island < island_count; ++island) {
                island_offsets[island] = offset;
                offset += island_sizes[island];
            }
        }
        if (!ordered_indices) {
            ordered_indices = workspace->ordered_indices;
            memset(ordered_indices, 0, sizeof(size_t) * workspace->ordered_indices_capacity);
            memset(workspace->island_sizes, 0, sizeof(size_t) * workspace->island_sizes_capacity);
            for (size_t i = 0; i < count; ++i) {
                int island = island_ids[i];
                if (island >= 0) {
                    size_t pos = island_offsets[island] + workspace->island_sizes[island];
                    ordered_indices[pos] = i;
                    workspace->island_sizes[island]++;
                }
            }
            memcpy(workspace->island_sizes, island_sizes, sizeof(size_t) * island_count);
        }
    }

    if (!use_parallel) {
        if (!external_workspace) {
            free(island_ids);
        }
        if (!workspace) {
            free(island_sizes);
            free(island_offsets);
            free(ordered_indices);
        }

        if (dt > 0.0) {
            for (size_t i = 0; i < count; ++i) {
                chrono_constraint2d_prepare(constraints[i], dt);
            }
        }

        for (size_t i = 0; i < count; ++i) {
            chrono_constraint2d_apply_warm_start(constraints[i]);
        }

        for (int iter = 0; iter < cfg.velocity_iterations; ++iter) {
            for (size_t i = 0; i < count; ++i) {
                chrono_constraint2d_solve_velocity(constraints[i]);
            }
        }

        for (int iter = 0; iter < cfg.position_iterations; ++iter) {
            for (size_t i = 0; i < count; ++i) {
                chrono_constraint2d_solve_position(constraints[i]);
            }
        }
        return;
    }

    if (dt > 0.0) {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (size_t island = 0; island < island_count; ++island) {
            size_t start = island_offsets[island];
            size_t items = island_sizes[island];
            for (size_t k = 0; k < items; ++k) {
                size_t idx = ordered_indices[start + k];
                chrono_constraint2d_prepare(constraints[idx], dt);
            }
        }
    }

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (size_t island = 0; island < island_count; ++island) {
        size_t start = island_offsets[island];
        size_t items = island_sizes[island];
        for (size_t k = 0; k < items; ++k) {
            size_t idx = ordered_indices[start + k];
            chrono_constraint2d_apply_warm_start(constraints[idx]);
        }
    }

    for (int iter = 0; iter < cfg.velocity_iterations; ++iter) {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (size_t island = 0; island < island_count; ++island) {
            size_t start = island_offsets[island];
            size_t items = island_sizes[island];
            for (size_t k = 0; k < items; ++k) {
                size_t idx = ordered_indices[start + k];
                chrono_constraint2d_solve_velocity(constraints[idx]);
            }
        }
    }

    for (int iter = 0; iter < cfg.position_iterations; ++iter) {
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
        for (size_t island = 0; island < island_count; ++island) {
            size_t start = island_offsets[island];
            size_t items = island_sizes[island];
            for (size_t k = 0; k < items; ++k) {
                size_t idx = ordered_indices[start + k];
                chrono_constraint2d_solve_position(constraints[idx]);
            }
        }
    }

    if (!external_workspace) {
        free(island_ids);
    }
    if (!workspace) {
        free(island_sizes);
        free(island_offsets);
        free(ordered_indices);
    }
}

void chrono_distance_constraint2d_prepare(ChronoDistanceConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_distance_constraint2d_apply_warm_start(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_distance_constraint2d_solve_velocity(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_distance_constraint2d_solve_position(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_distance_angle_constraint2d_prepare(ChronoDistanceAngleConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_distance_angle_constraint2d_apply_warm_start(ChronoDistanceAngleConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_distance_angle_constraint2d_solve_velocity(ChronoDistanceAngleConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_distance_angle_constraint2d_solve_position(ChronoDistanceAngleConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_coupled_constraint2d_prepare(ChronoCoupledConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_coupled_constraint2d_apply_warm_start(ChronoCoupledConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_coupled_constraint2d_solve_velocity(ChronoCoupledConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_coupled_constraint2d_solve_position(ChronoCoupledConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_revolute_constraint2d_prepare(ChronoRevoluteConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_revolute_constraint2d_apply_warm_start(ChronoRevoluteConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_revolute_constraint2d_solve_velocity(ChronoRevoluteConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_revolute_constraint2d_solve_position(ChronoRevoluteConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_prismatic_constraint2d_prepare(ChronoPrismaticConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_prismatic_constraint2d_apply_warm_start(ChronoPrismaticConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_prismatic_constraint2d_solve_velocity(ChronoPrismaticConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_prismatic_constraint2d_solve_position(ChronoPrismaticConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_planar_constraint2d_init(ChronoPlanarConstraint2D_C *constraint,
                                     ChronoBody2D_C *body_a,
                                     ChronoBody2D_C *body_b,
                                     const double local_anchor_a[2],
                                     const double local_anchor_b[2],
                                     const double local_axis_a[2]) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_planar_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    if (local_axis_a) {
        constraint->local_axis_a[0] = local_axis_a[0];
        constraint->local_axis_a[1] = local_axis_a[1];
    } else {
        constraint->local_axis_a[0] = 1.0;
        constraint->local_axis_a[1] = 0.0;
    }
    constraint->softness = 0.0;
    constraint->baumgarte_beta = 0.2;
    constraint->slop = 1e-3;
    constraint->max_correction = 0.2;
    constraint->orientation_target = 0.0;
    for (int axis = 0; axis < CHRONO_PLANAR_AXIS_COUNT; ++axis) {
        constraint->limit_lower[axis] = -1e9;
        constraint->limit_upper[axis] = 1e9;
        constraint->enable_limit[axis] = 0;
        constraint->limit_spring_stiffness[axis] = 0.0;
        constraint->limit_spring_damping[axis] = 0.0;
        constraint->limit_accumulated_impulse[axis] = 0.0;
        constraint->motor_speed[axis] = 0.0;
        constraint->motor_max_force[axis] = 0.0;
        constraint->enable_motor[axis] = 0;
        constraint->motor_mode[axis] = CHRONO_PLANAR_MOTOR_VELOCITY;
        constraint->motor_position_target[axis] = 0.0;
        constraint->motor_position_gain[axis] = 0.0;
        constraint->motor_position_damping[axis] = 0.0;
        constraint->motor_accumulated_impulse[axis] = 0.0;
        constraint->last_motor_force[axis] = 0.0;
        constraint->last_limit_force[axis] = 0.0;
        constraint->last_limit_spring_force[axis] = 0.0;
    }
    constraint->orientation_accumulated_impulse = 0.0;
}

void chrono_planar_constraint2d_set_axes(ChronoPlanarConstraint2D_C *constraint,
                                         const double local_axis_a[2]) {
    if (!constraint) {
        return;
    }
    if (local_axis_a) {
        constraint->local_axis_a[0] = local_axis_a[0];
        constraint->local_axis_a[1] = local_axis_a[1];
    }
}

void chrono_planar_constraint2d_set_baumgarte(ChronoPlanarConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_planar_constraint2d_set_softness(ChronoPlanarConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_planar_constraint2d_set_slop(ChronoPlanarConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_planar_constraint2d_set_max_correction(ChronoPlanarConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

void chrono_planar_constraint2d_enable_limit(ChronoPlanarConstraint2D_C *constraint,
                                             int axis,
                                             int enable,
                                             double lower,
                                             double upper) {
    if (!constraint || axis < 0 || axis >= CHRONO_PLANAR_AXIS_COUNT) {
        return;
    }
    constraint->enable_limit[axis] = enable ? 1 : 0;
    if (lower > upper) {
        double tmp = lower;
        lower = upper;
        upper = tmp;
    }
    constraint->limit_lower[axis] = lower;
    constraint->limit_upper[axis] = upper;
    if (!constraint->enable_limit[axis]) {
        constraint->limit_accumulated_impulse[axis] = 0.0;
    }
}

void chrono_planar_constraint2d_set_limit_spring(ChronoPlanarConstraint2D_C *constraint,
                                                 int axis,
                                                 double stiffness,
                                                 double damping) {
    if (!constraint || axis < 0 || axis >= CHRONO_PLANAR_AXIS_COUNT) {
        return;
    }
    constraint->limit_spring_stiffness[axis] = (stiffness > 0.0) ? stiffness : 0.0;
    constraint->limit_spring_damping[axis] = (damping > 0.0) ? damping : 0.0;
}

void chrono_planar_constraint2d_enable_motor(ChronoPlanarConstraint2D_C *constraint,
                                             int axis,
                                             int enable,
                                             double speed,
                                             double max_force) {
    if (!constraint || axis < 0 || axis >= CHRONO_PLANAR_AXIS_COUNT) {
        return;
    }
    constraint->enable_motor[axis] = enable ? 1 : 0;
    constraint->motor_mode[axis] = CHRONO_PLANAR_MOTOR_VELOCITY;
    constraint->motor_speed[axis] = speed;
    constraint->motor_max_force[axis] = (max_force >= 0.0) ? max_force : 0.0;
    if (!constraint->enable_motor[axis]) {
        constraint->motor_accumulated_impulse[axis] = 0.0;
    }
}

void chrono_planar_constraint2d_set_motor_position_target(ChronoPlanarConstraint2D_C *constraint,
                                                          int axis,
                                                          double target,
                                                          double proportional_gain,
                                                          double damping_gain) {
    if (!constraint || axis < 0 || axis >= CHRONO_PLANAR_AXIS_COUNT) {
        return;
    }
    constraint->enable_motor[axis] = 1;
    constraint->motor_mode[axis] = CHRONO_PLANAR_MOTOR_POSITION;
    constraint->motor_position_target[axis] = target;
    constraint->motor_position_gain[axis] = (proportional_gain > 0.0) ? proportional_gain : 0.0;
    constraint->motor_position_damping[axis] = (damping_gain > 0.0) ? damping_gain : 0.0;
}

void chrono_planar_constraint2d_set_orientation_target(ChronoPlanarConstraint2D_C *constraint, double target_angle) {
    if (!constraint) {
        return;
    }
    constraint->orientation_target = target_angle;
}

void chrono_planar_constraint2d_prepare(ChronoPlanarConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_planar_constraint2d_apply_warm_start(ChronoPlanarConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_planar_constraint2d_solve_velocity(ChronoPlanarConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_planar_constraint2d_solve_position(ChronoPlanarConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_gear_constraint2d_init(ChronoGearConstraint2D_C *constraint,
                                   ChronoBody2D_C *body_a,
                                   ChronoBody2D_C *body_b,
                                   double ratio,
                                   double phase) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_gear_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    constraint->ratio = ratio;
    constraint->phase = phase;
    constraint->softness = 0.0;
    constraint->baumgarte_beta = 0.2;
    constraint->bias = 0.0;
    constraint->motor_mass = 0.0;
    constraint->accumulated_impulse = 0.0;
}

void chrono_gear_constraint2d_set_ratio(ChronoGearConstraint2D_C *constraint, double ratio) {
    if (!constraint) {
        return;
    }
    constraint->ratio = ratio;
}

void chrono_gear_constraint2d_set_phase(ChronoGearConstraint2D_C *constraint, double phase) {
    if (!constraint) {
        return;
    }
    constraint->phase = phase;
}

void chrono_gear_constraint2d_set_baumgarte(ChronoGearConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_gear_constraint2d_set_softness(ChronoGearConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_gear_constraint2d_prepare(ChronoGearConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_gear_constraint2d_apply_warm_start(ChronoGearConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_gear_constraint2d_solve_velocity(ChronoGearConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_gear_constraint2d_solve_position(ChronoGearConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_spring_constraint2d_init(ChronoSpringConstraint2D_C *constraint,
                                     ChronoBody2D_C *body_a,
                                     ChronoBody2D_C *body_b,
                                     const double local_anchor_a[2],
                                     const double local_anchor_b[2],
                                     double rest_length,
                                     double stiffness,
                                     double damping) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_spring_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    constraint->rest_length = rest_length;
    constraint->stiffness = stiffness;
    constraint->damping = damping;
    constraint->cached_dt = 0.0;
    constraint->velocity_applied = 0;
}

void chrono_spring_constraint2d_set_rest_length(ChronoSpringConstraint2D_C *constraint, double rest_length) {
    if (!constraint) {
        return;
    }
    constraint->rest_length = rest_length;
}

void chrono_spring_constraint2d_set_stiffness(ChronoSpringConstraint2D_C *constraint, double stiffness) {
    if (!constraint) {
        return;
    }
    constraint->stiffness = stiffness;
}

void chrono_spring_constraint2d_set_damping(ChronoSpringConstraint2D_C *constraint, double damping) {
    if (!constraint) {
        return;
    }
    constraint->damping = damping;
}

void chrono_spring_constraint2d_prepare(ChronoSpringConstraint2D_C *constraint, double dt) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_prepare(&constraint->base, dt);
}

void chrono_spring_constraint2d_apply_warm_start(ChronoSpringConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_apply_warm_start(&constraint->base);
}

void chrono_spring_constraint2d_solve_velocity(ChronoSpringConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_velocity(&constraint->base);
}

void chrono_spring_constraint2d_solve_position(ChronoSpringConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    chrono_constraint2d_solve_position(&constraint->base);
}

void chrono_prismatic_constraint2d_init(ChronoPrismaticConstraint2D_C *constraint,
                                        ChronoBody2D_C *body_a,
                                        ChronoBody2D_C *body_b,
                                        const double local_anchor_a[2],
                                        const double local_anchor_b[2],
                                        const double local_axis_a[2]) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->base.ops = &chrono_prismatic_constraint2d_ops;
    constraint->base.body_a = body_a;
    constraint->base.body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    if (local_axis_a) {
        constraint->local_axis_a[0] = local_axis_a[0];
        constraint->local_axis_a[1] = local_axis_a[1];
    } else {
        constraint->local_axis_a[0] = 1.0;
        constraint->local_axis_a[1] = 0.0;
    }
    normalize(constraint->local_axis_a);
    constraint->softness = 0.0;
    constraint->baumgarte_beta = 0.2;
    constraint->slop = 1e-3;
    constraint->max_correction = 0.2;
    constraint->limit_lower = -1.0;
    constraint->limit_upper = 1.0;
    constraint->enable_limit = 0;
    constraint->limit_spring_stiffness = 0.0;
    constraint->limit_spring_damping = 0.0;
    constraint->motor_speed = 0.0;
    constraint->motor_max_force = 0.0;
    constraint->enable_motor = 0;
    constraint->motor_mode = CHRONO_PRISMATIC_MOTOR_VELOCITY;
    constraint->motor_position_target = 0.0;
    constraint->motor_position_gain = 0.0;
    constraint->motor_position_damping = 0.0;
    constraint->accumulated_motor_impulse = 0.0;
    constraint->limit_accumulated_impulse = 0.0;
    constraint->last_motor_force = 0.0;
    constraint->last_limit_force = 0.0;
    constraint->last_limit_spring_force = 0.0;
}

void chrono_prismatic_constraint2d_set_axis(ChronoPrismaticConstraint2D_C *constraint,
                                            const double local_axis_a[2]) {
    if (!constraint) {
        return;
    }
    if (local_axis_a) {
        constraint->local_axis_a[0] = local_axis_a[0];
        constraint->local_axis_a[1] = local_axis_a[1];
    } else {
        constraint->local_axis_a[0] = 1.0;
        constraint->local_axis_a[1] = 0.0;
    }
    normalize(constraint->local_axis_a);
}

void chrono_prismatic_constraint2d_set_baumgarte(ChronoPrismaticConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_prismatic_constraint2d_set_softness(ChronoPrismaticConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_prismatic_constraint2d_set_slop(ChronoPrismaticConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_prismatic_constraint2d_set_max_correction(ChronoPrismaticConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

void chrono_prismatic_constraint2d_enable_limit(ChronoPrismaticConstraint2D_C *constraint,
                                               int enable,
                                               double lower,
                                               double upper) {
    if (!constraint) {
        return;
    }
    constraint->enable_limit = enable ? 1 : 0;
    if (lower > upper) {
        double tmp = lower;
        lower = upper;
        upper = tmp;
    }
    constraint->limit_lower = lower;
    constraint->limit_upper = upper;
}

void chrono_prismatic_constraint2d_enable_motor(ChronoPrismaticConstraint2D_C *constraint,
                                               int enable,
                                               double speed,
                                               double max_force) {
    if (!constraint) {
        return;
    }
    constraint->enable_motor = enable ? 1 : 0;
    constraint->motor_mode = CHRONO_PRISMATIC_MOTOR_VELOCITY;
    constraint->motor_speed = speed;
    constraint->motor_max_force = (max_force >= 0.0) ? max_force : 0.0;
    if (!constraint->enable_motor) {
        constraint->accumulated_motor_impulse = 0.0;
    }
}

void chrono_prismatic_constraint2d_set_limit_spring(ChronoPrismaticConstraint2D_C *constraint,
                                                    double stiffness,
                                                    double damping) {
    if (!constraint) {
        return;
    }
    constraint->limit_spring_stiffness = (stiffness > 0.0) ? stiffness : 0.0;
    constraint->limit_spring_damping = (damping > 0.0) ? damping : 0.0;
}

void chrono_prismatic_constraint2d_set_motor_position_target(ChronoPrismaticConstraint2D_C *constraint,
                                                             double target_position,
                                                             double proportional_gain,
                                                             double damping_gain) {
    if (!constraint) {
        return;
    }
    constraint->enable_motor = 1;
    constraint->motor_mode = CHRONO_PRISMATIC_MOTOR_POSITION;
    constraint->motor_position_target = target_position;
    constraint->motor_position_gain = (proportional_gain > 0.0) ? proportional_gain : 0.0;
    constraint->motor_position_damping = (damping_gain > 0.0) ? damping_gain : 0.0;
}
