#include "coupled_run2d.h"

#include "../common/error.h"

#include <stdio.h>

/*
 * Owns coupled-run flex case validation and legacy no-flex fallback
 * validation/reporting. Inputs are the coupled run configuration, case data,
 * configured flex entries, and route state needed for validation. Outputs are
 * success/error status and validation/reporting messages; side effects are
 * limited to validation/reporting output and error reporting. Flex model
 * loading, route dispatch, solver steps, writers, snapshots, and history
 * capture remain outside this file. This extraction is for readability and
 * maintainability, not behavior change.
 */

fem_error_t coupled_legacy_no_flex_fallback_error(const coupled_run2d_t *run)
{
    const coupled_integrator_t integrator =
        run ? run->time.integrator : COUPLED_INTEGRATOR_NEWMARK_BETA;

    CHECK_NULL(run, "coupled_run2d");

    printf("Coupled mode contract snapshot (stub):\n");
    printf("  fem: nodes=%d elements=%d materials=%d analysis_ptr=%p\n",
           run->master_fem.num_nodes,
           run->master_fem.num_elements,
           run->master_fem.num_materials,
           (const void *)run->master_fem.analysis);
    printf("  mbd: bodies=%d constraints=%d bodies_ptr=%p constraints_ptr=%p\n",
           run->mbd_system.num_bodies,
           run->mbd_system.num_constraints,
           (const void *)run->mbd_system.bodies,
           (const void *)run->mbd_system.constraints);
    printf("  time: dt=%.6e steps=%d max_iter=%d residual_tol=%.6e\n",
           run->time.dt,
           run->time.num_steps,
           run->time.max_coupling_iterations,
           run->time.residual_tolerance);
    printf("  legacy_stub_role=non_default_no_flex_fallback\n");
    printf("  default_path_requires_flex_bodies=1\n");
    printf("  integrator=%s\n", coupled_integrator_to_string(integrator));
    printf("  coupling_scheme=%s\n", coupled_scheme_to_string(run->time.scheme));
    if (run->time.scheme_is_legacy_default) {
        printf("  coupling_scheme_source=legacy_default via integrator=%s\n",
               coupled_integrator_to_string(integrator));
    } else {
        printf("  coupling_scheme_source=explicit_request\n");
    }
    printf("  integrator_params: newmark_beta=%.6e newmark_gamma=%.6e hht_alpha=%.6e marker_relaxation=%.6e\n",
           run->time.newmark_beta,
           run->time.newmark_gamma,
           run->time.hht_alpha,
           run->time.marker_relaxation);

    return error_set(FEM_ERROR_INVALID_INPUT,
                     "Coupled FEM+MBD mode is not wired yet; legacy no-flex fallback is kept only for non-default stub checks");
}

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
