#include "flex_nodeset.h"

#include "../common/error.h"

#include <stdlib.h>
#include <string.h>

static fem_error_t node_set_clone_array(void **dst,
                                        const void *src,
                                        size_t item_size,
                                        int count,
                                        const char *label)
{
    *dst = NULL;

    if (count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s count must be non-negative",
                         label);
    }
    if (count == 0) {
        return FEM_SUCCESS;
    }
    if (!src) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s source array is NULL",
                         label);
    }

    *dst = calloc((size_t)count, item_size);
    CHECK_NULL(*dst, label);
    memcpy(*dst, src, (size_t)count * item_size);
    return FEM_SUCCESS;
}

static int node_set_find_node_index(const fem_model_t *model, int node_id)
{
    int i;

    if (!model) {
        return -1;
    }
    if (node_id >= 0 &&
        node_id < model->node_id_capacity &&
        model->node_id_to_index &&
        model->node_id_to_index[node_id] >= 0 &&
        model->node_id_to_index[node_id] < model->num_nodes) {
        return model->node_id_to_index[node_id];
    }
    if (!model->node_ids) {
        return -1;
    }

    for (i = 0; i < model->num_nodes; ++i) {
        if (model->node_ids[i] == node_id) {
            return i;
        }
    }

    return -1;
}

static fem_error_t node_set_get_node_xy(const node_set_t *set,
                                        const fem_model_t *model,
                                        int entry_index,
                                        double xy[2])
{
    int node_index;

    if (!set || !model || !xy) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "node_set_get_node_xy inputs must not be NULL");
    }
    if (entry_index < 0 || entry_index >= set->count) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set entry index %d is out of range",
                         entry_index);
    }
    if (!set->node_ids) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set ids are not initialized");
    }
    if (!model->node_coords) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Model node coordinates are not initialized");
    }

    node_index = node_set_find_node_index(model, set->node_ids[entry_index]);
    if (node_index < 0) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Node id %d is not present in model",
                         set->node_ids[entry_index]);
    }

    xy[0] = model->node_coords[node_index][0];
    xy[1] = model->node_coords[node_index][1];
    return FEM_SUCCESS;
}

void node_set_zero(node_set_t *set)
{
    if (!set) {
        return;
    }
    memset(set, 0, sizeof(*set));
}

fem_error_t node_set_clone(node_set_t *dst, const node_set_t *src)
{
    CHECK_NULL(dst, "Node set destination");

    if (!src) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set source is NULL");
    }

    node_set_zero(dst);
    dst->count = src->count;

    if (node_set_clone_array((void **)&dst->node_ids,
                             src->node_ids,
                             sizeof(*src->node_ids),
                             src->count,
                             "Node set ids") != FEM_SUCCESS) {
        node_set_free(dst);
        return error_get_last();
    }

    if (src->count > 0 && src->local_coords) {
        if (node_set_clone_array((void **)&dst->local_coords,
                                 src->local_coords,
                                 sizeof(*src->local_coords),
                                 src->count * 2,
                                 "Node set local coordinates") != FEM_SUCCESS) {
            node_set_free(dst);
            return error_get_last();
        }
    }

    return FEM_SUCCESS;
}

void node_set_free(node_set_t *set)
{
    if (!set) {
        return;
    }

    free(set->node_ids);
    free(set->local_coords);
    node_set_zero(set);
}

int node_set_contains(const node_set_t *set, int node_id)
{
    int i;

    if (!set || !set->node_ids || node_id < 0) {
        return 0;
    }

    for (i = 0; i < set->count; ++i) {
        if (set->node_ids[i] == node_id) {
            return 1;
        }
    }

    return 0;
}

fem_error_t node_set_center(const node_set_t *set,
                            const fem_model_t *model,
                            double center[2])
{
    int i;

    if (!set || !model || !center) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set center inputs must not be NULL");
    }
    if (set->count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set center requires at least one node");
    }

    center[0] = 0.0;
    center[1] = 0.0;
    for (i = 0; i < set->count; ++i) {
        double xy[2] = {0.0, 0.0};
        fem_error_t err = node_set_get_node_xy(set, model, i, xy);
        if (err != FEM_SUCCESS) {
            return err;
        }
        center[0] += xy[0];
        center[1] += xy[1];
    }

    center[0] /= (double)set->count;
    center[1] /= (double)set->count;
    return FEM_SUCCESS;
}

fem_error_t node_set_local_coordinates(node_set_t *set,
                                       const fem_model_t *model)
{
    int i;
    double center[2] = {0.0, 0.0};
    fem_error_t err;

    if (!set || !model) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set local coordinate inputs must not be NULL");
    }
    if (set->count < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set count must be non-negative");
    }
    if (set->count == 0) {
        return FEM_SUCCESS;
    }
    if (!set->node_ids) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Node set ids are required");
    }
    if (set->local_coords) {
        return FEM_SUCCESS;
    }

    err = node_set_center(set, model, center);
    if (err != FEM_SUCCESS) {
        return err;
    }

    set->local_coords = calloc((size_t)set->count * 2,
                               sizeof(*set->local_coords));
    CHECK_NULL(set->local_coords, "Node set local coordinates");

    for (i = 0; i < set->count; ++i) {
        double xy[2] = {0.0, 0.0};

        err = node_set_get_node_xy(set, model, i, xy);
        if (err != FEM_SUCCESS) {
            return err;
        }

        set->local_coords[2 * i] = xy[0] - center[0];
        set->local_coords[2 * i + 1] = xy[1] - center[1];
    }

    return FEM_SUCCESS;
}
