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

static void chrono_distance_constraint2d_prepare_impl(void *constraint_ptr, double dt);
static void chrono_distance_constraint2d_apply_warm_start_impl(void *constraint_ptr);
static void chrono_distance_constraint2d_solve_velocity_impl(void *constraint_ptr);
static void chrono_distance_constraint2d_solve_position_impl(void *constraint_ptr);

static const ChronoConstraint2DOps_C chrono_distance_constraint2d_ops = {
    chrono_distance_constraint2d_prepare_impl,
    chrono_distance_constraint2d_apply_warm_start_impl,
    chrono_distance_constraint2d_solve_velocity_impl,
    chrono_distance_constraint2d_solve_position_impl
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
    constraint->softness = 0.0;
    constraint->slop = 0.001;
    constraint->max_correction = 0.2;
    constraint->bias = 0.0;
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
    constraint->softness = softness;
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

static void chrono_distance_constraint2d_prepare_impl(void *constraint_ptr, double dt) {
    ChronoDistanceConstraint2D_C *constraint = (ChronoDistanceConstraint2D_C *)constraint_ptr;
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->base.body_a;
    ChronoBody2D_C *body_b = constraint->base.body_b;

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

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    if (inv_mass > 0.0) {
        constraint->base.effective_mass = 1.0 / inv_mass;
    } else {
        constraint->base.effective_mass = 0.0;
    }

    double C = dist - constraint->rest_length;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;

    double beta = constraint->baumgarte_beta;
    double bias = 0.0;
    if (beta > 0.0) {
        bias = -beta / dt * error;
    }
    constraint->bias = bias;

    if (constraint->softness > 0.0) {
        constraint->base.effective_mass = 1.0 / (inv_mass + constraint->softness);
    }
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

    double gamma = constraint->softness;
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
