#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

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

    body_a.position[0] = -0.4;
    body_a.position[1] = 0.0;
    body_b.position[0] = 0.4;
    body_b.position[1] = 0.0;

    body_a.linear_velocity[0] = 2.0;
    body_a.linear_velocity[1] = 0.0;
    body_b.linear_velocity[0] = -2.0;
    body_b.linear_velocity[1] = 0.0;

    ChronoContact2D_C contact;
    if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
        fprintf(stderr, "Failed to detect circle-circle contact.\n");
        return 1;
    }

    double rel_v_before[2] = {
        body_b.linear_velocity[0] - body_a.linear_velocity[0],
        body_b.linear_velocity[1] - body_a.linear_velocity[1]};
    double rel_normal_before = dot2(rel_v_before, contact.normal);
    if (rel_normal_before >= 0.0) {
        fprintf(stderr, "Bodies are not approaching before resolution.\n");
        return 1;
    }

    if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.8) != 0) {
        fprintf(stderr, "Failed to resolve circle-circle collision.\n");
        return 1;
    }

    double rel_v_after[2] = {
        body_b.linear_velocity[0] - body_a.linear_velocity[0],
        body_b.linear_velocity[1] - body_a.linear_velocity[1]};
    double rel_normal_after = dot2(rel_v_after, contact.normal);
    double expected_rel_after = 3.2; /* e * |rel_normal_before| */
    if (fabs(rel_normal_after - expected_rel_after) > 1e-2) {
        fprintf(stderr, "Unexpected post-impact relative normal velocity: %.6f (expected %.6f)\n",
                rel_normal_after, expected_rel_after);
        return 1;
    }

    double expected_v_a = -1.6;
    double expected_v_b = 1.6;
    if (fabs(body_a.linear_velocity[0] - expected_v_a) > 1e-2 ||
        fabs(body_b.linear_velocity[0] - expected_v_b) > 1e-2) {
        fprintf(stderr, "Post-impact linear velocities incorrect: va=%.6f vb=%.6f\n",
                body_a.linear_velocity[0], body_b.linear_velocity[0]);
        return 1;
    }

    double dx = body_b.position[0] - body_a.position[0];
    double dy = body_b.position[1] - body_a.position[1];
    double distance = sqrt(dx * dx + dy * dy);
    double min_distance = chrono_body2d_get_circle_radius(&body_a) +
                          chrono_body2d_get_circle_radius(&body_b) - 5e-3;
    if (distance < min_distance) {
        fprintf(stderr, "Bodies are still overlapping: distance=%.6f\n", distance);
        return 1;
    }

    printf("Circle collision regression test passed.\n");
    return 0;
}
