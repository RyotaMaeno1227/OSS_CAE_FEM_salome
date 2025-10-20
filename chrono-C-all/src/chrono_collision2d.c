#include "../include/chrono_collision2d.h"

#include <float.h>
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

static void normalize2(double v[2]) {
    double len = sqrt(v[0] * v[0] + v[1] * v[1]);
    if (len > 1e-12) {
        v[0] /= len;
        v[1] /= len;
    }
}

static void perpendicular2(const double v[2], double out[2]) {
    out[0] = v[1];
    out[1] = -v[0];
}

static void polygon_to_world(const ChronoBody2D_C *body, double (*out_vertices)[2]) {
    size_t count = chrono_body2d_get_polygon_vertex_count(body);
    const double *local_vertices = chrono_body2d_get_polygon_vertices(body);
    if (!local_vertices) {
        return;
    }
    double c = cos(body->angle);
    double s = sin(body->angle);
    for (size_t i = 0; i < count; ++i) {
        double lx = local_vertices[2 * i];
        double ly = local_vertices[2 * i + 1];
        double wx = c * lx - s * ly + body->position[0];
        double wy = s * lx + c * ly + body->position[1];
        out_vertices[i][0] = wx;
        out_vertices[i][1] = wy;
    }
}

static void polygon_face_normals(const double (*vertices)[2], size_t count, double (*normals)[2]) {
    for (size_t i = 0; i < count; ++i) {
        const double *a = vertices[i];
        const double *b = vertices[(i + 1) % count];
        double edge[2] = {b[0] - a[0], b[1] - a[1]};
        double normal[2];
        perpendicular2(edge, normal);
        normalize2(normal);
        normals[i][0] = normal[0];
        normals[i][1] = normal[1];
    }
}

static void project_polygon(const double (*vertices)[2], size_t count, const double axis[2], double *min, double *max) {
    double projection = dot2(vertices[0], axis);
    double min_val = projection;
    double max_val = projection;
    for (size_t i = 1; i < count; ++i) {
        projection = dot2(vertices[i], axis);
        if (projection < min_val) {
            min_val = projection;
        }
        if (projection > max_val) {
            max_val = projection;
        }
    }
    *min = min_val;
    *max = max_val;
}

static void support_polygon(const double (*vertices)[2], size_t count, const double axis[2], double out[2]) {
    double best = dot2(vertices[0], axis);
    out[0] = vertices[0][0];
    out[1] = vertices[0][1];
    for (size_t i = 1; i < count; ++i) {
        double projection = dot2(vertices[i], axis);
        if (projection > best) {
            best = projection;
            out[0] = vertices[i][0];
            out[1] = vertices[i][1];
        }
    }
}

static int clip_segment(const double (*input)[2],
                        int count,
                        const double normal[2],
                        double offset,
                        double (*output)[2]) {
    if (count < 2) {
        return 0;
    }
    double distances[2];
    distances[0] = dot2(normal, input[0]) - offset;
    distances[1] = dot2(normal, input[1]) - offset;

    int out_count = 0;

    if (distances[0] <= 0.0) {
        output[out_count][0] = input[0][0];
        output[out_count][1] = input[0][1];
        ++out_count;
    }

    if ((distances[0] <= 0.0 && distances[1] > 0.0) ||
        (distances[0] > 0.0 && distances[1] <= 0.0)) {
        double t = distances[0] / (distances[0] - distances[1]);
        double intersection[2] = {
            input[0][0] + t * (input[1][0] - input[0][0]),
            input[0][1] + t * (input[1][1] - input[0][1])
        };
        output[out_count][0] = intersection[0];
        output[out_count][1] = intersection[1];
        ++out_count;
    }

    if (distances[1] <= 0.0) {
        if (out_count < 2) {
            output[out_count][0] = input[1][0];
            output[out_count][1] = input[1][1];
            ++out_count;
        }
    }

    return out_count;
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
    contact->penetrations[0] = contact->penetration;
    contact->point_count = 1;
    contact->contact_point[0] = body_a->position[0] + contact->normal[0] * (radius_a - 0.5 * contact->penetration);
    contact->contact_point[1] = body_a->position[1] + contact->normal[1] * (radius_a - 0.5 * contact->penetration);
    contact->contact_points[0][0] = contact->contact_point[0];
    contact->contact_points[0][1] = contact->contact_point[1];
    contact->has_contact = 1;
    return 0;
}

