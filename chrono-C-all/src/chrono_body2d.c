#include "../include/chrono_body2d.h"

#include <math.h>
#include <string.h>

static void rotate2d(double angle, const double v[2], double out[2]) {
    double c = cos(angle);
    double s = sin(angle);
    out[0] = c * v[0] - s * v[1];
    out[1] = s * v[0] + c * v[1];
}

void chrono_body2d_init(ChronoBody2D_C *body) {
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
    body->inverse_mass = 0.0;
    body->inverse_inertia = 0.0;
    body->is_static = 1;
    body->circle_radius = 0.0;
    body->shape_type = CHRONO_BODY2D_SHAPE_NONE;
    body->polygon_vertex_count = 0;
    body->restitution = 0.0;
    body->friction_static = 0.0;
    body->friction_dynamic = 0.0;
}

void chrono_body2d_set_mass(ChronoBody2D_C *body, double mass, double inertia) {
    if (!body) {
        return;
    }
    body->is_static = 0;
    body->inverse_mass = (mass > 0.0) ? 1.0 / mass : 0.0;
    body->inverse_inertia = (inertia > 0.0) ? 1.0 / inertia : 0.0;
}

void chrono_body2d_set_static(ChronoBody2D_C *body) {
    if (!body) {
        return;
    }
    body->is_static = 1;
    body->inverse_mass = 0.0;
    body->inverse_inertia = 0.0;
}

void chrono_body2d_apply_force(ChronoBody2D_C *body, const double force[2]) {
    if (!body || body->is_static) {
        return;
    }
    body->force[0] += force[0];
    body->force[1] += force[1];
}

void chrono_body2d_apply_torque(ChronoBody2D_C *body, double torque) {
    if (!body || body->is_static) {
        return;
    }
    body->torque += torque;
}

void chrono_body2d_apply_impulse(ChronoBody2D_C *body, const double impulse[2], const double world_point[2]) {
    if (!body || body->is_static) {
        return;
    }
    body->linear_velocity[0] += impulse[0] * body->inverse_mass;
    body->linear_velocity[1] += impulse[1] * body->inverse_mass;

    double r_local[2];
    double p_local[2];
    chrono_body2d_world_to_local(body, world_point, p_local);
    r_local[0] = p_local[0];
    r_local[1] = p_local[1];

    double r_world[2];
    rotate2d(body->angle, r_local, r_world);

    double torque_impulse = r_world[0] * impulse[1] - r_world[1] * impulse[0];
    body->angular_velocity += torque_impulse * body->inverse_inertia;
}

void chrono_body2d_integrate_explicit(ChronoBody2D_C *body, double dt) {
    if (!body || body->is_static) {
        return;
    }
    double acc[2] = {body->force[0] * body->inverse_mass, body->force[1] * body->inverse_mass};
    body->linear_velocity[0] += acc[0] * dt;
    body->linear_velocity[1] += acc[1] * dt;
    body->position[0] += body->linear_velocity[0] * dt;
    body->position[1] += body->linear_velocity[1] * dt;

    double angular_acc = body->torque * body->inverse_inertia;
    body->angular_velocity += angular_acc * dt;
    body->angle += body->angular_velocity * dt;
}

void chrono_body2d_local_to_world(const ChronoBody2D_C *body, const double local[2], double world[2]) {
    double rotated[2];
    rotate2d(body->angle, local, rotated);
    world[0] = body->position[0] + rotated[0];
    world[1] = body->position[1] + rotated[1];
}

void chrono_body2d_world_to_local(const ChronoBody2D_C *body, const double world[2], double local[2]) {
    double dx = world[0] - body->position[0];
    double dy = world[1] - body->position[1];
    double c = cos(body->angle);
    double s = sin(body->angle);
    local[0] = c * dx + s * dy;
    local[1] = -s * dx + c * dy;
}

void chrono_body2d_reset_forces(ChronoBody2D_C *body) {
    if (!body) {
        return;
    }
    body->force[0] = 0.0;
    body->force[1] = 0.0;
    body->torque = 0.0;
}

