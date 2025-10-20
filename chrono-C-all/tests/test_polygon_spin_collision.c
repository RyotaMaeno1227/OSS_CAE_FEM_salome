#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_hexagon(ChronoBody2D_C *body, double radius, double density) {
    chrono_body2d_init(body);
    double vertices[12];
    for (int i = 0; i < 6; ++i) {
        double angle = i * (3.14159265358979323846 / 3.0);
        vertices[2 * i] = radius * cos(angle);
        vertices[2 * i + 1] = radius * sin(angle);
    }
    if (!chrono_body2d_set_polygon_shape_with_density(body, vertices, 6, density)) {
        fprintf(stderr, "Hexagon: set shape failed\n");
    }
    chrono_body2d_set_material(body, &(ChronoMaterial2D_C){0.2, 0.5, 0.4});
}

static int test_rotating_polygons(void) {
    ChronoBody2D_C body_a;
    init_hexagon(&body_a, 0.3, 1.5);
    body_a.position[0] = -0.35;
    body_a.position[1] = 0.0;
    body_a.linear_velocity[0] = 1.2;
    body_a.angular_velocity = 4.0;

    ChronoBody2D_C body_b;
    init_hexagon(&body_b, 0.3, 1.5);
    body_b.position[0] = 0.35;
    body_b.position[1] = 0.0;
    body_b.linear_velocity[0] = -0.8;
    body_b.angular_velocity = -3.0;

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.005;
    int contact_steps = 0;

    for (int step = 0; step < 160; ++step) {
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_polygon_polygon(&body_a, &body_b, &contact) != 0) {
            fprintf(stderr, "Spin test: detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_steps;
            if (!chrono_contact_manager2d_update_contact(&manager, &body_a, &body_b, &contact)) {
                fprintf(stderr, "Spin test: update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &body_a, &body_b);
            if (!manifold) {
                fprintf(stderr, "Spin test: manifold missing at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_contact(&body_a,
                                                   &body_b,
                                                   &contact,
                                                   manifold->combined_restitution,
                                                   manifold->combined_friction_static,
                                                   manifold->combined_friction_dynamic,
                                                   manifold) != 0) {
                fprintf(stderr, "Spin test: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&body_a, dt);
        chrono_body2d_reset_forces(&body_a);
        chrono_body2d_integrate_explicit(&body_b, dt);
        chrono_body2d_reset_forces(&body_b);
    }

    chrono_contact_manager2d_free(&manager);

    if (contact_steps == 0) {
        fprintf(stderr, "Spin test: no contact registered\n");
        return 1;
    }

    double vx = fabs(body_a.linear_velocity[0]) + fabs(body_b.linear_velocity[0]);
    double vy = fabs(body_a.linear_velocity[1]) + fabs(body_b.linear_velocity[1]);
    double ang = fabs(body_a.angular_velocity) + fabs(body_b.angular_velocity);
    if (vx + vy + ang < 0.01) {
        fprintf(stderr, "Spin test: motion damped excessively\n");
        return 1;
    }
    double separation = fabs(body_a.position[0] - body_b.position[0]);
    if (separation < 0.1) {
        fprintf(stderr, "Spin test: bodies still penetrating (sep=%.6f)\n", separation);
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_rotating_polygons() != 0) {
        return 1;
    }
    printf("Polygon spin collision test passed.\n");
    return 0;
}
