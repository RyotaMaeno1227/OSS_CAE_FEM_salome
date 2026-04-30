#ifndef FEM4C_COUPLED_STEP_COMMON2D_H
#define FEM4C_COUPLED_STEP_COMMON2D_H

#include "coupled_run2d.h"
#include "flex_body2d.h"

int coupled_step_common2d_count_defined_flex_bodies(const coupled_run2d_t *run);

/* flex_slot is validated against run->case_data.num_flex_bodies. */
fem_error_t coupled_step_common2d_get_body_const_for_slot(
    const coupled_run2d_t *run,
    int flex_slot,
    const coupled_case2d_flex_body_t **case_body_out,
    const mbd_body2d_t **body_out);

fem_error_t coupled_step_common2d_init_flex_body(
    flex_body2d_t *flex_body,
    const coupled_case2d_flex_body_t *case_body,
    const fem_model_t *model);

fem_error_t coupled_step_common2d_record_flex_snapshot_result(
    coupled_run2d_t *run,
    int flex_slot,
    flex_body2d_t *flex_body,
    int feedback_to_mbd,
    double total_body_force_out[3],
    coupled_step_history2d_t *history);

fem_error_t coupled_step_common2d_solve_flex_snapshot_for_slot(
    coupled_run2d_t *run,
    int flex_slot,
    const double current_pose_override[3],
    int feedback_to_mbd,
    const char *line_prefix,
    const char *detail_prefix,
    const char *current_pose_label,
    const char *force_label,
    double total_body_force_out[3],
    coupled_step_history2d_t *history);

fem_error_t coupled_step_common2d_compute_interface_marker_disp(
    const flex_body2d_t *flex_body,
    const double reference_pose[3],
    const double current_pose[3],
    double root_marker_disp[3],
    double tip_marker_disp[3]);

void coupled_step_common2d_log_flex_body_step(
    const char *line_prefix,
    const char *detail_prefix,
    int flex_slot,
    int body_id,
    const double reference_pose[3],
    const double current_pose[3],
    const double root_marker_disp[3],
    const double tip_marker_disp[3],
    int num_root_nodes,
    int num_tip_nodes,
    const flex_body2d_t *flex_body,
    const double total_body_force[3],
    const char *current_pose_label,
    const char *force_label,
    int feedback_to_mbd);

fem_error_t coupled_step_common2d_resolve_mbd_integrator(
    const coupled_run2d_t *run,
    mbd_integrator2d_t *integrator_out);

fem_error_t coupled_step_common2d_do_mbd_step(coupled_run2d_t *run);

void coupled_step_common2d_sync_mbd_time(coupled_run2d_t *run,
                                         mbd_integrator2d_t integrator);

fem_error_t coupled_step_common2d_capture_current_pose_for_slot(
    const coupled_run2d_t *run,
    int flex_slot,
    double current_pose[3]);

fem_error_t coupled_step_common2d_capture_reference_pose(
    const mbd_body2d_t *body,
    double reference_pose[3]);

fem_error_t coupled_step_common2d_capture_current_pose(
    const mbd_body2d_t *body,
    double current_pose[3]);

#endif /* FEM4C_COUPLED_STEP_COMMON2D_H */
