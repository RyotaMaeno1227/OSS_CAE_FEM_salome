#ifndef FEM4C_MBD_OUTPUT2D_H
#define FEM4C_MBD_OUTPUT2D_H

#include "system/system2d.h"
#include <stdio.h>

#define MBD_OUTPUT2D_HISTORY_FIELD_COUNT 33
#define MBD_OUTPUT2D_RIGID_COMPARE_FIELD_COUNT 14
#define MBD_OUTPUT2D_HISTORY_PROJECTION_TAIL_CSV \
    "position_projection_applied,position_projection_target_reached," \
    "position_projection_iterations_last,position_projection_max_iterations," \
    "position_projection_stop_reason,position_projection_residual_l2_after," \
    "position_projection_residual_reduction_ratio_last," \
    "position_projection_velocity_residual_l2_before," \
    "position_projection_velocity_residual_l2_after," \
    "position_projection_velocity_reduction_ratio_last," \
    "revolute_anchor_mismatch_max,revolute_body_j_com_radius_max\n"

enum {
    MBD_OUTPUT2D_HISTORY_FIELD_STEP = 0,
    MBD_OUTPUT2D_HISTORY_FIELD_TIME = 1,
    MBD_OUTPUT2D_HISTORY_FIELD_BODY_ID = 2,
    MBD_OUTPUT2D_HISTORY_FIELD_X = 3,
    MBD_OUTPUT2D_HISTORY_FIELD_Y = 4,
    MBD_OUTPUT2D_HISTORY_FIELD_THETA = 5,
    MBD_OUTPUT2D_HISTORY_FIELD_VX = 6,
    MBD_OUTPUT2D_HISTORY_FIELD_VY = 7,
    MBD_OUTPUT2D_HISTORY_FIELD_OMEGA = 8,
    MBD_OUTPUT2D_HISTORY_FIELD_AX = 9,
    MBD_OUTPUT2D_HISTORY_FIELD_AY = 10,
    MBD_OUTPUT2D_HISTORY_FIELD_ALPHA = 11,
    MBD_OUTPUT2D_HISTORY_FIELD_HHT_FORCE_HISTORY_MODE = 12,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_CONVERGED = 13,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_SCHEME = 14,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_REASON = 15,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_ITERATIONS_LAST = 16,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_RESIDUAL_MODE = 17,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_RESIDUAL_L2_LAST = 18,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_NUM_EQUATIONS_LAST = 19,
    MBD_OUTPUT2D_HISTORY_FIELD_IMPLICIT_RESULT_TOLERANCE_LAST = 20,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_APPLIED = 21,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_TARGET_REACHED = 22,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_ITERATIONS_LAST = 23,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_MAX_ITERATIONS = 24,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_STOP_REASON = 25,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_RESIDUAL_L2_AFTER = 26,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_REDUCTION_RATIO_LAST = 27,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_VELOCITY_L2_BEFORE = 28,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_VELOCITY_L2_AFTER = 29,
    MBD_OUTPUT2D_HISTORY_FIELD_POSITION_PROJECTION_VELOCITY_REDUCTION_RATIO_LAST = 30,
    MBD_OUTPUT2D_HISTORY_FIELD_REVOLUTE_ANCHOR_MISMATCH_MAX = 31,
    MBD_OUTPUT2D_HISTORY_FIELD_REVOLUTE_BODY_J_COM_RADIUS_MAX = 32
};

enum {
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_TIME = 0,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_THETA1 = 1,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_THETA2 = 2,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_OMEGA1 = 3,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_OMEGA2 = 4,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_TIP1_X = 5,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_TIP1_Y = 6,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_TIP2_X = 7,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_TIP2_Y = 8,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_ROOT_REACTION_X = 9,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_ROOT_REACTION_Y = 10,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_CONSTRAINT_RESIDUAL = 11,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_FULL_REASSEMBLY_COUNT_LINK1 = 12,
    MBD_OUTPUT2D_RIGID_COMPARE_FIELD_FULL_REASSEMBLY_COUNT_LINK2 = 13
};

const char *mbd_output2d_history_header_csv(void);
const char *mbd_output2d_rigid_compare_header_csv(void);
const char *mbd_output2d_rigid_compare_tip_geometry_contract(void);
const char *mbd_output2d_rigid_compare_root_reaction_surface(void);
const char *mbd_output2d_rigid_energy_surface(void);
fem_error_t mbd_output2d_write_header(FILE *out);

fem_error_t mbd_output2d_write_body_row(FILE *out,
                                        int step,
                                        double time,
                                        const mbd_body2d_t *body,
                                        const mbd_system2d_t *system);

fem_error_t mbd_output2d_write_system_snapshot(FILE *out,
                                               int step,
                                               double time,
                                               const mbd_system2d_t *system);
fem_error_t mbd_output2d_write_generalized_force_history_rows(
    FILE *out,
    const mbd_system2d_t *system);
fem_error_t mbd_output2d_write_rigid_compare_header(FILE *out);
fem_error_t mbd_output2d_write_rigid_compare_row(FILE *out,
                                                 double time,
                                                 const mbd_system2d_t *system,
                                                 int body1_id,
                                                 int body2_id);

#endif /* FEM4C_MBD_OUTPUT2D_H */
