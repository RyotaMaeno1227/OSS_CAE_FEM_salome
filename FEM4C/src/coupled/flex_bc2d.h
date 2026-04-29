#ifndef FEM4C_COUPLED_FLEX_BC2D_H
#define FEM4C_COUPLED_FLEX_BC2D_H

#include "flex_nodeset.h"

typedef struct {
    int node_id;
    int dof;
    double value;
} flex_bc2d_entry_t;

typedef struct flex_bc2d_list {
    flex_bc2d_entry_t *entries;
    int count;
    int capacity;
} flex_bc2d_list_t;

void flex_bc2d_list_zero(flex_bc2d_list_t *list);
void flex_bc2d_list_clear(flex_bc2d_list_t *list);
void flex_bc2d_list_free(flex_bc2d_list_t *list);
fem_error_t flex_bc2d_list_reserve(flex_bc2d_list_t *list, int capacity);
fem_error_t flex_bc2d_list_append(flex_bc2d_list_t *list,
                                  int node_id,
                                  int dof,
                                  double value);

fem_error_t flex_bc2d_interpolate_rigid_point(const double marker_disp[3],
                                              double x_local,
                                              double y_local,
                                              double node_disp[2]);

fem_error_t flex_bc2d_interpolate_node_set(const node_set_t *set,
                                           const double marker_disp[3],
                                           double *node_disp,
                                           int node_disp_size);
fem_error_t flex_bc2d_build_node_set_entries(const node_set_t *set,
                                             const double marker_disp[3],
                                             flex_bc2d_list_t *bc_list);

#endif /* FEM4C_COUPLED_FLEX_BC2D_H */
