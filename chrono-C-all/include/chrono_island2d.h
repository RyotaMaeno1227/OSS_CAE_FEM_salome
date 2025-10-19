#ifndef CHRONO_ISLAND2D_H
#define CHRONO_ISLAND2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "chrono_constraint2d.h"
#include "chrono_collision2d.h"

typedef struct ChronoIsland2D_C {
    ChronoConstraint2DBase_C **constraints;
    size_t constraint_count;
    ChronoContactPair2D_C **contacts;
    size_t contact_count;
} ChronoIsland2D_C;

struct ChronoIsland2DBodyMapEntry;

typedef struct ChronoIsland2DWorkspace_C {
    ChronoIsland2D_C *islands;
    size_t island_count;
    size_t island_capacity;

    ChronoConstraint2DBase_C **constraint_ptrs;
    size_t constraint_ptr_capacity;

    ChronoContactPair2D_C **contact_ptrs;
    size_t contact_ptr_capacity;

    int *constraint_island_ids;
    size_t constraint_island_ids_capacity;

    int *contact_island_ids;
    size_t contact_island_ids_capacity;

    size_t *constraint_offsets;
    size_t constraint_offsets_capacity;

    size_t *contact_offsets;
    size_t contact_offsets_capacity;

    int *constraint_body_indices;
    size_t constraint_body_indices_capacity;

    int *contact_body_indices;
    size_t contact_body_indices_capacity;

    ChronoBody2D_C **body_nodes;
    size_t body_nodes_capacity;

    int *parent;
    size_t parent_capacity;

    int *rank;
    size_t rank_capacity;

    struct ChronoIsland2DBodyMapEntry *body_map;
    size_t body_map_capacity;
} ChronoIsland2DWorkspace_C;

typedef struct ChronoIsland2DSolveConfig_C {
    ChronoConstraint2DBatchConfig_C constraint_config;
    int enable_parallel;
} ChronoIsland2DSolveConfig_C;

void chrono_island2d_workspace_init(ChronoIsland2DWorkspace_C *workspace);
void chrono_island2d_workspace_reset(ChronoIsland2DWorkspace_C *workspace);
void chrono_island2d_workspace_free(ChronoIsland2DWorkspace_C *workspace);

size_t chrono_island2d_build(ChronoConstraint2DBase_C **constraints,
                             size_t constraint_count,
                             ChronoContactPair2D_C *contact_pairs,
                             size_t contact_count,
                             ChronoIsland2DWorkspace_C *workspace);

void chrono_island2d_solve(const ChronoIsland2DWorkspace_C *workspace,
                           double dt,
                           const ChronoIsland2DSolveConfig_C *config);

#ifdef __cplusplus
}
#endif

#endif
