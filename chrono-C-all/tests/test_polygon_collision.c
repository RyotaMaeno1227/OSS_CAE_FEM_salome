#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static int test_polygon_polygon(void) {
    ChronoBody2D_C ground;
    chrono_body2d_init(&ground);
    chrono_body2d_set_static(&ground);
    double ground_vertices[] = {
        -0.5, -0.05,
         0.5, -0.05,
         0.5,  0.05,
        -0.5,  0.05
    };
    if (!chrono_body2d_set_polygon_shape(&ground, ground_vertices, 4)) {
        fprintf(stderr, "Failed to set ground polygon shape\n");
        return 1;
    }
    chrono_body2d_set_material(&ground, &(ChronoMaterial2D_C){0.1, 0.6, 0.4});

    ChronoBody2D_C box;
    chrono_body2d_init(&box);
    chrono_body2d_set_mass(&box, 2.0, 0.5);
    double box_vertices[] = {
        -0.3, -0.3,
         0.3, -0.3,
         0.3,  0.3,
        -0.3,  0.3
    };
    if (!chrono_body2d_set_polygon_shape(&box, box_vertices, 4)) {
        fprintf(stderr, "Failed to set box polygon shape\n");
        return 1;
    }
    chrono_body2d_set_material(&box, &(ChronoMaterial2D_C){0.2, 0.4, 0.3});
    box.position[0] = 0.05;
    box.position[1] = 0.25;
    box.linear_velocity[0] = 0.0;
    box.linear_velocity[1] = -1.2;

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    int contact_steps = 0;
    for (int step = 0; step < 8; ++step) {
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_polygon_polygon(&box, &ground, &contact) != 0) {
            fprintf(stderr, "Polygon vs polygon detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_steps;
            if (!chrono_contact_manager2d_update_contact(&manager, &box, &ground, &contact)) {
                fprintf(stderr, "Polygon vs polygon manager update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_polygon_polygon(&box,
                                                           &ground,
                                                           &contact,
                                                           0.0,
                                                           0.5,
                                                           0.3,
                                                           chrono_contact_manager2d_get_manifold(&manager, &box, &ground)) != 0) {
                fprintf(stderr, "Polygon vs polygon resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);
        chrono_body2d_integrate_explicit(&box, dt);
        chrono_body2d_reset_forces(&box);
    }

    chrono_contact_manager2d_free(&manager);

    if (box.position[1] < 0.2) {
        fprintf(stderr, "Polygon vs polygon: box penetrated ground (y=%.6f)\n", box.position[1]);
        return 1;
    }
    if (contact_steps == 0) {
        fprintf(stderr, "Polygon vs polygon: no contact registered\n");
        return 1;
    }
    if (box.linear_velocity[1] < -0.05) {
        fprintf(stderr, "Polygon vs polygon: velocity not resolved (vy=%.6f)\n", box.linear_velocity[1]);
        return 1;
    }
    return 0;
}

static int test_circle_polygon(void) {
    ChronoBody2D_C wall;
    chrono_body2d_init(&wall);
    chrono_body2d_set_static(&wall);
    double wall_vertices[] = {
        -0.05, -0.6,
         0.05, -0.6,
         0.05,  0.6,
        -0.05,  0.6
    };
    if (!chrono_body2d_set_polygon_shape(&wall, wall_vertices, 4)) {
        fprintf(stderr, "Failed to set wall polygon shape\n");
        return 1;
    }
    chrono_body2d_set_material(&wall, &(ChronoMaterial2D_C){0.0, 0.8, 0.4});

    ChronoBody2D_C ball;
    chrono_body2d_init(&ball);
    chrono_body2d_set_mass(&ball, 1.0, 0.2);
    chrono_body2d_set_circle_shape(&ball, 0.25);
    chrono_body2d_set_material(&ball, &(ChronoMaterial2D_C){0.1, 0.5, 0.4});
    ball.position[0] = -0.4;
    ball.position[1] = 0.0;
    ball.linear_velocity[0] = 2.0;
    ball.linear_velocity[1] = 0.1;

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    int contact_steps = 0;
    for (int step = 0; step < 40; ++step) {
        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_circle_polygon(&ball, &wall, &contact) != 0) {
            fprintf(stderr, "Circle vs polygon detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_steps;
            if (!chrono_contact_manager2d_update_contact(&manager, &ball, &wall, &contact)) {
                fprintf(stderr, "Circle vs polygon manager update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_circle_polygon(&ball,
                                                          &wall,
                                                          &contact,
                                                          0.1,
                                                          0.6,
                                                          0.4,
                                                          chrono_contact_manager2d_get_manifold(&manager, &ball, &wall)) != 0) {
                fprintf(stderr, "Circle vs polygon resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);
        chrono_body2d_integrate_explicit(&ball, dt);
        chrono_body2d_reset_forces(&ball);
    }

    chrono_contact_manager2d_free(&manager);

    if (contact_steps == 0) {
        fprintf(stderr, "Circle vs polygon: no contact registered\n");
        return 1;
    }
    if (ball.linear_velocity[0] > 0.6) {
        fprintf(stderr, "Circle vs polygon: velocity not sufficiently reduced (vx=%.6f)\n", ball.linear_velocity[0]);
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_polygon_polygon() != 0) {
        return 1;
    }
    if (test_circle_polygon() != 0) {
        return 1;
    }
    printf("Polygon collision tests passed.\n");
    return 0;
}
