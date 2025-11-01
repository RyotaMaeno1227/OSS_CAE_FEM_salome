#include "../include/chrono_body2d.h"
#include "../include/chrono_constraint2d.h"
#include "../include/chrono_logging.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct DemoLogger {
    int count;
    char last_message[256];
} DemoLogger;

static void demo_log_handler(ChronoLogLevel_C level,
                             ChronoLogCategory_C category,
                             const char *message,
                             void *user_data) {
    (void)level;
    (void)category;
    if (!user_data || !message) {
        return;
    }
    DemoLogger *logger = (DemoLogger *)user_data;
    logger->count += 1;
    snprintf(logger->last_message, sizeof(logger->last_message), "%s", message);
}

static void init_anchor(ChronoBody2D_C *anchor) {
    chrono_body2d_init(anchor);
    chrono_body2d_set_static(anchor);
    anchor->position[0] = -0.2;
    anchor->position[1] = 0.1;
}

static void init_body(ChronoBody2D_C *body) {
    chrono_body2d_init(body);
    chrono_body2d_set_mass(body, 1.0, 0.32);
    chrono_body2d_set_circle_shape(body, 0.2);
    body->position[0] = 0.55;
    body->position[1] = 0.45;
    body->angle = 0.35;
    body->linear_velocity[0] = 0.25;
    body->linear_velocity[1] = -0.22;
    body->angular_velocity = 0.5;
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
                                     0.3,
                                     1.0,
                                     0.25,
                                     0.0);

    ChronoCoupledConstraint2DEquationDesc_C stiff_a;
    memset(&stiff_a, 0, sizeof(stiff_a));
    stiff_a.ratio_distance = 1.0;
    chrono_coupled_constraint2d_add_equation(&constraint, &stiff_a);

    ChronoCoupledConstraint2DEquationDesc_C stiff_b;
    memset(&stiff_b, 0, sizeof(stiff_b));
    stiff_b.ratio_distance = 1.0 + 1e-8;
    stiff_b.ratio_angle = 1e-6;
    chrono_coupled_constraint2d_add_equation(&constraint, &stiff_b);

    ChronoCoupledConditionWarningPolicy_C policy;
    chrono_coupled_constraint2d_get_condition_warning_policy(&constraint, &policy);
    policy.enable_logging = 1;
    policy.log_cooldown = 0.0;
    policy.enable_auto_recover = 1;
    policy.max_drop = 2;
    chrono_coupled_constraint2d_set_condition_warning_policy(&constraint, &policy);

    DemoLogger logger = {0};
    chrono_log_set_handler(demo_log_handler, &logger);
    chrono_log_set_level(CHRONO_LOG_LEVEL_WARNING);

    ChronoConstraint2DBase_C *constraints[1] = {&constraint.base};
    ChronoConstraint2DBatchConfig_C cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.velocity_iterations = 24;
    cfg.position_iterations = 5;
    cfg.enable_parallel = 0;

    const double dt = 0.004;
    for (int step = 0; step < 16; ++step) {
        chrono_constraint2d_batch_solve(constraints, 1, dt, &cfg, NULL);
        chrono_body2d_integrate_explicit(&body, dt);
        chrono_body2d_reset_forces(&body);
    }

    printf("Captured %d warning(s).\n", logger.count);
    if (logger.count > 0) {
        printf("Last message: %s\n", logger.last_message);
    }
    return logger.count > 0 ? 0 : 1;
}
