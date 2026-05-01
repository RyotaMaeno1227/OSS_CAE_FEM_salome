#include "coupled_run2d.h"

/*
 * Writer metadata helpers own stable CSV/header strings requested by coupled
 * run output code. They take only existing route/writer metadata inputs and
 * return static string constants; they do not allocate, mutate route state, or
 * write files. Route execution and actual file writing stay in coupled_run2d.c;
 * this split is for readability and maintainability, not behavior change.
 */
const char *coupled_run2d_artifact_metadata_columns_csv(void)
{
    return COUPLED_RUN2D_ARTIFACT_METADATA_COLUMNS_CSV;
}

const char *coupled_run2d_interface_centers_header_csv(void)
{
    return COUPLED_RUN2D_ARTIFACT_METADATA_COLUMNS_CSV
           ",step_index,body_id,time,marker_x,marker_y,marker_theta,"
           "root_center_local_x,root_center_local_y,"
           "tip_center_local_x,tip_center_local_y,"
           "root_center_world_x,root_center_world_y,"
           "tip_center_world_x,tip_center_world_y\n";
}

const char *coupled_run2d_reaction_map_header_csv(void)
{
    return COUPLED_RUN2D_ARTIFACT_METADATA_COLUMNS_CSV
           ",step_index,body_id,time,"
           "root_reaction_fx,root_reaction_fy,root_reaction_mz,"
           "tip_reaction_fx,tip_reaction_fy,tip_reaction_mz,"
           "root_body_force_fx,root_body_force_fy,root_body_force_mz,"
           "tip_body_force_fx,tip_body_force_fy,tip_body_force_mz,"
           "total_body_force_fx,total_body_force_fy,total_body_force_mz\n";
}

const char *coupled_run2d_observation_points_header_csv(void)
{
    return COUPLED_RUN2D_ARTIFACT_METADATA_COLUMNS_CSV
           ",step_index,body_id,time,observation_label,"
           "observation_ref_local_x,observation_ref_local_y,"
           "observation_local_x,observation_local_y,"
           "observation_world_x,observation_world_y,"
           "observation_disp_local_x,observation_disp_local_y,"
           "observation_disp_world_x,observation_disp_world_y\n";
}

const char *coupled_run2d_step_flex_iteration_column_name(
    coupled_scheme_t scheme)
{
    if (scheme == COUPLED_SCHEME_ONEWAY_SNAPSHOT) {
        return "snapshot_iteration_index";
    }
    if (scheme == COUPLED_SCHEME_DELAYED_COSIM_V1_5) {
        return "communication_iteration_index";
    }
    return "coupling_iteration_index";
}
