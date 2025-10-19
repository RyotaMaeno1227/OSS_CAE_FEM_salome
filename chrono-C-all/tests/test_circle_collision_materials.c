#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_bodies(ChronoBody2D_C *body, double x_pos) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.5);
    chrono_body2d_set_circle_shape(body, 0.5);
    body->position[0] = x_pos;
    body->position[1] = 0.0;
}

static void compute_expected(const ChronoBody2D_C *body_a,
                             const ChronoBody2D_C *body_b,
                             const ChronoContact2D_C *contact,
                             double restitution,
                             double mu_static,
                             double mu_dynamic,
                             double out_vel_a[2],
                             double out_vel_b[2]) {
    double inv_mass_a = body_a->inverse_mass;
    double inv_mass_b = body_b->inverse_mass;
    double inv_inertia_a = body_a->inverse_inertia;
    double inv_inertia_b = body_b->inverse_inertia;

    double ra[2] = {contact->contact_point[0] - body_a->position[0],
                    contact->contact_point[1] - body_a->position[1]};
    double rb[2] = {contact->contact_point[0] - body_b->position[0],
                    contact->contact_point[1] - body_b->position[1]};

    double vel_a[2] = {body_a->linear_velocity[0] - body_a->angular_velocity * ra[1],
                       body_a->linear_velocity[1] + body_a->angular_velocity * ra[0]};
    double vel_b[2] = {body_b->linear_velocity[0] - body_b->angular_velocity * rb[1],
                       body_b->linear_velocity[1] + body_b->angular_velocity * rb[0]};

    double relative[2] = {vel_b[0] - vel_a[0], vel_b[1] - vel_a[1]};
    double rel_n = relative[0] * contact->normal[0] + relative[1] * contact->normal[1];

    double ra_cross_n = ra[0] * contact->normal[1] - ra[1] * contact->normal[0];
    double rb_cross_n = rb[0] * contact->normal[1] - rb[1] * contact->normal[0];
    double denom_n = inv_mass_a + inv_mass_b +
                     (ra_cross_n * ra_cross_n) * inv_inertia_a +
                     (rb_cross_n * rb_cross_n) * inv_inertia_b;

    double normal_impulse = -(1.0 + restitution) * rel_n / denom_n;
    if (normal_impulse < 0.0) {
        normal_impulse = 0.0;
    }

    double impulse_vec[2] = {contact->normal[0] * normal_impulse,
                             contact->normal[1] * normal_impulse};

    double tangent[2] = {-contact->normal[1], contact->normal[0]};

    double vel_a_after[2] = {body_a->linear_velocity[0] - impulse_vec[0] * inv_mass_a,
                             body_a->linear_velocity[1] - impulse_vec[1] * inv_mass_a};
    double vel_b_after[2] = {body_b->linear_velocity[0] + impulse_vec[0] * inv_mass_b,
                             body_b->linear_velocity[1] + impulse_vec[1] * inv_mass_b};

    double ang_a_after = body_a->angular_velocity - (ra[0] * impulse_vec[1] - ra[1] * impulse_vec[0]) * inv_inertia_a;
    double ang_b_after = body_b->angular_velocity + (rb[0] * impulse_vec[1] - rb[1] * impulse_vec[0]) * inv_inertia_b;

    double vel_a_t[2] = {vel_a_after[0] - ang_a_after * ra[1],
                         vel_a_after[1] + ang_a_after * ra[0]};
    double vel_b_t[2] = {vel_b_after[0] - ang_b_after * rb[1],
                         vel_b_after[1] + ang_b_after * rb[0]};
    double rel_t = (vel_b_t[0] - vel_a_t[0]) * tangent[0] +
                   (vel_b_t[1] - vel_a_t[1]) * tangent[1];

    double ra_cross_t = ra[0] * tangent[1] - ra[1] * tangent[0];
    double rb_cross_t = rb[0] * tangent[1] - rb[1] * tangent[0];
    double denom_t = inv_mass_a + inv_mass_b +
                     (ra_cross_t * ra_cross_t) * inv_inertia_a +
                     (rb_cross_t * rb_cross_t) * inv_inertia_b;

    double tangent_impulse = 0.0;
    if (denom_t > 0.0) {
        double target = -rel_t / denom_t;
        double max_static = mu_static * normal_impulse;
        double max_dynamic = mu_dynamic * normal_impulse;
        if (fabs(target) <= max_static) {
            tangent_impulse = target;
        } else {
            tangent_impulse = fmax(fmin(target, max_dynamic), -max_dynamic);
        }
    }

    double tangent_vec[2] = {tangent[0] * tangent_impulse,
                             tangent[1] * tangent_impulse};

    out_vel_a[0] = vel_a_after[0] - tangent_vec[0] * inv_mass_a;
    out_vel_a[1] = vel_a_after[1] - tangent_vec[1] * inv_mass_a;
    out_vel_b[0] = vel_b_after[0] + tangent_vec[0] * inv_mass_b;
    out_vel_b[1] = vel_b_after[1] + tangent_vec[1] * inv_mass_b;
}

