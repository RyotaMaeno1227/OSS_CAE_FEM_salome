#include <math.h>
#include <stdio.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"

static void init_slope(ChronoBody2D_C *slope, double angle_deg) {
    chrono_body2d_init(slope);
    chrono_body2d_set_static(slope);
    double angle = angle_deg * (3.14159265358979323846 / 180.0);
    double half_length = 1.0;
    double half_thickness = 0.05;
    double verts[] = {
        -half_length, -half_thickness,
         half_length, -half_thickness,
         half_length,  half_thickness,
        -half_length,  half_thickness
    };
    if (!chrono_body2d_set_polygon_shape_with_density(slope, verts, 4, 0.0)) {
        fprintf(stderr, "Slope: set shape failed\n");
    }
    slope->angle = angle;
    slope->position[0] = 0.0;
    slope->position[1] = 0.0;
    chrono_body2d_set_material(slope, &(ChronoMaterial2D_C){0.0, 0.8, 0.6});
}

static void init_block(ChronoBody2D_C *block, double x, double y) {
    chrono_body2d_init(block);
    double half = 0.25;
    double verts[] = {
        -half, -half,
         half, -half,
         half,  half,
        -half,  half
    };
    if (!chrono_body2d_set_polygon_shape_with_density(block, verts, 4, 1.0)) {
        fprintf(stderr, "Block: set shape failed\n");
    }
    chrono_body2d_set_material(block, &(ChronoMaterial2D_C){0.0, 0.6, 0.45});
    block->position[0] = x;
    block->position[1] = y;
    block->linear_velocity[0] = 0.0;
    block->linear_velocity[1] = 0.0;
}

static int test_block_sliding(void) {
    ChronoBody2D_C slope;
    init_slope(&slope, 20.0);

    ChronoBody2D_C block;
    init_block(&block, -0.5, 0.4);

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    const double dt = 0.01;
    const double gravity = 9.81;
    int contact_steps = 0;

    for (int step = 0; step < 200; ++step) {
        block.linear_velocity[1] -= gravity * dt;

        chrono_contact_manager2d_begin_step(&manager);
        ChronoContact2D_C contact;
        if (chrono_collision2d_detect_polygon_polygon(&block, &slope, &contact) != 0) {
            fprintf(stderr, "Slope test: detection failed at step %d\n", step);
            chrono_contact_manager2d_free(&manager);
            return 1;
        }
        if (contact.has_contact) {
            ++contact_steps;
            if (!chrono_contact_manager2d_update_contact(&manager, &block, &slope, &contact)) {
                fprintf(stderr, "Slope test: update failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(&manager, &block, &slope);
            if (!manifold) {
                fprintf(stderr, "Slope test: manifold missing at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
            if (chrono_collision2d_resolve_contact(&block,
                                                   &slope,
                                                   &contact,
                                                   manifold->combined_restitution,
                                                   manifold->combined_friction_static,
                                                   manifold->combined_friction_dynamic,
                                                   manifold) != 0) {
                fprintf(stderr, "Slope test: resolve failed at step %d\n", step);
                chrono_contact_manager2d_free(&manager);
                return 1;
            }
        }
        chrono_contact_manager2d_end_step(&manager);

        chrono_body2d_integrate_explicit(&block, dt);
        chrono_body2d_reset_forces(&block);
    }

    chrono_contact_manager2d_free(&manager);

    if (contact_steps == 0) {
        fprintf(stderr, "Slope test: no contact registered\n");
        return 1;
    }
    if (block.position[0] >= -0.1) {
        fprintf(stderr, "Slope test: block did not glide along slope (x=%.6f)\n", block.position[0]);
        return 1;
    }
    if (block.linear_velocity[0] >= 0.0) {
        fprintf(stderr, "Slope test: tangential velocity not negative (vx=%.6f)\n", block.linear_velocity[0]);
        return 1;
    }
    return 0;
}

int main(void) {
    if (test_block_sliding() != 0) {
        return 1;
    }
    printf("Polygon slope friction test passed.\n");
    return 0;
}
