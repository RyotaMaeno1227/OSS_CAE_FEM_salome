#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void setup_bodies(ChronoBody2D_C *body_a,
                         ChronoBody2D_C *body_b,
                         double pos_offset,
                         const double vel_a[2],
                         const double vel_b[2]) {
    chrono_body2d_init(body_a);
    chrono_body2d_set_mass(body_a, 1.0, 0.5);
    chrono_body2d_set_circle_shape(body_a, 0.5);
    chrono_body2d_set_restitution(body_a, 0.0);
    chrono_body2d_set_friction_static(body_a, 0.0);
    chrono_body2d_set_friction_dynamic(body_a, 0.0);
    body_a->position[0] = -0.5;
    body_a->position[1] = 0.0;
    body_a->linear_velocity[0] = vel_a[0];
    body_a->linear_velocity[1] = vel_a[1];

    chrono_body2d_init(body_b);
    chrono_body2d_set_mass(body_b, 1.0, 0.5);
    chrono_body2d_set_circle_shape(body_b, 0.5);
    chrono_body2d_set_restitution(body_b, 0.0);
    chrono_body2d_set_friction_static(body_b, 0.0);
    chrono_body2d_set_friction_dynamic(body_b, 0.0);
    body_b->position[0] = 0.5 - pos_offset;
    body_b->position[1] = 0.0;
    body_b->linear_velocity[0] = vel_b[0];
    body_b->linear_velocity[1] = vel_b[1];
}

static double compute_relative_tangent(const ChronoBody2D_C *body_a,
                                       const ChronoBody2D_C *body_b,
                                       const ChronoContact2D_C *contact) {
    double tangent[2] = {-contact->normal[1], contact->normal[0]};
    double ra[2] = {contact->contact_point[0] - body_a->position[0],
                    contact->contact_point[1] - body_a->position[1]};
    double rb[2] = {contact->contact_point[0] - body_b->position[0],
                    contact->contact_point[1] - body_b->position[1]};
    double vel_a[2] = {body_a->linear_velocity[0] - body_a->angular_velocity * ra[1],
                       body_a->linear_velocity[1] + body_a->angular_velocity * ra[0]};
    double vel_b[2] = {body_b->linear_velocity[0] - body_b->angular_velocity * rb[1],
                       body_b->linear_velocity[1] + body_b->angular_velocity * rb[0]};
    double rel[2] = {vel_b[0] - vel_a[0], vel_b[1] - vel_a[1]};
    return rel[0] * tangent[0] + rel[1] * tangent[1];
}

static int test_static_friction(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    double vel_a[2] = {0.0, 0.0};
    double vel_b[2] = {-1.2, 0.15};
    setup_bodies(&body_a, &body_b, 0.02, vel_a, vel_b);
    chrono_body2d_set_friction_static(&body_a, 0.9);
    chrono_body2d_set_friction_dynamic(&body_a, 0.6);
    chrono_body2d_set_friction_static(&body_b, 0.9);
    chrono_body2d_set_friction_dynamic(&body_b, 0.6);

    ChronoContact2D_C contact;
    if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
        fprintf(stderr, "Static friction test: contact detection failed\n");
        return 1;
    }

    double rel_tangent_before = compute_relative_tangent(&body_a, &body_b, &contact);

    if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.0, 0.9, 0.6, NULL) != 0) {
        fprintf(stderr, "Static friction test: collision resolve failed\n");
        return 1;
    }

    double rel_tangent_after = compute_relative_tangent(&body_a, &body_b, &contact);

    if (fabs(rel_tangent_after) > 1e-3) {
        fprintf(stderr, "Static friction test failed: residual tangent velocity %.6f (before %.6f)\n",
                rel_tangent_after, rel_tangent_before);
        return 1;
    }

    return 0;
}

static int test_dynamic_friction(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    double vel_a[2] = {0.0, 0.0};
    double vel_b[2] = {-1.2, 0.6};
    setup_bodies(&body_a, &body_b, 0.02, vel_a, vel_b);
    chrono_body2d_set_friction_static(&body_a, 0.2);
    chrono_body2d_set_friction_dynamic(&body_a, 0.4);
    chrono_body2d_set_friction_static(&body_b, 0.2);
    chrono_body2d_set_friction_dynamic(&body_b, 0.4);

    ChronoContact2D_C contact;
    if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
        fprintf(stderr, "Dynamic friction test: contact detection failed\n");
        return 1;
    }

    double rel_tangent_before = compute_relative_tangent(&body_a, &body_b, &contact);

    if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.0, 0.2, 0.4, NULL) != 0) {
        fprintf(stderr, "Dynamic friction test: collision resolve failed\n");
        return 1;
    }

    double rel_tangent_after = compute_relative_tangent(&body_a, &body_b, &contact);

    if (fabs(rel_tangent_after) >= fabs(rel_tangent_before) - 1e-6) {
        fprintf(stderr, "Dynamic friction test failed: tangent not reduced (before %.6f, after %.6f)\n",
                rel_tangent_before, rel_tangent_after);
        return 1;
    }

    if (rel_tangent_after * rel_tangent_before > 0.0 && fabs(rel_tangent_after) < 1e-3) {
        return 0;
    }

    return 0;
}

