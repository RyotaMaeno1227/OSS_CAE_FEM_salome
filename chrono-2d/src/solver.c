#include <math.h>
#include <stdio.h>
#include <string.h>

#include "solver.h"

static double dot(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

static double cross2(const double a[2], const double b[2]) {
    return a[0] * b[1] - a[1] * b[0];
}

static void rotate(const double v[2], double angle, double out[2]) {
    double c = cos(angle);
    double s = sin(angle);
    out[0] = c * v[0] - s * v[1];
    out[1] = s * v[0] + c * v[1];
}

static void world_anchor(const Body2D *b, const double local[2], double out[2]) {
    double r[2];
    rotate(local, b->angle, r);
    out[0] = b->pos[0] + r[0];
    out[1] = b->pos[1] + r[1];
}

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

/* Effective mass for a single row J*[v_a, w_a, v_b, w_b]^T */
static double row_effective_mass(const Constraint2D *c, const double J[6]) {
    double denom = 0.0;
    if (c->a) {
        denom += (J[0] * J[0] + J[1] * J[1]) * c->a->inv_mass;
        denom += J[2] * J[2] * c->a->inv_inertia;
    }
    if (c->b) {
        denom += (J[3] * J[3] + J[4] * J[4]) * c->b->inv_mass;
        denom += J[5] * J[5] * c->b->inv_inertia;
    }
    if (denom <= 1e-12) {
        return 0.0;
    }
    return 1.0 / denom;
}

static void fill_linear_row(const Constraint2D *c,
                            const double dir[2],
                            const double world_a[2],
                            const double world_b[2],
                            double J[6]) {
    double ra[2] = {world_a[0] - c->a->pos[0], world_a[1] - c->a->pos[1]};
    double rb[2] = {world_b[0] - c->b->pos[0], world_b[1] - c->b->pos[1]};

    J[0] = -dir[0];
    J[1] = -dir[1];
    J[2] = -cross2(ra, dir);

    J[3] = dir[0];
    J[4] = dir[1];
    J[5] = cross2(rb, dir);
}

static void accumulate_stats(const double *pivots,
                             int count,
                             ConstraintStats *stats) {
    stats->min_pivot = 1e30;
    stats->max_pivot = 0.0;
    stats->condition_bound = 0.0;
    stats->condition_spectral = 0.0;
    for (int i = 0; i < count; ++i) {
        double p = pivots[i];
        if (p <= 0.0 || !isfinite(p)) {
            stats->min_pivot = 0.0;
            stats->max_pivot = 0.0;
            stats->condition_bound = 1e9;
            stats->condition_spectral = 1e9;
            return;
        }
        if (p < stats->min_pivot)
            stats->min_pivot = p;
        if (p > stats->max_pivot)
            stats->max_pivot = p;
    }
    stats->condition_bound =
        (stats->min_pivot > 0.0) ? (stats->max_pivot / stats->min_pivot) : 1e9;
    stats->condition_spectral = stats->condition_bound;
}

ConstraintStats compute_stats(const Constraint2D *c) {
    ConstraintStats stats = {0};
    double pivots[4];
    int pivot_count = 0;
    stats.j_row_count = 0;

    double world_a[2] = {0, 0};
    double world_b[2] = {0, 0};
    if (c->a)
        world_anchor(c->a, c->anchor_a, world_a);
    if (c->b)
        world_anchor(c->b, c->anchor_b, world_b);

    switch (c->type) {
        case CONSTRAINT_DISTANCE: {
            double diff[2] = {world_b[0] - world_a[0], world_b[1] - world_a[1]};
            double len = sqrt(diff[0] * diff[0] + diff[1] * diff[1]);
            double dir[2] = {diff[0] / (len + 1e-12), diff[1] / (len + 1e-12)};
            double J[6];
            fill_linear_row(c, dir, world_a, world_b, J);
            pivots[pivot_count++] = row_effective_mass(c, J);
            memcpy(stats.j_rows[stats.j_row_count++], J, sizeof(J));
            break;
        }
        case CONSTRAINT_REVOLUTE: {
            /* Two orthogonal rows to pin the anchors together */
            const double dirs[2][2] = {{1.0, 0.0}, {0.0, 1.0}};
            for (int i = 0; i < 2; ++i) {
                double J[6];
                fill_linear_row(c, dirs[i], world_a, world_b, J);
                pivots[pivot_count++] = row_effective_mass(c, J);
                if (stats.j_row_count < 2) {
                    memcpy(stats.j_rows[stats.j_row_count++], J, sizeof(J));
                }
            }
            break;
        }
        case CONSTRAINT_PLANAR:
        case CONSTRAINT_PRISMATIC: {
            double normal[2] = {-c->axis[1], c->axis[0]};
            double J[6];
            fill_linear_row(c, normal, world_a, world_b, J);
            pivots[pivot_count++] = row_effective_mass(c, J);
            memcpy(stats.j_rows[stats.j_row_count++], J, sizeof(J));
            break;
        }
        case CONSTRAINT_GEAR: {
            double J[6] = {0};
            J[2] = -1.0;
            J[5] = 1.0;
            pivots[pivot_count++] = row_effective_mass(c, J);
            memcpy(stats.j_rows[stats.j_row_count++], J, sizeof(J));
            break;
        }
        case CONSTRAINT_CONTACT: {
            double ra[2] = {c->contact_point[0] - c->a->pos[0],
                            c->contact_point[1] - c->a->pos[1]};
            double rb[2] = {c->contact_point[0] - c->b->pos[0],
                            c->contact_point[1] - c->b->pos[1]};
            double n[2] = {c->normal[0], c->normal[1]};
            double t[2] = {-n[1], n[0]};

            /* Relative velocity at contact to decide stick/slip */
            double va[2] = {c->a->vel[0] - c->a->ang_vel * ra[1],
                            c->a->vel[1] + c->a->ang_vel * ra[0]};
            double vb[2] = {c->b->vel[0] - c->b->ang_vel * rb[1],
                            c->b->vel[1] + c->b->ang_vel * rb[0]};
            double rel[2] = {vb[0] - va[0], vb[1] - va[1]};
            double vn = dot(rel, n);
            double vt = dot(rel, t);
            stats.vn = vn;
            stats.vt = vt;
            stats.mu_s = c->friction_static;
            stats.mu_d = c->friction_dynamic;
            double slip_tol = 1e-4;
            int stick = fabs(vt) <= (c->friction_static * fabs(vn) + slip_tol);
            stats.stick = stick;

            double Jn[6];
            fill_linear_row(c, n, c->contact_point, c->contact_point, Jn);
            pivots[pivot_count++] = row_effective_mass(c, Jn);
            memcpy(stats.j_rows[stats.j_row_count++], Jn, sizeof(Jn));

            double Jt[6];
            fill_linear_row(c, t, c->contact_point, c->contact_point, Jt);
            double pivot_t = row_effective_mass(c, Jt);
            if (!stick) {
                double scale = c->friction_dynamic * 0.5;
                if (scale < 0.1)
                    scale = 0.1;
                if (scale > 1.0)
                    scale = 1.0;
                pivot_t *= scale;
            }
            pivots[pivot_count++] = pivot_t;
            if (stats.j_row_count < 2) {
                memcpy(stats.j_rows[stats.j_row_count++], Jt, sizeof(Jt));
            }
            break;
        }
        default:
            break;
    }

    accumulate_stats(pivots, pivot_count, &stats);
    return stats;
}

double compute_pivot(const Constraint2D *c) {
    ConstraintStats stats = compute_stats(c);
    return stats.max_pivot;
}

double compute_condition(const Constraint2D *c) {
    ConstraintStats stats = compute_stats(c);
    return stats.condition_bound;
}

static void add_case(SolveResult *res,
                     const Constraint2D *c,
                     double time,
                     const ConstraintStats *stats) {
    if (res->count >= MAX_CASES)
        return;
    ConstraintCase *cc = &res->cases[res->count++];
    cc->name = c->name;
    cc->time = time;
    cc->condition_bound = stats->condition_bound;
    cc->condition_spectral = stats->condition_spectral;
    cc->min_pivot = stats->min_pivot;
    cc->max_pivot = stats->max_pivot;
    cc->vn = stats->vn;
    cc->vt = stats->vt;
    cc->mu_s = stats->mu_s;
    cc->mu_d = stats->mu_d;
    cc->stick = stats->stick;
    cc->type = c->type;
    memcpy(cc->axis, c->axis, sizeof(cc->axis));
    memcpy(cc->anchor_a, c->anchor_a, sizeof(cc->anchor_a));
    memcpy(cc->anchor_b, c->anchor_b, sizeof(cc->anchor_b));
    memcpy(cc->contact_point, c->contact_point, sizeof(cc->contact_point));
    memcpy(cc->normal, c->normal, sizeof(cc->normal));
    cc->mass_a = c->a ? c->a->mass : 0.0;
    cc->mass_b = c->b ? c->b->mass : 0.0;
    cc->inertia_a = c->a ? c->a->inertia : 0.0;
    cc->inertia_b = c->b ? c->b->inertia : 0.0;
    cc->j_row_count = stats->j_row_count;
    for (int i = 0; i < stats->j_row_count; ++i) {
        memcpy(cc->j_rows[i], stats->j_rows[i], sizeof(cc->j_rows[i]));
    }
}

static void build_default_bodies(Body2D *anchor, Body2D *bob, Body2D *slider) {
    body_init(anchor, 0.0, 0.0, 0.0, 0.0); /* static anchor */
    body_init(bob, 1.0, 0.2, 0.0, -0.5);
    bob->vel[0] = 0.05;
    bob->vel[1] = -0.02;
    bob->ang_vel = 0.5;
    body_init(slider, 1.1, 0.28, 0.2, 0.1);
    slider->vel[0] = -0.03;
    slider->vel[1] = 0.04;
    slider->ang_vel = -0.2;
}

static void setup_contact(Constraint2D *c, Body2D *a, Body2D *b, const double point[2], const double normal[2]) {
    c->a = a;
    c->b = b;
    c->type = CONSTRAINT_CONTACT;
    c->contact_point[0] = point[0];
    c->contact_point[1] = point[1];
    c->normal[0] = normal[0];
    c->normal[1] = normal[1];
    c->friction_static = 0.6;
    c->friction_dynamic = 0.4;
    c->restitution = 0.0;
}

static void push_constraint_case(SolveResult *res,
                                 const Constraint2D *c,
                                 double time) {
    ConstraintStats stats = compute_stats(c);
    add_case(res, c, time, &stats);
}

SolveResult run_coupled_constraint(void) {
    SolveResult res;
    memset(&res, 0, sizeof(res));

    Body2D anchor, bob, slider;
    build_default_bodies(&anchor, &bob, &slider);

    const double axis_x[2] = {1.0, 0.0};
    const double axis_y[2] = {0.0, 1.0};

    /* Stress: mass ratio 1:100 */
    Body2D heavy = slider;
    heavy.mass = 100.0;
    heavy.inv_mass = 0.01;
    heavy.inertia = 10.0;
    heavy.inv_inertia = 0.1;

    Constraint2D distance = {.name = "optic_alignment_trim",
                             .type = CONSTRAINT_DISTANCE,
                             .a = &anchor,
                             .b = &slider,
                             .anchor_a = {0.0, 0.0},
                             .anchor_b = {0.1, -0.05}};
    push_constraint_case(&res, &distance, 0.0);

    Constraint2D revolute = {.name = "tele_yaw_control",
                             .type = CONSTRAINT_REVOLUTE,
                             .a = &anchor,
                             .b = &bob,
                             .anchor_a = {0.0, 0.0},
                             .anchor_b = {0.0, 0.0}};
    push_constraint_case(&res, &revolute, 1.0);

    Constraint2D planar = {.name = "cam_follow_adjust",
                           .type = CONSTRAINT_PLANAR,
                           .a = &anchor,
                           .b = &slider,
                           .anchor_a = {0.0, 0.0},
                           .anchor_b = {0.0, 0.0},
                           .axis = {axis_x[0], axis_x[1]}};
    push_constraint_case(&res, &planar, 2.0);

    Constraint2D prismatic = {.name = "optic_alignment_trim_prismatic",
                              .type = CONSTRAINT_PRISMATIC,
                              .a = &anchor,
                              .b = &slider,
                              .anchor_a = {0.0, 0.0},
                              .anchor_b = {0.0, 0.0},
                              .axis = {axis_y[0], axis_y[1]}};
    push_constraint_case(&res, &prismatic, 3.0);

    Constraint2D gear = {.name = "counterbalance_beam",
                         .type = CONSTRAINT_GEAR,
                         .a = &anchor,
                         .b = &slider,
                         .anchor_a = {0.0, 0.0},
                         .anchor_b = {0.0, 0.0}};
    push_constraint_case(&res, &gear, 4.0);

    Constraint2D heavy_prismatic = {.name = "mass_ratio_100",
                                    .type = CONSTRAINT_PRISMATIC,
                                    .a = &anchor,
                                    .b = &heavy,
                                    .anchor_a = {0.0, 0.0},
                                    .anchor_b = {0.0, 0.0},
                                    .axis = {axis_y[0], axis_y[1]}};
    push_constraint_case(&res, &heavy_prismatic, 4.5);

    /* Composite constraint: distance + planar on mixed masses to check interference */
    Constraint2D composite_planar = {.name = "composite_planar_distance",
                                     .type = CONSTRAINT_PLANAR,
                                     .a = &anchor,
                                     .b = &heavy,
                                     .anchor_a = {0.0, 0.0},
                                     .anchor_b = {0.2, -0.05},
                                     .axis = {axis_x[0], axis_x[1]}};
    push_constraint_case(&res, &composite_planar, 5.0);
    Constraint2D composite_distance = {.name = "composite_distance",
                                       .type = CONSTRAINT_DISTANCE,
                                       .a = &anchor,
                                       .b = &heavy,
                                       .anchor_a = {0.0, 0.0},
                                       .anchor_b = {0.2, -0.05}};
    push_constraint_case(&res, &composite_distance, 5.1);

    /* Composite constraint: prismatic + distance on bob to surface interference */
    Constraint2D composite_prismatic = {.name = "composite_prismatic_distance",
                                        .type = CONSTRAINT_PRISMATIC,
                                        .a = &anchor,
                                        .b = &bob,
                                        .anchor_a = {0.0, 0.0},
                                        .anchor_b = {0.15, 0.02},
                                        .axis = {axis_y[0], axis_y[1]}};
    push_constraint_case(&res, &composite_prismatic, 5.2);
    Constraint2D composite_prismatic_dist = {.name = "composite_prismatic_distance_aux",
                                             .type = CONSTRAINT_DISTANCE,
                                             .a = &anchor,
                                             .b = &bob,
                                             .anchor_a = {0.0, 0.0},
                                             .anchor_b = {0.15, 0.02}};
    push_constraint_case(&res, &composite_prismatic_dist, 5.3);

    /* Contact stick (low tangential velocity) */
    Constraint2D contact_stick;
    double contact_point[2] = {0.05, -0.05};
    double normal[2] = {0.0, 1.0};
    slider.vel[0] = 0.0;
    slider.vel[1] = 0.0;
    slider.ang_vel = 0.0;
    setup_contact(&contact_stick, &anchor, &slider, contact_point, normal);
    contact_stick.name = "hydraulic_lift_sync";
    push_constraint_case(&res, &contact_stick, 5.0);

    /* Contact slip (higher tangential velocity) */
    Constraint2D contact_slip;
    Body2D slider_fast = slider;
    slider_fast.vel[0] = 0.4;
    slider_fast.vel[1] = 0.0;
    slider_fast.ang_vel = 0.0;
    setup_contact(&contact_slip, &anchor, &slider_fast, contact_point, normal);
    contact_slip.name = "hydraulic_lift_sync_slip";
    push_constraint_case(&res, &contact_slip, 6.0);

    return res;
}

SolveResult run_contact_regression(void) {
    SolveResult res;
    memset(&res, 0, sizeof(res));

    Body2D anchor, bob, slider;
    build_default_bodies(&anchor, &bob, &slider);

    Constraint2D contact_stick;
    double normal[2] = {0.0, 1.0};
    double point[2] = {0.05, -0.05};
    slider.vel[0] = 0.0;
    slider.vel[1] = 0.0;
    slider.ang_vel = 0.0;
    setup_contact(&contact_stick, &anchor, &slider, point, normal);
    contact_stick.name = "contact_stick";
    push_constraint_case(&res, &contact_stick, 0.0);

    Body2D slider_fast = slider;
    slider_fast.vel[0] = 0.35;
    slider_fast.vel[1] = 0.0;
    slider_fast.ang_vel = 0.0;
    Constraint2D contact_slip;
    setup_contact(&contact_slip, &anchor, &slider_fast, point, normal);
    contact_slip.name = "contact_slip";
    push_constraint_case(&res, &contact_slip, 0.0);

    return res;
}
