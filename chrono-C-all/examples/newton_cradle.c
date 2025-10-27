#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

#define PENDULUM_COUNT 4

typedef struct NewtonCradleConfig {
    double pivot_height;
    double pendulum_length;
    double bob_radius;
    double bob_mass;
    double anchor_spacing;
    double gravity;
    double damping;
    double dt;
    double total_time;
    int sample_stride;
} NewtonCradleConfig;

static void init_anchor(ChronoBody2D_C *anchor, double x, double y) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = x;
    anchor->position[1] = y;
}

static void init_bob(ChronoBody2D_C *bob,
                     double radius,
                     double mass,
                     double restitution,
                     double friction_static,
                     double friction_dynamic) {
    chrono_body2d_init(bob);
    chrono_body2d_set_mass(bob, mass, 0.5 * mass * radius * radius);
    chrono_body2d_set_circle_shape(bob, radius);
    chrono_body2d_set_restitution(bob, restitution);
    chrono_body2d_set_friction_static(bob, friction_static);
    chrono_body2d_set_friction_dynamic(bob, friction_dynamic);
}

static void configure_pendulum(const NewtonCradleConfig *config,
                               ChronoBody2D_C *anchor,
                               ChronoBody2D_C *bob,
                               ChronoDistanceConstraint2D_C *constraint,
                               double anchor_x,
                               double initial_angle) {
    init_anchor(anchor, anchor_x, config->pivot_height);
    init_bob(bob,
             config->bob_radius,
             config->bob_mass,
             0.95,
             0.02,
             0.015);

    double sin_theta = sin(initial_angle);
    double cos_theta = cos(initial_angle);
    bob->position[0] = anchor_x + config->pendulum_length * sin_theta;
    bob->position[1] = config->pivot_height - config->pendulum_length * cos_theta;
    bob->linear_velocity[0] = 0.0;
    bob->linear_velocity[1] = 0.0;
    bob->angular_velocity = 0.0;

    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(constraint,
                                      anchor,
                                      bob,
                                      local_anchor,
                                      local_anchor,
                                      config->pendulum_length);
    chrono_distance_constraint2d_set_baumgarte(constraint, 0.35);
    chrono_distance_constraint2d_set_max_correction(constraint, 0.05);
    chrono_distance_constraint2d_set_slop(constraint, 1e-4);
    chrono_distance_constraint2d_set_softness(constraint, 0.0);
}

static void apply_external_effects(ChronoBody2D_C *bob,
                                   const NewtonCradleConfig *config,
                                   double dt) {
    if (!bob || bob->is_static) {
        return;
    }
    bob->linear_velocity[1] -= config->gravity * dt;
    bob->linear_velocity[0] *= config->damping;
    bob->linear_velocity[1] *= config->damping;
    bob->angular_velocity *= config->damping;
}

static void write_header(FILE *fp) {
    fprintf(fp,
            "time,"
            "x1,y1,vx1,vy1,"
            "x2,y2,vx2,vy2,"
            "x3,y3,vx3,vy3,"
            "x4,y4,vx4,vy4\n");
}

static void write_state(FILE *fp, double time, ChronoBody2D_C bobs[PENDULUM_COUNT]) {
    fprintf(fp,
            "%.6f,"
            "%.6f,%.6f,%.6f,%.6f,"
            "%.6f,%.6f,%.6f,%.6f,"
            "%.6f,%.6f,%.6f,%.6f,"
            "%.6f,%.6f,%.6f,%.6f\n",
            time,
            bobs[0].position[0], bobs[0].position[1], bobs[0].linear_velocity[0], bobs[0].linear_velocity[1],
            bobs[1].position[0], bobs[1].position[1], bobs[1].linear_velocity[0], bobs[1].linear_velocity[1],
            bobs[2].position[0], bobs[2].position[1], bobs[2].linear_velocity[0], bobs[2].linear_velocity[1],
            bobs[3].position[0], bobs[3].position[1], bobs[3].linear_velocity[0], bobs[3].linear_velocity[1]);
}

