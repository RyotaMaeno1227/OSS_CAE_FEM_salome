#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_logging.h"

typedef struct TestLogger {
    int count;
    char last_message[256];
} TestLogger;

static void test_log_handler(ChronoLogLevel_C level,
                             ChronoLogCategory_C category,
                             const char *message,
                             void *user_data) {
    (void)level;
    (void)category;
    if (!user_data || !message) {
        return;
    }
    TestLogger *logger = (TestLogger *)user_data;
    logger->count += 1;
    snprintf(logger->last_message, sizeof(logger->last_message), "%s", message);
}

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = -0.2;
    anchor->position[1] = 0.12;
}

static void init_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.28);
    chrono_body2d_set_circle_shape(body, 0.18);
    body->position[0] = 0.55;
    body->position[1] = 0.4;
    body->angle = 0.28;
    body->linear_velocity[0] = 0.22;
    body->linear_velocity[1] = -0.18;
    body->angular_velocity = 0.42;
}

int main(void) {
    ChronoBody2D_C anchor;
    ChronoBody2D_C body;
    init_anchor(&anchor);
    init_body(&body);

    double local_anchor[2] = {0.0, 0.0};
    double axis_local[2] = {1.0, 0.0};

    ChronoCoupledConstraint2D_C constraint;
    chrono_coupled_constraint2d_init(&constraint,
                                     &anchor,
                                     &body,
                                     local_anchor,
                                     local_anchor,
                                     axis_local,
                                     0.6,
                                     0.28,
                                     1.0,
                                     0.25,
                                     0.0);

    ChronoCoupledConstraint2DEquationDesc_C eq_primary;
    memset(&eq_primary, 0, sizeof(eq_primary));
    eq_primary.ratio_distance = 1.0;
    chrono_coupled_constraint2d_add_equation(&constraint, &eq_primary);

    ChronoCoupledConstraint2DEquationDesc_C eq_stiff;
    memset(&eq_stiff, 0, sizeof(eq_stiff));
    eq_stiff.ratio_distance = 1.0 + 1e-8;
    eq_stiff.ratio_angle = 1e-6;
    chrono_coupled_constraint2d_add_equation(&constraint, &eq_stiff);

    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(&constraint, &policy);
    policy.enable_logging = 1;
    policy.log_cooldown = 0.0;
    policy.enable_auto_recover = 1;
    policy.max_drop = 2;
    chrono_coupled_constraint2d_set_condition_warning_policy(&constraint, &policy);

    TestLogger logger = {0};
    chrono_log_set_handler(test_log_handler, &logger);
    chrono_log_set_level(CHRONO_LOG_LEVEL_WARNING);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 5;

    const double dt = 0.004;
    for (int step = 0; step < 16; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
    }

    const ChronoCoupledConstraint2DDiagnostics_C *diag =
        chrono_coupled_constraint2d_get_diagnostics(&constraint);
    if (!diag || (diag->flags & CHRONO_COUPLED_DIAG_CONDITION_WARNING) == 0) {
        fprintf(stderr, "Expected condition warning flag, but it was not set.\n");
        return 1;
    }

    if (logger.count <= 0) {
        fprintf(stderr, "Expected custom logger to receive at least one warning.\n");
        return 1;
    }

    if (strstr(logger.last_message, "condition warning") == NULL) {
        fprintf(stderr, "Unexpected log message: %s\n", logger.last_message);
        return 1;
    }

    printf("Captured %d warning(s): %s\n", logger.count, logger.last_message);
    return 0;
}
