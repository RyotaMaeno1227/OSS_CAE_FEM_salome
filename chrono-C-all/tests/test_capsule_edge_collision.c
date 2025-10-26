#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_capsule(ChronoBody2D_C *body, double x, double y, double angle, double half_length, double radius) {
    chrono_body2d_init(body);
    chrono_body2d_set_capsule_shape(body, half_length, radius);
    chrono_body2d_set_mass(body, 1.5, 0.4);
    body->position[0] = x;
    body->position[1] = y;
    body->angle = angle;
    chrono_body2d_set_material(body, &(ChronoMaterial2D_C){0.1, 0.5, 0.35});
}

static void init_edge(ChronoBody2D_C *edge, const double start[2], const double end[2]) {
    chrono_body2d_init(edge);
    chrono_body2d_set_static(edge);
    chrono_body2d_set_edge_shape(edge, start, end);
    chrono_body2d_set_material(edge, &(ChronoMaterial2D_C){0.0, 0.8, 0.4});
}

static int test_capsule_capsule_collision(void) {
    ChronoBody2D_C a;
    ChronoBody2D_C b;
    init_capsule(&a, -0.6, 0.1, 0.2, 0.35, 0.1);
    init_capsule(&b, 0.6, -0.1, -0.15, 0.35, 0.1);

    a.linear_velocity[0] = 2.0;
    a.linear_velocity[1] = 0.3;
    a.angular_velocity = 1.5;
    b.linear_velocity[0] = -1.5;
    b.linear_velocity[1] = 0.1;
    b.angular_velocity = -1.0;

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    int contact_hits = 0;

    for (int step = 0; step < 40; ++step) {
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_capsule_capsule(&a, &b, &contact) != 0) {
            fprintf(stderr, "Capsule test: detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_hits;
            if (!chrono_contact_manager2d_update_contact(&manager, &a, &b, &contact)) {
                fprintf(stderr, "Capsule test: update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &a, &b);
            if (!manifold) {
                fprintf(stderr, "Capsule test: manifold missing at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_contact(&a,
                                                   &b,
                                                   &contact,
                                                   manifold->combined_restitution,
                                                   manifold->combined_friction_static,
                                                   manifold->combined_friction_dynamic,
                                                   manifold) != 0) {
                fprintf(stderr, "Capsule test: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&a, dt);
        chrono_body2d_reset_forces(&a);
        chrono_body2d_integrate_explicit(&b, dt);
        chrono_body2d_reset_forces(&b);
    }

    chrono_contact_manager2d_free(&manager);

    if (contact_hits == 0) {
        fprintf(stderr, "Capsule test: no contacts registered\n");
        return 1;
    }
    if (fabs(a.linear_velocity[0]) < 0.1 && fabs(b.linear_velocity[0]) < 0.1) {
        fprintf(stderr, "Capsule test: velocities damped excessively\n");
        return 1;
    }
    double sep = fabs(a.position[0] - b.position[0]);
    if (sep < 0.1) {
        fprintf(stderr, "Capsule test: objects still penetrating (sep=%.6f)\n", sep);
        return 1;
    }
    return 0;
}

static int test_edge_circle_collision(void) {
    double start[2] = {-1.0, 0.0};
    double end[2] = {1.0, 0.0};
    ChronoBody2D_C edge;
    init_edge(&edge, start, end);

    ChronoBody2D_C circle;
    chrono_body2d_init(&circle);
    chrono_body2d_set_circle_shape(&circle, 0.2);
    chrono_body2d_set_mass(&circle, 1.0, 0.08);
    circle.position[0] = 0.0;
    circle.position[1] = 0.6;
    circle.linear_velocity[1] = -1.0;
    chrono_body2d_set_material(&circle, &(ChronoMaterial2D_C){0.05, 0.4, 0.3});

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    int contact_hits = 0;

    for (int step = 0; step < 80; ++step) {
        circle.linear_velocity[1] -= 9.81 * dt;
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_edge(&circle, &edge, &contact) != 0) {
            fprintf(stderr, "Edge test: detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_hits;
            if (!chrono_contact_manager2d_update_contact(&manager, &circle, &edge, &contact)) {
                fprintf(stderr, "Edge test: update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &circle, &edge);
            if (!manifold) {
                fprintf(stderr, "Edge test: manifold missing at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_contact(&circle,
                                                   &edge,
                                                   &contact,
                                                   manifold->combined_restitution,
                                                   manifold->combined_friction_static,
                                                   manifold->combined_friction_dynamic,
                                                   manifold) != 0) {
                fprintf(stderr, "Edge test: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&circle, dt);
        chrono_body2d_reset_forces(&circle);
    }

    chrono_contact_manager2d_free(&manager);

    if (contact_hits == 0) {
        fprintf(stderr, "Edge test: no contact registered\n");
        return 1;
    }
    if (circle.position[1] <= 0.0) {
        fprintf(stderr, "Edge test: circle penetrated edge (y=%.6f)\n", circle.position[1]);
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_capsule_capsule_collision() != 0) {
        return 1;
    }
    if (test_edge_circle_collision() != 0) {
        return 1;
    }
    printf("Capsule/edge collision tests passed.\n");
    return 0;
}
