#include "coupled_run2d.h"

#include "../common/error.h"
#include "../common/globals.h"
#include "../io/input.h"

/*
 * Owns coupled-run master FEM/MBD/case input loading and setup. Inputs are
 * the coupled run state and configured master input path/settings; outputs are
 * initialized master FEM state, MBD system state, and coupled case data stored
 * in the run. Side effects include input loading, global FEM state assignment,
 * MBD system loading, case cloning/setup, reserve/setup calls, and error
 * reporting. Route dispatch, solvers, flex model loading, validation, writers,
 * snapshots, and history capture remain outside this file. This extraction is
 * for readability and maintainability, not behavior change.
 */

fem_error_t coupled_run2d_load_master_input(coupled_run2d_t *run,
                                            const char *input_filename)
{
    CHECK_NULL(run, "coupled_run2d");
    CHECK_NULL(input_filename, "coupled input filename");

    CHECK_ERROR(input_read_data(input_filename));

    run->master_fem.analysis = &g_analysis;
    run->master_fem.num_nodes = g_num_nodes;
    run->master_fem.num_elements = g_num_elements;
    run->master_fem.num_materials = g_num_materials;

    CHECK_ERROR(mbd_system2d_load(&run->mbd_system, input_filename));
    CHECK_ERROR(coupled_case2d_clone(&run->case_data, coupled_case2d_view()));
    if (run->case_data.num_flex_bodies > 0) {
        CHECK_ERROR(coupled_run2d_reserve_flex_model_storage(
            run,
            run->case_data.num_flex_bodies));
    }
    return FEM_SUCCESS;
}
