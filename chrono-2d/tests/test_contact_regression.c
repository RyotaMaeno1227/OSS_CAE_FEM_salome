#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/solver.h"

typedef struct {
    char name[64];
    double vn;
    double vt;
    double mu_s;
    double mu_d;
    int expected_stick;
} ContactCaseDef;

static int load_contact_cases(const char *path, ContactCaseDef *defs, int max_defs) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open contact dataset %s\n", path);
        return 0;
    }
    char line[256];
    /* header */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0;
    }
    int count = 0;
    while (fgets(line, sizeof(line), fp) && count < max_defs) {
        ContactCaseDef def = {0};
        int stick;
        if (sscanf(line, "%63[^,],%lf,%lf,%lf,%lf,%d",
                   def.name, &def.vn, &def.vt, &def.mu_s, &def.mu_d, &stick) == 6) {
            def.expected_stick = stick;
            defs[count++] = def;
        }
    }
    fclose(fp);
    return count;
}

static const ConstraintCase *find_case(const SolveResult *res, const char *name) {
    for (int i = 0; i < res->count; ++i) {
        if (strcmp(res->cases[i].name, name) == 0) {
            return &res->cases[i];
        }
    }
    return NULL;
}

int main(void) {
    ContactCaseDef defs[16];
    int case_count = load_contact_cases("data/contact_cases.csv", defs, 16);
    if (case_count <= 0) {
        fprintf(stderr, "No contact dataset loaded\n");
        return 1;
    }

    for (int idx = 0; idx < case_count; ++idx) {
        ContactCaseDef *d = &defs[idx];
        Body2D a, b;
        body_init(&a, 0.0, 0.0, 0.0, 0.0);
        body_init(&b, 1.0, 0.2, 0.0, 0.0);
        b.vel[0] = d->vt; /* tangential along x */
        b.vel[1] = d->vn; /* normal along y */
        Constraint2D c = {0};
        c.name = d->name;
        c.type = CONSTRAINT_CONTACT;
        c.a = &a;
        c.b = &b;
        c.contact_point[0] = 0.0;
        c.contact_point[1] = 0.0;
        c.normal[0] = 0.0;
        c.normal[1] = 1.0;
        c.friction_static = d->mu_s;
        c.friction_dynamic = d->mu_d;
        c.restitution = 0.0;

        ConstraintStats stats = compute_stats(&c);
        if (stats.stick != d->expected_stick) {
            fprintf(stderr, "%s stick flag mismatch (got %d, expected %d)\n",
                    d->name, stats.stick, d->expected_stick);
            return 1;
        }
        if (stats.condition_bound <= 0.0 || stats.min_pivot <= 0.0) {
            fprintf(stderr, "%s invalid stats\n", d->name);
            return 1;
        }
        if (!stats.stick) {
            if (stats.condition_bound < 3.0 || stats.condition_bound > 50.0) {
                fprintf(stderr, "%s slip cond out of range %.3f\n", d->name, stats.condition_bound);
                return 1;
            }
        } else {
            if (stats.condition_bound < 1.0 || stats.condition_bound > 10.0) {
                fprintf(stderr, "%s stick cond out of range %.3f\n", d->name, stats.condition_bound);
                return 1;
            }
        }
    }
    printf("Contact regression test passed.\n");
    return 0;
}
