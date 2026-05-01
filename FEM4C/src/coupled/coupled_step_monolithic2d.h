#ifndef FEM4C_COUPLED_STEP_MONOLITHIC2D_H
#define FEM4C_COUPLED_STEP_MONOLITHIC2D_H

#include "coupled_run2d.h"

/*
 * year1 experimental comparison lane scaffold.
 *
 * NOTE:
 * - monolithic_strong_v1 has a dedicated runtime surface and 1-link Newton loop.
 * - current fixed_point_strong must not be treated as monolithic.
 * - this entrypoint exists so MS_010/MS_020/MS_030 can land on a dedicated path
 *   without reusing the legacy fixed-point files.
 */

fem_error_t coupled_step_monolithic2d_run(coupled_run2d_t *run,
                                          int step_index,
                                          coupled_step_history2d_t *history);

#endif /* FEM4C_COUPLED_STEP_MONOLITHIC2D_H */
