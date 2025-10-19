#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"

static void init_anchor(ChronoBody2D_C *anchor, double x, double y) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = x;
    anchor->position[1] = y;
}

static void init_dynamic(ChronoBody2D_C *body, double x, double y, double vx, double vy) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(body, 0.25);
    chrono_body2d_set_restitution(body, 0.05);
    chrono_body2d_set_friction_static(body, 0.3);
    chrono_body2d_set_friction_dynamic(body, 0.2);
    body->position[0] = x;
    body->position[1] = y;
    body->linear_velocity[0] = vx;
    body->linear_velocity[1] = vy;
}

static void apply_constraint(ChronoConstraint2DBase_C *constraint, double dt) {
    for (int iter = 0; iter < 5; ++iter) {
        chrono_constraint2d_prepare(constraint, dt);
        chrono_constraint2d_apply_warm_start(constraint);
        chrono_constraint2d_solve_velocity(constraint);
    }
    chrono_constraint2d_solve_position(constraint);
}

int main(void) {
    ChronoBody2D_C anchor_left;
    ChronoBody2D_C anchor_right;
    init_anchor(&anchor_left, -0.5, 0.0);
    init_anchor(&anchor_right, 1.0, 0.0);

    ChronoBody2D_C body_mid;
    ChronoBody2D_C body_contact;
    init_dynamic(&body_mid, 0.0, 0.0, 0.6, 0.0);
    init_dynamic(&body_contact, 0.5, 0.4, -0.2, -0.3);

    ChronoDistanceConstraint2D_C distance_constraint;
    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(&distance_constraint,
                                      &anchor_left,
                                      &body_mid,
                                      local_anchor,
                                      local_anchor,
                                      0.5);
    chrono_distance_constraint2d_set_baumgarte(&distance_constraint, 0.4);
    chrono_distance_constraint2d_set_max_correction(&distance_constraint, 0.1);

    ChronoContactManager2D_C contact_manager;
    chrono_contact_manager2d_init(&contact_manager);

    const double dt = 0.01;

    for (int step = 0; step < 80; ++step) {
        chrono_contact_manager2d_begin_step(&contact_manager);

        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_circle(&body_contact, &body_mid, &contact) == 0 &&
            contact.has_contact) {
            ChronoContactManifold2D_C *manifold =
                chrono_contact_manager2d_get_manifold(&contact_manager, &body_contact, &body_mid);
            if (!manifold) {
                fprintf(stderr, "island test: manifold allocation failed at step %d\n", step);
                chrono_contact_manager2d_free(&contact_manager);
                return 1;
            }
            if (!chrono_contact_manager2d_update_circle_circle(&contact_manager, &body_contact, &body_mid, &contact)) {
                fprintf(stderr, "island test: update returned NULL at step %d\n", step);
                chrono_contact_manager2d_free(&contact_manager);
                return 1;
            }
            if (chrono_collision2d_resolve_circle_circle(&body_contact, &body_mid, &contact,
                                                          manifold->combined_restitution,
                                                          manifold->combined_friction_static,
                                                          manifold->combined_friction_dynamic,
                                                          manifold) != 0) {
                fprintf(stderr, "island test: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&contact_manager);
                return 1;
            }
        }

        chrono_contact_manager2d_end_step(&contact_manager);

        apply_constraint(&distance_constraint.base, dt);

        chrono_body2d_integrate_explicit(&body_contact, dt);
        chrono_body2d_reset_forces(&body_contact);
        chrono_body2d_integrate_explicit(&body_mid, dt);
        chrono_body2d_reset_forces(&body_mid);
    }

    chrono_contact_manager2d_free(&contact_manager);

    double dist = fabs(body_mid.position[0] - anchor_left.position[0]);
    if (fabs(dist - 0.5) > 0.02) {
        fprintf(stderr, "Island test failed: distance constraint drifted (%.6f)\n", dist);
        return 1;
    }
    if (fabs(body_contact.position[1]) > 0.8) {
        fprintf(stderr, "Island test failed: body_contact left corridor (y=%.6f)\n", body_contact.position[1]);
        return 1;
    }

    printf("Island contact constraint integration test passed.\n");
    return 0;
}
