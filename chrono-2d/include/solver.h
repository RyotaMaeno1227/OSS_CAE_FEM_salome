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
    double axis[2];         /* For planar/prismatic/gear (unit vector) */
    double anchor_a[2];     /* Local anchors (body A) */
    double anchor_b[2];     /* Local anchors (body B) */
    double contact_point[2];/* World contact point (for contact) */
    double normal[2];       /* World normal (for contact) */
    double friction_static; /* Static friction coeff (contact) */
    double friction_dynamic;/* Dynamic friction coeff (contact) */
    double restitution;     /* Normal restitution (contact) */
} Constraint2D;

typedef struct {
    const char *name;
    double condition_bound;
    double condition_spectral;
    double min_pivot;
    double max_pivot;
    double time;
    /* Diagnostics (contact only uses vn/vt/mu/stick) */
    double vn;
    double vt;
    double mu_s;
    double mu_d;
    int stick; /* 1=stick, 0=slip */
    ConstraintType type;
    double axis[2];
    double anchor_a[2];
    double anchor_b[2];
    double contact_point[2];
    double normal[2];
    double mass_a;
    double mass_b;
    double inertia_a;
    double inertia_b;
    double j_rows[2][6];
    int j_row_count;
} ConstraintCase;

typedef struct {
    ConstraintCase cases[MAX_CASES];
    int count;
} SolveResult;

typedef struct {
    double min_pivot;
    double max_pivot;
    double condition_bound;
    double condition_spectral;
    double vn;
    double vt;
    double mu_s;
    double mu_d;
    int stick;
    double j_rows[2][6];
    int j_row_count;
} ConstraintStats;

void body_init(Body2D *b, double mass, double inertia, double x, double y);
double compute_pivot(const Constraint2D *c);
double compute_condition(const Constraint2D *c);
SolveResult run_coupled_constraint(void);
SolveResult run_contact_regression(void);
ConstraintStats compute_stats(const Constraint2D *c);

#endif  // CHRONO2D_SOLVER_H
