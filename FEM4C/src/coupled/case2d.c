#include "case2d.h"
#include "../common/error.h"

#include <stdlib.h>
#include <string.h>

static coupled_case2d_t g_coupled_case2d_current;

static coupled_case2d_flex_body_t *coupled_case2d_find_body(coupled_case2d_t *case_data,
                                                            int body_id);
static const coupled_case2d_flex_body_t *coupled_case2d_find_body_const(
    const coupled_case2d_t *case_data,
    int body_id);
static void coupled_case2d_flex_body_zero(coupled_case2d_flex_body_t *body);
static void coupled_case2d_flex_body_free(coupled_case2d_flex_body_t *body);
static fem_error_t coupled_case2d_set_node_ids(int **dst,
                                               int *dst_count,
                                               const int *node_ids,
                                               int count,
                                               const char *label);
static fem_error_t coupled_case2d_build_node_set(const int *node_ids,
                                                 int count,
                                                 node_set_t *set,
                                                 const char *label);

void coupled_case2d_zero(coupled_case2d_t *case_data)
{
    if (!case_data) {
        return;
    }
    memset(case_data, 0, sizeof(*case_data));
}

void coupled_case2d_free(coupled_case2d_t *case_data)
{
    int i;

    if (!case_data) {
        return;
    }

    for (i = 0; i < case_data->flex_body_capacity; ++i) {
        coupled_case2d_flex_body_free(&case_data->flex_bodies[i]);
    }
    free(case_data->flex_bodies);
    coupled_case2d_zero(case_data);
}

fem_error_t coupled_case2d_reserve_flex_bodies(coupled_case2d_t *case_data,
                                               int required_capacity)
{
    coupled_case2d_flex_body_t *new_bodies = NULL;

    CHECK_NULL(case_data, "coupled_case2d");
    if (required_capacity <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled flex body capacity %d must be positive",
                         required_capacity);
    }
    if (case_data->flex_body_capacity >= required_capacity &&
        case_data->flex_bodies) {
        return FEM_SUCCESS;
    }

    new_bodies = calloc((size_t)required_capacity, sizeof(*new_bodies));
    CHECK_NULL(new_bodies, "coupled flex body storage");
    if (case_data->num_flex_bodies > 0 && case_data->flex_bodies) {
        memcpy(new_bodies,
               case_data->flex_bodies,
               (size_t)case_data->num_flex_bodies * sizeof(*new_bodies));
    }

    free(case_data->flex_bodies);
    case_data->flex_bodies = new_bodies;
    case_data->flex_body_capacity = required_capacity;
    return FEM_SUCCESS;
}

coupled_case2d_t *coupled_case2d_current(void)
{
    return &g_coupled_case2d_current;
}

const coupled_case2d_t *coupled_case2d_view(void)
{
    return &g_coupled_case2d_current;
}

void coupled_case2d_reset_current(void)
{
    coupled_case2d_free(&g_coupled_case2d_current);
}

fem_error_t coupled_case2d_clone(coupled_case2d_t *dst,
                                 const coupled_case2d_t *src)
{
    int i;

    CHECK_NULL(dst, "coupled_case2d destination");
    CHECK_NULL(src, "coupled_case2d source");

    coupled_case2d_free(dst);

    for (i = 0; i < src->num_flex_bodies; ++i) {
        const coupled_case2d_flex_body_t *body = &src->flex_bodies[i];

        CHECK_ERROR(coupled_case2d_add_flex_body(dst,
                                                 body->body_id,
                                                 body->fem_input_path));
        if (body->num_root_nodes > 0) {
            CHECK_ERROR(coupled_case2d_set_root_set(dst,
                                                    body->body_id,
                                                    body->root_node_ids,
                                                    body->num_root_nodes));
        }
        if (body->num_tip_nodes > 0) {
            CHECK_ERROR(coupled_case2d_set_tip_set(dst,
                                                   body->body_id,
                                                   body->tip_node_ids,
                                                   body->num_tip_nodes));
        }
    }

    return FEM_SUCCESS;
}

fem_error_t coupled_case2d_add_flex_body(coupled_case2d_t *case_data,
                                         int body_id,
                                         const char *fem_input_path)
{
    coupled_case2d_flex_body_t *body = NULL;

    CHECK_NULL(case_data, "coupled_case2d");
    CHECK_NULL(fem_input_path, "coupled fem_input_path");

    if (body_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled body id must be non-negative");
    }
    if (fem_input_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled fem_input_path must not be empty");
    }
    if (strlen(fem_input_path) >= MAX_FILENAME_LEN) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Coupled fem_input_path exceeds %d characters",
                         MAX_FILENAME_LEN - 1);
    }

    if (coupled_case2d_find_body_const(case_data, body_id)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Duplicate COUPLED_FLEX_BODY body_id %d",
                         body_id);
    }
    CHECK_ERROR(coupled_case2d_reserve_flex_bodies(case_data,
                                                   case_data->num_flex_bodies + 1));

    body = &case_data->flex_bodies[case_data->num_flex_bodies];
    coupled_case2d_flex_body_zero(body);
    body->is_defined = 1;
    body->body_id = body_id;
    strncpy(body->fem_input_path, fem_input_path, sizeof(body->fem_input_path) - 1);
    body->fem_input_path[sizeof(body->fem_input_path) - 1] = '\0';
    case_data->num_flex_bodies++;
    return FEM_SUCCESS;
}

