#include "output2d.h"
#include "../../coupled/case2d.h"
#include "../../common/error.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *mbd_output2d_history_header_csv(void)
{
    return "step,time,body_id,x,y,theta,vx,vy,omega,ax,ay,alpha,hht_force_history_mode,implicit_result_converged,implicit_result_scheme,implicit_result_reason,implicit_result_iterations_last,implicit_result_residual_mode,implicit_result_residual_l2_last,implicit_result_residual_num_equations_last,implicit_result_residual_tolerance_last,"
           MBD_OUTPUT2D_HISTORY_PROJECTION_TAIL_CSV;
}

const char *mbd_output2d_rigid_compare_header_csv(void)
{
    return "time,theta1,theta2,omega1,omega2,tip1_x,tip1_y,tip2_x,tip2_y,root_reaction_x,root_reaction_y,constraint_residual,full_reassembly_count_link1,full_reassembly_count_link2\n";
}

const char *mbd_output2d_rigid_compare_tip_geometry_contract(void)
{
    return "optional_coupled_flex_geometry";
}

const char *mbd_output2d_rigid_compare_root_reaction_surface(void)
{
    return "nan_placeholder_unsupported";
}

const char *mbd_output2d_rigid_energy_surface(void)
{
    return "history_csv_postprocess_only";
}

const char *mbd_output2d_contact_trace_header_csv(void)
{
    return "step,time,pair_id,active,gap,penetration,nx,ny,vn,fn,cp1_x,cp1_y,cp2_x,cp2_y,f1_x,f1_y,m1_z,f2_x,f2_y,m2_z\n";
}

const char *mbd_output2d_contact_feedback_header_csv(void)
{
    return "step,time,pair_id,active,gap,penetration,fn,kn_base,kn_used,kn_out,coupling_mode\n";
}

const char *mbd_output2d_contact_feedback_use_header_csv(void)
{
    return "step,time,pair_id,mu_base,mu_used,gamma_n_used,k_base,k_used,source_mode,fallback_reason\n";
}

const char *mbd_output2d_contact_reduced_data_header_csv(void)
{
    return "step,time,pair_id,source_mode,fallback_reason,record_step,record_iter,mu_eff,gamma_n,delta_g_eff,fn_ref,p_max,h_min,regime_flag,valid_flag,status_ok,status\n";
}

const char *mbd_output2d_same_time_reduced_iteration_header_csv(void)
{
    return "step,iter,pair_id,mu_guess,mu_new,gamma_n_guess,gamma_n_new,fn,gap,vt,stop_reason\n";
}

const char *mbd_output2d_same_time_contact_request_header_csv(void)
{
    return "step,iter,pair_id,body_i,body_j,request_valid,contact_active,x_cp,y_cp,n_x,n_y,t_x,t_y,gap,penetration,v_n,v_t,source_mode_before_lookup\n";
}

