#include "../include/chrono_collision2d.h"

#include <math.h>
#include <string.h>

static double dot2(const double a[2], const double b[2]) {
    return a[0] * b[0] + a[1] * b[1];
}

static double length2(const double v[2]) {
    return sqrt(dot2(v, v));
}

static double clamp(double value, double min_val, double max_val) {
    if (value < min_val) {
        return min_val;
    }
    if (value > max_val) {
        return max_val;
    }
    return value;
}

int chrono_collision2d_detect_circle_circle(const ChronoBody2D_C *body_a,
                                            const ChronoBody2D_C *body_b,
                                            ChronoContact2D_C *contact) {
    if (!body_a || !body_b || !contact) {
        return -1;
    }

    memset(contact, 0, sizeof(*contact));

    double radius_a = chrono_body2d_get_circle_radius(body_a);
    double radius_b = chrono_body2d_get_circle_radius(body_b);
    if (radius_a <= 0.0 || radius_b <= 0.0) {
        return -1;
    }

    double delta[2] = {body_b->position[0] - body_a->position[0],
                       body_b->position[1] - body_a->position[1]};
    double distance = length2(delta);
    double radius_sum = radius_a + radius_b;

    if (distance >= radius_sum) {
        contact->has_contact = 0;
        return 0;
    }

    if (distance > 1e-9) {
        contact->normal[0] = delta[0] / distance;
        contact->normal[1] = delta[1] / distance;
    } else {
        contact->normal[0] = 1.0;
        contact->normal[1] = 0.0;
    }

    contact->penetration = radius_sum - distance;
    contact->contact_point[0] = body_a->position[0] + contact->normal[0] * (radius_a - 0.5 * contact->penetration);
    contact->contact_point[1] = body_a->position[1] + contact->normal[1] * (radius_a - 0.5 * contact->penetration);
    contact->has_contact = 1;
    return 0;
}

static double cross2(const double a[2], const double b[2]) {
    return a[0] * b[1] - a[1] * b[0];
}

int chrono_collision2d_resolve_circle_circle(ChronoBody2D_C *body_a,
                                             ChronoBody2D_C *body_b,
                                             const ChronoContact2D_C *contact,
                                             double restitution) {
    if (!body_a || !body_b || !contact || !contact->has_contact) {
        return -1;
    }

    restitution = clamp(restitution, 0.0, 1.0);

    double inv_mass_a = body_a->inverse_mass;
    double inv_mass_b = body_b->inverse_mass;
    double inv_inertia_a = body_a->inverse_inertia;
    double inv_inertia_b = body_b->inverse_inertia;

    if (inv_mass_a + inv_mass_b + inv_inertia_a + inv_inertia_b <= 0.0) {
        return 0;
    }

    double ra[2] = {contact->contact_point[0] - body_a->position[0],
                    contact->contact_point[1] - body_a->position[1]};
    double rb[2] = {contact->contact_point[0] - body_b->position[0],
                    contact->contact_point[1] - body_b->position[1]};

    double vel_a[2] = {body_a->linear_velocity[0] - body_a->angular_velocity * ra[1],
                       body_a->linear_velocity[1] + body_a->angular_velocity * ra[0]};
    double vel_b[2] = {body_b->linear_velocity[0] - body_b->angular_velocity * rb[1],
                       body_b->linear_velocity[1] + body_b->angular_velocity * rb[0]};

    double relative_vel[2] = {vel_b[0] - vel_a[0], vel_b[1] - vel_a[1]};
    double rel_normal = dot2(relative_vel, contact->normal);

    if (rel_normal > 0.0) {
        return 0;
    }

    double ra_cross_n = cross2(ra, contact->normal);
    double rb_cross_n = cross2(rb, contact->normal);
    double denom = inv_mass_a + inv_mass_b +
                   (ra_cross_n * ra_cross_n) * inv_inertia_a +
                   (rb_cross_n * rb_cross_n) * inv_inertia_b;

    if (denom <= 0.0) {
        return 0;
    }

    double impulse_mag = -(1.0 + restitution) * rel_normal / denom;
    double impulse[2] = {contact->normal[0] * impulse_mag,
                         contact->normal[1] * impulse_mag};

    body_a->linear_velocity[0] -= impulse[0] * inv_mass_a;
    body_a->linear_velocity[1] -= impulse[1] * inv_mass_a;
    body_b->linear_velocity[0] += impulse[0] * inv_mass_b;
    body_b->linear_velocity[1] += impulse[1] * inv_mass_b;

    double angular_impulse_a = cross2(ra, impulse);
    double angular_impulse_b = cross2(rb, impulse);
    body_a->angular_velocity -= angular_impulse_a * inv_inertia_a;
    body_b->angular_velocity += angular_impulse_b * inv_inertia_b;

    const double correction_percent = 1.0;
    const double slop = 1e-3;
    double correction_mag = fmax(contact->penetration - slop, 0.0) * correction_percent;
    double total_inv_mass = inv_mass_a + inv_mass_b;
    if (correction_mag > 0.0 && total_inv_mass > 0.0) {
        double correction[2] = {contact->normal[0] * correction_mag / total_inv_mass,
                                contact->normal[1] * correction_mag / total_inv_mass};
        body_a->position[0] -= correction[0] * inv_mass_a;
        body_a->position[1] -= correction[1] * inv_mass_a;
        body_b->position[0] += correction[0] * inv_mass_b;
        body_b->position[1] += correction[1] * inv_mass_b;
    }

    return 0;
}
