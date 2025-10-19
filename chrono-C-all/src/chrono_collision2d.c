#include "../include/chrono_collision2d.h"

#include <math.h>
#include <stdlib.h>
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
                                             double restitution,
                                             double friction_static,
                                             double friction_dynamic,
                                             ChronoContactManifold2D_C *manifold) {
    if (!body_a || !body_b || !contact || !contact->has_contact) {
        return -1;
    }

    restitution = clamp(restitution, 0.0, 1.0);
    friction_static = fmax(friction_static, 0.0);
    friction_dynamic = fmax(friction_dynamic, 0.0);

    double effective_restitution = restitution;
    double effective_friction_static = friction_static;
    double effective_friction_dynamic = friction_dynamic;

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

    ChronoContactPoint2D_C local_point;
    ChronoContactPoint2D_C *point = NULL;
    if (manifold) {
        if (manifold->body_a != body_a || manifold->body_b != body_b) {
            chrono_contact_manifold2d_set_bodies(manifold, body_a, body_b);
        }
        point = chrono_contact_manifold2d_add_or_update(manifold, contact);
        effective_restitution = manifold->combined_restitution;
        effective_friction_static = manifold->combined_friction_static;
        effective_friction_dynamic = manifold->combined_friction_dynamic;
        effective_restitution = clamp(effective_restitution, 0.0, 1.0);
    }
    if (effective_friction_static < 0.0) {
        effective_friction_static = 0.0;
    }
    if (effective_friction_dynamic < 0.0) {
        effective_friction_dynamic = 0.0;
    }
    if (!point) {
        memset(&local_point, 0, sizeof(local_point));
        local_point.contact = *contact;
        local_point.is_active = 1;
        point = &local_point;
    }

    double prev_normal_impulse = point->normal_impulse;
    double prev_tangent_impulse = point->tangent_impulse;

    if (prev_normal_impulse != 0.0 || prev_tangent_impulse != 0.0) {
        double warm_normal[2] = {contact->normal[0] * prev_normal_impulse,
                                 contact->normal[1] * prev_normal_impulse};
        body_a->linear_velocity[0] -= warm_normal[0] * inv_mass_a;
        body_a->linear_velocity[1] -= warm_normal[1] * inv_mass_a;
        body_b->linear_velocity[0] += warm_normal[0] * inv_mass_b;
        body_b->linear_velocity[1] += warm_normal[1] * inv_mass_b;
        double ang_norm_a = cross2(ra, warm_normal);
        double ang_norm_b = cross2(rb, warm_normal);
        body_a->angular_velocity -= ang_norm_a * inv_inertia_a;
        body_b->angular_velocity += ang_norm_b * inv_inertia_b;

        double tangent_prefetch[2] = {-contact->normal[1], contact->normal[0]};
        double warm_tangent[2] = {tangent_prefetch[0] * prev_tangent_impulse,
                                  tangent_prefetch[1] * prev_tangent_impulse};
        body_a->linear_velocity[0] -= warm_tangent[0] * inv_mass_a;
        body_a->linear_velocity[1] -= warm_tangent[1] * inv_mass_a;
        body_b->linear_velocity[0] += warm_tangent[0] * inv_mass_b;
        body_b->linear_velocity[1] += warm_tangent[1] * inv_mass_b;
        double ang_t_a = cross2(ra, warm_tangent);
        double ang_t_b = cross2(rb, warm_tangent);
        body_a->angular_velocity -= ang_t_a * inv_inertia_a;
        body_b->angular_velocity += ang_t_b * inv_inertia_b;
    }

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

    double impulse_mag = -(1.0 + effective_restitution) * rel_normal / denom;
    double total_normal_impulse = prev_normal_impulse + impulse_mag;
    if (total_normal_impulse < 0.0) {
        total_normal_impulse = 0.0;
    }
    double delta_normal_impulse = total_normal_impulse - prev_normal_impulse;
    double impulse[2] = {contact->normal[0] * delta_normal_impulse,
                         contact->normal[1] * delta_normal_impulse};

    body_a->linear_velocity[0] -= impulse[0] * inv_mass_a;
    body_a->linear_velocity[1] -= impulse[1] * inv_mass_a;
    body_b->linear_velocity[0] += impulse[0] * inv_mass_b;
    body_b->linear_velocity[1] += impulse[1] * inv_mass_b;

    double angular_impulse_a = cross2(ra, impulse);
    double angular_impulse_b = cross2(rb, impulse);
    body_a->angular_velocity -= angular_impulse_a * inv_inertia_a;
    body_b->angular_velocity += angular_impulse_b * inv_inertia_b;

    double tangent[2] = {-contact->normal[1], contact->normal[0]};

    double vel_a_after[2] = {body_a->linear_velocity[0] - body_a->angular_velocity * ra[1],
                             body_a->linear_velocity[1] + body_a->angular_velocity * ra[0]};
    double vel_b_after[2] = {body_b->linear_velocity[0] - body_b->angular_velocity * rb[1],
                             body_b->linear_velocity[1] + body_b->angular_velocity * rb[0]};
    double relative_after[2] = {vel_b_after[0] - vel_a_after[0],
                                vel_b_after[1] - vel_a_after[1]};

    double rel_tangent = dot2(relative_after, tangent);

    double ra_cross_t = cross2(ra, tangent);
    double rb_cross_t = cross2(rb, tangent);
    double denom_t = inv_mass_a + inv_mass_b +
                     (ra_cross_t * ra_cross_t) * inv_inertia_a +
                     (rb_cross_t * rb_cross_t) * inv_inertia_b;

    double total_tangent_impulse = prev_tangent_impulse;
    if (denom_t > 0.0) {
        double friction_delta = -rel_tangent / denom_t;
        double candidate = prev_tangent_impulse + friction_delta;
        double max_static_impulse = effective_friction_static * total_normal_impulse;
        double max_dynamic_impulse = effective_friction_dynamic * total_normal_impulse;

        if (fabs(candidate) <= max_static_impulse) {
            total_tangent_impulse = candidate;
        } else {
            if (max_dynamic_impulse > 0.0) {
                total_tangent_impulse = clamp(candidate, -max_dynamic_impulse, max_dynamic_impulse);
            } else {
                total_tangent_impulse = 0.0;
            }
        }

        double delta_tangent_impulse = total_tangent_impulse - prev_tangent_impulse;
        if (delta_tangent_impulse != 0.0) {
            double friction_vec[2] = {tangent[0] * delta_tangent_impulse,
                                      tangent[1] * delta_tangent_impulse};
            body_a->linear_velocity[0] -= friction_vec[0] * inv_mass_a;
            body_a->linear_velocity[1] -= friction_vec[1] * inv_mass_a;
            body_b->linear_velocity[0] += friction_vec[0] * inv_mass_b;
            body_b->linear_velocity[1] += friction_vec[1] * inv_mass_b;

            double angular_impulse_t_a = cross2(ra, friction_vec);
            double angular_impulse_t_b = cross2(rb, friction_vec);
            body_a->angular_velocity -= angular_impulse_t_a * inv_inertia_a;
            body_b->angular_velocity += angular_impulse_t_b * inv_inertia_b;
        }
    }

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

    if (point) {
        point->contact = *contact;
        point->normal_impulse = total_normal_impulse;
        point->tangent_impulse = total_tangent_impulse;
        point->is_active = 1;
    }

    return 0;
}
void chrono_contact_manifold2d_init(ChronoContactManifold2D_C *manifold) {
    if (!manifold) {
        return;
    }
    memset(manifold, 0, sizeof(*manifold));
    manifold->num_points = 0;
    manifold->body_a = NULL;
    manifold->body_b = NULL;
    manifold->combined_restitution = 0.0;
    manifold->combined_friction_static = 0.0;
    manifold->combined_friction_dynamic = 0.0;
}