typedef struct MaterialCase2D {
    ChronoMaterial2D_C material_a;
    ChronoMaterial2D_C material_b;
    double expected_restitution;
    double expected_mu_s;
    double expected_mu_d;
} MaterialCase2D;

int main(void) {
    const MaterialCase2D cases[] = {
        {chrono_material2d_make(0.1, 0.5, 0.3),
         chrono_material2d_make(0.4, 0.2, 0.25),
         0.4,
         sqrt(0.5 * 0.2),
         sqrt(0.3 * 0.25)},
        {chrono_material2d_make(0.2, 0.7, 0.4),
         chrono_material2d_make(0.2, 0.8, 0.6),
         0.2,
         sqrt(0.7 * 0.8),
         sqrt(0.4 * 0.6)},
        {chrono_material2d_make(0.0, 0.3, 0.1),
         chrono_material2d_make(1.0, 0.6, 0.4),
         1.0,
         sqrt(0.3 * 0.6),
         sqrt(0.1 * 0.4)},
    };

    for (size_t idx = 0; idx < sizeof(cases) / sizeof(cases[0]); ++idx) {
        ChronoBody2D_C body_a;
        ChronoBody2D_C body_b;
        init_bodies(&body_a, -0.5);
        init_bodies(&body_b, 0.48);

        body_a.linear_velocity[0] = 1.2;
        body_a.linear_velocity[1] = 0.7;
        body_b.linear_velocity[0] = -0.5;
        body_b.linear_velocity[1] = -0.3;

        chrono_body2d_set_material(&body_a, &cases[idx].material_a);
        chrono_body2d_set_material(&body_b, &cases[idx].material_b);

        ChronoContactManager2D_C manager;
        chrono_contact_manager2d_init(&manager);

        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
            fprintf(stderr, "Material test %zu: contact detection failed\n", idx);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        if (!chrono_contact_manager2d_update_circle_circle(&manager, &body_a, &body_b, &contact)) {
            fprintf(stderr, "Material test %zu: manifold update failed\n", idx);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &body_a, &body_b);
        if (!manifold) {
            fprintf(stderr, "Material test %zu: manifold retrieval failed\n", idx);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        double expected_a[2];
        double expected_b[2];
        compute_expected(&body_a, &body_b, &contact,
                         cases[idx].expected_restitution,
                         cases[idx].expected_mu_s,
                         cases[idx].expected_mu_d,
                         expected_a, expected_b);

        double initial_speed_sum = fabs(body_a.linear_velocity[0]) + fabs(body_a.linear_velocity[1]) +
                                   fabs(body_b.linear_velocity[0]) + fabs(body_b.linear_velocity[1]);

        if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact,
                                                     0.0, 0.0, 0.0, manifold) != 0) {
            fprintf(stderr, "Material test %zu: resolve failed\n", idx);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        double tol = 1e-6 * (1.0 + initial_speed_sum);
        if (fabs(body_a.linear_velocity[0] - expected_a[0]) > tol ||
            fabs(body_a.linear_velocity[1] - expected_a[1]) > tol ||
            fabs(body_b.linear_velocity[0] - expected_b[0]) > tol ||
            fabs(body_b.linear_velocity[1] - expected_b[1]) > tol) {
            fprintf(stderr, "Material test %zu failed:\n", idx);
            fprintf(stderr, "  Body A expected (%.6f, %.6f) got (%.6f, %.6f)\n",
                    expected_a[0], expected_a[1], body_a.linear_velocity[0], body_a.linear_velocity[1]);
            fprintf(stderr, "  Body B expected (%.6f, %.6f) got (%.6f, %.6f)\n",
                    expected_b[0], expected_b[1], body_b.linear_velocity[0], body_b.linear_velocity[1]);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }

        chrono_contact_manager2d_free(&manager);
    }

    printf("Circle collision material combination test passed.\n");
    return 0;
}
