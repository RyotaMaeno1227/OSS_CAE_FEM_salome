#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

static void init_body(ChronoBody2D_C *body, double x) {
    chrono_body2d_init(body);
    body->position[0] = x;
    body->position[1] = 0.0;
}

static void init_constraint(ChronoDistanceConstraint2D_C *constraint,
                            ChronoBody2D_C *a,
                            ChronoBody2D_C *b) {
    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(constraint, a, b, local_anchor, local_anchor, 0.0);
}

static void init_contact_pair(ChronoContactPair2D_C *pair,
                              ChronoBody2D_C *a,
                              ChronoBody2D_C *b) {
    pair->body_a = a;
    pair->body_b = b;
    chrono_contact_manifold2d_init(&pair->manifold);
    chrono_contact_manifold2d_set_bodies(&pair->manifold, a, b);
}

static int expect_island_layout(const ChronoIsland2DWorkspace_C *workspace,
                                size_t expected_islands,
                                const size_t *expected_constraint_counts,
                                const size_t *expected_contact_counts) {
    if (workspace->island_count != expected_islands) {
        fprintf(stderr, "Expected %zu islands, got %zu\n",
                expected_islands, workspace->island_count);
        return 0;
    }
    for (size_t i = 0; i < expected_islands; ++i) {
        const ChronoIsland2D_C *island = &workspace->islands[i];
        if (island->constraint_count != expected_constraint_counts[i]) {
            fprintf(stderr,
                    "Island %zu: expected %zu constraints, got %zu\n",
                    i, expected_constraint_counts[i], island->constraint_count);
            return 0;
        }
        if (island->contact_count != expected_contact_counts[i]) {
            fprintf(stderr,
                    "Island %zu: expected %zu contacts, got %zu\n",
                    i, expected_contact_counts[i], island->contact_count);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    ChronoBody2D_C bodies[8];
    for (int i = 0; i < 8; ++i) {
        init_body(&bodies[i], (double)i);
    }

    ChronoDistanceConstraint2D_C constraints[3];
    init_constraint(&constraints[0], &bodies[0], &bodies[1]);
    init_constraint(&constraints[1], &bodies[1], &bodies[2]);
    init_constraint(&constraints[2], &bodies[4], &bodies[5]);

    ChronoContactPair2D_C contact_pairs[2];
    init_contact_pair(&contact_pairs[0], &bodies[2], &bodies[3]);
    init_contact_pair(&contact_pairs[1], &bodies[6], &bodies[7]);

    ChronoConstraint2DBase_C *constraint_ptrs[3] = {
        &constraints[0].base,
        &constraints[1].base,
        &constraints[2].base
    };

    ChronoIsland2DWorkspace_C workspace;
    chrono_island2d_workspace_init(&workspace);

    /* Scenario 1: connected constraints only -> single island */
    size_t constraint_counts1[1] = {2};
    size_t contact_counts1[1] = {0};
    size_t islands = chrono_island2d_build(constraint_ptrs,
                                           2,
                                           NULL,
                                           0,
                                           &workspace);
    if (!expect_island_layout(&workspace, 1, constraint_counts1, contact_counts1)) {
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }
    if (islands != 1) {
        fprintf(stderr, "Scenario1: expected 1 island, got %zu\n", islands);
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }

    /* Scenario 2: contact pairs only -> two islands */
    size_t constraint_counts2[2] = {0, 0};
    size_t contact_counts2[2] = {1, 1};
    islands = chrono_island2d_build(NULL,
                                    0,
                                    contact_pairs,
                                    2,
                                    &workspace);
    if (!expect_island_layout(&workspace, 2, constraint_counts2, contact_counts2)) {
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }
    if (islands != 2) {
        fprintf(stderr, "Scenario2: expected 2 islands, got %zu\n", islands);
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }

    /* Scenario 3: mixed shared graph -> single island */
    ChronoContactPair2D_C shared_contact;
    init_contact_pair(&shared_contact, &bodies[1], &bodies[3]);
    size_t constraint_counts3[1] = {2};
    size_t contact_counts3[1] = {1};
    ChronoContactPair2D_C mix_contacts[1] = {shared_contact};
    islands = chrono_island2d_build(constraint_ptrs,
                                    2,
                                    mix_contacts,
                                    1,
                                    &workspace);
    if (!expect_island_layout(&workspace, 1, constraint_counts3, contact_counts3)) {
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }

    /* Scenario 4: mixed disjoint sets -> three islands */
    ChronoConstraint2DBase_C *constraint_ptrs_disjoint[3] = {
        &constraints[0].base,
        &constraints[1].base,
        &constraints[2].base
    };
    ChronoContactPair2D_C disjoint_contacts[2] = {
        shared_contact,
        contact_pairs[1]
    };
    islands = chrono_island2d_build(constraint_ptrs_disjoint,
                                    3,
                                    disjoint_contacts,
                                    2,
                                    &workspace);
    size_t constraint_counts4_expected[3] = {2, 1, 0};
    size_t contact_counts4_expected[3] = {1, 0, 1};
    if (!expect_island_layout(&workspace, 3, constraint_counts4_expected, contact_counts4_expected)) {
        chrono_island2d_workspace_free(&workspace);
        return 1;
    }

    chrono_island2d_workspace_free(&workspace);
    printf("Island builder tests passed.\n");
    return 0;
}
