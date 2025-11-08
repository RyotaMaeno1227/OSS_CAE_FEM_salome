#ifndef CHRONO_ISLAND2D_ISLAND_STEP_H
#define CHRONO_ISLAND2D_ISLAND_STEP_H

#include "../include/chrono_island2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"

/**
 * @brief Run the constraint batch solve and contact resolution for a single island.
 *
 * The helper is shared between the OpenMP loop and the oneTBB backend so both
 * paths apply identical constraint/contact logic.
 *
 * @param island   Island handle populated by chrono_island2d_build.
 * @param dt       Simulation time step.
 * @param cfg      Constraint batch configuration (parallel flag is ignored).
 */
static inline void chrono_island2d_step_island(ChronoIsland2D_C *island,
                                               double dt,
                                               const ChronoConstraint2DBatchConfig_C *cfg) {
    if (!island || !cfg) {
        return;
    }

    if (island->constraint_count > 0) {
        chrono_constraint2d_batch_solve(island->constraints,
                                        island->constraint_count,
                                        dt,
                                        cfg,
                                        NULL);
    }

    if (island->contact_count == 0) {
        return;
    }

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

#endif /* CHRONO_ISLAND2D_ISLAND_STEP_H */
