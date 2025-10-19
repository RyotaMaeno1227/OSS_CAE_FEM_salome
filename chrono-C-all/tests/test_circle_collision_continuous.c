#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_dynamic_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.05);
    chrono_body2d_set_friction_static(body, 0.35);
    chrono_body2d_set_friction_dynamic(body, 0.25);
    body->position[0] = -0.7;
    body->position[1] = 0.0;
    body->linear_velocity[0] = 1.25;
    body->linear_velocity[1] = 0.05;
}

static void init_wall_body(ChronoBody2D_C *body, double x, double y) {
    chrono_body2d_init(body);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.1);
    chrono_body2d_set_friction_static(body, 0.6);
    chrono_body2d_set_friction_dynamic(body, 0.45);
    body->position[0] = x;
    body->position[1] = y;
    chrono_body2d_set_static(body);
}

int main(void) {
    ChronoBody2D_C dynamic_body;
    ChronoBody2D_C wall_top;
    ChronoBody2D_C wall_bottom;
    init_dynamic_body(&dynamic_body);
    init_wall_body(&wall_top, 0.25, 0.3);
    init_wall_body(&wall_bottom, 0.25, -0.3);

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    bool top_contact_any = false;
    bool bottom_contact_any = false;
    double max_normal_impulse = 0.0;

    for (int step = 0; step < 120; ++step) {
        double vertical_drive = (step < 60) ? -0.002 : 0.002;
        dynamic_body.linear_velocity[1] += vertical_drive;

        chrono_contact_manager2d_begin_step(&manager);

        ChronoBody2D_C *walls[2] = {&wall_top, &wall_bottom};
        bool step_top = false;
        bool step_bottom = false;

        for (int i = 0; i < 2; ++i) {
            ChronoContact2D_C contact;
            if (chrono_collision2d_detect_circle_circle(&dynamic_body, walls[i], &contact) == 0 &&
                contact.has_contact) {
                ChronoContactPoint2D_C *point =
                    chrono_contact_manager2d_update_circle_circle(&manager, &dynamic_body, walls[i], &contact);
                ChronoContactManifold2D_C *manifold =
                    chrono_contact_manager2d_get_manifold(&manager, &dynamic_body, walls[i]);
                if (!point || !manifold ||
                    chrono_collision2d_resolve_circle_circle(&dynamic_body, walls[i], &contact,
                                                              0.0, 0.0, 0.0, manifold) != 0) {
                    fprintf(stderr, "Continuous simulation: resolve failed at step %d\n", step);
                    chrono_contact_manager2d_free(&manager);
                    return 1;
                }
                if (manifold->num_points > 0) {
                    if (manifold->points[0].normal_impulse > max_normal_impulse) {
                        max_normal_impulse = manifold->points[0].normal_impulse;
                    }
                }
                if (walls[i] == &wall_top) {
                    step_top = true;
                } else {
                    step_bottom = true;
                }
            }
        }

        if (step_top) {
            top_contact_any = true;
        }
        if (step_bottom) {
            bottom_contact_any = true;
        }

        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&dynamic_body, dt);
        chrono_body2d_reset_forces(&dynamic_body);
    }

    chrono_contact_manager2d_free(&manager);

    if (!top_contact_any || !bottom_contact_any) {
        fprintf(stderr, "Continuous simulation: both walls were not contacted\n");
        return 1;
    }

    if (fabs(dynamic_body.position[1]) > 0.5) {
        fprintf(stderr, "Continuous simulation: body left corridor (y=%.6f)\n", dynamic_body.position[1]);
        return 1;
    }

    if (max_normal_impulse <= 0.0) {
        fprintf(stderr, "Continuous simulation: no normal impulse recorded\n");
        return 1;
    }

    printf("Circle collision continuous simulation test passed.\n");
    return 0;
}
