#ifndef CHRONO_CONSTRAINT2D_H
#define CHRONO_CONSTRAINT2D_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chrono_body2d.h"

typedef struct ChronoDistanceConstraint2D_C {
    ChronoBody2D_C *body_a;
    ChronoBody2D_C *body_b;
    double local_anchor_a[2];
    double local_anchor_b[2];
    double rest_length;
    double baumgarte_beta;
    double softness;
    double slop;
    double max_correction;
    double accumulated_impulse;
    double effective_mass;
    double normal[2];
    double ra[2];
    double rb[2];
    double bias;
} ChronoDistanceConstraint2D_C;

void chrono_distance_constraint2d_init(ChronoDistanceConstraint2D_C *constraint,
                                       ChronoBody2D_C *body_a,
                                       ChronoBody2D_C *body_b,
                                       const double local_anchor_a[2],
                                       const double local_anchor_b[2],
                                       double rest_length);

void chrono_distance_constraint2d_set_baumgarte(ChronoDistanceConstraint2D_C *constraint, double beta);
void chrono_distance_constraint2d_set_softness(ChronoDistanceConstraint2D_C *constraint, double softness);
void chrono_distance_constraint2d_set_slop(ChronoDistanceConstraint2D_C *constraint, double slop);
void chrono_distance_constraint2d_set_max_correction(ChronoDistanceConstraint2D_C *constraint, double max_correction);

void chrono_distance_constraint2d_prepare(ChronoDistanceConstraint2D_C *constraint, double dt);
void chrono_distance_constraint2d_apply_warm_start(ChronoDistanceConstraint2D_C *constraint);
void chrono_distance_constraint2d_solve_velocity(ChronoDistanceConstraint2D_C *constraint);
void chrono_distance_constraint2d_solve_position(ChronoDistanceConstraint2D_C *constraint);

#ifdef __cplusplus
}
#endif

#endif