fem_error_t mbd_output2d_write_contact_trace_header(FILE *out)
{
    CHECK_NULL(out, "mbd contact trace output stream");

    if (fprintf(out, "%s", mbd_output2d_contact_trace_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD contact trace CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_feedback_header(FILE *out)
{
    CHECK_NULL(out, "mbd contact feedback output stream");

    if (fprintf(out, "%s", mbd_output2d_contact_feedback_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD contact feedback CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_feedback_use_header(FILE *out)
{
    CHECK_NULL(out, "mbd contact feedback-use output stream");

    if (fprintf(out, "%s", mbd_output2d_contact_feedback_use_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD contact feedback-use CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_reduced_data_header(FILE *out)
{
    CHECK_NULL(out, "mbd contact reduced-data output stream");

    if (fprintf(out, "%s", mbd_output2d_contact_reduced_data_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD contact reduced-data CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_same_time_reduced_iteration_header(FILE *out)
{
    CHECK_NULL(out, "mbd same-time reduced iteration output stream");

    if (fprintf(out, "%s", mbd_output2d_same_time_reduced_iteration_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD same-time reduced iteration CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_same_time_contact_request_header(FILE *out)
{
    CHECK_NULL(out, "mbd same-time contact request output stream");

    if (fprintf(out, "%s", mbd_output2d_same_time_contact_request_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD same-time contact request CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_trace_snapshot(FILE *out,
                                                      int step,
                                                      double time,
                                                      const mbd_system2d_t *system)
{
    int pair_index;

    CHECK_NULL(out, "mbd contact trace output stream");
    CHECK_NULL(system, "mbd contact trace system");

    if (step < 0 || !isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD contact trace requires non-negative finite step/time");
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_contact_trace2d_t *trace = &system->current_contact_trace[pair_index];

        if (!trace->is_defined) {
            continue;
        }
        if (!isfinite(trace->gap) ||
            !isfinite(trace->penetration) ||
            !isfinite(trace->normal[0]) ||
            !isfinite(trace->normal[1]) ||
            !isfinite(trace->vn) ||
            !isfinite(trace->fn) ||
            !isfinite(trace->cp1[0]) ||
            !isfinite(trace->cp1[1]) ||
            !isfinite(trace->cp2[0]) ||
            !isfinite(trace->cp2[1]) ||
            !isfinite(trace->f1[0]) ||
            !isfinite(trace->f1[1]) ||
            !isfinite(trace->m1_z) ||
            !isfinite(trace->f2[0]) ||
            !isfinite(trace->f2[1]) ||
            !isfinite(trace->m2_z)) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD contact trace values must be finite");
        }

        if (fprintf(out,
                    "%d,%.16e,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e\n",
                    step,
                    time,
                    trace->pair_id,
                    trace->active,
                    trace->gap,
                    trace->penetration,
                    trace->normal[0],
                    trace->normal[1],
                    trace->vn,
                    trace->fn,
                    trace->cp1[0],
                    trace->cp1[1],
                    trace->cp2[0],
                    trace->cp2[1],
                    trace->f1[0],
                    trace->f1[1],
                    trace->m1_z,
                    trace->f2[0],
                    trace->f2[1],
                    trace->m2_z) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD contact trace CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_feedback_snapshot(FILE *out,
                                                         int step,
                                                         double time,
                                                         const mbd_system2d_t *system)
{
    int pair_index;

    CHECK_NULL(out, "mbd contact feedback output stream");
    CHECK_NULL(system, "mbd contact feedback system");

    if (step < 0 || !isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD contact feedback requires non-negative finite step/time");
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_contact_feedback_trace2d_t *trace = &system->current_contact_feedback[pair_index];

        if (!trace->is_defined) {
            continue;
        }
        if (!isfinite(trace->gap) ||
            !isfinite(trace->penetration) ||
            !isfinite(trace->fn) ||
            !isfinite(trace->kn_base) ||
            !isfinite(trace->kn_used) ||
            !isfinite(trace->kn_out)) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD contact feedback values must be finite");
        }

        if (fprintf(out,
                    "%d,%.16e,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s\n",
                    step,
                    time,
                    trace->pair_id,
                    trace->active,
                    trace->gap,
                    trace->penetration,
                    trace->fn,
                    trace->kn_base,
                    trace->kn_used,
                    trace->kn_out,
                    mbd_contact_coupling_mode_to_string(system->contact_coupling_mode)) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD contact feedback CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_feedback_use_snapshot(FILE *out,
                                                             int step,
                                                             double time,
                                                             const mbd_system2d_t *system)
{
    int pair_index;

    CHECK_NULL(out, "mbd contact feedback-use output stream");
    CHECK_NULL(system, "mbd contact feedback-use system");

    if (step < 0 || !isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD contact feedback-use requires non-negative finite step/time");
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_contact_feedback_use_trace2d_t *trace = &system->current_contact_feedback_use[pair_index];

        if (!trace->is_defined) {
            continue;
        }
        if (!isfinite(trace->reduced_data.mu_eff) ||
            !isfinite(trace->reduced_data.gamma_n) ||
            !isfinite(trace->reduced_data.delta_g_eff) ||
            !isfinite(trace->reduced_data.fn_ref) ||
            !isfinite(trace->reduced_data.p_max) ||
            !isfinite(trace->reduced_data.h_min) ||
            !isfinite(trace->mu_base) ||
            !isfinite(trace->mu_used) ||
            !isfinite(trace->gamma_n_used) ||
            !isfinite(trace->k_base) ||
            !isfinite(trace->k_used) ||
            trace->source_mode[0] == '\0' ||
            trace->fallback_reason[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD contact feedback-use values must be finite and populated");
        }

        if (fprintf(out,
                    "%d,%.16e,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%s\n",
                    step,
                    time,
                    trace->pair_id,
                    trace->mu_base,
                    trace->mu_used,
                    trace->gamma_n_used,
                    trace->k_base,
                    trace->k_used,
                    trace->source_mode,
                    trace->fallback_reason) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD contact feedback-use CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_contact_reduced_data_snapshot(FILE *out,
                                                             int step,
                                                             double time,
                                                             const mbd_system2d_t *system)
{
    int pair_index;

    CHECK_NULL(out, "mbd contact reduced-data output stream");
    CHECK_NULL(system, "mbd contact reduced-data system");

    if (step < 0 || !isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD contact reduced-data requires non-negative finite step/time");
    }

    for (pair_index = 0; pair_index < system->num_contact_pairs; ++pair_index) {
        const mbd_contact_feedback_use_trace2d_t *trace = &system->current_contact_feedback_use[pair_index];

        if (!trace->is_defined) {
            continue;
        }
        if (!isfinite(trace->reduced_data.mu_eff) ||
            !isfinite(trace->reduced_data.gamma_n) ||
            !isfinite(trace->reduced_data.delta_g_eff) ||
            !isfinite(trace->reduced_data.fn_ref) ||
            !isfinite(trace->reduced_data.p_max) ||
            !isfinite(trace->reduced_data.h_min) ||
            trace->record_step < -1 ||
            trace->record_iter < -1 ||
            trace->source_mode[0] == '\0' ||
            trace->fallback_reason[0] == '\0' ||
            trace->status[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD contact reduced-data values must be finite and populated");
        }

        if (fprintf(out,
                    "%d,%.16e,%d,%s,%s,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%u,%u,%d,%s\n",
                    step,
                    time,
                    trace->pair_id,
                    trace->source_mode,
                    trace->fallback_reason,
                    trace->record_step,
                    trace->record_iter,
                    trace->reduced_data.mu_eff,
                    trace->reduced_data.gamma_n,
                    trace->reduced_data.delta_g_eff,
                    trace->reduced_data.fn_ref,
                    trace->reduced_data.p_max,
                    trace->reduced_data.h_min,
                    trace->reduced_data.regime_flag,
                    trace->reduced_data.valid_flag,
                    trace->status_ok,
                    trace->status) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD contact reduced-data CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_same_time_reduced_iteration_snapshot(
    FILE *out,
    const mbd_system2d_t *system)
{
    int row_index = 0;

    CHECK_NULL(out, "mbd same-time reduced iteration output stream");
    CHECK_NULL(system, "mbd same-time reduced iteration system");

    /* Rows are emitted per explicit step; the final per-step state lives in contact_reduced_data.csv. */
    for (row_index = 0; row_index < system->num_current_same_time_reduced_iterations; ++row_index) {
        const mbd_same_time_reduced_iteration2d_t *row =
            &system->current_same_time_reduced_iterations[row_index];

        if (!row->is_defined) {
            continue;
        }
        if (row->step < 0 ||
            row->iter <= 0 ||
            row->pair_id < 0 ||
            !isfinite(row->mu_guess) ||
            !isfinite(row->mu_new) ||
            !isfinite(row->gamma_n_guess) ||
            !isfinite(row->gamma_n_new) ||
            !isfinite(row->fn) ||
            !isfinite(row->gap) ||
            !isfinite(row->vt) ||
            row->stop_reason[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD same-time reduced iteration values must be finite and populated");
        }

        if (fprintf(out,
                    "%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s\n",
                    row->step,
                    row->iter,
                    row->pair_id,
                    row->mu_guess,
                    row->mu_new,
                    row->gamma_n_guess,
                    row->gamma_n_new,
                    row->fn,
                    row->gap,
                    row->vt,
                    row->stop_reason) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD same-time reduced iteration CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_same_time_contact_request_rows(
    FILE *out,
    const mbd_system2d_t *system)
{
    int row_index = 0;

    CHECK_NULL(out, "mbd same-time contact request output stream");
    CHECK_NULL(system, "mbd same-time contact request system");

    for (row_index = 0; row_index < system->num_same_time_contact_request_rows; ++row_index) {
        const mbd_same_time_contact_request2d_t *row =
            &system->same_time_contact_requests[row_index];

        if (!row->is_defined) {
            continue;
        }
        if (row->step < 0 ||
            row->iter <= 0 ||
            row->pair_id < 0 ||
            !isfinite(row->x_cp) ||
            !isfinite(row->y_cp) ||
            !isfinite(row->n_x) ||
            !isfinite(row->n_y) ||
            !isfinite(row->t_x) ||
            !isfinite(row->t_y) ||
            !isfinite(row->gap) ||
            !isfinite(row->penetration) ||
            !isfinite(row->v_n) ||
            !isfinite(row->v_t) ||
            row->source_mode_before_lookup[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD same-time contact request values must be finite and populated");
        }

        if (fprintf(out,
                    "%d,%d,%d,%d,%d,%d,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s\n",
                    row->step,
                    row->iter,
                    row->pair_id,
                    row->body_i,
                    row->body_j,
                    row->request_valid,
                    row->contact_active,
                    row->x_cp,
                    row->y_cp,
                    row->n_x,
                    row->n_y,
                    row->t_x,
                    row->t_y,
                    row->gap,
                    row->penetration,
                    row->v_n,
                    row->v_t,
                    row->source_mode_before_lookup) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD same-time contact request CSV row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_header(FILE *out)
{
    CHECK_NULL(out, "mbd output stream");

    if (fprintf(out, "%s", mbd_output2d_history_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD output CSV header");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_body_row(FILE *out,
                                        int step,
                                        double time,
                                        const mbd_body2d_t *body,
                                        const mbd_system2d_t *system)
{
    int i;
    double q[MBD_BODY2D_DOF];
    double v[MBD_BODY2D_DOF];
    double a[MBD_BODY2D_DOF];
    double revolute_anchor_mismatch_max = 0.0;
    double revolute_body_j_com_radius_max = 0.0;
    int revolute_count = 0;
    const char *hht_force_history_mode = NULL;
    const char *implicit_scheme_mode = NULL;
    const char *implicit_convergence_reason = NULL;
    const char *implicit_residual_mode = NULL;
    const char *position_projection_stop_reason = NULL;

    CHECK_NULL(out, "mbd output stream");
    CHECK_NULL(body, "mbd body");
    CHECK_NULL(system, "mbd system trace");

    if (step < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output step must be non-negative");
    }
    if (!isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output time must be finite and non-negative");
    }
    CHECK_ERROR(mbd_body2d_get_generalized_state(body, q, v, a));
    CHECK_ERROR(mbd_system2d_compute_revolute_metrics(system,
                                                      &revolute_anchor_mismatch_max,
                                                      &revolute_body_j_com_radius_max,
                                                      &revolute_count));
    for (i = 0; i < MBD_BODY2D_DOF; ++i) {
        if (!isfinite(q[i]) || !isfinite(v[i]) || !isfinite(a[i])) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD output body state must be finite");
        }
    }
    if (!isfinite(revolute_anchor_mismatch_max) ||
        !isfinite(revolute_body_j_com_radius_max) ||
        revolute_count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD revolute metrics must be finite");
    }
    hht_force_history_mode = system->hht_force_history_mode
                                 ? system->hht_force_history_mode
                                 : "not_applicable";
    implicit_scheme_mode = system->implicit_scheme_mode
                               ? system->implicit_scheme_mode
                               : "not_applicable";
    implicit_convergence_reason = system->implicit_convergence_reason
                                      ? system->implicit_convergence_reason
                                      : "not_run";
    implicit_residual_mode = system->implicit_residual_mode
                                 ? system->implicit_residual_mode
                                 : "constraint_residual_l2";
    position_projection_stop_reason = system->position_projection_stop_reason
                                          ? system->position_projection_stop_reason
                                          : "disabled";
    if (!isfinite(system->implicit_residual_l2_last) ||
        !isfinite(system->implicit_residual_tolerance_last) ||
        !isfinite(system->position_projection_residual_l2_after) ||
        !isfinite(system->position_projection_residual_reduction_ratio_last) ||
        !isfinite(system->position_projection_velocity_residual_l2_before) ||
        !isfinite(system->position_projection_velocity_residual_l2_after) ||
        !isfinite(system->position_projection_velocity_reduction_ratio_last)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output trace values must be finite");
    }

    if (fprintf(out,
                "%d,%.16e,%d,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%s,%d,%s,%s,%d,%s,%.16e,%d,%.16e,%d,%d,%d,%d,%s,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e\n",
                step,
                time,
                body->id,
                q[0],
                q[1],
                q[2],
                v[0],
                v[1],
                v[2],
                a[0],
                a[1],
                a[2],
                hht_force_history_mode,
                system->implicit_converged,
                implicit_scheme_mode,
                implicit_convergence_reason,
                system->time.implicit_iterations_last,
                implicit_residual_mode,
                system->implicit_residual_l2_last,
                system->implicit_residual_num_equations_last,
                system->implicit_residual_tolerance_last,
                system->position_projection_applied,
                system->position_projection_target_reached,
                system->position_projection_iterations_last,
                system->position_projection_max_iterations,
                position_projection_stop_reason,
                system->position_projection_residual_l2_after,
                system->position_projection_residual_reduction_ratio_last,
                system->position_projection_velocity_residual_l2_before,
                system->position_projection_velocity_residual_l2_after,
                system->position_projection_velocity_reduction_ratio_last,
                revolute_anchor_mismatch_max,
                revolute_body_j_com_radius_max) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD output CSV row");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_system_snapshot(FILE *out,
                                               int step,
                                               double time,
                                               const mbd_system2d_t *system)
{
    int body_index;

    CHECK_NULL(out, "mbd output stream");
    CHECK_NULL(system, "mbd system");

    if (system->num_bodies < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output body count %d must be non-negative",
                         system->num_bodies);
    }
    if (system->num_bodies > 0 && !system->bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD output requires body storage for %d bodies",
                         system->num_bodies);
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        CHECK_ERROR(mbd_output2d_write_body_row(out,
                                                step,
                                                time,
                                                &system->bodies[body_index],
                                                system));
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_generalized_force_history_rows(
    FILE *out,
    const mbd_system2d_t *system)
{
    int body_index;
    int dof;

    CHECK_NULL(out, "mbd output stream");
    CHECK_NULL(system, "mbd system");

    if (system->num_bodies < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD generalized force history body count %d must be non-negative",
                         system->num_bodies);
    }
    if (system->num_bodies > 0 && !system->bodies) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD generalized force history requires body storage for %d bodies",
                         system->num_bodies);
    }
    if (system->num_bodies > 0 &&
        (!system->current_generalized_force ||
         !system->previous_generalized_force)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD generalized force history requires force storage for %d bodies",
                         system->num_bodies);
    }
    if (fprintf(out,
                "generalized_force_history_valid,%d\n",
                system->generalized_force_history_valid ? 1 : 0) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write MBD generalized force history validity row");
    }
    if (!system->generalized_force_history_valid) {
        return FEM_SUCCESS;
    }

    for (body_index = 0; body_index < system->num_bodies; ++body_index) {
        for (dof = 0; dof < MBD_BODY2D_DOF; ++dof) {
            if (!isfinite(system->current_generalized_force[body_index][dof]) ||
                !isfinite(system->previous_generalized_force[body_index][dof])) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD generalized force history must be finite");
            }
        }
        if (fprintf(out,
                    "generalized_force_current,%d,%d,%.16e,%.16e,%.16e\n",
                    body_index,
                    system->bodies[body_index].id,
                    system->current_generalized_force[body_index][0],
                    system->current_generalized_force[body_index][1],
                    system->current_generalized_force[body_index][2]) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD current generalized force row");
        }
        if (fprintf(out,
                    "generalized_force_previous,%d,%d,%.16e,%.16e,%.16e\n",
                    body_index,
                    system->bodies[body_index].id,
                    system->previous_generalized_force[body_index][0],
                    system->previous_generalized_force[body_index][1],
                    system->previous_generalized_force[body_index][2]) < 0) {
            return error_set(FEM_ERROR_FILE_WRITE,
                             "Failed to write MBD previous generalized force row");
        }
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_rigid_compare_header(FILE *out)
{
    CHECK_NULL(out, "mbd rigid compare output stream");

    if (fprintf(out, "%s", mbd_output2d_rigid_compare_header_csv()) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write rigid compare CSV header");
    }

    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_capture_body_compare_fields(
    const mbd_body2d_t *body,
    double *theta,
    double *omega)
{
    double origin[2];
    double q[MBD_BODY2D_DOF];
    double v[MBD_BODY2D_DOF];
    double a[MBD_BODY2D_DOF];

    CHECK_NULL(body, "mbd rigid compare body");
    CHECK_NULL(theta, "mbd rigid compare theta");
    CHECK_NULL(omega, "mbd rigid compare omega");
    CHECK_ERROR(mbd_body2d_get_current_pose(body, origin, theta));
    CHECK_ERROR(mbd_body2d_get_generalized_state(body, q, v, a));
    if (!isfinite(origin[0]) || !isfinite(origin[1]) || !isfinite(*theta)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD rigid compare pose must be finite");
    }
    if (!isfinite(q[0]) || !isfinite(q[1]) || !isfinite(q[2]) ||
        !isfinite(v[0]) || !isfinite(v[1]) || !isfinite(v[2]) ||
        !isfinite(a[0]) || !isfinite(a[1]) || !isfinite(a[2])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD rigid compare state must be finite");
    }
    *omega = v[2];

    return FEM_SUCCESS;
}

static int mbd_output2d_is_ignorable_line(const char *line)
{
    const unsigned char *cursor = (const unsigned char *)line;

    if (!cursor) {
        return 1;
    }
    while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n') {
        ++cursor;
    }
    return *cursor == '\0' || *cursor == '#';
}

static int mbd_output2d_node_set_contains(const int *node_ids,
                                          int count,
                                          int node_id)
{
    int i;

    for (i = 0; i < count; ++i) {
        if (node_ids[i] == node_id) {
            return 1;
        }
    }
    return 0;
}

static const coupled_case2d_flex_body_t *mbd_output2d_find_case_body_geometry(int body_id)
{
    const coupled_case2d_t *case_data = coupled_case2d_view();
    int i;

    if (!case_data ||
        case_data->num_flex_bodies <= 0 ||
        !case_data->flex_bodies) {
        return NULL;
    }

    for (i = 0; i < case_data->num_flex_bodies; ++i) {
        if (case_data->flex_bodies[i].is_defined &&
            case_data->flex_bodies[i].body_id == body_id) {
            return &case_data->flex_bodies[i];
        }
    }

    return NULL;
}

static fem_error_t mbd_output2d_load_case_tip_vector(
    const coupled_case2d_flex_body_t *case_body,
    double tip_vector[2])
{
    FILE *fp = NULL;
    char line[512];
    int noncomment_index = 0;
    int num_nodes = 0;
    int node_lines_read = 0;
    int root_hits = 0;
    int tip_hits = 0;
    double root_sum[2] = {0.0, 0.0};
    double tip_sum[2] = {0.0, 0.0};

    CHECK_NULL(case_body, "coupled flex body geometry");
    CHECK_NULL(tip_vector, "coupled tip vector");

    fp = fopen(case_body->fem_input_path, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND,
                         "Cannot open coupled flex geometry file: %s",
                         case_body->fem_input_path);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        int node_id = -1;
        double x = 0.0;
        double y = 0.0;

        if (mbd_output2d_is_ignorable_line(line)) {
            continue;
        }
        ++noncomment_index;
        if (noncomment_index == 2) {
            if (sscanf(line, "%d", &num_nodes) != 1 || num_nodes <= 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Invalid node count line in coupled flex geometry file: %s",
                                 case_body->fem_input_path);
            }
            continue;
        }
        if (noncomment_index <= 2) {
            continue;
        }
        if (node_lines_read >= num_nodes) {
            break;
        }
        if (sscanf(line, "%d %lf %lf", &node_id, &x, &y) != 3) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid node line in coupled flex geometry file: %s",
                             case_body->fem_input_path);
        }
        if (mbd_output2d_node_set_contains(case_body->root_node_ids,
                                           case_body->num_root_nodes,
                                           node_id)) {
            root_sum[0] += x;
            root_sum[1] += y;
            ++root_hits;
        }
        if (mbd_output2d_node_set_contains(case_body->tip_node_ids,
                                           case_body->num_tip_nodes,
                                           node_id)) {
            tip_sum[0] += x;
            tip_sum[1] += y;
            ++tip_hits;
        }
        ++node_lines_read;
    }
    fclose(fp);

    if (root_hits != case_body->num_root_nodes ||
        tip_hits != case_body->num_tip_nodes) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled flex geometry sets do not match node coordinates for body id %d",
                         case_body->body_id);
    }

    root_sum[0] /= (double)root_hits;
    root_sum[1] /= (double)root_hits;
    tip_sum[0] /= (double)tip_hits;
    tip_sum[1] /= (double)tip_hits;
    tip_vector[0] = tip_sum[0] - root_sum[0];
    tip_vector[1] = tip_sum[1] - root_sum[1];
    return FEM_SUCCESS;
}

static fem_error_t mbd_output2d_try_capture_tip_position(const mbd_body2d_t *body,
                                                         int body_id,
                                                         double *tip_x,
                                                         double *tip_y)
{
    const coupled_case2d_flex_body_t *case_body = NULL;
    double origin[2];
    double theta = 0.0;
    double tip_vector[2];
    double c_value = 0.0;
    double s_value = 0.0;

    CHECK_NULL(body, "mbd rigid compare body");
    CHECK_NULL(tip_x, "mbd rigid compare tip x");
    CHECK_NULL(tip_y, "mbd rigid compare tip y");

    *tip_x = NAN;
    *tip_y = NAN;
    case_body = mbd_output2d_find_case_body_geometry(body_id);
    if (!case_body) {
        return FEM_SUCCESS;
    }

    CHECK_ERROR(mbd_output2d_load_case_tip_vector(case_body, tip_vector));
    CHECK_ERROR(mbd_body2d_get_current_pose(body, origin, &theta));
    c_value = cos(theta);
    s_value = sin(theta);
    *tip_x = origin[0] + c_value * tip_vector[0] - s_value * tip_vector[1];
    *tip_y = origin[1] + s_value * tip_vector[0] + c_value * tip_vector[1];
    return FEM_SUCCESS;
}

fem_error_t mbd_output2d_write_rigid_compare_row(FILE *out,
                                                 double time,
                                                 const mbd_system2d_t *system,
                                                 int body1_id,
                                                 int body2_id)
{
    const mbd_body2d_t *body1 = NULL;
    const mbd_body2d_t *body2 = NULL;
    double theta1 = 0.0;
    double theta2 = 0.0;
    double omega1 = 0.0;
    double omega2 = 0.0;
    double constraint_residual = 0.0;
    int num_equations = 0;
    double tip1_x = NAN;
    double tip1_y = NAN;
    double tip2_x = NAN;
    double tip2_y = NAN;
    const double root_reaction_x = NAN;
    const double root_reaction_y = NAN;
    const double full_reassembly_count_link1 = 0.0;
    const double full_reassembly_count_link2 = 0.0;

    CHECK_NULL(out, "mbd rigid compare output stream");
    CHECK_NULL(system, "mbd rigid compare system");

    if (!isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD rigid compare time must be finite and non-negative");
    }

    CHECK_ERROR(mbd_system2d_get_body_const(system, body1_id, &body1));
    CHECK_ERROR(mbd_system2d_get_body_const(system, body2_id, &body2));
    CHECK_ERROR(mbd_output2d_capture_body_compare_fields(body1, &theta1, &omega1));
    CHECK_ERROR(mbd_output2d_capture_body_compare_fields(body2, &theta2, &omega2));
    CHECK_ERROR(mbd_output2d_try_capture_tip_position(body1, body1_id, &tip1_x, &tip1_y));
    CHECK_ERROR(mbd_output2d_try_capture_tip_position(body2, body2_id, &tip2_x, &tip2_y));
    CHECK_ERROR(mbd_system2d_compute_constraint_residual_l2(system,
                                                            &constraint_residual,
                                                            &num_equations));
    (void)num_equations;
    if (!isfinite(constraint_residual)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD rigid compare constraint residual must be finite");
    }

    if (fprintf(out,
                "%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e,%.16e\n",
                time,
                theta1,
                theta2,
                omega1,
                omega2,
                tip1_x,
                tip1_y,
                tip2_x,
                tip2_y,
                root_reaction_x,
                root_reaction_y,
                constraint_residual,
                full_reassembly_count_link1,
                full_reassembly_count_link2) < 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "Failed to write rigid compare CSV row");
    }

    return FEM_SUCCESS;
}
