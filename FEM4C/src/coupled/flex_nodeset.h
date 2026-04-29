#ifndef FEM4C_COUPLED_FLEX_NODESET_H
#define FEM4C_COUPLED_FLEX_NODESET_H

#include "fem_model_copy.h"

typedef struct {
    int *node_ids;
    double *local_coords;    /* Packed as [x0, y0, x1, y1, ...] */
    int count;
} node_set_t;

void node_set_zero(node_set_t *set);
fem_error_t node_set_clone(node_set_t *dst, const node_set_t *src);
void node_set_free(node_set_t *set);

int node_set_contains(const node_set_t *set, int node_id);
fem_error_t node_set_center(const node_set_t *set,
                            const fem_model_t *model,
                            double center[2]);
fem_error_t node_set_local_coordinates(node_set_t *set,
                                       const fem_model_t *model);

#endif /* FEM4C_COUPLED_FLEX_NODESET_H */
