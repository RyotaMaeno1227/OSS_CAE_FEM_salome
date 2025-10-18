#include "../include/chrono_constraint2d.h"

#include <math.h>
#include <string.h>

static double length(const double v[2]) {
    return sqrt(v[0] * v[0] + v[1] * v[1]);
}

static double dot(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

static double cross(const double a[2], const double b[2]) {
    return a[0] * b[1] - a[1] * b[0];
}

static void scale(const double v[2], double s, double out[2]) {
    out[0] = v[0] * s;
    out[1] = v[1] * s;
}

static void add(const double a[2], const double b[2], double out[2]) {
    out[0] = a[0] + b[0];
    out[1] = a[1] + b[1];
}

static void sub(const double a[2], const double b[2], double out[2]) {
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
}

static void apply_impulse(ChronoBody2D_C *body, const double impulse[2], const double r[2]) {
    if (!body || body->is_static) {
        return;
    }
    body->linear_velocity[0] += impulse[0] * body->inverse_mass;
    body->linear_velocity[1] += impulse[1] * body->inverse_mass;
    double angular_impulse = cross(r, impulse);
    body->angular_velocity += angular_impulse * body->inverse_inertia;
}

void chrono_distance_constraint2d_init(ChronoDistanceConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2],
                                       double rest_length) {
    if (!constraint) {
        return;
    }
    memset(constraint, 0, sizeof(*constraint));
    constraint->body_a = body_a;
    constraint->body_b = body_b;
    if (local_anchor_a) {
        constraint->local_anchor_a[0] = local_anchor_a[0];
        constraint->local_anchor_a[1] = local_anchor_a[1];
    }
    if (local_anchor_b) {
        constraint->local_anchor_b[0] = local_anchor_b[0];
        constraint->local_anchor_b[1] = local_anchor_b[1];
    }
    constraint->rest_length = rest_length;
    constraint->baumgarte_beta = 0.2;
    constraint->softness = 0.0;
    constraint->slop = 0.001;
    constraint->max_correction = 0.2;
    constraint->accumulated_impulse = 0.0;
    constraint->effective_mass = 0.0;
    constraint->bias = 0.0;
}

void chrono_distance_constraint2d_set_baumgarte(ChronoDistanceConstraint2D_C *constraint, double beta) {
    if (!constraint) {
        return;
    }
    if (beta < 0.0) {
        beta = 0.0;
    }
    if (beta > 1.0) {
        beta = 1.0;
    }
    constraint->baumgarte_beta = beta;
}

void chrono_distance_constraint2d_set_softness(ChronoDistanceConstraint2D_C *constraint, double softness) {
    if (!constraint) {
        return;
    }
    if (softness < 0.0) {
        softness = 0.0;
    }
    constraint->softness = softness;
}

void chrono_distance_constraint2d_set_slop(ChronoDistanceConstraint2D_C *constraint, double slop) {
    if (!constraint) {
        return;
    }
    if (slop < 0.0) {
        slop = 0.0;
    }
    constraint->slop = slop;
}

void chrono_distance_constraint2d_set_max_correction(ChronoDistanceConstraint2D_C *constraint, double max_correction) {
    if (!constraint) {
        return;
    }
    if (max_correction < 0.0) {
        max_correction = 0.0;
    }
    constraint->max_correction = max_correction;
}

void chrono_distance_constraint2d_prepare(ChronoDistanceConstraint2D_C *constraint, double dt) {
    if (!constraint || dt <= 0.0) {
        return;
    }

    ChronoBody2D_C *body_a = constraint->body_a;
    ChronoBody2D_C *body_b = constraint->body_b;

    chrono_body2d_local_to_world(body_a, constraint->local_anchor_a, constraint->ra);
    sub(constraint->ra, body_a->position, constraint->ra);
    chrono_body2d_local_to_world(body_b, constraint->local_anchor_b, constraint->rb);
    sub(constraint->rb, body_b->position, constraint->rb);

    double pa[2];
    add(body_a->position, constraint->ra, pa);
    double pb[2];
    add(body_b->position, constraint->rb, pb);

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist > 1e-9) {
        constraint->normal[0] = delta[0] / dist;
        constraint->normal[1] = delta[1] / dist;
    } else {
        constraint->normal[0] = 1.0;
        constraint->normal[1] = 0.0;
        dist = 0.0;
    }

    double ra_cross_n = cross(constraint->ra, constraint->normal);
    double rb_cross_n = cross(constraint->rb, constraint->normal);

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }

    if (inv_mass > 0.0) {
        constraint->effective_mass = 1.0 / inv_mass;
    } else {
        constraint->effective_mass = 0.0;
    }

    double C = dist - constraint->rest_length;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;

    double beta = constraint->baumgarte_beta;
    double bias = 0.0;
    if (beta > 0.0) {
        bias = -beta / dt * error;
    }
    constraint->bias = bias;

    if (constraint->softness > 0.0) {
        constraint->effective_mass = 1.0 / (inv_mass + constraint->softness);
    }
}

