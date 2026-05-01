#ifndef FEM4C_COUPLED_STEP_ONEWAY2D_H
#define FEM4C_COUPLED_STEP_ONEWAY2D_H

#include "coupled_run2d.h"

fem_error_t coupled_step_oneway2d_run(coupled_run2d_t *run,
                                      int step_index,
                                      coupled_step_history2d_t *history);

#endif /* FEM4C_COUPLED_STEP_ONEWAY2D_H */
