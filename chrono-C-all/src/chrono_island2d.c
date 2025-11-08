#include "../include/chrono_island2d.h"
#include "../include/chrono_logging.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifndef _OPENMP
static int g_tbb_stub_warned = 0;
#endif

typedef struct ChronoIsland2DBodyMapEntry {
    ChronoBody2D_C *key;
    int value;
} ChronoIsland2DBodyMapEntry;

static size_t chrono_island2d_hash_body(const ChronoBody2D_C *body) {
    uintptr_t ptr = (uintptr_t)body;
    return (ptr >> 4) ^ (ptr >> 9);
}

static int chrono_island2d_find_root(int *parent, int idx) {
    while (parent[idx] != idx) {
        parent[idx] = parent[parent[idx]];
        idx = parent[idx];
    }
    return idx;
}

static void chrono_island2d_union(int *parent, int *rank, int a, int b) {
    if (a < 0 || b < 0) {
        return;
    }
    int root_a = chrono_island2d_find_root(parent, a);
    int root_b = chrono_island2d_find_root(parent, b);
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

static int chrono_island2d_body_lookup_or_add(ChronoIsland2DBodyMapEntry *map,
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
    size_t idx = chrono_island2d_hash_body(body) & mask;
    while (1) {
        ChronoIsland2DBodyMapEntry *entry = &map[idx];
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

static double *chrono_island2d_workspace_acquire_vectors(ChronoIslandVectorBuffer_C *buffer,
                                                         size_t count,
                                                         size_t vector_length) {
    if (!buffer || vector_length == 0) {
        return NULL;
    }
    if (buffer->vector_length != vector_length) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->capacity = 0;
        buffer->vector_length = vector_length;
    }
    size_t required = count * vector_length;
    if (required == 0) {
        return buffer->data;
    }
    if (buffer->capacity < required) {
        double *new_data = (double *)realloc(buffer->data, required * sizeof(double));
        if (!new_data) {
            return NULL;
        }
        size_t previous = buffer->capacity;
        if (required > previous) {
            memset(new_data + previous, 0, (required - previous) * sizeof(double));
        }
        buffer->data = new_data;
        buffer->capacity = required;
    }
    return buffer->data;
}

void chrono_island2d_workspace_init(ChronoIsland2DWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    memset(workspace, 0, sizeof(*workspace));
}

void chrono_island2d_workspace_reset(ChronoIsland2DWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    workspace->island_count = 0;
    if (workspace->islands) {
        for (size_t i = 0; i < workspace->island_capacity; ++i) {
            workspace->islands[i].constraints = NULL;
            workspace->islands[i].constraint_count = 0;
            workspace->islands[i].contacts = NULL;
            workspace->islands[i].contact_count = 0;
        }
    }
    if (workspace->constraint_ptrs) {
        memset(workspace->constraint_ptrs, 0, workspace->constraint_ptr_capacity * sizeof(*workspace->constraint_ptrs));
    }
    if (workspace->contact_ptrs) {
        memset(workspace->contact_ptrs, 0, workspace->contact_ptr_capacity * sizeof(*workspace->contact_ptrs));
    }
    if (workspace->constraint_island_ids) {
        memset(workspace->constraint_island_ids, 0, workspace->constraint_island_ids_capacity * sizeof(*workspace->constraint_island_ids));
    }
    if (workspace->contact_island_ids) {
        memset(workspace->contact_island_ids, 0, workspace->contact_island_ids_capacity * sizeof(*workspace->contact_island_ids));
    }
    if (workspace->constraint_offsets) {
        memset(workspace->constraint_offsets, 0, workspace->constraint_offsets_capacity * sizeof(*workspace->constraint_offsets));
    }
    if (workspace->contact_offsets) {
        memset(workspace->contact_offsets, 0, workspace->contact_offsets_capacity * sizeof(*workspace->contact_offsets));
    }
    if (workspace->constraint_body_indices) {
        memset(workspace->constraint_body_indices, 0, workspace->constraint_body_indices_capacity * sizeof(*workspace->constraint_body_indices));
    }
    if (workspace->contact_body_indices) {
        memset(workspace->contact_body_indices, 0, workspace->contact_body_indices_capacity * sizeof(*workspace->contact_body_indices));
    }
    if (workspace->body_nodes) {
        memset(workspace->body_nodes, 0, workspace->body_nodes_capacity * sizeof(*workspace->body_nodes));
    }
    if (workspace->parent) {
        memset(workspace->parent, 0, workspace->parent_capacity * sizeof(*workspace->parent));
    }
    if (workspace->rank) {
        memset(workspace->rank, 0, workspace->rank_capacity * sizeof(*workspace->rank));
    }
    if (workspace->body_map) {
        memset(workspace->body_map, 0, workspace->body_map_capacity * sizeof(*workspace->body_map));
    }
    if (workspace->constraint_vectors.data && workspace->constraint_vectors.capacity > 0) {
        memset(workspace->constraint_vectors.data,
               0,
               workspace->constraint_vectors.capacity * sizeof(double));
    }
    if (workspace->contact_vectors.data && workspace->contact_vectors.capacity > 0) {
        memset(workspace->contact_vectors.data,
               0,
               workspace->contact_vectors.capacity * sizeof(double));
    }
}

void chrono_island2d_workspace_free(ChronoIsland2DWorkspace_C *workspace) {
    if (!workspace) {
        return;
    }
    free(workspace->islands);
    free(workspace->constraint_ptrs);
    free(workspace->contact_ptrs);
    free(workspace->constraint_island_ids);
    free(workspace->contact_island_ids);
    free(workspace->constraint_offsets);
    free(workspace->contact_offsets);
    free(workspace->constraint_body_indices);
    free(workspace->contact_body_indices);
    free(workspace->body_nodes);
    free(workspace->parent);
    free(workspace->rank);
    free(workspace->body_map);
    free(workspace->constraint_vectors.data);
    free(workspace->contact_vectors.data);
    chrono_island2d_workspace_init(workspace);
}

double *chrono_island2d_workspace_get_constraint_vectors(ChronoIsland2DWorkspace_C *workspace,
                                                         size_t count,
                                                         size_t vector_length) {
    if (!workspace) {
        return NULL;
    }
    return chrono_island2d_workspace_acquire_vectors(&workspace->constraint_vectors, count, vector_length);
}

double *chrono_island2d_workspace_get_contact_vectors(ChronoIsland2DWorkspace_C *workspace,
                                                      size_t count,
                                                      size_t vector_length) {
    if (!workspace) {
        return NULL;
    }
    return chrono_island2d_workspace_acquire_vectors(&workspace->contact_vectors, count, vector_length);
}

static int chrono_island2d_ensure_capacity(void **ptr,
                                           size_t *current_capacity,
                                           size_t required,
                                           size_t element_size) {
    if (required == 0) {
        return 1;
    }
    if (*current_capacity >= required) {
        return 1;
    }
    size_t new_capacity = *current_capacity == 0 ? required : *current_capacity;
    while (new_capacity < required) {
        new_capacity *= 2;
        if (new_capacity == 0) {
            new_capacity = required;
            break;
        }
    }
    void *new_ptr = realloc(*ptr, new_capacity * element_size);
    if (!new_ptr) {
        return 0;
    }
    memset((unsigned char *)new_ptr + (*current_capacity * element_size), 0, (new_capacity - *current_capacity) * element_size);
    *ptr = new_ptr;
    *current_capacity = new_capacity;
    return 1;
}

size_t chrono_island2d_build(ChronoConstraint2DBase_C **constraints,
                             size_t constraint_count,
                             ChronoContactPair2D_C *contact_pairs,
                             size_t contact_count,
                             ChronoIsland2DWorkspace_C *workspace) {
    if (!workspace) {
        return 0;
    }

    chrono_island2d_workspace_reset(workspace);

    size_t total_edges = constraint_count + contact_count;
    if (total_edges == 0) {
        return 0;
    }

    size_t max_bodies = total_edges * 2;
    if (max_bodies == 0) {
        return 0;
    }

    size_t map_capacity = 1;
    while (map_capacity < max_bodies * 2) {
        map_capacity <<= 1;
    }
    if (map_capacity < 8) {
        map_capacity = 8;
    }

    if (!chrono_island2d_ensure_capacity((void **)&workspace->body_map,
                                         &workspace->body_map_capacity,
                                         map_capacity,
                                         sizeof(*workspace->body_map))) {
        return 0;
    }
    memset(workspace->body_map, 0, workspace->body_map_capacity * sizeof(*workspace->body_map));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->body_nodes,
                                         &workspace->body_nodes_capacity,
                                         max_bodies,
                                         sizeof(*workspace->body_nodes))) {
        return 0;
    }
    if (!chrono_island2d_ensure_capacity((void **)&workspace->parent,
                                         &workspace->parent_capacity,
                                         max_bodies,
                                         sizeof(*workspace->parent))) {
        return 0;
    }
    if (!chrono_island2d_ensure_capacity((void **)&workspace->rank,
                                         &workspace->rank_capacity,
                                         max_bodies,
                                         sizeof(*workspace->rank))) {
        return 0;
    }
    memset(workspace->body_nodes, 0, workspace->body_nodes_capacity * sizeof(*workspace->body_nodes));
    memset(workspace->parent, 0, workspace->parent_capacity * sizeof(*workspace->parent));
    memset(workspace->rank, 0, workspace->rank_capacity * sizeof(*workspace->rank));

    size_t constraint_indices_required = constraint_count * 2;
    if (!chrono_island2d_ensure_capacity((void **)&workspace->constraint_body_indices,
                                         &workspace->constraint_body_indices_capacity,
                                         constraint_indices_required,
                                         sizeof(*workspace->constraint_body_indices))) {
        return 0;
    }
    memset(workspace->constraint_body_indices,
           0,
           workspace->constraint_body_indices_capacity * sizeof(*workspace->constraint_body_indices));

    size_t contact_indices_required = contact_count * 2;
    if (!chrono_island2d_ensure_capacity((void **)&workspace->contact_body_indices,
                                         &workspace->contact_body_indices_capacity,
                                         contact_indices_required,
                                         sizeof(*workspace->contact_body_indices))) {
        return 0;
    }
    memset(workspace->contact_body_indices,
           0,
           workspace->contact_body_indices_capacity * sizeof(*workspace->contact_body_indices));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->constraint_island_ids,
                                         &workspace->constraint_island_ids_capacity,
                                         constraint_count,
                                         sizeof(*workspace->constraint_island_ids))) {
        return 0;
    }
    memset(workspace->constraint_island_ids,
           0,
           workspace->constraint_island_ids_capacity * sizeof(*workspace->constraint_island_ids));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->contact_island_ids,
                                         &workspace->contact_island_ids_capacity,
                                         contact_count,
                                         sizeof(*workspace->contact_island_ids))) {
        return 0;
    }
    memset(workspace->contact_island_ids,
           0,
           workspace->contact_island_ids_capacity * sizeof(*workspace->contact_island_ids));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->constraint_ptrs,
                                         &workspace->constraint_ptr_capacity,
                                         constraint_count,
                                         sizeof(*workspace->constraint_ptrs))) {
        return 0;
    }
    memset(workspace->constraint_ptrs,
           0,
           workspace->constraint_ptr_capacity * sizeof(*workspace->constraint_ptrs));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->contact_ptrs,
                                         &workspace->contact_ptr_capacity,
                                         contact_count,
                                         sizeof(*workspace->contact_ptrs))) {
        return 0;
    }
    memset(workspace->contact_ptrs,
           0,
           workspace->contact_ptr_capacity * sizeof(*workspace->contact_ptrs));

    size_t island_capacity_required = constraint_count + contact_count;
    if (!chrono_island2d_ensure_capacity((void **)&workspace->islands,
                                         &workspace->island_capacity,
                                         island_capacity_required > 0 ? island_capacity_required : 1,
                                         sizeof(*workspace->islands))) {
        return 0;
    }
    for (size_t i = 0; i < workspace->island_capacity; ++i) {
        workspace->islands[i].constraints = NULL;
        workspace->islands[i].constraint_count = 0;
        workspace->islands[i].contacts = NULL;
        workspace->islands[i].contact_count = 0;
    }

    size_t offsets_required = island_capacity_required > 0 ? island_capacity_required : 1;
    if (!chrono_island2d_ensure_capacity((void **)&workspace->constraint_offsets,
                                         &workspace->constraint_offsets_capacity,
                                         offsets_required,
                                         sizeof(*workspace->constraint_offsets))) {
        return 0;
    }
    memset(workspace->constraint_offsets,
           0,
           workspace->constraint_offsets_capacity * sizeof(*workspace->constraint_offsets));

    if (!chrono_island2d_ensure_capacity((void **)&workspace->contact_offsets,
                                         &workspace->contact_offsets_capacity,
                                         offsets_required,
                                         sizeof(*workspace->contact_offsets))) {
        return 0;
    }
    memset(workspace->contact_offsets,
           0,
           workspace->contact_offsets_capacity * sizeof(*workspace->contact_offsets));

    size_t unique_bodies = 0;
    ChronoIsland2DBodyMapEntry *map = workspace->body_map;
    size_t mask = workspace->body_map_capacity - 1;

    for (size_t i = 0; i < constraint_count; ++i) {
        ChronoConstraint2DBase_C *constraint = constraints ? constraints[i] : NULL;
        ChronoBody2D_C *body_a = constraint ? constraint->body_a : NULL;
        ChronoBody2D_C *body_b = constraint ? constraint->body_b : NULL;

        int idx_a = chrono_island2d_body_lookup_or_add(map,
                                                       mask,
                                                       workspace->body_nodes,
                                                       workspace->parent,
                                                       workspace->rank,
                                                       &unique_bodies,
                                                       body_a);
        int idx_b = chrono_island2d_body_lookup_or_add(map,
                                                       mask,
                                                       workspace->body_nodes,
                                                       workspace->parent,
                                                       workspace->rank,
                                                       &unique_bodies,
                                                       body_b);

        workspace->constraint_body_indices[2 * i] = idx_a;
        workspace->constraint_body_indices[2 * i + 1] = idx_b;

        if (idx_a >= 0 && idx_b >= 0) {
            chrono_island2d_union(workspace->parent, workspace->rank, idx_a, idx_b);
        }
    }

    for (size_t i = 0; i < contact_count; ++i) {
        ChronoContactPair2D_C *pair = contact_pairs ? &contact_pairs[i] : NULL;
        ChronoBody2D_C *body_a = pair ? pair->body_a : NULL;
        ChronoBody2D_C *body_b = pair ? pair->body_b : NULL;

        int idx_a = chrono_island2d_body_lookup_or_add(map,
                                                       mask,
                                                       workspace->body_nodes,
                                                       workspace->parent,
                                                       workspace->rank,
                                                       &unique_bodies,
                                                       body_a);
        int idx_b = chrono_island2d_body_lookup_or_add(map,
                                                       mask,
                                                       workspace->body_nodes,
                                                       workspace->parent,
                                                       workspace->rank,
                                                       &unique_bodies,
                                                       body_b);

        workspace->contact_body_indices[2 * i] = idx_a;
        workspace->contact_body_indices[2 * i + 1] = idx_b;

        if (idx_a >= 0 && idx_b >= 0) {
            chrono_island2d_union(workspace->parent, workspace->rank, idx_a, idx_b);
        }
    }

    int *root_to_island = NULL;
    if (unique_bodies > 0) {
        root_to_island = (int *)malloc(unique_bodies * sizeof(int));
        if (!root_to_island) {
            return 0;
        }
        for (size_t i = 0; i < unique_bodies; ++i) {
            root_to_island[i] = -1;
        }
    }

    size_t island_count = 0;
    for (size_t i = 0; i < constraint_count; ++i) {
        int idx_a = workspace->constraint_body_indices[2 * i];
        int idx_b = workspace->constraint_body_indices[2 * i + 1];
        int first_idx = idx_a >= 0 ? idx_a : idx_b;
        int island_id = -1;
        if (first_idx >= 0 && root_to_island) {
            int root = chrono_island2d_find_root(workspace->parent, first_idx);
            if (root_to_island[root] < 0) {
                root_to_island[root] = (int)island_count;
                island_count++;
            }
            island_id = root_to_island[root];
        } else {
            island_id = (int)island_count;
            island_count++;
        }
        workspace->constraint_island_ids[i] = island_id;
    }

    for (size_t i = 0; i < contact_count; ++i) {
        int idx_a = workspace->contact_body_indices[2 * i];
        int idx_b = workspace->contact_body_indices[2 * i + 1];
        int first_idx = idx_a >= 0 ? idx_a : idx_b;
        int island_id = -1;
        if (first_idx >= 0 && root_to_island) {
            int root = chrono_island2d_find_root(workspace->parent, first_idx);
            if (root_to_island[root] < 0) {
                root_to_island[root] = (int)island_count;
                island_count++;
            }
            island_id = root_to_island[root];
        } else {
            island_id = (int)island_count;
            island_count++;
        }
        workspace->contact_island_ids[i] = island_id;
    }

    free(root_to_island);

    if (island_count == 0) {
        workspace->island_count = 0;
        return 0;
    }

    if (island_count > workspace->island_capacity) {
        if (!chrono_island2d_ensure_capacity((void **)&workspace->islands,
                                             &workspace->island_capacity,
                                             island_count,
                                             sizeof(*workspace->islands))) {
            workspace->island_count = 0;
            return 0;
        }
    }

    for (size_t i = 0; i < island_count; ++i) {
        workspace->islands[i].constraints = NULL;
        workspace->islands[i].constraint_count = 0;
        workspace->islands[i].contacts = NULL;
        workspace->islands[i].contact_count = 0;
        workspace->constraint_offsets[i] = 0;
        workspace->contact_offsets[i] = 0;
    }

    for (size_t i = island_count; i < workspace->constraint_offsets_capacity; ++i) {
        workspace->constraint_offsets[i] = 0;
    }
    for (size_t i = island_count; i < workspace->contact_offsets_capacity; ++i) {
        workspace->contact_offsets[i] = 0;
    }

    for (size_t i = 0; i < constraint_count; ++i) {
        int island_id = workspace->constraint_island_ids[i];
        if (island_id >= 0) {
            workspace->constraint_offsets[island_id] += 1;
        }
    }
    for (size_t i = 0; i < contact_count; ++i) {
        int island_id = workspace->contact_island_ids[i];
        if (island_id >= 0) {
            workspace->contact_offsets[island_id] += 1;
        }
    }

    size_t constraint_cursor = 0;
    size_t contact_cursor = 0;
    for (size_t i = 0; i < island_count; ++i) {
        size_t constraint_items = workspace->constraint_offsets[i];
        size_t contact_items = workspace->contact_offsets[i];

        workspace->islands[i].constraint_count = constraint_items;
        workspace->islands[i].contact_count = contact_items;

        if (constraint_items > 0) {
            workspace->islands[i].constraints = &workspace->constraint_ptrs[constraint_cursor];
        } else {
            workspace->islands[i].constraints = NULL;
        }
        if (contact_items > 0) {
            workspace->islands[i].contacts = &workspace->contact_ptrs[contact_cursor];
        } else {
            workspace->islands[i].contacts = NULL;
        }
        constraint_cursor += constraint_items;
        contact_cursor += contact_items;

        workspace->constraint_offsets[i] = 0;
        workspace->contact_offsets[i] = 0;
    }

    for (size_t i = 0; i < constraint_count; ++i) {
        int island_id = workspace->constraint_island_ids[i];
        if (island_id < 0) {
            continue;
        }
        ChronoIsland2D_C *island = &workspace->islands[island_id];
        size_t write_index = workspace->constraint_offsets[island_id]++;
        island->constraints[write_index] = constraints ? constraints[i] : NULL;
    }

    for (size_t i = 0; i < contact_count; ++i) {
        int island_id = workspace->contact_island_ids[i];
        if (island_id < 0) {
            continue;
        }
        ChronoIsland2D_C *island = &workspace->islands[island_id];
        size_t write_index = workspace->contact_offsets[island_id]++;
        island->contacts[write_index] = contact_pairs ? &contact_pairs[i] : NULL;
    }

    workspace->island_count = island_count;

    for (size_t i = 0; i < island_count; ++i) {
        workspace->constraint_offsets[i] = 0;
        workspace->contact_offsets[i] = 0;
    }

    return island_count;
}