void chrono_body2d_set_circle_shape(ChronoBody2D_C *body, double radius) {
    if (!body) {
        return;
    }
    body->circle_radius = (radius > 0.0) ? radius : 0.0;
    body->shape_type = CHRONO_BODY2D_SHAPE_CIRCLE;
    body->polygon_vertex_count = 0;
}

double chrono_body2d_get_circle_radius(const ChronoBody2D_C *body) {
    if (!body) {
        return 0.0;
    }
    return body->circle_radius;
}

int chrono_body2d_set_polygon_shape(ChronoBody2D_C *body, const double *vertices, size_t vertex_count) {
    if (!body || !vertices || vertex_count < 3 || vertex_count > CHRONO_BODY2D_MAX_POLYGON_VERTICES) {
        return 0;
    }
    for (size_t i = 0; i < vertex_count; ++i) {
        body->polygon_vertices[i][0] = vertices[2 * i];
        body->polygon_vertices[i][1] = vertices[2 * i + 1];
    }
    body->polygon_vertex_count = vertex_count;
    body->shape_type = CHRONO_BODY2D_SHAPE_POLYGON;
    body->circle_radius = 0.0;
    return 1;
}

size_t chrono_body2d_get_polygon_vertex_count(const ChronoBody2D_C *body) {
    if (!body || body->shape_type != CHRONO_BODY2D_SHAPE_POLYGON) {
        return 0;
    }
    return body->polygon_vertex_count;
}

const double *chrono_body2d_get_polygon_vertices(const ChronoBody2D_C *body) {
    if (!body || body->shape_type != CHRONO_BODY2D_SHAPE_POLYGON || body->polygon_vertex_count == 0) {
        return NULL;
    }
    return &body->polygon_vertices[0][0];
}

ChronoBody2DShapeType_C chrono_body2d_get_shape_type(const ChronoBody2D_C *body) {
    if (!body) {
        return CHRONO_BODY2D_SHAPE_NONE;
    }
    return (ChronoBody2DShapeType_C)body->shape_type;
}

void chrono_body2d_set_restitution(ChronoBody2D_C *body, double restitution) {
    if (!body) {
        return;
    }
    if (restitution < 0.0) {
        restitution = 0.0;
    } else if (restitution > 1.0) {
        restitution = 1.0;
    }
    body->restitution = restitution;
}

double chrono_body2d_get_restitution(const ChronoBody2D_C *body) {
    if (!body) {
        return 0.0;
    }
    return body->restitution;
}

void chrono_body2d_set_friction_static(ChronoBody2D_C *body, double mu_s) {
    if (!body) {
        return;
    }
    if (mu_s < 0.0) {
        mu_s = 0.0;
    }
    body->friction_static = mu_s;
}

double chrono_body2d_get_friction_static(const ChronoBody2D_C *body) {
    if (!body) {
        return 0.0;
    }
    return body->friction_static;
}

void chrono_body2d_set_friction_dynamic(ChronoBody2D_C *body, double mu_d) {
    if (!body) {
        return;
    }
    if (mu_d < 0.0) {
        mu_d = 0.0;
    }
    body->friction_dynamic = mu_d;
}

double chrono_body2d_get_friction_dynamic(const ChronoBody2D_C *body) {
    if (!body) {
        return 0.0;
    }
    return body->friction_dynamic;
}

ChronoMaterial2D_C chrono_material2d_make(double restitution, double mu_s, double mu_d) {
    ChronoMaterial2D_C material;
    material.restitution = (restitution < 0.0) ? 0.0 : (restitution > 1.0 ? 1.0 : restitution);
    material.friction_static = (mu_s < 0.0) ? 0.0 : mu_s;
    material.friction_dynamic = (mu_d < 0.0) ? 0.0 : mu_d;
    return material;
}

void chrono_body2d_set_material(ChronoBody2D_C *body, const ChronoMaterial2D_C *material) {
    if (!body || !material) {
        return;
    }
    chrono_body2d_set_restitution(body, material->restitution);
    chrono_body2d_set_friction_static(body, material->friction_static);
    chrono_body2d_set_friction_dynamic(body, material->friction_dynamic);
}
