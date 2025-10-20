#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"

static int double_equals(double a, double b, double tol) {
    return fabs(a - b) <= tol;
}

static int test_square_density(void) {
    ChronoBody2D_C body;
    chrono_body2d_init(&body);
    double vertices[] = {
        -0.5, -0.5,
         0.5, -0.5,
         0.5,  0.5,
        -0.5,  0.5
    };
    double density = 2.0;
    if (!chrono_body2d_set_polygon_shape_with_density(&body, vertices, 4, density)) {
        fprintf(stderr, "Failed to set polygon shape with density\n");
        return 1;
    }
    if (body.is_static) {
        fprintf(stderr, "Body unexpectedly static\n");
        return 1;
    }
    double mass = (body.inverse_mass > 0.0) ? 1.0 / body.inverse_mass : 0.0;
    double inertia = (body.inverse_inertia > 0.0) ? 1.0 / body.inverse_inertia : 0.0;
    double expected_mass = density * 1.0; /* area = 1 */
    double expected_inertia = expected_mass * (1.0 * 1.0 + 1.0 * 1.0) / 12.0;
    if (!double_equals(mass, expected_mass, 1e-9)) {
        fprintf(stderr, "Square mass mismatch: expected %.9f got %.9f\n", expected_mass, mass);
        return 1;
    }
    if (!double_equals(inertia, expected_inertia, 1e-9)) {
        fprintf(stderr, "Square inertia mismatch: expected %.9f got %.9f\n", expected_inertia, inertia);
        return 1;
    }
    return 0;
}

static int test_triangle_density(void) {
    ChronoBody2D_C body;
    chrono_body2d_init(&body);
    double vertices[] = {
        0.0, 0.0,
        1.0, 0.0,
        0.0, 1.0
    };
    double density = 3.0;
    if (!chrono_body2d_set_polygon_shape_with_density(&body, vertices, 3, density)) {
        fprintf(stderr, "Failed to set triangle with density\n");
        return 1;
    }
    double mass = (body.inverse_mass > 0.0) ? 1.0 / body.inverse_mass : 0.0;
    double inertia = (body.inverse_inertia > 0.0) ? 1.0 / body.inverse_inertia : 0.0;
    double area = 0.5;
    double expected_mass = density * area;
    /* Inertia about origin for right triangle with legs 1: I = density * (1/6). */
    double expected_inertia = density * (1.0 / 6.0);
    if (!double_equals(mass, expected_mass, 1e-9)) {
        fprintf(stderr, "Triangle mass mismatch: expected %.9f got %.9f\n", expected_mass, mass);
        return 1;
    }
    if (!double_equals(inertia, expected_inertia, 1e-9)) {
        fprintf(stderr, "Triangle inertia mismatch: expected %.9f got %.9f\n", expected_inertia, inertia);
        return 1;
    }
    return 0;
}

static int test_zero_density(void) {
    ChronoBody2D_C body;
    chrono_body2d_init(&body);
    double vertices[] = {
        -0.5, 0.0,
         0.5, 0.0,
         0.0, 0.5
    };
    if (!chrono_body2d_set_polygon_shape_with_density(&body, vertices, 3, 0.0)) {
        fprintf(stderr, "Zero density should succeed\n");
        return 1;
    }
    if (!body.is_static) {
        fprintf(stderr, "Zero density body should be static\n");
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_square_density() != 0) {
        return 1;
    }
    if (test_triangle_density() != 0) {
        return 1;
    }
    if (test_zero_density() != 0) {
        return 1;
    }
    printf("Polygon mass properties tests passed.\n");
    return 0;
}
