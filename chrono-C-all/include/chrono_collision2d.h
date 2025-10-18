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

int chrono_collision2d_detect_circle_circle(const ChronoBody2D_C *body_a,
                                            const ChronoBody2D_C *body_b,
                                            ChronoContact2D_C *contact);

int chrono_collision2d_resolve_circle_circle(ChronoBody2D_C *body_a,
                                             ChronoBody2D_C *body_b,
                                             const ChronoContact2D_C *contact,
                                             double restitution);

#ifdef __cplusplus
}
#endif

#endif
