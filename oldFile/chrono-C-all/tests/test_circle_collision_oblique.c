#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void compute_tangent(const double normal[2], double tangent[2]) {
    tangent[0] = -normal[1];
    tangent[1] = normal[0];
}

static double dot2(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

int main(void) {
    ChronoBody2D_C body_a;
    ChronoBody2D_C body_b;
    chrono_body2d_init(&body_a);
    chrono_body2d_init(&body_b);

    chrono_body2d_set_mass(&body_a, 1.0, 0.5);
    chrono_body2d_set_mass(&body_b, 1.0, 0.5);
    chrono_body2d_set_circle_shape(&body_a, 0.5);
    chrono_body2d_set_circle_shape(&body_b, 0.5);

    body_a.position[0] = -0.45;
    body_a.position[1] = 0.0;
    body_b.position[0] = 0.45;
    body_b.position[1] = 0.0;

    body_a.linear_velocity[0] = 2.0;
    body_a.linear_velocity[1] = 0.5;
    body_b.linear_velocity[0] = -1.0;
    body_b.linear_velocity[1] = -0.2;

    ChronoContact2D_C contact;
    if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
        fprintf(stderr, "Failed to detect circle-circle contact for oblique case.\n");
        return 1;
    }

    double tangent[2];
    compute_tangent(contact.normal, tangent);

    double va_t_before = dot2(body_a.linear_velocity, tangent);
    double vb_t_before = dot2(body_b.linear_velocity, tangent);

    if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.8, 0.0, 0.0, NULL) != 0) {
        fprintf(stderr, "Failed to resolve oblique circle collision.\n");
        return 1;
    }

    double va_n_after = dot2(body_a.linear_velocity, contact.normal);
    double vb_n_after = dot2(body_b.linear_velocity, contact.normal);
    double expected_va_n = -0.7;
    double expected_vb_n = 1.7;
    if (fabs(va_n_after - expected_va_n) > 1e-2 || fabs(vb_n_after - expected_vb_n) > 1e-2) {
        fprintf(stderr, "Normal velocities incorrect: va=%.6f vb=%.6f (expected %.6f / %.6f)\n",
                va_n_after, vb_n_after, expected_va_n, expected_vb_n);
        return 1;
    }

    double va_t_after = dot2(body_a.linear_velocity, tangent);
    double vb_t_after = dot2(body_b.linear_velocity, tangent);
    if (fabs(va_t_after - va_t_before) > 1e-6 || fabs(vb_t_after - vb_t_before) > 1e-6) {
        fprintf(stderr, "Tangential velocities changed despite zero friction: va=%.6f vb=%.6f\n",
                va_t_after, vb_t_after);
        return 1;
    }

    double dx = body_b.position[0] - body_a.position[0];
    double dy = body_b.position[1] - body_a.position[1];
    double distance = sqrt(dx * dx + dy * dy);
    double min_distance = chrono_body2d_get_circle_radius(&body_a) +
                          chrono_body2d_get_circle_radius(&body_b) - 5e-3;
    if (distance < min_distance) {
        fprintf(stderr, "Oblique collision position correction insufficient: distance=%.6f\n", distance);
        return 1;
    }

    printf("Circle collision oblique regression test passed.\n");
    return 0;
}