void chrono_contact_manifold2d_reset(ChronoContactManifold2D_C *manifold) {
    if (!manifold) {
        return;
    }
    manifold->num_points = 0;
    for (int i = 0; i < CHRONO_CONTACT2D_MAX_POINTS; ++i) {
        manifold->points[i].is_active = 0;
        manifold->points[i].normal_impulse = 0.0;
        manifold->points[i].tangent_impulse = 0.0;
    }
    manifold->combined_restitution = 0.0;
    manifold->combined_friction_static = 0.0;
    manifold->combined_friction_dynamic = 0.0;
}

void chrono_contact_manifold2d_set_bodies(ChronoContactManifold2D_C *manifold,
                                          ChronoBody2D_C *body_a,
                                          ChronoBody2D_C *body_b) {
    if (!manifold) {
        return;
    }
    manifold->body_a = body_a;
    manifold->body_b = body_b;
    manifold->combined_restitution = fmax(chrono_body2d_get_restitution(body_a),
                                          chrono_body2d_get_restitution(body_b));
    double mu_s_a = chrono_body2d_get_friction_static(body_a);
    double mu_s_b = chrono_body2d_get_friction_static(body_b);
    manifold->combined_friction_static = sqrt(fmax(mu_s_a, 0.0) * fmax(mu_s_b, 0.0));
    double mu_d_a = chrono_body2d_get_friction_dynamic(body_a);
    double mu_d_b = chrono_body2d_get_friction_dynamic(body_b);
    manifold->combined_friction_dynamic = sqrt(fmax(mu_d_a, 0.0) * fmax(mu_d_b, 0.0));
}

