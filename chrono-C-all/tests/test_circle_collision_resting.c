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
    chrono_body2d_set_mass(&body_b, 1.5, 0.8);
    chrono_body2d_set_circle_shape(&body_a, 0.5);
    chrono_body2d_set_circle_shape(&body_b, 0.5);

    body_a.position[0] = 0.0;
    body_a.position[1] = 0.0;
    body_b.position[0] = 0.99; /* small overlap (penetration 0.01) */
    body_b.position[1] = 0.0;

    body_a.linear_velocity[0] = 0.0;
    body_a.linear_velocity[1] = 0.0;
    body_b.linear_velocity[0] = 0.0;
    body_b.linear_velocity[1] = 0.0;

    body_a.angular_velocity = 0.0;
    body_b.angular_velocity = 0.0;

    ChronoContact2D_C contact;
    if (chrono_collision2d_detect_circle_circle(&body_a, &body_b, &contact) != 0 || !contact.has_contact) {
        fprintf(stderr, "Failed to detect contact in resting case.\n");
        return 1;
    }

    double rel_normal_before = dot2((double[2]){body_b.linear_velocity[0] - body_a.linear_velocity[0],
                                               body_b.linear_velocity[1] - body_a.linear_velocity[1]},
                                    contact.normal);
    if (fabs(rel_normal_before) > 1e-9) {
        fprintf(stderr, "Relative normal velocity should be zero before resolution.\n");
        return 1;
    }

    if (chrono_collision2d_resolve_circle_circle(&body_a, &body_b, &contact, 0.0, 0.0, 0.0, NULL) != 0) {
        fprintf(stderr, "Resolution failed for resting contact.\n");
        return 1;
    }

    double rel_normal_after = dot2((double[2]){body_b.linear_velocity[0] - body_a.linear_velocity[0],
                                              body_b.linear_velocity[1] - body_a.linear_velocity[1]},
                                   contact.normal);
    if (fabs(rel_normal_after) > 1e-9) {
        fprintf(stderr, "Relative normal velocity should remain zero after resolution, got %.6e\n", rel_normal_after);
        return 1;
    }

    double dx = body_b.position[0] - body_a.position[0];
    double dy = body_b.position[1] - body_a.position[1];
    double distance = sqrt(dx * dx + dy * dy);
    double min_distance = chrono_body2d_get_circle_radius(&body_a) +
                          chrono_body2d_get_circle_radius(&body_b) - 5e-3;
    if (distance < min_distance) {
        fprintf(stderr, "Resting contact correction insufficient: distance=%.6f\n", distance);
        return 1;
    }

    printf("Circle collision resting contact test passed.\n");
    return 0;
}
