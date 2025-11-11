#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_collision2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_island2d.h"

static const double kJacobianTolerance = 1e-9;
static FILE *g_jacobian_log_fp = NULL;
static double g_jacobian_max_error = 0.0;

static int nearly_equal(double a, double b, double tol);

static void jacobian_log_component(int contact_index,
                                   const char *row,
                                   const char *component,
                                   double expected,
                                   double actual,
                                   double diff) {
    if (!g_jacobian_log_fp) {
        return;
    }
    fprintf(g_jacobian_log_fp,
            "%d,%s,%s,%.12e,%.12e,%.12e\n",
            contact_index,
            row,
            component,
            expected,
            actual,
            diff);
}

static int jacobian_check_component(int contact_index,
                                    const char *row,
                                    const char *component,
                                    double expected,
                                    double actual,
                                    double tol) {
    double diff = actual - expected;
    double abs_diff = fabs(diff);
    if (abs_diff > g_jacobian_max_error) {
        g_jacobian_max_error = abs_diff;
    }
    jacobian_log_component(contact_index, row, component, expected, actual, diff);
    return nearly_equal(actual, expected, tol);
}

static int update_jacobian_status_doc(const char *path, double max_error, double tol);

static int nearly_equal(double a, double b, double tol) {
    return fabs(a - b) <= tol;
}

static int verify_contact_jacobian(const ChronoContactPair2D_C *pair,
                                   const char *label,
                                   int contact_index) {
    if (!pair) {
        return 0;
    }
    ChronoContactManifold2D_C *manifold = &pair->manifold;
    for (int point_idx = 0; point_idx < manifold->num_points; ++point_idx) {
        ChronoContactPoint2D_C *point = &manifold->points[point_idx];
        if (!point->is_active || !point->contact.has_contact) {
            continue;
        }
        ChronoContactJacobian3DOF_C jac;
        chrono_contact2d_build_jacobian_3dof(pair->body_a,
                                             pair->body_b,
                                             point->contact.contact_point,
                                             point->contact.normal,
                                             &jac);
        if (jac.active_rows != CHRONO_CONTACT_JACOBIAN_MAX_ROWS) {
            fprintf(stderr,
                    "%s: expected %d rows, got %d\n",
                    label,
                    CHRONO_CONTACT_JACOBIAN_MAX_ROWS,
                    jac.active_rows);
            return 0;
        }
        const double normal[2] = {
            point->contact.normal[0],
            point->contact.normal[1],
        };
        const double tangent[2] = {-normal[1], normal[0]};
        const double ra[2] = {
            point->contact.contact_point[0] - pair->body_a->position[0],
            point->contact.contact_point[1] - pair->body_a->position[1],
        };
        const double rb[2] = {
            point->contact.contact_point[0] - pair->body_b->position[0],
            point->contact.contact_point[1] - pair->body_b->position[1],
        };
        const double cross_na = ra[0] * normal[1] - ra[1] * normal[0];
        const double cross_nb = rb[0] * normal[1] - rb[1] * normal[0];
        const double cross_ta = ra[0] * tangent[1] - ra[1] * tangent[0];
        const double cross_tb = rb[0] * tangent[1] - rb[1] * tangent[0];

        if (!jacobian_check_component(contact_index,
                                       "normal.linear_a",
                                       "x",
                                       -normal[0],
                                       jac.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][0],
                                       kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "normal.linear_a",
                                      "y",
                                      -normal[1],
                                      jac.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][1],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "normal.linear_b",
                                      "x",
                                      normal[0],
                                      jac.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][0],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "normal.linear_b",
                                      "y",
                                      normal[1],
                                      jac.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL][1],
                                      kJacobianTolerance)) {
            fprintf(stderr, "%s: normal row linear terms mismatch\n", label);
            return 0;
        }
        if (!jacobian_check_component(contact_index,
                                      "normal.angular",
                                      "a",
                                      cross_na,
                                      jac.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "normal.angular",
                                      "b",
                                      cross_nb,
                                      jac.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_NORMAL],
                                      kJacobianTolerance)) {
            fprintf(stderr, "%s: normal row angular terms mismatch\n", label);
            return 0;
        }
        if (!jacobian_check_component(contact_index,
                                      "rolling.linear_a",
                                      "x",
                                      -tangent[0],
                                      jac.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][0],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "rolling.linear_a",
                                      "y",
                                      -tangent[1],
                                      jac.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][1],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "rolling.linear_b",
                                      "x",
                                      tangent[0],
                                      jac.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][0],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "rolling.linear_b",
                                      "y",
                                      tangent[1],
                                      jac.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING][1],
                                      kJacobianTolerance)) {
            fprintf(stderr, "%s: rolling row linear terms mismatch\n", label);
            return 0;
        }
        if (!jacobian_check_component(contact_index,
                                      "rolling.angular",
                                      "a",
                                      cross_ta,
                                      jac.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "rolling.angular",
                                      "b",
                                      cross_tb,
                                      jac.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_ROLLING],
                                      kJacobianTolerance)) {
            fprintf(stderr, "%s: rolling row angular terms mismatch\n", label);
            return 0;
        }
        if (!jacobian_check_component(contact_index,
                                      "torsional.angular",
                                      "a",
                                      -1.0,
                                      jac.angular_a[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "torsional.angular",
                                      "b",
                                      1.0,
                                      jac.angular_b[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "torsional.linear_a",
                                      "x",
                                      0.0,
                                      jac.linear_a[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL][0],
                                      kJacobianTolerance) ||
            !jacobian_check_component(contact_index,
                                      "torsional.linear_b",
                                      "x",
                                      0.0,
                                      jac.linear_b[CHRONO_CONTACT_JACOBIAN_ROW_TORSIONAL][0],
                                      kJacobianTolerance)) {
            fprintf(stderr, "%s: torsional row mismatch\n", label);
            return 0;
        }
    }
    return 1;
}