static ChronoContactPoint2D_C *chrono_contact_manifold2d_select_slot(ChronoContactManifold2D_C *manifold) {
    if (manifold->num_points < CHRONO_CONTACT2D_MAX_POINTS) {
        ChronoContactPoint2D_C *slot = &manifold->points[manifold->num_points++];
        slot->normal_impulse = 0.0;
        slot->tangent_impulse = 0.0;
        slot->is_active = 1;
        return slot;
    }
    /* For now overwrite the point with smallest penetration. */
    int index = 0;
    double min_pen = manifold->points[0].contact.penetration;
    for (int i = 1; i < CHRONO_CONTACT2D_MAX_POINTS; ++i) {
        if (manifold->points[i].contact.penetration < min_pen) {
            index = i;
            min_pen = manifold->points[i].contact.penetration;
        }
    }
    ChronoContactPoint2D_C *slot = &manifold->points[index];
    slot->normal_impulse = 0.0;
    slot->tangent_impulse = 0.0;
    slot->is_active = 1;
    return slot;
}

ChronoContactPoint2D_C *chrono_contact_manifold2d_add_or_update(ChronoContactManifold2D_C *manifold,
                                                                const ChronoContact2D_C *contact) {
    if (!manifold || !contact || !contact->has_contact) {
        return NULL;
    }

    /* Attempt to reuse existing point with similar position. */
    const double match_threshold = 0.01;
    for (int i = 0; i < manifold->num_points; ++i) {
        ChronoContactPoint2D_C *point = &manifold->points[i];
        if (!point->is_active) {
            continue;
        }
        double dx = point->contact.contact_point[0] - contact->contact_point[0];
        double dy = point->contact.contact_point[1] - contact->contact_point[1];
        double dist = sqrt(dx * dx + dy * dy);
        double dot_n = point->contact.normal[0] * contact->normal[0] +
                       point->contact.normal[1] * contact->normal[1];
        if (dist < match_threshold && dot_n > 0.95) {
            point->contact = *contact;
            return point;
        }
    }

    ChronoContactPoint2D_C *slot = chrono_contact_manifold2d_select_slot(manifold);
    if (!slot) {
        return NULL;
    }
    slot->contact = *contact;
    return slot;
}

void chrono_contact_manager2d_init(ChronoContactManager2D_C *manager) {
    if (!manager) {
        return;
    }
    manager->pairs = NULL;
    manager->count = 0;
    manager->capacity = 0;
}

void chrono_contact_manager2d_reset(ChronoContactManager2D_C *manager) {
    if (!manager) {
        return;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        chrono_contact_manifold2d_reset(&manager->pairs[i].manifold);
    }
    manager->count = 0;
}

void chrono_contact_manager2d_free(ChronoContactManager2D_C *manager) {
    if (!manager) {
        return;
    }
    free(manager->pairs);
    manager->pairs = NULL;
    manager->count = 0;
    manager->capacity = 0;
}

void chrono_contact_manager2d_begin_step(ChronoContactManager2D_C *manager) {
    if (!manager) {
        return;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        ChronoContactManifold2D_C *manifold = &manager->pairs[i].manifold;
        for (int j = 0; j < manifold->num_points; ++j) {
            manifold->points[j].is_active = 0;
        }
    }
}

