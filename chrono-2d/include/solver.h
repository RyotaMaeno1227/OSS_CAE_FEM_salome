#ifndef CHRONO2D_SOLVER_H
#define CHRONO2D_SOLVER_H

#include <stddef.h>

#define MAX_CASES 32

typedef struct {
    double mass;
    double inv_mass;
    double inertia;
    double inv_inertia;
    double pos[2];
    double vel[2];
    double angle;
    double ang_vel;
} Body2D;

typedef enum {
    CONSTRAINT_DISTANCE,
    CONSTRAINT_REVOLUTE,
    CONSTRAINT_PLANAR,
    CONSTRAINT_PRISMATIC,
    CONSTRAINT_GEAR,
    CONSTRAINT_CONTACT
} ConstraintType;

typedef struct {
    const char *name;
    ConstraintType type;
    Body2D *a;
    Body2D *b;
    double axis[2];        /* For planar/prismatic/gear */
    double anchor_a[2];    /* Local anchors */
    double anchor_b[2];
    double friction;       /* For contact */
} Constraint2D;

typedef struct {
    const char *name;
    double condition_bound;
    double condition_spectral;
    double min_pivot;
    double max_pivot;
    double time;
} ConstraintCase;

typedef struct {
    ConstraintCase cases[MAX_CASES];
    int count;
} SolveResult;

void body_init(Body2D *b, double mass, double inertia, double x, double y);
double compute_pivot(const Constraint2D *c);
double compute_condition(const Constraint2D *c);
SolveResult run_coupled_constraint(void);

#endif  // CHRONO2D_SOLVER_H