static int test_manifold_warm_start(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    double vel_a[2] = {0.0, 0.0};
    double vel_b[2] = {-1.2, 0.15};
    setup_bodies(&body_a, &body_b, 0.02, vel_a, vel_b);
    chrono_body2d_set_friction_static(&body_a, 0.9);
    chrono_body2d_set_friction_dynamic(&body_a, 0.6);
    chrono_body2d_set_friction_static(&body_b, 0.9);
    chrono_body2d_set_friction_dynamic(&body_b, 0.6);

    ChronoContactManifold2D_C manifold;
    chrono_contact_manifold2d_init(&manifold);

    for (int step = 0; step < 2; ++step) {
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
            fprintf(stderr, "Manifold warm start test: contact detection failed at step %d\n", step);
            return 1;
        }
        if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.0, 0.9, 0.6, &manifold) != 0) {
            fprintf(stderr, "Manifold warm start test: resolve failed at step %d\n", step);
            return 1;
        }
    }

    if (manifold.num_points == 0 || manifold.points[0].normal_impulse <= 0.0) {
        fprintf(stderr, "Manifold warm start test: manifold not cached\n");
        return 1;
    }

    ChronoContact2D_C final_contact;
    chrono_collision2d_detect_circle_circle(&body_a, &body_b, &final_contact);
    if (final_contact.has_contact) {
        double limit_static = 0.9 * manifold.points[0].normal_impulse + 1e-9;
        if (fabs(manifold.points[0].tangent_impulse) > limit_static) {
            fprintf(stderr, "Manifold warm start test: tangent impulse exceeds static limit\n");
            return 1;
        }
    }

    return 0;
}

static int test_manager_persistence(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    double vel_a[2] = {0.0, 0.0};
    double vel_b[2] = {-0.8, 0.3};
    setup_bodies(&body_a, &body_b, 0.01, vel_a, vel_b);
    chrono_body2d_set_friction_static(&body_a, 0.6);
    chrono_body2d_set_friction_dynamic(&body_a, 0.3);
    chrono_body2d_set_friction_static(&body_b, 0.4);
    chrono_body2d_set_friction_dynamic(&body_b, 0.2);

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    for (int step = 0; step < 3; ++step) {
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
            fprintf(stderr, "Manager persistence test: contact detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        ChronoContactPoint2D_C *point = chrono_contact_manager2d_update_circle_circle(&manager, &body_a, &body_b, &contact);
        if (!point) {
            fprintf(stderr, "Manager persistence test: update returned NULL\n");
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &body_a, &body_b);
        if (!manifold) {
            fprintf(stderr, "Manager persistence test: manifold acquisition failed\n");
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.0, 0.6, 0.4, manifold) != 0) {
            fprintf(stderr, "Manager persistence test: resolve failed\n");
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
    }

    ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &body_a, &body_b);
    if (!manifold || manifold->num_points == 0) {
        fprintf(stderr, "Manager persistence test: manifold missing\n");
        chrono_contact_manager2d_free(&manager);
        return 1;
    }
    double expected_mu_s = sqrt(0.6 * 0.4);
    double expected_mu_d = sqrt(0.3 * 0.2);
    if (fabs(manifold->combined_friction_static - expected_mu_s) > 1e-6 ||
        fabs(manifold->combined_friction_dynamic - expected_mu_d) > 1e-6) {
        fprintf(stderr, "Manager persistence test: combined friction mismatch (%.6f, %.6f)\n",
                manifold->combined_friction_static, manifold->combined_friction_dynamic);
        chrono_contact_manager2d_free(&manager);
        return 1;
    }
    if (manifold->points[0].normal_impulse <= 0.0) {
        fprintf(stderr, "Manager persistence test: impulses not cached\n");
        chrono_contact_manager2d_free(&manager);
        return 1;
    }

    chrono_contact_manager2d_free(&manager);
    return 0;
}

int main(void) {
    if (test_static_friction() != 0) {
        return 1;
    }
    if (test_dynamic_friction() != 0) {
        return 1;
    }
    if (test_manifold_warm_start() != 0) {
        return 1;
    }
    if (test_manager_persistence() != 0) {
        return 1;
    }

    printf("Circle collision friction tests passed.\n");
    return 0;
}
