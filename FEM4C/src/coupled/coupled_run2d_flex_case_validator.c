#include "coupled_run2d.h"

#include "../common/error.h"

/*
 * Owns coupled-run flex case validation. Inputs are the coupled run state,
 * case data, configured flex entries, and MBD system needed for body lookup.
 * Output is success/error status; side effects are limited to validation
 * error reporting. Flex model loading, route dispatch, solver steps, writers,
 * history buffers, snapshots, and flex model preparation remain outside this
 * file. This extraction is for readability/maintainability, not behavior
 * change.
 */

fem_error_t coupled_run2d_validate_flex_case(const coupled_run2d_t *run)
{
    int i;

    CHECK_NULL(run, "coupled_run2d");

    if (run->case_data.num_flex_bodies <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled run requires at least one COUPLED_FLEX_BODY");
    }

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];
        int body_index = -1;

        if (body->fem_input_path[0] == '\0') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires fem input path for body_id %d",
                             body->body_id);
        }
        if (mbd_system2d_find_body_index_by_id(&run->mbd_system,
                                               body->body_id,
                                               &body_index) != FEM_SUCCESS) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "COUPLED_FLEX_BODY body_id %d is not present in MBD system",
                             body->body_id);
        }
        if (body->num_root_nodes <= 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires COUPLED_FLEX_ROOT_SET for body_id %d",
                             body->body_id);
        }
        if (body->num_tip_nodes <= 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Coupled run requires COUPLED_FLEX_TIP_SET for body_id %d",
                             body->body_id);
        }
    }

    return FEM_SUCCESS;
}
