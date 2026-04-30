#ifndef FEM4C_COUPLED_FLEX_REACTION2D_H
#define FEM4C_COUPLED_FLEX_REACTION2D_H

#include "flex_nodeset.h"

fem_error_t flex_reaction2d_from_node_set(const node_set_t *set,
                                          const fem_model_t *model,
                                          const double *nodal_reaction,
                                          int reaction_size,
                                          double generalized_force[3]);
void flex_reaction2d_sum_interface_forces(const double root_interface_force[3],
                                          const double tip_interface_force[3],
                                          double total_interface_force[3]);

void flex_reaction2d_to_root_body_force(const double interface_force[3],
                                        double body_force[3]);
void flex_reaction2d_to_tip_body_force(const double interface_force[3],
                                       double body_force[3]);

#endif /* FEM4C_COUPLED_FLEX_REACTION2D_H */