fem_error_t coupled_case2d_set_root_set(coupled_case2d_t *case_data,
                                        int body_id,
                                        const int *node_ids,
                                        int count)
{
    coupled_case2d_flex_body_t *body = NULL;

    CHECK_NULL(case_data, "coupled_case2d");
    body = coupled_case2d_find_body(case_data, body_id);
    if (!body) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "COUPLED_FLEX_ROOT_SET references undefined body_id %d",
                         body_id);
    }
    if (body->num_root_nodes > 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Duplicate COUPLED_FLEX_ROOT_SET for body_id %d",
                         body_id);
    }

    return coupled_case2d_set_node_ids(&body->root_node_ids,
                                       &body->num_root_nodes,
                                       node_ids,
                                       count,
                                       "COUPLED_FLEX_ROOT_SET");
}

fem_error_t coupled_case2d_set_tip_set(coupled_case2d_t *case_data,
                                       int body_id,
                                       const int *node_ids,
                                       int count)
{
    coupled_case2d_flex_body_t *body = NULL;

    CHECK_NULL(case_data, "coupled_case2d");
    body = coupled_case2d_find_body(case_data, body_id);
    if (!body) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "COUPLED_FLEX_TIP_SET references undefined body_id %d",
                         body_id);
    }
    if (body->num_tip_nodes > 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Duplicate COUPLED_FLEX_TIP_SET for body_id %d",
                         body_id);
    }

    return coupled_case2d_set_node_ids(&body->tip_node_ids,
                                       &body->num_tip_nodes,
                                       node_ids,
                                       count,
                                       "COUPLED_FLEX_TIP_SET");
}

fem_error_t coupled_case2d_build_root_node_set(
    const coupled_case2d_flex_body_t *body,
    node_set_t *set)
{
    CHECK_NULL(body, "coupled root body");
    return coupled_case2d_build_node_set(body->root_node_ids,
                                         body->num_root_nodes,
                                         set,
                                         "COUPLED_FLEX_ROOT_SET");
}

fem_error_t coupled_case2d_build_tip_node_set(
    const coupled_case2d_flex_body_t *body,
    node_set_t *set)
{
    CHECK_NULL(body, "coupled tip body");
    return coupled_case2d_build_node_set(body->tip_node_ids,
                                         body->num_tip_nodes,
                                         set,
                                         "COUPLED_FLEX_TIP_SET");
}

static coupled_case2d_flex_body_t *coupled_case2d_find_body(coupled_case2d_t *case_data,
                                                            int body_id)
{
    int i;

    if (!case_data) {
        return NULL;
    }

    for (i = 0; i < case_data->num_flex_bodies; ++i) {
        if (case_data->flex_bodies[i].is_defined &&
            case_data->flex_bodies[i].body_id == body_id) {
            return &case_data->flex_bodies[i];
        }
    }

    return NULL;
}

static const coupled_case2d_flex_body_t *coupled_case2d_find_body_const(
    const coupled_case2d_t *case_data,
    int body_id)
{
    int i;

    if (!case_data) {
        return NULL;
    }

    for (i = 0; i < case_data->num_flex_bodies; ++i) {
        if (case_data->flex_bodies[i].is_defined &&
            case_data->flex_bodies[i].body_id == body_id) {
            return &case_data->flex_bodies[i];
        }
    }

    return NULL;
}

static void coupled_case2d_flex_body_zero(coupled_case2d_flex_body_t *body)
{
    if (!body) {
        return;
    }
    memset(body, 0, sizeof(*body));
}

static void coupled_case2d_flex_body_free(coupled_case2d_flex_body_t *body)
{
    if (!body) {
        return;
    }

    free(body->root_node_ids);
    free(body->tip_node_ids);
    coupled_case2d_flex_body_zero(body);
}

static fem_error_t coupled_case2d_set_node_ids(int **dst,
                                               int *dst_count,
                                               const int *node_ids,
                                               int count,
                                               const char *label)
{
    int *copy = NULL;
    int i;

    CHECK_NULL(dst, "coupled node-set storage");
    CHECK_NULL(dst_count, "coupled node-set count");
    CHECK_NULL(label, "coupled node-set label");

    if (count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s count must be positive",
                         label);
    }
    CHECK_NULL(node_ids, label);

    copy = calloc((size_t)count, sizeof(*copy));
    CHECK_NULL(copy, label);

    for (i = 0; i < count; ++i) {
        if (node_ids[i] <= 0) {
            free(copy);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "%s node id must be positive (index=%d value=%d)",
                             label,
                             i,
                             node_ids[i]);
        }
        copy[i] = node_ids[i];
    }

    free(*dst);
    *dst = copy;
    *dst_count = count;
    return FEM_SUCCESS;
}

static fem_error_t coupled_case2d_build_node_set(const int *node_ids,
                                                 int count,
                                                 node_set_t *set,
                                                 const char *label)
{
    CHECK_NULL(set, "coupled node_set output");

    node_set_zero(set);
    if (count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "%s count must be positive",
                         label ? label : "Coupled node set");
    }
    CHECK_NULL(node_ids, label ? label : "Coupled node set ids");

    set->node_ids = calloc((size_t)count, sizeof(*set->node_ids));
    CHECK_NULL(set->node_ids, label ? label : "Coupled node set ids");
    memcpy(set->node_ids, node_ids, (size_t)count * sizeof(*node_ids));
    set->count = count;
    return FEM_SUCCESS;
}