static void chrono_contact_manifold2d_finalize(ChronoContactManifold2D_C *manifold) {
    if (!manifold) {
        return;
    }
    int write_idx = 0;
    for (int i = 0; i < manifold->num_points; ++i) {
        if (manifold->points[i].is_active) {
            if (write_idx != i) {
                manifold->points[write_idx] = manifold->points[i];
            }
            ++write_idx;
        }
    }
    for (int i = write_idx; i < manifold->num_points; ++i) {
        memset(&manifold->points[i], 0, sizeof(manifold->points[i]));
    }
    manifold->num_points = write_idx;
}

void chrono_contact_manager2d_end_step(ChronoContactManager2D_C *manager) {
    if (!manager) {
        return;
    }
    size_t write_idx = 0;
    for (size_t i = 0; i < manager->count; ++i) {
        ChronoContactPair2D_C *pair = &manager->pairs[i];
        chrono_contact_manifold2d_finalize(&pair->manifold);
        if (pair->manifold.num_points > 0) {
            if (write_idx != i) {
                manager->pairs[write_idx] = *pair;
            }
            ++write_idx;
        }
    }
    manager->count = write_idx;
}

static ChronoContactPair2D_C *chrono_contact_manager2d_find_pair(ChronoContactManager2D_C *manager,
                                                                 ChronoBody2D_C *body_a,
                                                                 ChronoBody2D_C *body_b) {
    if (!manager) {
        return NULL;
    }
    for (size_t i = 0; i < manager->count; ++i) {
        ChronoContactPair2D_C *pair = &manager->pairs[i];
        if ((pair->body_a == body_a && pair->body_b == body_b) ||
            (pair->body_a == body_b && pair->body_b == body_a)) {
            return pair;
        }
    }
    return NULL;
}

ChronoContactManifold2D_C *chrono_contact_manager2d_get_manifold(ChronoContactManager2D_C *manager,
                                                                 ChronoBody2D_C *body_a,
                                                                 ChronoBody2D_C *body_b) {
    if (!manager) {
        return NULL;
    }
    ChronoContactPair2D_C *pair = chrono_contact_manager2d_find_pair(manager, body_a, body_b);
    if (pair) {
        return &pair->manifold;
    }

    if (manager->count >= manager->capacity) {
        size_t new_capacity = manager->capacity == 0 ? 8 : manager->capacity * 2;
        ChronoContactPair2D_C *new_pairs = (ChronoContactPair2D_C *)realloc(manager->pairs,
                                                                            new_capacity * sizeof(ChronoContactPair2D_C));
        if (!new_pairs) {
            return NULL;
        }
        manager->pairs = new_pairs;
        manager->capacity = new_capacity;
    }

    pair = &manager->pairs[manager->count++];
    pair->body_a = body_a;
    pair->body_b = body_b;
    chrono_contact_manifold2d_init(&pair->manifold);
    chrono_contact_manifold2d_set_bodies(&pair->manifold, body_a, body_b);
    return &pair->manifold;
}

ChronoContactPoint2D_C *chrono_contact_manager2d_update_circle_circle(ChronoContactManager2D_C *manager,
                                                                      ChronoBody2D_C *body_a,
                                                                      ChronoBody2D_C *body_b,
                                                                      const ChronoContact2D_C *contact) {
    ChronoContactManifold2D_C *manifold = chrono_contact_manager2d_get_manifold(manager, body_a, body_b);
    if (!manifold) {
        return NULL;
    }
    double restitution = fmax(chrono_body2d_get_restitution(body_a), chrono_body2d_get_restitution(body_b));
    double mu_s = sqrt(fmax(chrono_body2d_get_friction_static(body_a), 0.0) *
                       fmax(chrono_body2d_get_friction_static(body_b), 0.0));
    double mu_d = sqrt(fmax(chrono_body2d_get_friction_dynamic(body_a), 0.0) *
                       fmax(chrono_body2d_get_friction_dynamic(body_b), 0.0));
    manifold->combined_restitution = restitution;
    manifold->combined_friction_static = mu_s;
    manifold->combined_friction_dynamic = mu_d;
    return chrono_contact_manifold2d_add_or_update(manifold, contact);
}