static void init_static_circle(ChronoBody2D_C *body, double x) {
    chrono_body2d_init(body);
    chrono_body2d_set_static(body);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.0);
    chrono_body2d_set_friction_static(body, 0.3);
    chrono_body2d_set_friction_dynamic(body, 0.2);
    body->position[0] = x;
    body->position[1] = 0.0;
}

static void init_dynamic_circle(ChronoBody2D_C *body, double x) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.4);
    chrono_body2d_set_circle_shape(body, 0.3);
    chrono_body2d_set_restitution(body, 0.0);
    chrono_body2d_set_friction_static(body, 0.4);
    chrono_body2d_set_friction_dynamic(body, 0.25);
    body->position[0] = x;
    body->position[1] = 0.0;
    body->linear_velocity[0] = 0.0;
    body->linear_velocity[1] = 0.0;
}

int main(int argc, char **argv) {
    const char *jacobian_report_path = NULL;
    const char *jacobian_log_path = NULL;
    for (int arg = 1; arg < argc; ++arg) {
        if (strcmp(argv[arg], "--jacobian-report") == 0) {
            if (arg + 1 >= argc) {
                fprintf(stderr, "--jacobian-report requires a path\n");
                return 1;
            }
            jacobian_report_path = argv[++arg];
        } else if (strcmp(argv[arg], "--jacobian-log") == 0) {
            if (arg + 1 >= argc) {
                fprintf(stderr, "--jacobian-log requires a path\n");
                return 1;
            }
            jacobian_log_path = argv[++arg];
        } else if (strcmp(argv[arg], "--jacobian-log-default") == 0) {
            jacobian_log_path = "contact_jacobian_log.csv";
        } else if (strcmp(argv[arg], "--help") == 0 || strcmp(argv[arg], "-h") == 0) {
            fprintf(stderr,
                    "Usage: %s [--jacobian-report docs/...md] [--jacobian-log path.csv] "
                    "[--jacobian-log-default]\n",
                    argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown argument: %s\n", argv[arg]);
            return 1;
        }
    }

    if (jacobian_log_path) {
        g_jacobian_log_fp = fopen(jacobian_log_path, "w");
        if (!g_jacobian_log_fp) {
            fprintf(stderr, "Failed to open Jacobian log at %s\n", jacobian_log_path);
            return 1;
        }
        fprintf(g_jacobian_log_fp, "contact_index,row,component,expected,actual,diff\n");
    }

    ChronoBody2D_C anchors[2];
    ChronoBody2D_C statics[2];
    ChronoBody2D_C dynamics[2];

    init_static_circle(&anchors[0], -1.0);
    init_static_circle(&anchors[1], 1.0);

    init_static_circle(&statics[0], -0.1);
    init_static_circle(&statics[1], 0.9);

    init_dynamic_circle(&dynamics[0], -0.5);
    init_dynamic_circle(&dynamics[1], 0.5);

    ChronoDistanceConstraint2D_C constraints[2];
    double local_anchor[2] = {0.0, 0.0};
    chrono_distance_constraint2d_init(&constraints[0],
                                      &anchors[0],
                                      &dynamics[0],
                                      local_anchor,
                                      local_anchor,
                                      fabs(anchors[0].position[0] - dynamics[0].position[0]));
    chrono_distance_constraint2d_set_baumgarte(&constraints[0], 0.3);
    chrono_distance_constraint2d_set_max_correction(&constraints[0], 0.05);

    chrono_distance_constraint2d_init(&constraints[1],
                                      &anchors[1],
                                      &dynamics[1],
                                      local_anchor,
                                      local_anchor,
                                      fabs(anchors[1].position[0] - dynamics[1].position[0]));
    chrono_distance_constraint2d_set_baumgarte(&constraints[1], 0.3);
    chrono_distance_constraint2d_set_max_correction(&constraints[1], 0.05);

    ChronoConstraint2DBase_C *constraint_ptrs[2] = {
        &constraints[0].base,
        &constraints[1].base
    };

    ChronoContactManager2D_C manager;
    chrono_contact_manager2d_init(&manager);

    ChronoIsland2DWorkspace_C workspace;
    chrono_island2d_workspace_init(&workspace);
    int status = 1;
    g_jacobian_max_error = 0.0;

    ChronoIsland2DSolveConfig_C config;
    memset(&config, 0, sizeof(config));
    config.constraint_config.velocity_iterations = 6;
    config.constraint_config.position_iterations = 2;
    config.constraint_config.enable_parallel = 0;
    config.enable_parallel = 1;

    const double dt = 0.01;
    size_t last_island_count = 0;

    for (int step = 0; step < 40; ++step) {
        chrono_contact_manager2d_begin_step(&manager);

        ChronoContact2D_C contact01;
        if (chrono_collision2d_detect_circle_circle(&dynamics[0], &statics[0], &contact01) == 0 &&
            contact01.has_contact) {
            if (!chrono_contact_manager2d_update_circle_circle(&manager, &dynamics[0], &statics[0], &contact01)) {
                fprintf(stderr, "parallel island test: unable to update contact pair 0 at step %d\n", step);
                goto cleanup;
            }
        } else {
            fprintf(stderr, "parallel island test: contact pair 0 missing at step %d\n", step);
            goto cleanup;
        }

        ChronoContact2D_C contact23;
        if (chrono_collision2d_detect_circle_circle(&dynamics[1], &statics[1], &contact23) == 0 &&
            contact23.has_contact) {
            if (!chrono_contact_manager2d_update_circle_circle(&manager, &dynamics[1], &statics[1], &contact23)) {
                fprintf(stderr, "parallel island test: unable to update contact pair 1 at step %d\n", step);
                goto cleanup;
            }
        } else {
            fprintf(stderr, "parallel island test: contact pair 1 missing at step %d\n", step);
            goto cleanup;
        }

        size_t island_count = chrono_island2d_build(constraint_ptrs,
                                                    2,
                                                    manager.pairs,
                                                    manager.count,
                                                    &workspace);
        if (island_count != 2) {
            fprintf(stderr, "parallel island test: expected 2 islands, got %zu at step %d\n",
                    island_count, step);
            goto cleanup;
        }

        chrono_island2d_solve(&workspace, dt, &config);
        last_island_count = island_count;

        chrono_contact_manager2d_end_step(&manager);

        for (int i = 0; i < 2; ++i) {
            chrono_body2d_integrate_explicit(&dynamics[i], dt);
            chrono_body2d_reset_forces(&dynamics[i]);
            dynamics[i].linear_velocity[0] *= 0.8;
            dynamics[i].linear_velocity[1] *= 0.8;
        }
    }

    if (last_island_count != 2 || manager.count != 2) {
        fprintf(stderr, "parallel island test: final island/contact counts unexpected (islands=%zu, contacts=%zu)\n",
                last_island_count, manager.count);
        goto cleanup;
    }

    for (size_t i = 0; i < manager.count; ++i) {
        ChronoContactPair2D_C *pair = &manager.pairs[i];
        ChronoContactManifold2D_C *manifold = &pair->manifold;
        if (manifold->num_points <= 0) {
            fprintf(stderr, "parallel island test: manifold %zu has no points\n", i);
            goto cleanup;
        }
        int active_found = 0;
        for (int j = 0; j < manifold->num_points; ++j) {
            if (manifold->points[j].is_active) {
                active_found = 1;
                break;
            }
        }
        if (!active_found) {
            fprintf(stderr, "parallel island test: manifold %zu lacks active points\n", i);
            goto cleanup;
        }
        ChronoContact2D_C final_contact;
        if (chrono_collision2d_detect_circle_circle(pair->body_a, pair->body_b, &final_contact) != 0 ||
            !final_contact.has_contact) {
            fprintf(stderr, "parallel island test: manifold %zu lost contact state\n", i);
            goto cleanup;
        }
        if (!verify_contact_jacobian(pair, "parallel island test", (int)i)) {
            goto cleanup;
        }
    }

    status = 0;

cleanup:
    chrono_contact_manager2d_free(&manager);
    chrono_island2d_workspace_free(&workspace);
    if (g_jacobian_log_fp) {
        fclose(g_jacobian_log_fp);
        g_jacobian_log_fp = NULL;
    }
    if (status == 0) {
        if (jacobian_report_path && !update_jacobian_status_doc(jacobian_report_path, g_jacobian_max_error, kJacobianTolerance)) {
            fprintf(stderr,
                    "Warning: failed to update Jacobian status block in %s\n",
                    jacobian_report_path);
        }
        printf("Island parallel contact stress test passed.\n");
    }
    return status;
}

static int update_jacobian_status_doc(const char *path, double max_error, double tol) {
    if (!path) {
        return 1;
    }
    const char *start_marker = "<!-- jacobian-status:start -->";
    const char *end_marker = "<!-- jacobian-status:end -->";
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return 0;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return 0;
    }
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return 0;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return 0;
    }
    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(fp);
        return 0;
    }
    if (fread(buffer, 1, (size_t)size, fp) != (size_t)size) {
        free(buffer);
        fclose(fp);
        return 0;
    }
    buffer[size] = '\0';
    fclose(fp);

    char *start = strstr(buffer, start_marker);
    char *end = strstr(buffer, end_marker);
    if (!start || !end || end <= start) {
        free(buffer);
        return 0;
    }

    start += strlen(start_marker);

    time_t now = time(NULL);
    struct tm tm_buf;
