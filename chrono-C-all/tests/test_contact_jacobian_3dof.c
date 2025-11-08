#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static int nearly_equal(double a, double b, double tol) {
    return fabs(a - b) <= tol;
}

int main(void) {
    ChronoBody2D_C ground;
    chrono_body2d_init(&ground);
    chrono_body2d_set_static(&ground);
    chrono_body2d_set_polygon_shape(&ground, (double[]){-1.0, 0.0, 1.0, 0.0, 1.0, 0.1, -1.0, 0.1}, 4);

    ChronoBody2D_C block;
    chrono_body2d_init(&block);
    chrono_body2d_set_mass(&block, 1.0, 0.2);
    chrono_body2d_set_polygon_shape(&block, (double[]){-0.1, -0.1, 0.1, -0.1, 0.1, 0.1, -0.1, 0.1}, 4);
    block.position[0] = 0.3;
    block.position[1] = 0.25;

    double contact_point[2] = {0.3, 0.05};
    double normal[2] = {0.0, 1.0};

    ChronoContactJacobian3DOF_C jacobian;
    chrono_contact2d_build_jacobian_3dof(&ground, &block, contact_point, normal, &jacobian);

    if (jacobian.active_rows != CHRONO_CONTACT_JACOBIAN_MAX_ROWS) {
        fprintf(stderr,
                "contact_jacobian_3dof: expected %d rows, got %d\n",
                CHRONO_CONTACT_JACOBIAN_MAX_ROWS,
                jacobian.active_rows);
        return 1;
    }

    const double tangent[2] = {-normal[1], normal[0]};
    const double ra[2] = {contact_point[0] - ground.position[0], contact_point[1] - ground.position[1]};
    const double rb[2] = {contact_point[0] - block.position[0], contact_point[1] - block.position[1]};
    const double expected_cross_normal_a = ra[0] * normal[1] - ra[1] * normal[0];
    const double expected_cross_normal_b = rb[0] * normal[1] - rb[1] * normal[0];
    const double expected_cross_tangent_a = ra[0] * tangent[1] - ra[1] * tangent[0];
    const double expected_cross_tangent_b = rb[0] * tangent[1] - rb[1] * tangent[0];

    const double tol = 1e-9;

    if (!nearly_equal(jacobian.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][0], -normal[0], tol) ||
        !nearly_equal(jacobian.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][1], -normal[1], tol) ||
        !nearly_equal(jacobian.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][0], normal[0], tol) ||
        !nearly_equal(jacobian.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][1], normal[1], tol)) {
        fprintf(stderr, "contact_jacobian_3dof: normal row linear terms mismatch\n");
        return 1;
    }

    if (!nearly_equal(jacobian.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL], expected_cross_normal_a, tol) ||
        !nearly_equal(jacobian.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL], expected_cross_normal_b, tol)) {
        fprintf(stderr, "contact_jacobian_3dof: normal row angular terms mismatch\n");
        return 1;
    }

    if (!nearly_equal(jacobian.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][0], -tangent[0], tol) ||
        !nearly_equal(jacobian.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][1], -tangent[1], tol) ||
        !nearly_equal(jacobian.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][0], tangent[0], tol) ||
        !nearly_equal(jacobian.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][1], tangent[1], tol)) {
        fprintf(stderr, "contact_jacobian_3dof: rolling row linear terms mismatch\n");
        return 1;
    }

    if (!nearly_equal(jacobian.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING], expected_cross_tangent_a, tol) ||
        !nearly_equal(jacobian.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING], expected_cross_tangent_b, tol)) {
        fprintf(stderr, "contact_jacobian_3dof: rolling row angular terms mismatch\n");
        return 1;
    }

    if (!nearly_equal(jacobian.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL], -1.0, tol) ||
        !nearly_equal(jacobian.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL], 1.0, tol) ||
        !nearly_equal(jacobian.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL][0], 0.0, tol) ||
        !nearly_equal(jacobian.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL][0], 0.0, tol)) {
        fprintf(stderr, "contact_jacobian_3dof: torsional row mismatch\n");
        return 1;
    }

    printf("Contact Jacobian 3DOF test passed.\n");
    return 0;
}
