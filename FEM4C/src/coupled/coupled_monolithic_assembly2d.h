#ifndef FEM4C_COUPLED_MONOLITHIC_ASSEMBLY2D_H
#define FEM4C_COUPLED_MONOLITHIC_ASSEMBLY2D_H

#include "coupled_monolithic_state2d.h"

fem_error_t coupled_monolithic_assembly2d_prepare(coupled_monolithic_state2d_t *state,
                                                  const coupled_run2d_t *run);
fem_error_t coupled_monolithic_assembly2d_assemble(
    coupled_monolithic_state2d_t *state,
    const coupled_run2d_t *run,
    int iteration_index);
fem_error_t coupled_monolithic_assembly2d_solve(
    coupled_monolithic_state2d_t *state);
void coupled_monolithic_assembly2d_apply_update(
    coupled_monolithic_state2d_t *state);

#endif /* FEM4C_COUPLED_MONOLITHIC_ASSEMBLY2D_H */