int chrono_collision2d_detect_circle_polygon(const ChronoBody2D_C *circle_body,
                                             const ChronoBody2D_C *polygon_body,
                                             ChronoContact2D_C *contact) {
    if (!circle_body || !polygon_body || !contact) {
        return -1;
    }
    if (chrono_body2d_get_shape_type(circle_body) != CHRONO_BODY2D_SHAPE_CIRCLE ||
        chrono_body2d_get_shape_type(polygon_body) != CHRONO_BODY2D_SHAPE_POLYGON) {
        return -1;
    }
    size_t vertex_count = chrono_body2d_get_polygon_vertex_count(polygon_body);
    if (vertex_count < 3) {
        return -1;
    }
    memset(contact, 0, sizeof(*contact));

    double polygon_vertices[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];
    double normals[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];
    polygon_to_world(polygon_body, polygon_vertices);
    const double (*poly_ptr)[2] = (const double (*)[2])polygon_vertices;
    polygon_face_normals(poly_ptr, vertex_count, normals);

    double best_axis[2] = {0.0, 0.0};
    double best_overlap = DBL_MAX;
    double circle_center[2] = {circle_body->position[0], circle_body->position[1]};
    double circle_radius = chrono_body2d_get_circle_radius(circle_body);
    if (circle_radius <= 0.0) {
        return -1;
    }

    for (size_t i = 0; i < vertex_count; ++i) {
        double axis[2] = {normals[i][0], normals[i][1]};
        double min_poly, max_poly;
        project_polygon(poly_ptr, vertex_count, axis, &min_poly, &max_poly);
        double center_proj = dot2(circle_center, axis);
        double min_circle = center_proj - circle_radius;
        double max_circle = center_proj + circle_radius;
        double overlap = fmin(max_poly, max_circle) - fmax(min_poly, min_circle);
        if (overlap <= 0.0) {
            contact->has_contact = 0;
            return 0;
        }
        if (overlap < best_overlap) {
            best_overlap = overlap;
            best_axis[0] = axis[0];
            best_axis[1] = axis[1];
        }
    }

    size_t closest_index = 0;
    double closest_dist2 = DBL_MAX;
    for (size_t i = 0; i < vertex_count; ++i) {
        double dx = circle_center[0] - polygon_vertices[i][0];
        double dy = circle_center[1] - polygon_vertices[i][1];
        double dist2 = dx * dx + dy * dy;
        if (dist2 < closest_dist2) {
            closest_dist2 = dist2;
            closest_index = i;
        }
    }
    if (closest_dist2 > 1e-12) {
        double axis[2] = {circle_center[0] - polygon_vertices[closest_index][0],
                          circle_center[1] - polygon_vertices[closest_index][1]};
        normalize2(axis);
        double min_poly, max_poly;
        project_polygon(poly_ptr, vertex_count, axis, &min_poly, &max_poly);
        double center_proj = dot2(circle_center, axis);
        double min_circle = center_proj - circle_radius;
        double max_circle = center_proj + circle_radius;
        double overlap = fmin(max_poly, max_circle) - fmax(min_poly, min_circle);
        if (overlap <= 0.0) {
            contact->has_contact = 0;
            return 0;
        }
        if (overlap < best_overlap) {
            best_overlap = overlap;
            best_axis[0] = axis[0];
            best_axis[1] = axis[1];
        }
    }

    if (best_overlap == DBL_MAX) {
        contact->has_contact = 0;
        return 0;
    }

    double to_polygon[2] = {polygon_body->position[0] - circle_body->position[0],
                            polygon_body->position[1] - circle_body->position[1]};
    if (dot2(to_polygon, best_axis) < 0.0) {
        best_axis[0] = -best_axis[0];
        best_axis[1] = -best_axis[1];
    }

    double circle_point[2] = {circle_center[0] + best_axis[0] * circle_radius,
                              circle_center[1] + best_axis[1] * circle_radius};
    double polygon_point[2];
    double negative_axis[2] = {-best_axis[0], -best_axis[1]};
    support_polygon(poly_ptr, vertex_count, negative_axis, polygon_point);

    contact->normal[0] = best_axis[0];
    contact->normal[1] = best_axis[1];
    contact->contact_point[0] = 0.5 * (circle_point[0] + polygon_point[0]);
    contact->contact_point[1] = 0.5 * (circle_point[1] + polygon_point[1]);
    contact->penetration = best_overlap;
    contact->penetrations[0] = contact->penetration;
    contact->contact_points[0][0] = contact->contact_point[0];
    contact->contact_points[0][1] = contact->contact_point[1];
    contact->point_count = 1;
    contact->has_contact = 1;
    return 0;
}

