#include <stddef.h>
#include <stdio.h>

static const double kRatios[] = {0.85, 1.0, 1.15};

static void log_condition_metrics(double ratio_distance, double ratio_angle)
{
    /* TODO: Replace placeholders with chrono_coupled_constraint2d_* API calls. */
    printf("ratio_distance=%.2f ratio_angle=%.2f cond_est=%.3f pivot_min=%.5f pivot_max=%.5f\n",
           ratio_distance,
           ratio_angle,
           1.0 / ratio_distance,
           0.001 * ratio_distance,
           0.01 * ratio_distance);
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