#if defined(_WIN32)
    struct tm *utc = gmtime(&now);
#else
    struct tm *utc = gmtime_r(&now, &tm_buf);
#endif
    if (!utc) {
        free(buffer);
        return 0;
    }
    char timestamp[64];
    if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", utc) == 0) {
        free(buffer);
        return 0;
    }

    char payload[256];
    snprintf(payload,
             sizeof(payload),
             "\n_Last verified: %s (max abs error %.3e ≤ tol %.3e)._\n",
             timestamp,
             max_error,
             tol);

    size_t head_len = (size_t)(start - buffer);
    size_t tail_len = strlen(end);
    size_t payload_len = strlen(payload);
    char *updated = (char *)malloc(head_len + payload_len + tail_len + 1);
    if (!updated) {
        free(buffer);
        return 0;
    }
    memcpy(updated, buffer, head_len);
    memcpy(updated + head_len, payload, payload_len);
    memcpy(updated + head_len + payload_len, end, tail_len + 1);

    fp = fopen(path, "w");
    if (!fp) {
        free(buffer);
        free(updated);
        return 0;
    }
    size_t written = fwrite(updated, 1, head_len + payload_len + tail_len, fp);
    fclose(fp);
    free(buffer);
    free(updated);
    return written == head_len + payload_len + tail_len;
}
