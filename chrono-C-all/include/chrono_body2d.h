#ifndef CHRONO_BODY2D_H
#define CHRONO_BODY2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct ChronoBody2D_C {
    double position[2];
    double angle;
    double linear_velocity[2];
    double angular_velocity;
    double force[2];
    double torque;
    double inverse_mass;
    double inverse_inertia;
    int is_static;
    double circle_radius;
    double restitution;
    double friction_static;
    double friction_dynamic;
    int shape_type;
    size_t polygon_vertex_count;
    double polygon_vertices[16][2];
} ChronoBody2D_C;

typedef struct ChronoMaterial2D_C {
    double restitution;
    double friction_static;
    double friction_dynamic;
} ChronoMaterial2D_C;

#define CHRONO_BODY2D_MAX_POLYGON_VERTICES 16

typedef enum ChronoBody2DShapeType_C {
    CHRONO_BODY2D_SHAPE_NONE = 0,
    CHRONO_BODY2D_SHAPE_CIRCLE = 1,
    CHRONO_BODY2D_SHAPE_POLYGON = 2
} ChronoBody2DShapeType_C;

void chrono_body2d_init(ChronoBody2D_C *body);
void chrono_body2d_set_mass(ChronoBody2D_C *body, double mass, double inertia);
void chrono_body2d_set_static(ChronoBody2D_C *body);
void chrono_body2d_apply_force(ChronoBody2D_C *body, const double force[2]);
void chrono_body2d_apply_torque(ChronoBody2D_C *body, double torque);
void chrono_body2d_apply_impulse(ChronoBody2D_C *body, const double impulse[2], const double world_point[2]);
void chrono_body2d_integrate_explicit(ChronoBody2D_C *body, double dt);
void chrono_body2d_local_to_world(const ChronoBody2D_C *body, const double local[2], double world[2]);
void chrono_body2d_world_to_local(const ChronoBody2D_C *body, const double world[2], double local[2]);
void chrono_body2d_reset_forces(ChronoBody2D_C *body);
void chrono_body2d_set_circle_shape(ChronoBody2D_C *body, double radius);
double chrono_body2d_get_circle_radius(const ChronoBody2D_C *body);
int chrono_body2d_set_polygon_shape(ChronoBody2D_C *body, const double *vertices, size_t vertex_count);
size_t chrono_body2d_get_polygon_vertex_count(const ChronoBody2D_C *body);
const double *chrono_body2d_get_polygon_vertices(const ChronoBody2D_C *body);
ChronoBody2DShapeType_C chrono_body2d_get_shape_type(const ChronoBody2D_C *body);
int chrono_body2d_set_polygon_shape_with_density(ChronoBody2D_C *body,
                                                 const double *vertices,
                                                 size_t vertex_count,
                                                 double density);
void chrono_body2d_set_restitution(ChronoBody2D_C *body, double restitution);
double chrono_body2d_get_restitution(const ChronoBody2D_C *body);
void chrono_body2d_set_friction_static(ChronoBody2D_C *body, double mu_s);
double chrono_body2d_get_friction_static(const ChronoBody2D_C *body);
void chrono_body2d_set_friction_dynamic(ChronoBody2D_C *body, double mu_d);
double chrono_body2d_get_friction_dynamic(const ChronoBody2D_C *body);
ChronoMaterial2D_C chrono_material2d_make(double restitution, double mu_s, double mu_d);
void chrono_body2d_set_material(ChronoBody2D_C *body, const ChronoMaterial2D_C *material);

#ifdef __cplusplus
}
#endif

#endif
