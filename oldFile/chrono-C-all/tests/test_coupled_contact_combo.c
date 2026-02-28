#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_collision2d.h"

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = 0.0;
    anchor->position[1] = 0.5;
}

static void init_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.25);
    chrono_body2d_set_circle_shape(body, 0.18);
    body->position[0] = -0.2;
    body->position[1] = 0.45;
    body->angle = 0.0;
    body->linear_velocity[0] = 0.3;
    body->linear_velocity[1] = -0.25;
    body->angular_velocity = 0.0;
}

static void init_ground(ChronoBody2D_C *ground) {
    chrono_body2d_init(ground);
    chrono_body2d_set_static(ground);
    double vertices[] = {
        -2.0, -0.05,
         2.0, -0.05,
         2.0,  0.05,
        -2.0,  0.05
    };
    chrono_body2d_set_polygon_shape(ground, vertices, 4);
    chrono_body2d_set_material(ground, &(ChronoMaterial2D_C){0.0, 0.7, 0.4});
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    ChronoBody2D_C ground;
    init_anchor(&anchor);
    init_body(&body);
    init_ground(&ground);

    ChronoCoupledConstraint2D_C constraint;
    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};
    double rest_distance = 0.25;
    double rest_angle = 0.0;
    chrono_coupled_constraint2d_init(&constraint,
                                     &anchor,
                                     &body,
                                     local_anchor,
                                     local_anchor,
                                     axis_local,
                                     rest_distance,
                                     rest_angle,
                                     1.0,
                                     -0.2,
                                     0.0);
    chrono_coupled_constraint2d_set_softness_distance(&constraint, 0.015);
    chrono_coupled_constraint2d_set_softness_angle(&constraint, 0.03);
    chrono_coupled_constraint2d_set_distance_spring(&constraint, 32.0, 2.5);
    chrono_coupled_constraint2d_set_angle_spring(&constraint, 14.0, 0.8);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 20;
    cfg.position_iterations = 4;

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.003;
    int contact_frames = 0;

    for (int step = 0; step < 1200; ++step) {
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_polygon(&body, &ground, &contact) != 0) {
            fprintf(stderr, "Coupled contact combo: detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_frames;
            if (!chrono_contact_manager2d_update_contact(&manager, &body, &ground, &contact)) {
                fprintf(stderr, "Coupled contact combo: manager update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_circle_polygon(&body,
                                                          &ground,
                                                          &contact,
                                                          0.1,
                                                          0.6,
                                                          0.4,
                                                          chrono_contact_manager2d_get_manifold(&manager, &body, &ground)) != 0) {
                fprintf(stderr, "Coupled contact combo: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }

        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
        body.linear_velocity[0] *= 0.999;
        body.linear_velocity[1] *= 0.999;
    }

    const ChronoCoupledConstraint2DDiagnostics_C *diag =
        chrono_coupled_constraint2d_get_diagnostics(&constraint);
    if (!diag || diag->condition_number_spectral <= 0.0) {
        fprintf(stderr, "Coupled contact combo: diagnostics missing or invalid.\n");
        chrono_contact_manager2d_free(&manager);
        return 1;
    }
    if (diag->flags & CHRONO_COUPLED_DIAG_RANK_DEFICIENT) {
        fprintf(stderr, "Coupled contact combo: rank deficiency detected.\n");
        chrono_contact_manager2d_free(&manager);
        return 1;
    }
    if (contact_frames < 20) {
        fprintf(stderr, "Coupled contact combo: insufficient contact frames recorded (%d).\n", contact_frames);
        chrono_contact_manager2d_free(&manager);
        return 1;
    }

    chrono_contact_manager2d_free(&manager);
    printf("Coupled contact combo test passed.\n");
    return 0;
}
