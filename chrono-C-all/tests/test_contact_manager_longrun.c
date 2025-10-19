#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_ground_body(ChronoBody2D_C *ground) {
    chrono_body2d_init(ground);
    chrono_body2d_set_circle_shape(ground, 0.4);
    chrono_body2d_set_static(ground);
    chrono_body2d_set_restitution(ground, 0.05);
    chrono_body2d_set_friction_static(ground, 0.6);
    chrono_body2d_set_friction_dynamic(ground, 0.45);
    ground->position[0] = 0.0;
    ground->position[1] = 0.0;
}

static void init_dynamic_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.45);
    chrono_body2d_set_circle_shape(body, 0.4);
    chrono_body2d_set_restitution(body, 0.05);
    chrono_body2d_set_friction_static(body, 0.55);
    chrono_body2d_set_friction_dynamic(body, 0.4);
    body->position[0] = 0.0;
    body->position[1] = 0.79;
    body->linear_velocity[0] = 0.0;
    body->linear_velocity[1] = -0.2;
}

int main(void) {
    ChronoBody2D_C ground;
    ChronoBody2D_C dynamic_body;
    init_ground_body(&ground);
    init_dynamic_body(&dynamic_body);

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    const int total_steps = 1000;
    const double gravity = 9.81;

    int persistent_contacts = 0;
    int total_missed_contacts = 0;
    int consecutive_misses = 0;
    double impulse_accumulator = 0.0;
    double max_penetration = 0.0;
    double previous_contact_point[2] = {0.0, 0.0};
    int previous_contact_valid = 0;
    const int max_consecutive_misses = 50;
    const int max_total_misses = 80;

    for (int step = 0; step < total_steps; ++step) {
        dynamic_body.linear_velocity[1] -= gravity * dt;
        dynamic_body.linear_velocity[0] = 0.0;
        if (dynamic_body.linear_velocity[1] > 1.0) {
            dynamic_body.linear_velocity[1] = 1.0;
        }
        dynamic_body.position[0] = 0.0;
        if (dynamic_body.position[1] > 0.78) {
            dynamic_body.position[1] = 0.78;
        }

        chrono_contact_manager2d_begin_step(&manager);

        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_circle(&dynamic_body, &ground, &contact) != 0 ||
            !contact.has_contact) {
            consecutive_misses += 1;
            total_missed_contacts += 1;
            if (consecutive_misses > max_consecutive_misses ||
                total_missed_contacts > max_total_misses) {
                fprintf(stderr, "Long-run contact test: excessive contact loss at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            chrono_contact_manager2d_end_step(&manager);
            chrono_body2d_integrate_explicit(&dynamic_body, dt);
            chrono_body2d_reset_forces(&dynamic_body);
            if (dynamic_body.position[1] < 0.4) {
                dynamic_body.position[1] = 0.4;
            }
            dynamic_body.position[0] = 0.0;
            dynamic_body.linear_velocity[1] = fmin(dynamic_body.linear_velocity[1], 0.0);
            continue;
        }
        consecutive_misses = 0;

        ChronoContactPoint2D_C *point =
            chrono_contact_manager2d_update_circle_circle(&manager, &dynamic_body, &ground, &contact);
        ChronoContactManifold2D_C *manifold =
            chrono_contact_manager2d_get_manifold(&manager, &dynamic_body, &ground);
        if (!point || !manifold) {
            fprintf(stderr, "Long-run contact test: manifold acquisition failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (manifold->num_points <= 0) {
            fprintf(stderr, "Long-run contact test: empty manifold at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        if (chrono_collision2d_resolve_circle_circle(&dynamic_body,
                                                     &ground,
                                                     &contact,
                                                     manifold->combined_restitution,
                                                     manifold->combined_friction_static,
                                                     manifold->combined_friction_dynamic,
                                                     manifold) != 0) {
            fprintf(stderr, "Long-run contact test: resolve failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        double warm_after = point->normal_impulse;
        if (step > 10 && warm_after <= 0.0) {
            fprintf(stderr, "Long-run contact test: non-positive impulse at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        impulse_accumulator += warm_after;
        if (contact.penetration > max_penetration) {
            max_penetration = contact.penetration;
        }

        if (previous_contact_valid) {
            double dx = point->contact.contact_point[0] - previous_contact_point[0];
            double dy = point->contact.contact_point[1] - previous_contact_point[1];
            double dist = sqrt(dx * dx + dy * dy);
            if (dist > 5e-2) {
                fprintf(stderr, "Long-run contact test: contact point shifted excessively (d=%.6f) at step %d\n",
                        dist, step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }

        previous_contact_point[0] = point->contact.contact_point[0];
        previous_contact_point[1] = point->contact.contact_point[1];
        previous_contact_valid = 1;
        persistent_contacts += 1;

        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&dynamic_body, dt);
        chrono_body2d_reset_forces(&dynamic_body);
        if (dynamic_body.linear_velocity[1] > 0.0) {
            dynamic_body.linear_velocity[1] *= 0.4;
        }
        dynamic_body.position[0] = 0.0;

        if (dynamic_body.position[1] < 0.4) {
            dynamic_body.position[1] = 0.4;
        }
    }

    chrono_contact_manager2d_free(&manager);

    if (persistent_contacts < total_steps - max_total_misses) {
        fprintf(stderr, "Long-run contact test: insufficient persistent contacts (%d/%d)\n",
                persistent_contacts, total_steps);
        return 1;
    }

    if (impulse_accumulator <= 0.0) {
        fprintf(stderr, "Long-run contact test: no impulse accumulated\n");
        return 1;
    }

    if (max_penetration > 0.12) {
        fprintf(stderr, "Long-run contact test: penetration drifted (max=%.6f)\n", max_penetration);
        return 1;
    }

    printf("Contact manager long-run test passed.\n");
    return 0;
}
