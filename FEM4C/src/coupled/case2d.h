#ifndef FEM4C_COUPLED_CASE2D_H
#define FEM4C_COUPLED_CASE2D_H

#include "flex_nodeset.h"

#include "../common/constants.h"
#include "../common/types.h"

#define COUPLED_CASE2D_MAX_FLEX_BODIES 2

typedef struct {
    int is_defined;
    int body_id;
    char fem_input_path[MAX_FILENAME_LEN];
    int *root_node_ids;
    int num_root_nodes;
    int *tip_node_ids;
    int num_tip_nodes;
} coupled_case2d_flex_body_t;

typedef struct {
    coupled_case2d_flex_body_t *flex_bodies;
    int num_flex_bodies;
    int flex_body_capacity;
} coupled_case2d_t;

void coupled_case2d_zero(coupled_case2d_t *case_data);
void coupled_case2d_free(coupled_case2d_t *case_data);
fem_error_t coupled_case2d_reserve_flex_bodies(coupled_case2d_t *case_data,
                                               int required_capacity);

coupled_case2d_t *coupled_case2d_current(void);
const coupled_case2d_t *coupled_case2d_view(void);
void coupled_case2d_reset_current(void);
fem_error_t coupled_case2d_clone(coupled_case2d_t *dst,
                                 const coupled_case2d_t *src);

fem_error_t coupled_case2d_add_flex_body(coupled_case2d_t *case_data,
                                         int body_id,
                                         const char *fem_input_path);
fem_error_t coupled_case2d_set_root_set(coupled_case2d_t *case_data,
                                        int body_id,
                                        const int *node_ids,
                                        int count);
fem_error_t coupled_case2d_set_tip_set(coupled_case2d_t *case_data,
                                       int body_id,
                                       const int *node_ids,
                                       int count);
fem_error_t coupled_case2d_build_root_node_set(
    const coupled_case2d_flex_body_t *body,
    node_set_t *set);
fem_error_t coupled_case2d_build_tip_node_set(
    const coupled_case2d_flex_body_t *body,
    node_set_t *set);

#endif /* FEM4C_COUPLED_CASE2D_H */