int chrono_collision2d_detect_polygon_circle(const ChronoBody2D_C *polygon_body,
                                             const ChronoBody2D_C *circle_body,
                                             ChronoContact2D_C *contact) {
    return chrono_collision2d_detect_circle_polygon(circle_body, polygon_body, contact);
}

int chrono_collision2d_detect_polygon_polygon(const ChronoBody2D_C *body_a,
                                              const ChronoBody2D_C *body_b,
                                              ChronoContact2D_C *contact) {
    if (!body_a || !body_b || !contact) {
        return -1;
    }
    if (chrono_body2d_get_shape_type(body_a) != CHRONO_BODY2D_SHAPE_POLYGON ||
        chrono_body2d_get_shape_type(body_b) != CHRONO_BODY2D_SHAPE_POLYGON) {
        return -1;
    }
    size_t count_a = chrono_body2d_get_polygon_vertex_count(body_a);
    size_t count_b = chrono_body2d_get_polygon_vertex_count(body_b);
    if (count_a < 3 || count_b < 3) {
        return -1;
    }
    memset(contact, 0, sizeof(*contact));

    double vertices_a[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];
    double vertices_b[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];
    double normals_a[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];
    double normals_b[CHRONO_BODY2D_MAX_POLYGON_VERTICES][2];

    polygon_to_world(body_a, vertices_a);
    polygon_to_world(body_b, vertices_b);
    const double (*verts_a_ptr)[2] = (const double (*)[2])vertices_a;
    const double (*verts_b_ptr)[2] = (const double (*)[2])vertices_b;
    polygon_face_normals(verts_a_ptr, count_a, normals_a);
    polygon_face_normals(verts_b_ptr, count_b, normals_b);

    double best_axis[2] = {0.0, 0.0};
    double best_overlap = DBL_MAX;
    int reference_is_a = 1;
    size_t reference_index = 0;

    for (size_t i = 0; i < count_a; ++i) {
        double axis[2] = {normals_a[i][0], normals_a[i][1]};
        double min_a, max_a, min_b, max_b;
        project_polygon(verts_a_ptr, count_a, axis, &min_a, &max_a);
        project_polygon(verts_b_ptr, count_b, axis, &min_b, &max_b);
        double overlap = fmin(max_a, max_b) - fmax(min_a, min_b);
        if (overlap <= 0.0) {
            contact->has_contact = 0;
            return 0;
        }
        if (overlap < best_overlap) {
            best_overlap = overlap;
            best_axis[0] = axis[0];
            best_axis[1] = axis[1];
            reference_is_a = 1;
            reference_index = i;
        }
    }

    for (size_t i = 0; i < count_b; ++i) {
        double axis[2] = {normals_b[i][0], normals_b[i][1]};
        double min_a, max_a, min_b, max_b;
        project_polygon(verts_a_ptr, count_a, axis, &min_a, &max_a);
        project_polygon(verts_b_ptr, count_b, axis, &min_b, &max_b);
        double overlap = fmin(max_a, max_b) - fmax(min_a, min_b);
        if (overlap <= 0.0) {
            contact->has_contact = 0;
            return 0;
        }
        if (overlap < best_overlap) {
            best_overlap = overlap;
            best_axis[0] = axis[0];
            best_axis[1] = axis[1];
            reference_is_a = 0;
            reference_index = i;
        }
    }

    double to_b[2] = {body_b->position[0] - body_a->position[0],
                      body_b->position[1] - body_a->position[1]};
    if (dot2(to_b, best_axis) < 0.0) {
        best_axis[0] = -best_axis[0];
        best_axis[1] = -best_axis[1];
    }

    const double (*ref_vertices)[2] = reference_is_a ? verts_a_ptr : verts_b_ptr;
    const double (*inc_vertices)[2] = reference_is_a ? verts_b_ptr : verts_a_ptr;
    const double (*inc_normals)[2] = reference_is_a ? (const double (*)[2])normals_b : (const double (*)[2])normals_a;
    size_t ref_count = reference_is_a ? count_a : count_b;
    size_t inc_count = reference_is_a ? count_b : count_a;

    const double *ref_v1 = ref_vertices[reference_index];
    const double *ref_v2 = ref_vertices[(reference_index + 1) % ref_count];

    double ref_tangent[2] = {ref_v2[0] - ref_v1[0], ref_v2[1] - ref_v1[1]};
    normalize2(ref_tangent);
    double ref_normal[2] = {ref_tangent[1], -ref_tangent[0]};
    if (dot2(ref_normal, best_axis) < 0.0) {
        ref_tangent[0] = -ref_tangent[0];
        ref_tangent[1] = -ref_tangent[1];
        ref_normal[0] = -ref_normal[0];
        ref_normal[1] = -ref_normal[1];
    }

    double ref_offset = dot2(ref_normal, ref_v1);

    size_t incident_index = 0;
    double min_dot = DBL_MAX;
    for (size_t i = 0; i < inc_count; ++i) {
        double dot = dot2(inc_normals[i], ref_normal);
        if (dot < min_dot) {
            min_dot = dot;
            incident_index = i;
        }
    }

    double incident_edge[2][2];
    incident_edge[0][0] = inc_vertices[incident_index][0];
    incident_edge[0][1] = inc_vertices[incident_index][1];
    incident_edge[1][0] = inc_vertices[(incident_index + 1) % inc_count][0];
    incident_edge[1][1] = inc_vertices[(incident_index + 1) % inc_count][1];

    double clip_buffer1[2][2];
    double clip_buffer2[2][2];

    double neg_tangent[2] = {-ref_tangent[0], -ref_tangent[1]};
    double neg_offset = dot2(neg_tangent, ref_v1);
    int clip_count = clip_segment((const double (*)[2])incident_edge, 2, neg_tangent, neg_offset, clip_buffer1);
    if (clip_count == 0) {
        contact->has_contact = 0;
        return 0;
    }

    double pos_offset = dot2(ref_tangent, ref_v2);
    clip_count = clip_segment((const double (*)[2])clip_buffer1, clip_count, ref_tangent, pos_offset, clip_buffer2);
    if (clip_count == 0) {
        contact->has_contact = 0;
        return 0;
    }

    contact->normal[0] = ref_normal[0];
    contact->normal[1] = ref_normal[1];
    contact->has_contact = 1;
    contact->point_count = 0;

    const double penetration_safety = 1e-6;
    for (int i = 0; i < clip_count && contact->point_count < CHRONO_CONTACT2D_MAX_POINTS; ++i) {
        double separation = dot2(ref_normal, clip_buffer2[i]) - ref_offset;
        double penetration = best_overlap - separation;
        if (penetration >= -penetration_safety) {
            int idx = contact->point_count;
            contact->contact_points[idx][0] = clip_buffer2[i][0];
            contact->contact_points[idx][1] = clip_buffer2[i][1];
            contact->penetrations[idx] = penetration;
            contact->point_count = idx + 1;
        }
    }

    if (contact->point_count == 0) {
        double point_a[2];
        double point_b[2];
        support_polygon(verts_a_ptr, count_a, best_axis, point_a);
        double negative_axis[2] = {-best_axis[0], -best_axis[1]};
        support_polygon(verts_b_ptr, count_b, negative_axis, point_b);
        contact->normal[0] = best_axis[0];
        contact->normal[1] = best_axis[1];
        contact->contact_point[0] = 0.5 * (point_a[0] + point_b[0]);
        contact->contact_point[1] = 0.5 * (point_a[1] + point_b[1]);
        contact->penetration = best_overlap;
        contact->contact_points[0][0] = contact->contact_point[0];
        contact->contact_points[0][1] = contact->contact_point[1];
        contact->penetrations[0] = contact->penetration;
        contact->point_count = 1;
        return 0;
    }

    contact->penetration = contact->penetrations[0];
    contact->contact_point[0] = contact->contact_points[0][0];
    contact->contact_point[1] = contact->contact_points[0][1];
    return 0;
}
static double cross2(const double a[2], const double b[2]) {
    return a[0] * b[1] - a[1] * b[0];
}