void chrono_island2d_solve(const ChronoIsland2DWorkspace_C *workspace,
                           double dt,
                           const ChronoIsland2DSolveConfig_C *config) {
    if (!workspace || workspace->island_count == 0) {
        return;
    }

    ChronoConstraint2DBatchConfig_C constraint_cfg = {0};
    int enable_parallel = 0;
    ChronoIslandSchedulerBackend_C scheduler = CHRONO_ISLAND_SCHED_AUTO;

    if (config) {
        constraint_cfg = config->constraint_config;
        enable_parallel = config->enable_parallel;
        scheduler = config->scheduler;
    }

    if (constraint_cfg.velocity_iterations <= 0) {
        constraint_cfg.velocity_iterations = 10;
    }
    if (constraint_cfg.position_iterations <= 0) {
        constraint_cfg.position_iterations = 3;
    }
    constraint_cfg.enable_parallel = 0;

    int prefer_tbb = (scheduler == CHRONO_ISLAND_SCHED_TBB);
    int use_parallel =
#ifdef _OPENMP
        (!prefer_tbb && enable_parallel && workspace->island_count > 1);
#else
        (void)enable_parallel, 0;
#endif
    int use_tbb_backend = (prefer_tbb && enable_parallel && workspace->island_count > 1);

    if (use_tbb_backend) {
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
        for (size_t island_idx = 0; island_idx < workspace->island_count; ++island_idx) {
            ChronoIsland2D_C *island = &workspace->islands[island_idx];
            if (island->constraint_count > 0) {
                chrono_constraint2d_batch_solve(island->constraints,
                                                island->constraint_count,
                                                dt,
                                                &constraint_cfg,
                                                NULL);
            }
            if (island->contact_count > 0) {
                for (size_t i = 0; i < island->contact_count; ++i) {
                    ChronoContactPair2D_C *pair = island->contacts[i];
                    if (pair) {
                        chrono_contact_manager2d_end_step((ChronoContactManager2D_C *)pair);
                    }
                }
            }
        }
#ifndef _OPENMP
        if (!g_tbb_stub_warned) {
            chrono_log_write(CHRONO_LOG_LEVEL_INFO,
                             CHRONO_LOG_CATEGORY_SOLVER,
                             "TBB backend requested but OpenMP is unavailable; ran islands serially.");
            g_tbb_stub_warned = 1;
        }
#endif
        return;
    }

#ifdef _OPENMP
#pragma omp parallel for schedule(static) if (use_parallel)
#endif
    for (size_t island_idx = 0; island_idx < workspace->island_count; ++island_idx) {
        ChronoIsland2D_C *island = &workspace->islands[island_idx];

        if (island->constraint_count > 0) {
            chrono_constraint2d_batch_solve(island->constraints,
                                            island->constraint_count,
                                            dt,
                                            &constraint_cfg,
                                            NULL);
        }

        if (island->contact_count > 0) {
            for (size_t contact_idx = 0; contact_idx < island->contact_count; ++contact_idx) {
                ChronoContactPair2D_C *pair = island->contacts[contact_idx];
                if (!pair) {
                    continue;
                }
                ChronoContactManifold2D_C *manifold = &pair->manifold;
                ChronoBody2D_C *body_a = pair->body_a;
                ChronoBody2D_C *body_b = pair->body_b;
                for (int point_idx = 0; point_idx < manifold->num_points; ++point_idx) {
                    ChronoContactPoint2D_C *point = &manifold->points[point_idx];
                    if (!point->is_active || !point->contact.has_contact) {
                        continue;
                    }
                    chrono_collision2d_resolve_contact(body_a,
                                                       body_b,
                                                       &point->contact,
                                                       manifold->combined_restitution,
                                                       manifold->combined_friction_static,
                                                       manifold->combined_friction_dynamic,
                                                       manifold);
                }
            }
        }
    }
}
