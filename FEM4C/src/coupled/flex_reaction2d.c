#include "flex_reaction2d.h"

#include "../common/error.h"

static int flex_reaction2d_find_node_index(const fem_model_t *model, int node_id)
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

fem_error_t flex_reaction2d_from_node_set(const node_set_t *set,
                                          const fem_model_t *model,
                                          const double *nodal_reaction,
                                          int reaction_size,
                                          double generalized_force[3])
{
    int i;

    if (!set || !model || !nodal_reaction || !generalized_force) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "flex_reaction2d_from_node_set requires non-NULL inputs");
    }

    generalized_force[0] = 0.0;
    generalized_force[1] = 0.0;
    generalized_force[2] = 0.0;

    for (i = 0; i < set->count; ++i) {
        int node_index = flex_reaction2d_find_node_index(model, set->node_ids[i]);
        int dof_u;
        int dof_v;
        double fx;
        double fy;
        double x_local;
        double y_local;

        if (node_index < 0) {
            return error_set(FEM_ERROR_INVALID_NODE,
                             "Reaction node id %d is not present in model",
                             set->node_ids[i]);
        }

        dof_u = node_index * 2;
        dof_v = dof_u + 1;
        if (dof_v >= reaction_size) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Reaction vector is smaller than required nodal DOF");
        }

        fx = nodal_reaction[dof_u];
        fy = nodal_reaction[dof_v];
        x_local = set->local_coords ? set->local_coords[2 * i] : 0.0;
        y_local = set->local_coords ? set->local_coords[2 * i + 1] : 0.0;

        generalized_force[0] += fx;
        generalized_force[1] += fy;
        generalized_force[2] += x_local * fy - y_local * fx;
    }

    return FEM_SUCCESS;
}

void flex_reaction2d_sum_interface_forces(const double root_interface_force[3],
                                          const double tip_interface_force[3],
                                          double total_interface_force[3])
{
    if (!total_interface_force) {
        return;
    }

    total_interface_force[0] = 0.0;
    total_interface_force[1] = 0.0;
    total_interface_force[2] = 0.0;

    if (root_interface_force) {
        total_interface_force[0] += root_interface_force[0];
        total_interface_force[1] += root_interface_force[1];
        total_interface_force[2] += root_interface_force[2];
    }
    if (tip_interface_force) {
        total_interface_force[0] += tip_interface_force[0];
        total_interface_force[1] += tip_interface_force[1];
        total_interface_force[2] += tip_interface_force[2];
    }
}

static void flex_reaction2d_to_body_force(const double interface_force[3],
                                          double body_force[3])
{
    if (!interface_force || !body_force) {
        return;
    }

    /* The rigid body receives the equal-and-opposite interface load from the FE link. */
    body_force[0] = -interface_force[0];
    body_force[1] = -interface_force[1];
    body_force[2] = -interface_force[2];
}

void flex_reaction2d_to_root_body_force(const double interface_force[3],
                                        double body_force[3])
{
    flex_reaction2d_to_body_force(interface_force, body_force);
}

void flex_reaction2d_to_tip_body_force(const double interface_force[3],
                                       double body_force[3])
{
    flex_reaction2d_to_body_force(interface_force, body_force);
}
