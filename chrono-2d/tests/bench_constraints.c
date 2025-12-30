#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

int main(int argc, char **argv) {
    int iterations = 10000;
    const char *out_path = "artifacts/bench_constraints.csv";
    const char *baseline_path = CHRONO2D_DATA_PATH("bench_baseline.csv");
    int warn_only = 0;
    double threshold_scale = 1.5;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--iterations") == 0 && i + 1 < argc) {
            iterations = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            out_path = argv[i + 1];
        } else if (strcmp(argv[i], "--warn-only") == 0) {
            warn_only = 1;
        } else if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            threshold_scale = atof(argv[i + 1]);
        }
    }

    FILE *fp = fopen(out_path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", out_path);
        return 1;
    }
    fprintf(fp, "case,threads,time_us\n");

    /* load baseline map */
    int base_threads[16] = {0};
    double base_time[16] = {0};
    int base_count = 0;
    FILE *base = fopen(baseline_path, "r");
    if (base) {
        char line[256];
        if (!fgets(line, sizeof(line), base)) {
            fclose(base);
            fclose(fp);
            return 1;
        }
        while (fgets(line, sizeof(line), base) && base_count < 16) {
            char casename[64];
            int th = 0;
            double bt = 0.0;
            if (sscanf(line, "%63[^,],%d,%lf", casename, &th, &bt) == 3) {
                base_threads[base_count] = th;
                base_time[base_count] = bt;
                base_count++;
            }
        }
        fclose(base);
    }

    int thread_list[] = {1, 2, 4, 8};
    int thread_len = 4;

    for (int t = 0; t < thread_len; ++t) {
        int th = thread_list[t];
        if (th > omp_get_max_threads()) {
            th = omp_get_max_threads();
        }
        omp_set_num_threads(th);
        double start = omp_get_wtime();
        SolveResult last = run_coupled_constraint();
        for (int i = 1; i < iterations; ++i) {
            (void)run_coupled_constraint();
        }
        double elapsed = omp_get_wtime() - start;
        double time_us = (elapsed / iterations) * 1e6;
        fprintf(fp, "%s,%d,%.3f\n", "run_coupled_constraint", th, time_us);

        for (int i = 0; i < last.count; ++i) {
            if (!isfinite(last.cases[i].condition_bound) || last.cases[i].condition_bound <= 0.0) {
                fprintf(stderr, "Invalid condition after bench for %s\n", last.cases[i].name);
                fclose(fp);
                return 1;
            }
        }

        for (int b = 0; b < base_count; ++b) {
            if (base_threads[b] == th && base_time[b] > 0.0) {
                double threshold = base_time[b] * threshold_scale;
                if (time_us > threshold) {
                    fprintf(stderr,
                            "Benchmark regression (threads=%d): %.3f us > 1.5x baseline %.3f us\n",
                            th, time_us, threshold);
                    if (!warn_only) {
                        fclose(fp);
                        return 1;
                    }
                }
            }
        }
    }

    fclose(fp);
    printf("Benchmark completed: %s\n", out_path);
    return 0;
}
