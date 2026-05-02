#include "coupled_run2d.h"

#include "../common/error.h"
#include "../io/input.h"
#include "flex_solver2d.h"

/* Owns flex FEM model loading and preparation for coupled runs.
 * Inputs are the coupled run state and configured flex input paths; outputs are
 * prepared flex models stored in the run object. Side effects include input
 * loading, model reset/free/clone/prepare, reserve/storage updates,
 * loaded-count updates, and related error reporting. Validation, route
 * dispatch, solver steps, history buffers, snapshots, and writers remain
 * outside this file. This extraction improves readability and maintainability
 * without changing behavior.
 */

static fem_error_t coupled_run2d_load_single_flex_model(fem_model_t *model,
                                                        const char *input_filename)
{
    CHECK_NULL(model, "flex model");
    CHECK_NULL(input_filename, "flex input filename");

    CHECK_ERROR(input_read_data(input_filename));
    CHECK_ERROR(fem_model_clone_from_globals(model));
    CHECK_ERROR(flex_solver2d_prepare_model(model));
    return FEM_SUCCESS;
}

fem_error_t coupled_run2d_load_flex_models(coupled_run2d_t *run)
{
    int i;
    int loaded = 0;

    CHECK_NULL(run, "coupled_run2d");
    CHECK_ERROR(coupled_run2d_reserve_flex_model_storage(run,
                                                         run->case_data.num_flex_bodies));

    for (i = 0; i < run->case_data.num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &run->case_data.flex_bodies[i];

        fem_model_free(&run->flex_models[i]);
        fem_model_zero(&run->flex_models[i]);
        CHECK_ERROR(coupled_run2d_load_single_flex_model(&run->flex_models[i],
                                                         body->fem_input_path));
        ++loaded;
    }

    run->flex_model_count = loaded;
    return FEM_SUCCESS;
}
