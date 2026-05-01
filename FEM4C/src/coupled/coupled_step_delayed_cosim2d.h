#ifndef FEM4C_COUPLED_STEP_DELAYED_COSIM2D_H
#define FEM4C_COUPLED_STEP_DELAYED_COSIM2D_H

#include "coupled_run2d.h"

/*
 * year1 experimental delayed co-simulation comparison scaffold.
 *
 * NOTE:
 * - delayed_cosim_v1_5 stays isolated from oneway_snapshot and monolithic_strong_v1.
 * - DC_020 wires a minimal 1-link lag-1 sample-hold skeleton without changing
 *   the official one-way mainline or the monolithic comparison lane.
 */

fem_error_t coupled_step_delayed_cosim2d_run(coupled_run2d_t *run,
                                             int step_index,
                                             coupled_step_history2d_t *history);

#endif /* FEM4C_COUPLED_STEP_DELAYED_COSIM2D_H */
