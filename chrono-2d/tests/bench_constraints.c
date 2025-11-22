#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

int main(int argc, char **argv) {
    int iterations = 10000;
    const char *out_path = "artifacts/bench_constraints.csv";
    const char *baseline_path = "data/bench_baseline.csv";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            out_path = argv[i + 1];
        }
    }

    double start = omp_get_wtime();
    SolveResult last = run_coupled_constraint();
    for (int i = 1; i < iterations; ++i) {
        (void)run_coupled_constraint();
    }
    double elapsed = omp_get_wtime() - start;
    double time_us = (elapsed / iterations) * 1e6;

    FILE *fp = fopen(out_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", out_path);
        return 1;
    }
    fprintf(fp, "case,threads,time_us\n");
    int threads = omp_get_max_threads();
    fprintf(fp, "%s,%d,%.3f\n", "run_coupled_constraint", threads, time_us);
    fclose(fp);

    /* Compare against baseline if present */
    FILE *base = fopen(baseline_path, "r");
    if (base) {
        char line[256];
        fgets(line, sizeof(line), base); /* header */
        double base_time = 0.0;
        int base_threads = 0;
        if (fgets(line, sizeof(line), base)) {
            char casename[64];
            if (sscanf(line, "%63[^,],%d,%lf", casename, &base_threads, &base_time) == 3) {
                double threshold = base_time * 1.5;
                if (time_us > threshold) {
                    fprintf(stderr,
                            "Benchmark regression: %.3f us > 1.5x baseline %.3f us\n",
                            time_us, threshold);
                    fclose(base);
                    return 1;
                }
            }
        }
        fclose(base);
    }

    /* Sanity: ensure outputs remain finite */
    for (int i = 0; i < last.count; ++i) {
        if (!isfinite(last.cases[i].condition_bound) || last.cases[i].condition_bound <= 0.0) {
            fprintf(stderr, "Invalid condition after bench for %s\n", last.cases[i].name);
            return 1;
        }
    }

    printf("Benchmark completed: %s (%.3f us per iter, threads=%d)\n", out_path, time_us, threads);
    return 0;
}
