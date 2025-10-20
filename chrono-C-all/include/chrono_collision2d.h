#ifndef CHRONO_COLLISION2D_H
#define CHRONO_COLLISION2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chrono_body2d.h"

typedef struct ChronoContact2D_C {
    double normal[2];
    double contact_point[2];
    double penetration;
    int has_contact;
} ChronoContact2D_C;

#define CHRONO_CONTACT2D_MAX_POINTS 2

typedef struct ChronoContactPoint2D_C {
    ChronoContact2D_C contact;
    double normal_impulse;
    double tangent_impulse;
    int is_active;
} ChronoContactPoint2D_C;

typedef struct ChronoContactManifold2D_C {
    ChronoBody2D_C *body_a;
    ChronoBody2D_C *body_b;
    ChronoContactPoint2D_C points[CHRONO_CONTACT2D_MAX_POINTS];
    int num_points;
    double combined_restitution;
    double combined_friction_static;
    double combined_friction_dynamic;
} ChronoContactManifold2D_C;

void chrono_contact_manifold2d_init(ChronoContactManifold2D_C *manifold);
void chrono_contact_manifold2d_reset(ChronoContactManifold2D_C *manifold);
void chrono_contact_manifold2d_set_bodies(ChronoContactManifold2D_C *manifold,
                                          ChronoBody2D_C *body_a,
                                          ChronoBody2D_C *body_b);
ChronoContactPoint2D_C *chrono_contact_manifold2d_add_or_update(ChronoContactManifold2D_C *manifold,
                                                                const ChronoContact2D_C *contact);

double chrono_body2d_get_restitution(const ChronoBody2D_C *body);
double chrono_body2d_get_friction_static(const ChronoBody2D_C *body);
double chrono_body2d_get_friction_dynamic(const ChronoBody2D_C *body);
void chrono_body2d_set_restitution(ChronoBody2D_C *body, double restitution);
void chrono_body2d_set_friction_static(ChronoBody2D_C *body, double mu_s);
void chrono_body2d_set_friction_dynamic(ChronoBody2D_C *body, double mu_d);

typedef struct ChronoContactPair2D_C {
    ChronoBody2D_C *body_a;
    ChronoBody2D_C *body_b;
    ChronoContactManifold2D_C manifold;
} ChronoContactPair2D_C;

typedef struct ChronoContactManager2D_C {
    ChronoContactPair2D_C *pairs;
    size_t count;
    size_t capacity;
} ChronoContactManager2D_C;

void chrono_contact_manager2d_init(ChronoContactManager2D_C *manager);
void chrono_contact_manager2d_reset(ChronoContactManager2D_C *manager);
void chrono_contact_manager2d_free(ChronoContactManager2D_C *manager);
void chrono_contact_manager2d_begin_step(ChronoContactManager2D_C *manager);
void chrono_contact_manager2d_end_step(ChronoContactManager2D_C *manager);
ChronoContactManifold2D_C *chrono_contact_manager2d_get_manifold(ChronoContactManager2D_C *manager,
                                                                 ChronoBody2D_C *body_a,
                                                                 ChronoBody2D_C *body_b);
ChronoContactPoint2D_C *chrono_contact_manager2d_update_circle_circle(ChronoContactManager2D_C *manager,
                                                                      ChronoBody2D_C *body_a,
                                                                      ChronoBody2D_C *body_b,
                                                                      const ChronoContact2D_C *contact);
ChronoContactPoint2D_C *chrono_contact_manager2d_update_contact(ChronoContactManager2D_C *manager,
                                                                ChronoBody2D_C *body_a,
                                                                ChronoBody2D_C *body_b,
                                                                const ChronoContact2D_C *contact);

int chrono_collision2d_detect_circle_circle(const ChronoBody2D_C *body_a,
                                            const ChronoBody2D_C *body_b,
                                            ChronoContact2D_C *contact);
int chrono_collision2d_detect_circle_polygon(const ChronoBody2D_C *circle_body,
                                             const ChronoBody2D_C *polygon_body,
                                             ChronoContact2D_C *contact);
int chrono_collision2d_detect_polygon_circle(const ChronoBody2D_C *polygon_body,
                                             const ChronoBody2D_C *circle_body,
                                             ChronoContact2D_C *contact);
int chrono_collision2d_detect_polygon_polygon(const ChronoBody2D_C *body_a,
                                              const ChronoBody2D_C *body_b,
                                              ChronoContact2D_C *contact);

int chrono_collision2d_resolve_circle_circle(ChronoBody2D_C *body_a,
                                             ChronoBody2D_C *body_b,
                                             const ChronoContact2D_C *contact,
                                             double restitution,
                                             double friction_static,
                                             double friction_dynamic,
                                             ChronoContactManifold2D_C *manifold);
int chrono_collision2d_resolve_contact(ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const ChronoContact2D_C *contact,
                                       double restitution,
                                       double friction_static,
                                       double friction_dynamic,
                                       ChronoContactManifold2D_C *manifold);
int chrono_collision2d_resolve_polygon_polygon(ChronoBody2D_C *body_a,
                                               ChronoBody2D_C *body_b,
                                               const ChronoContact2D_C *contact,
                                               double restitution,
                                               double friction_static,
                                               double friction_dynamic,
                                               ChronoContactManifold2D_C *manifold);
int chrono_collision2d_resolve_circle_polygon(ChronoBody2D_C *circle_body,
                                              ChronoBody2D_C *polygon_body,
                                              const ChronoContact2D_C *contact,
                                              double restitution,
                                              double friction_static,
                                              double friction_dynamic,
                                              ChronoContactManifold2D_C *manifold);

#ifdef __cplusplus
}
#endif

#endif
