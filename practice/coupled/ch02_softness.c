#include <stdio.h>

static void write_csv_header(FILE *fp)
{
    fputs("phase,softness_distance,softness_angle,min_pivot,max_pivot\n", fp);
}

static void emit_phase(FILE *fp, const char *phase, double softness_d, double softness_a)
{
    /* TODO: Collect real diagnostics via chrono_coupled_constraint2d_get_diagnostics. */
    double min_pivot = softness_d * 0.08;
    double max_pivot = softness_a * 0.12;
    fprintf(fp, "%s,%.4f,%.4f,%.6f,%.6f\n", phase, softness_d, softness_a, min_pivot, max_pivot);
}

int main(void)
{
    const char *csv_path = "data/diagnostics/ch02_softness_sample.csv";
    FILE *fp = fopen(csv_path, "w");
    if (!fp)
    {
        perror("fopen");
        return 1;
    }
    write_csv_header(fp);
    emit_phase(fp, "stage_a", 0.015, 0.028);
    emit_phase(fp, "stage_b", 0.018, 0.032);
    emit_phase(fp, "stage_c", 0.020, 0.034);
    fclose(fp);
    printf("[ch02_softness] wrote placeholder CSV to %s\n", csv_path);
    puts("Replace emit_phase with actual bench_coupled_constraint invocations as described in the Hands-on guide.");
    return 0;
}