static int detect_circle_contacts(ChronoContactManager2D_C *manager,
                                  ChronoBody2D_C bobs[PENDULUM_COUNT]) {
    ChronoContact2D_C contact;
    int has_failure = 0;
    for (int i = 0; i < PENDULUM_COUNT; ++i) {
        for (int j = i + 1; j < PENDULUM_COUNT; ++j) {
            if (chrono_collision2d_detect_circle_circle(&bobs[i], &bobs[j], &contact) == 0 &&
                contact.has_contact) {
                ChronoContactPoint2D_C *cp =
                    chrono_contact_manager2d_update_circle_circle(manager, &bobs[i], &bobs[j], &contact);
                if (!cp) {
                    has_failure = 1;
                }
            }
        }
    }
    return has_failure;
}

int main(int argc, char **argv) {
    const char *output_path = "newton_cradle.csv";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        output_path = argv[1];
    }

    FILE *fp = fopen(output_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open output file '%s'\n", output_path);
        return 1;
    }

    NewtonCradleConfig config;
    config.pivot_height = 0.5;
    config.pendulum_length = 0.8;
    config.bob_radius = 0.1;
    config.bob_mass = 0.45;
    config.anchor_spacing = 0.2;
    config.gravity = 9.81;
    config.damping = 0.9995;
    config.dt = 0.0005;
    config.total_time = 6.0;
    config.sample_stride = 10;

    ChronoBody2D_C anchors[PENDULUM_COUNT];
    ChronoBody2D_C bobs[PENDULUM_COUNT];
    ChronoDistanceConstraint2D_C constraints[PENDULUM_COUNT];
    ChronoConstraint2DBase_C *constraint_ptrs[PENDULUM_COUNT];

    double center_offset = (PENDULUM_COUNT - 1) * 0.5 * config.anchor_spacing;
    double initial_angles[PENDULUM_COUNT] = {-0.6, 0.0, 0.0, 0.0};

    for (int i = 0; i < PENDULUM_COUNT; ++i) {
        double anchor_x = (i * config.anchor_spacing) - center_offset;
        configure_pendulum(&config,
                           &anchors[i],
                           &bobs[i],
                           &constraints[i],
                           anchor_x,
                           initial_angles[i]);
        constraint_ptrs[i] = &constraints[i].base;
    }

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    ChronoIsland2DWorkspace_C workspace;
    chrono_island2d_workspace_init(&workspace);

    ChronoIsland2DSolveConfig_C solve_config;
    memset(&solve_config, 0, sizeof(solve_config));
    solve_config.constraint_config.velocity_iterations = 18;
    solve_config.constraint_config.position_iterations = 4;
    solve_config.constraint_config.enable_parallel = 0;
    solve_config.enable_parallel = 0;

    const int total_steps = (int)(config.total_time / config.dt);
    write_header(fp);
    write_state(fp, 0.0, bobs);

    double time = 0.0;
    for (int step = 0; step < total_steps; ++step) {
        time += config.dt;

        for (int i = 0; i < PENDULUM_COUNT; ++i) {
            apply_external_effects(&bobs[i], &config, config.dt);
        }

        chrono_contact_manager2d_begin_step(&manager);
        if (detect_circle_contacts(&manager, bobs)) {
            fprintf(stderr, "Contact update failure at step %d\n", step);
            fclose(fp);
            chrono_contact_manager2d_free(&manager);
            chrono_island2d_workspace_free(&workspace);
            return 1;
        }

        chrono_island2d_workspace_reset(&workspace);
        size_t island_count = chrono_island2d_build(constraint_ptrs,
                                                    PENDULUM_COUNT,
                                                    manager.pairs,
                                                    manager.count,
                                                    &workspace);
        if (island_count == 0) {
            chrono_constraint2d_batch_solve(constraint_ptrs,
                                            PENDULUM_COUNT,
                                            config.dt,
                                            &solve_config.constraint_config,
                                            NULL);
        } else {
            chrono_island2d_solve(&workspace, config.dt, &solve_config);
        }

        chrono_contact_manager2d_end_step(&manager);

        for (int i = 0; i < PENDULUM_COUNT; ++i) {
            chrono_body2d_integrate_explicit(&bobs[i], config.dt);
            chrono_body2d_reset_forces(&bobs[i]);
        }

        if ((step + 1) % config.sample_stride == 0) {
            write_state(fp, time, bobs);
        }
    }

    fclose(fp);
    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&workspace);

    printf("Simulation complete. Data written to %s\n", output_path);
    return 0;
}