void chrono_distance_constraint2d_apply_warm_start(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    double impulse_vec[2];
    scale(constraint->normal, constraint->accumulated_impulse, impulse_vec);
    apply_impulse(constraint->body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(constraint->body_b, impulse_vec, constraint->rb);
}

void chrono_distance_constraint2d_solve_velocity(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->body_a;
    ChronoBody2D_C *body_b = constraint->body_b;

    double va[2] = {0.0, 0.0};
    double vb[2] = {0.0, 0.0};

    if (body_a && !body_a->is_static) {
        va[0] = body_a->linear_velocity[0];
        va[1] = body_a->linear_velocity[1];
        double cross_ra = -body_a->angular_velocity * constraint->ra[1];
        double cross_ra_y = body_a->angular_velocity * constraint->ra[0];
        va[0] += cross_ra;
        va[1] += cross_ra_y;
    }
    if (body_b && !body_b->is_static) {
        vb[0] = body_b->linear_velocity[0];
        vb[1] = body_b->linear_velocity[1];
        double cross_rb = -body_b->angular_velocity * constraint->rb[1];
        double cross_rb_y = body_b->angular_velocity * constraint->rb[0];
        vb[0] += cross_rb;
        vb[1] += cross_rb_y;
    }

    double dv[2];
    sub(vb, va, dv);
    double Cdot = dot(dv, constraint->normal);

    double gamma = constraint->softness;
    double denom = constraint->effective_mass;
    if (denom == 0.0) {
        return;
    }
    double lambda = -(Cdot + constraint->bias + gamma * constraint->accumulated_impulse) * denom;
    constraint->accumulated_impulse += lambda;

    double impulse_vec[2];
    scale(constraint->normal, lambda, impulse_vec);
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);
}

void chrono_distance_constraint2d_solve_position(ChronoDistanceConstraint2D_C *constraint) {
    if (!constraint) {
        return;
    }
    ChronoBody2D_C *body_a = constraint->body_a;
    ChronoBody2D_C *body_b = constraint->body_b;

    double pa[2];
    add(body_a->position, constraint->ra, pa);
    double pb[2];
    add(body_b->position, constraint->rb, pb);

    double delta[2];
    sub(pb, pa, delta);
    double dist = length(delta);
    if (dist < 1e-9) {
        return;
    }
    double n[2] = {delta[0] / dist, delta[1] / dist};
    double C = dist - constraint->rest_length;
    double error = fabs(C) > constraint->slop ? C - constraint->slop * (C > 0 ? 1 : -1) : 0.0;
    double correction = -constraint->baumgarte_beta * error;
    if (correction > constraint->max_correction) {
        correction = constraint->max_correction;
    } else if (correction < -constraint->max_correction) {
        correction = -constraint->max_correction;
    }

    double ra_cross_n = cross(constraint->ra, n);
    double rb_cross_n = cross(constraint->rb, n);

    double inv_mass = 0.0;
    if (body_a && !body_a->is_static) {
        inv_mass += body_a->inverse_mass + ra_cross_n * ra_cross_n * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        inv_mass += body_b->inverse_mass + rb_cross_n * rb_cross_n * body_b->inverse_inertia;
    }
    if (inv_mass == 0.0) {
        return;
    }
    double lambda = correction / inv_mass;

    double impulse_vec[2];
    scale(n, lambda, impulse_vec);
    apply_impulse(body_a, (double[2]){-impulse_vec[0], -impulse_vec[1]}, constraint->ra);
    apply_impulse(body_b, impulse_vec, constraint->rb);

    if (body_a && !body_a->is_static) {
        body_a->position[0] -= impulse_vec[0] * body_a->inverse_mass;
        body_a->position[1] -= impulse_vec[1] * body_a->inverse_mass;
        body_a->angle -= ra_cross_n * lambda * body_a->inverse_inertia;
    }
    if (body_b && !body_b->is_static) {
        body_b->position[0] += impulse_vec[0] * body_b->inverse_mass;
        body_b->position[1] += impulse_vec[1] * body_b->inverse_mass;
        body_b->angle += rb_cross_n * lambda * body_b->inverse_inertia;
    }
}

