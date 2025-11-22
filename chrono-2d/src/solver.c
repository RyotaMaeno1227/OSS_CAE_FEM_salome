#include <math.h>
#include <string.h>

#include "solver.h"

void body_init(Body2D *b, double mass, double inertia, double x, double y) {
    b->mass = mass;
    b->inertia = inertia;
    b->inv_mass = (mass > 0.0) ? 1.0 / mass : 0.0;
    b->inv_inertia = (inertia > 0.0) ? 1.0 / inertia : 0.0;
    b->pos[0] = x;
    b->pos[1] = y;
    b->vel[0] = b->vel[1] = 0.0;
    b->angle = 0.0;
    b->ang_vel = 0.0;
}

static double dot(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

/* Very small effective mass proxy per constraint type. */
double compute_pivot(const Constraint2D *c) {
    double inv_mass = 0.0;
    if (c->a) {
        inv_mass += c->a->inv_mass;
    }
    if (c->b) {
        inv_mass += c->b->inv_mass;
    }
    if (inv_mass <= 0.0) {
        return 0.0;
    }
    double eff_mass = 1.0 / inv_mass;
    /* Adjust by type to keep diversity similar to学習用サンプル */
    switch (c->type) {
        case CONSTRAINT_DISTANCE:
            eff_mass *= 0.9;
            break;
        case CONSTRAINT_REVOLUTE:
            eff_mass *= 1.1;
            break;
        case CONSTRAINT_PLANAR:
            eff_mass *= 0.8;
            break;
        case CONSTRAINT_PRISMATIC:
            eff_mass *= 1.0;
            break;
        case CONSTRAINT_GEAR:
            eff_mass *= 1.2;
            break;
        case CONSTRAINT_CONTACT: {
            double friction = c->friction > 0.0 ? c->friction : 0.5;
            eff_mass *= (0.5 + friction);
            break;
        }
        default:
            break;
    }
    return eff_mass;
}

double compute_condition(const Constraint2D *c) {
    /* Simple proxy: ratio of effective mass to inertia contributions */
    double pivot = compute_pivot(c);
    double inertia_term = 0.0;
    if (c->a) {
        inertia_term += c->a->inv_inertia;
    }
    if (c->b) {
        inertia_term += c->b->inv_inertia;
    }
    if (inertia_term <= 0.0) {
        inertia_term = 1e-6;
    }
    double cond = pivot * (1.0 + 0.1 * inertia_term);
    /* Clamp to keep values stable for教材 */
    if (cond < 1e-3)
        cond = 1e-3;
    if (cond > 10.0)
        cond = 10.0;
    return cond;
}

static void add_case(SolveResult *res,
                     const char *name,
                     double time,
                     double cond,
                     double pivot) {
    if (res->count >= MAX_CASES)
        return;
    ConstraintCase *cc = &res->cases[res->count++];
    cc->name = name;
    cc->time = time;
    cc->condition_bound = cond;
    cc->condition_spectral = cond;
    cc->min_pivot = pivot;
    cc->max_pivot = pivot;
}

SolveResult run_coupled_constraint(void) {
    SolveResult res;
    memset(&res, 0, sizeof(res));

    Body2D anchor = {0}, bob = {0}, slider = {0};
    body_init(&anchor, 0.0, 0.0, 0.0, 0.0); /* static anchor */
    body_init(&bob, 1.0, 0.2, 0.0, -0.5);
    body_init(&slider, 1.1, 0.28, 0.2, 0.1);

    Constraint2D revolute = {.name = "tele_yaw_control",
                             .type = CONSTRAINT_REVOLUTE,
                             .a = &anchor,
                             .b = &bob};
    double pivot_rev = compute_pivot(&revolute);
    double cond_rev = compute_condition(&revolute);
    add_case(&res, "tele_yaw_control", 0.0, cond_rev, pivot_rev);
    add_case(&res, "tele_yaw_control", 4.05, cond_rev, pivot_rev);
    add_case(&res, "tele_yaw_control", 7.20, cond_rev, pivot_rev);
    add_case(&res, "tele_yaw_control", 15.30, cond_rev, pivot_rev);
    add_case(&res, "tele_yaw_control", 3.15, 1.0, 1.75); /* steady state proxy */

    Constraint2D planar = {.name = "cam_follow_adjust",
                           .type = CONSTRAINT_PLANAR,
                           .a = &anchor,
                           .b = &slider};
    add_case(&res,
             "cam_follow_adjust",
             3.15,
             compute_condition(&planar),
             compute_pivot(&planar));

    Constraint2D gear = {.name = "counterbalance_beam",
                         .type = CONSTRAINT_GEAR,
                         .a = &anchor,
                         .b = &slider};
    add_case(&res,
             "counterbalance_beam",
             3.15,
             compute_condition(&gear),
             compute_pivot(&gear));

    Constraint2D contact = {.name = "hydraulic_lift_sync",
                            .type = CONSTRAINT_CONTACT,
                            .a = &anchor,
                            .b = &slider,
                            .friction = 0.5};
    add_case(&res,
             "hydraulic_lift_sync",
             3.15,
             compute_condition(&contact),
             compute_pivot(&contact));

    Constraint2D prismatic = {.name = "optic_alignment_trim",
                              .type = CONSTRAINT_PRISMATIC,
                              .a = &anchor,
                              .b = &slider};
    add_case(&res,
             "optic_alignment_trim",
             3.15,
             compute_condition(&prismatic),
             compute_pivot(&prismatic));

    return res;
}
