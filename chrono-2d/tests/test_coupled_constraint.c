#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

static void write_descriptor_csv(const char *path, const SolveResult *res) {
    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "Failed to open %s for write\n", path);
        return;
    }
    fprintf(fp, "time,case,method,condition_bound,condition_spectral,min_pivot,max_pivot\n");
    for (int i = 0; i < res->count; ++i) {
        const ConstraintCase *c = &res->cases[i];
        fprintf(fp,
                "%.6f,%s,%s,%.6e,%.6e,%.6e,%.6e\n",
                c->time,
                c->name,
                "actions",
                c->condition_bound,
                c->condition_spectral,
                c->min_pivot,
                c->max_pivot);
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    const char *out_path = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--descriptor-log") == 0 && i + 1 < argc) {
            out_path = argv[i + 1];
        }
    }

    SolveResult res = run_coupled_constraint();

    if (out_path) {
        write_descriptor_csv(out_path, &res);
        printf("Descriptor log written to %s\n", out_path);
    }

    // Basic sanity checks: finite pivots and reasonable bounds
    for (int i = 0; i < res.count; ++i) {
        const ConstraintCase *c = &res.cases[i];
        if (c->min_pivot <= 0.0 || c->max_pivot <= 0.0 || !isfinite(c->min_pivot) ||
            !isfinite(c->max_pivot)) {
            fprintf(stderr, "Invalid pivot for case %s\n", c->name);
            return 1;
        }
        if (c->condition_spectral <= 0.0 || !isfinite(c->condition_spectral)) {
            fprintf(stderr, "Invalid condition for case %s\n", c->name);
            return 1;
        }
    }

    printf("Coupled constraint test passed.\n");
    return 0;
}
