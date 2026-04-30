#include "flex_bc2d.h"

#include "../common/error.h"
#include <stdlib.h>

static int flex_bc2d_list_find_entry_index(const flex_bc2d_list_t *list,
                                           int node_id,
                                           int dof)
{
    int i;

    if (!list) {
        return -1;
    }

    for (i = 0; i < list->count; ++i) {
        if (list->entries[i].node_id == node_id &&
            list->entries[i].dof == dof) {
            return i;
        }
    }

    return -1;
}

void flex_bc2d_list_zero(flex_bc2d_list_t *list)
{
    if (!list) {
        return;
    }
    list->entries = NULL;
    list->count = 0;
    list->capacity = 0;
}

void flex_bc2d_list_clear(flex_bc2d_list_t *list)
{
    if (!list) {
        return;
    }
    list->count = 0;
}

void flex_bc2d_list_free(flex_bc2d_list_t *list)
{
    if (!list) {
        return;
    }

    free(list->entries);
    flex_bc2d_list_zero(list);
}

fem_error_t flex_bc2d_list_reserve(flex_bc2d_list_t *list, int capacity)
{
    flex_bc2d_entry_t *entries = NULL;

    if (!list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "BC list reserve requires a destination list");
    }
    if (capacity < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "BC list capacity must be non-negative");
    }
    if (capacity <= list->capacity) {
        return FEM_SUCCESS;
    }

    entries = realloc(list->entries, (size_t)capacity * sizeof(*entries));
    if (!entries) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to grow BC list to %d entries",
                         capacity);
    }

    list->entries = entries;
    list->capacity = capacity;
    return FEM_SUCCESS;
}

fem_error_t flex_bc2d_list_append(flex_bc2d_list_t *list,
                                  int node_id,
                                  int dof,
                                  double value)
{
    int new_capacity;
    int existing_index;

    if (!list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "BC list append requires a destination list");
    }
    if (node_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "BC node id must be non-negative");
    }
    if (dof < 0 || dof > 1) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "BC dof must be 0 or 1");
    }

    existing_index = flex_bc2d_list_find_entry_index(list, node_id, dof);
    if (existing_index >= 0) {
        list->entries[existing_index].value = value;
        return FEM_SUCCESS;
    }

    if (list->count >= list->capacity) {
        new_capacity = (list->capacity > 0) ? list->capacity * 2 : 8;
        if (new_capacity < list->count + 1) {
            new_capacity = list->count + 1;
        }
        if (flex_bc2d_list_reserve(list, new_capacity) != FEM_SUCCESS) {
            return error_get_last();
        }
    }

    list->entries[list->count].node_id = node_id;
    list->entries[list->count].dof = dof;
    list->entries[list->count].value = value;
    list->count++;
    return FEM_SUCCESS;
}

fem_error_t flex_bc2d_interpolate_rigid_point(const double marker_disp[3],
                                              double x_local,
                                              double y_local,
                                              double node_disp[2])
{
    if (!marker_disp || !node_disp) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Rigid interpolation inputs must not be NULL");
    }

    node_disp[0] = marker_disp[0] - marker_disp[2] * y_local;
    node_disp[1] = marker_disp[1] + marker_disp[2] * x_local;
    return FEM_SUCCESS;
}

fem_error_t flex_bc2d_interpolate_node_set(const node_set_t *set,
                                           const double marker_disp[3],
                                           double *node_disp,
                                           int node_disp_size)
{
    int i;
    int required_size;

    if (!set || !marker_disp || !node_disp) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node-set interpolation inputs must not be NULL");
    }
    if (set->count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set count must be non-negative");
    }

    required_size = set->count * 2;
    if (node_disp_size < required_size) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node-set interpolation output is too small");
    }
    if (set->count > 0 && !set->local_coords) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set local coordinates are required");
    }

    for (i = 0; i < set->count; ++i) {
        double x_local = set->local_coords[2 * i];
        double y_local = set->local_coords[2 * i + 1];
        fem_error_t err = flex_bc2d_interpolate_rigid_point(marker_disp,
                                                            x_local,
                                                            y_local,
                                                            &node_disp[2 * i]);
        if (err != FEM_SUCCESS) {
            return err;
        }
    }

    return FEM_SUCCESS;
}

fem_error_t flex_bc2d_build_node_set_entries(const node_set_t *set,
                                             const double marker_disp[3],
                                             flex_bc2d_list_t *bc_list)
{
    int i;
    double node_disp[2];

    if (!set || !marker_disp || !bc_list) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node-set BC build inputs must not be NULL");
    }
    if (set->count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set count must be non-negative");
    }
    if (set->count > 0 && (!set->local_coords || !set->node_ids)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set ids and local coordinates are required");
    }

    for (i = 0; i < set->count; ++i) {
        fem_error_t err = flex_bc2d_interpolate_rigid_point(marker_disp,
                                                            set->local_coords[2 * i],
                                                            set->local_coords[2 * i + 1],
                                                            node_disp);
        if (err != FEM_SUCCESS) {
            return err;
        }

        err = flex_bc2d_list_append(bc_list, set->node_ids[i], 0, node_disp[0]);
        if (err != FEM_SUCCESS) {
            return err;
        }
        err = flex_bc2d_list_append(bc_list, set->node_ids[i], 1, node_disp[1]);
        if (err != FEM_SUCCESS) {
            return err;
        }
    }

    return FEM_SUCCESS;
}
