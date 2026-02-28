#include <stddef.h>
#include <stdio.h>

#if __has_include("../chrono-C-all/include/chrono_constraint2d.h")
#  include "../chrono-C-all/include/chrono_constraint2d.h"
#  define CHRONO_COUPLED_AVAILABLE 1
#endif

static const double kRatios[] = {0.85, 1.0, 1.15};

static void log_condition_metrics(double ratio_distance, double ratio_angle)
{
#if CHRONO_COUPLED_AVAILABLE
    ChronoCoupledConstraint2D_C constraint = {0};
    ChronoCoupledConstraint2DEquationDesc_C eq = {
        .ratio_distance = ratio_distance,
        .ratio_angle = ratio_angle,
        .softness_distance = 0.015,
        .softness_angle = 0.028,
    };
    chrono_coupled_constraint2d_init(&constraint, &eq);
    ChronoCoupledConstraint2DDiagnostics_C diag = {0};
    chrono_coupled_constraint2d_get_diagnostics(&constraint, &diag);
    printf("ratio_distance=%.2f ratio_angle=%.2f cond_row=%.3f cond_spectral=%.3f pivot_min=%.5f pivot_max=%.5f\n",
           ratio_distance,
           ratio_angle,
           diag.condition_number_row_sum,
           diag.condition_number_spectral,
           diag.min_pivot,
           diag.max_pivot);
#else
    /* Placeholder diagnostics when Chrono headers are unavailable. */
    printf("ratio_distance=%.2f ratio_angle=%.2f cond_est=%.3f pivot_min=%.5f pivot_max=%.5f\n",
           ratio_distance,
           ratio_angle,
           1.0 / ratio_distance,
           0.001 * ratio_distance,
           0.01 * ratio_distance);
#endif
}

int main(void)
{
    puts("[ch01_ratio_sweep] scanning ratios for tutorial alignment");
    for (size_t i = 0; i < sizeof(kRatios) / sizeof(kRatios[0]); ++i)
    {
        const double ratio = kRatios[i];
        log_condition_metrics(ratio, ratio * 0.35);
    }
    puts("See docs/coupled_constraint_hands_on.md Chapter 01 for analysis template.");
    return 0;
}