static int chrono_collision2d_resolve_single(ChronoBody2D_C *body_a,
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

int chrono_collision2d_resolve_contact(ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const ChronoContact2D_C *contact,
                                       double restitution,
                                       double friction_static,
                                       double friction_dynamic,
                                       ChronoContactManifold2D_C *manifold) {
    if (!contact || !contact->has_contact) {
        return -1;
    }
    int count = contact->point_count;
    if (count <= 0) {
        count = 1;
    }
    int status = 0;
    for (int i = 0; i < count; ++i) {
        ChronoContact2D_C single;
        memset(&single, 0, sizeof(single));
        single.has_contact = 1;
        single.point_count = 1;
        single.normal[0] = contact->normal[0];
        single.normal[1] = contact->normal[1];
        double cp[2];
        double pen = contact->penetration;
        if (i < contact->point_count) {
            cp[0] = contact->contact_points[i][0];
            cp[1] = contact->contact_points[i][1];
            pen = contact->penetrations[i];
        } else {
            cp[0] = contact->contact_point[0];
            cp[1] = contact->contact_point[1];
        }
        single.contact_points[0][0] = cp[0];
        single.contact_points[0][1] = cp[1];
        single.penetrations[0] = pen;
        single.contact_point[0] = cp[0];
        single.contact_point[1] = cp[1];
        single.penetration = pen;
        status |= chrono_collision2d_resolve_single(body_a,
                                                    body_b,
                                                    &single,
                                                    restitution,
                                                    friction_static,
                                                    friction_dynamic,
                                                    manifold);
    }
    return status;
}

int chrono_collision2d_resolve_circle_circle(ChronoBody2D_C *body_a,
                                             ChronoBody2D_C *body_b,
                                             const ChronoContact2D_C *contact,
                                             double restitution,
                                             double friction_static,
                                             double friction_dynamic,
                                             ChronoContactManifold2D_C *manifold) {
    return chrono_collision2d_resolve_contact(body_a,
                                              body_b,
                                              contact,
                                              restitution,
                                              friction_static,
                                              friction_dynamic,
                                              manifold);
}

int chrono_collision2d_resolve_polygon_polygon(ChronoBody2D_C *body_a,
                                               ChronoBody2D_C *body_b,
                                               const ChronoContact2D_C *contact,
                                               double restitution,
                                               double friction_static,
                                               double friction_dynamic,
                                               ChronoContactManifold2D_C *manifold) {
    return chrono_collision2d_resolve_contact(body_a,
                                              body_b,
                                              contact,
                                              restitution,
                                              friction_static,
                                              friction_dynamic,
                                              manifold);
}

int chrono_collision2d_resolve_circle_polygon(ChronoBody2D_C *circle_body,
                                              ChronoBody2D_C *polygon_body,
                                              const ChronoContact2D_C *contact,
                                              double restitution,
                                              double friction_static,
                                              double friction_dynamic,
                                              ChronoContactManifold2D_C *manifold) {
    return chrono_collision2d_resolve_contact(circle_body,
                                              polygon_body,
                                              contact,
                                              restitution,
                                              friction_static,
                                              friction_dynamic,
                                              manifold);
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

static ChronoContactPoint2D_C *chrono_contact_manager2d_update_pair(ChronoContactManager2D_C *manager,
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
    int count = contact->point_count;
    if (count <= 0) {
        count = contact->has_contact ? 1 : 0;
    }
    ChronoContactPoint2D_C *last_point = NULL;
    for (int i = 0; i < count; ++i) {
        ChronoContact2D_C single;
        memset(&single, 0, sizeof(single));
        single.has_contact = contact->has_contact;
        single.point_count = 1;
        single.normal[0] = contact->normal[0];
        single.normal[1] = contact->normal[1];
        double pen = contact->penetration;
        double cp[2];
        if (i < contact->point_count) {
            pen = contact->penetrations[i];
            cp[0] = contact->contact_points[i][0];
            cp[1] = contact->contact_points[i][1];
        } else {
            cp[0] = contact->contact_point[0];
            cp[1] = contact->contact_point[1];
        }
        single.penetration = pen;
        single.contact_point[0] = cp[0];
        single.contact_point[1] = cp[1];
        single.contact_points[0][0] = cp[0];
        single.contact_points[0][1] = cp[1];
        single.penetrations[0] = pen;
        last_point = chrono_contact_manifold2d_add_or_update(manifold, &single);
    }
    return last_point;
}

ChronoContactPoint2D_C *chrono_contact_manager2d_update_circle_circle(ChronoContactManager2D_C *manager,
                                                                      ChronoBody2D_C *body_a,
                                                                      ChronoBody2D_C *body_b,
                                                                      const ChronoContact2D_C *contact) {
    return chrono_contact_manager2d_update_pair(manager, body_a, body_b, contact);
}

ChronoContactPoint2D_C *chrono_contact_manager2d_update_contact(ChronoContactManager2D_C *manager,
                                                                ChronoBody2D_C *body_a,
                                                                ChronoBody2D_C *body_b,
                                                                const ChronoContact2D_C *contact) {
    return chrono_contact_manager2d_update_pair(manager, body_a, body_b, contact);
}
