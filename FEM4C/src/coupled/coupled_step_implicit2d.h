#ifndef FEM4C_COUPLED_STEP_IMPLICIT2D_H
#define FEM4C_COUPLED_STEP_IMPLICIT2D_H

#include "coupled_run2d.h"

fem_error_t coupled_step_implicit2d_run(coupled_run2d_t *run,
                                        int step_index,
                                        coupled_step_history2d_t *history);

#endif /* FEM4C_COUPLED_STEP_IMPLICIT2D_H */
