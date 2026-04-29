#include "fem_model_copy.h"

#include "../common/error.h"
#include <stdlib.h>
#include <string.h>

static fem_error_t fem_model_copy_block(void **dest, const void *src, size_t size)
{
    void *copy = NULL;

    *dest = NULL;
    if (!src || size == 0) {
        return FEM_SUCCESS;
    }

    copy = malloc(size);
    if (!copy) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate FEM model snapshot block (%zu bytes)", size);
    }

    memcpy(copy, src, size);
    *dest = copy;
    return FEM_SUCCESS;
}

void fem_model_zero(fem_model_t *model)
{
    if (!model) {
        return;
    }
    memset(model, 0, sizeof(*model));
}

void fem_model_free(fem_model_t *model)
{
    if (!model) {
        return;
    }

    free(model->node_coords);
    free(model->node_displ);
    free(model->node_bc_values);
    free(model->node_force);
    free(model->node_bc_flags);
    free(model->node_ids);
    free(model->node_id_to_index);

    free(model->element_nodes);
    free(model->element_type);
    free(model->element_material);
    free(model->element_ids);
    free(model->element_id_to_index);

    free(model->material_props);
    free(model->material_type);
    free(model->material_ids);
    free(model->material_id_to_index);

    free(model->global_force);
    free(model->global_displ);
    free(model->global_stiffness_values);
    free(model->stiffness_profile);
    free(model->stiffness_offsets);

    fem_model_zero(model);
}

fem_error_t fem_model_clone(fem_model_t *dest, const fem_model_t *src)
{
    fem_model_t *snapshot = NULL;
    fem_error_t err = FEM_SUCCESS;

    if (!dest || !src) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem_model_clone requires non-NULL source and destination");
    }

    snapshot = calloc(1, sizeof(*snapshot));
    if (!snapshot) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate FEM model snapshot");
    }
    fem_model_zero(snapshot);

    snapshot->node_capacity = src->node_capacity;
    snapshot->element_capacity = src->element_capacity;
    snapshot->material_capacity = src->material_capacity;
    snapshot->node_id_capacity = src->node_id_capacity;
    snapshot->element_id_capacity = src->element_id_capacity;
    snapshot->material_id_capacity = src->material_id_capacity;
    snapshot->stiffness_value_count = src->stiffness_value_count;
    snapshot->stiffness_bandwidth = src->stiffness_bandwidth;
    snapshot->pressure_value = src->pressure_value;
    snapshot->has_body_force = src->has_body_force;
    snapshot->has_base_body_force = src->has_base_body_force;
    snapshot->has_pressure = src->has_pressure;
    snapshot->has_inertial_load = src->has_inertial_load;
    snapshot->num_tractions = src->num_tractions;
    snapshot->num_pressure_surfaces = src->num_pressure_surfaces;
    snapshot->analysis = src->analysis;
    snapshot->solver_info = src->solver_info;
    snapshot->num_nodes = src->num_nodes;
    snapshot->num_elements = src->num_elements;
    snapshot->num_materials = src->num_materials;
    snapshot->total_dof = src->total_dof;
    snapshot->full_reassembly_count = src->full_reassembly_count;
    snapshot->static_solve_count = src->static_solve_count;
    snapshot->num_threads = src->num_threads;
    snapshot->t3_strict_orientation = src->t3_strict_orientation;

    memcpy(snapshot->body_force, src->body_force, sizeof(snapshot->body_force));
    memcpy(snapshot->base_body_force, src->base_body_force, sizeof(snapshot->base_body_force));
    snapshot->inertial_load = src->inertial_load;
    memcpy(snapshot->traction_surfaces, src->traction_surfaces, sizeof(snapshot->traction_surfaces));
    memcpy(snapshot->traction_values, src->traction_values, sizeof(snapshot->traction_values));
    memcpy(snapshot->pressure_surfaces, src->pressure_surfaces, sizeof(snapshot->pressure_surfaces));
    memcpy(snapshot->input_filename, src->input_filename, sizeof(snapshot->input_filename));
    memcpy(snapshot->output_filename, src->output_filename, sizeof(snapshot->output_filename));

    err = fem_model_copy_block((void **)&snapshot->node_coords, src->node_coords,
                               (size_t)src->node_capacity * sizeof(*src->node_coords));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_displ, src->node_displ,
                               (size_t)src->node_capacity * sizeof(*src->node_displ));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_bc_values, src->node_bc_values,
                               (size_t)src->node_capacity * sizeof(*src->node_bc_values));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_force, src->node_force,
                               (size_t)src->node_capacity * sizeof(*src->node_force));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_bc_flags, src->node_bc_flags,
                               (size_t)src->node_capacity * sizeof(*src->node_bc_flags));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_ids, src->node_ids,
                               (size_t)src->node_capacity * sizeof(*src->node_ids));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->node_id_to_index, src->node_id_to_index,
                               (size_t)src->node_id_capacity * sizeof(*src->node_id_to_index));
    if (err != FEM_SUCCESS) {
        goto fail;
    }

    err = fem_model_copy_block((void **)&snapshot->element_nodes, src->element_nodes,
                               (size_t)src->element_capacity * sizeof(*src->element_nodes));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->element_type, src->element_type,
                               (size_t)src->element_capacity * sizeof(*src->element_type));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->element_material, src->element_material,
                               (size_t)src->element_capacity * sizeof(*src->element_material));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->element_ids, src->element_ids,
                               (size_t)src->element_capacity * sizeof(*src->element_ids));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->element_id_to_index, src->element_id_to_index,
                               (size_t)src->element_id_capacity * sizeof(*src->element_id_to_index));
    if (err != FEM_SUCCESS) {
        goto fail;
    }

    err = fem_model_copy_block((void **)&snapshot->material_props, src->material_props,
                               (size_t)src->material_capacity * sizeof(*src->material_props));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->material_type, src->material_type,
                               (size_t)src->material_capacity * sizeof(*src->material_type));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->material_ids, src->material_ids,
                               (size_t)src->material_capacity * sizeof(*src->material_ids));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->material_id_to_index, src->material_id_to_index,
                               (size_t)src->material_id_capacity * sizeof(*src->material_id_to_index));
    if (err != FEM_SUCCESS) {
        goto fail;
    }

    err = fem_model_copy_block((void **)&snapshot->global_force, src->global_force,
                               (size_t)src->total_dof * sizeof(*src->global_force));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->global_displ, src->global_displ,
                               (size_t)src->total_dof * sizeof(*src->global_displ));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->global_stiffness_values, src->global_stiffness_values,
                               (size_t)src->stiffness_value_count * sizeof(*src->global_stiffness_values));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->stiffness_profile, src->stiffness_profile,
                               (size_t)src->total_dof * sizeof(*src->stiffness_profile));
    if (err != FEM_SUCCESS) {
        goto fail;
    }
    err = fem_model_copy_block((void **)&snapshot->stiffness_offsets, src->stiffness_offsets,
                               (size_t)src->total_dof * sizeof(*src->stiffness_offsets));
    if (err != FEM_SUCCESS) {
        goto fail;
    }

    fem_model_free(dest);
    *dest = *snapshot;
    free(snapshot);
    return FEM_SUCCESS;

fail:
    fem_model_free(snapshot);
    free(snapshot);
    return err;
}

fem_error_t fem_model_clone_from_globals(fem_model_t *dest)
{
    fem_model_t *current = NULL;
    fem_error_t err = FEM_SUCCESS;

    if (!dest) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem_model_clone_from_globals requires a destination");
    }

    current = calloc(1, sizeof(*current));
    if (!current) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate FEM model globals snapshot");
    }

    fem_model_zero(current);
    current->node_coords = g_node_coords;
    current->node_displ = g_node_displ;
    current->node_bc_values = g_node_bc_values;
    current->node_force = g_node_force;
    current->node_bc_flags = g_node_bc_flags;
    current->node_ids = g_node_ids;
    current->node_id_to_index = g_node_id_to_index;
    current->element_nodes = g_element_nodes;
    current->element_type = g_element_type;
    current->element_material = g_element_material;
    current->element_ids = g_element_ids;
    current->element_id_to_index = g_element_id_to_index;
    current->material_props = g_material_props;
    current->material_type = g_material_type;
    current->material_ids = g_material_ids;
    current->material_id_to_index = g_material_id_to_index;
    current->node_capacity = g_node_capacity;
    current->element_capacity = g_element_capacity;
    current->material_capacity = g_material_capacity;
    current->node_id_capacity = g_node_id_capacity;
    current->element_id_capacity = g_element_id_capacity;
    current->material_id_capacity = g_material_id_capacity;
    current->global_force = g_global_force;
    current->global_displ = g_global_displ;
    current->global_stiffness_values = g_global_stiffness_values;
    current->stiffness_profile = g_stiffness_profile;
    current->stiffness_offsets = g_stiffness_offsets;
    current->stiffness_value_count = g_stiffness_value_count;
    current->stiffness_bandwidth = g_stiffness_bandwidth;
    memcpy(current->body_force, g_body_force, sizeof(current->body_force));
    memcpy(current->base_body_force, g_body_force, sizeof(current->base_body_force));
    current->pressure_value = g_pressure_value;
    current->has_body_force = g_has_body_force;
    current->has_base_body_force = g_has_body_force;
    current->has_pressure = g_has_pressure;
    current->has_inertial_load = 0;
    current->num_tractions = g_num_tractions;
    memcpy(current->traction_surfaces, g_traction_surfaces, sizeof(current->traction_surfaces));
    memcpy(current->traction_values, g_traction_values, sizeof(current->traction_values));
    current->num_pressure_surfaces = g_num_pressure_surfaces;
    memcpy(current->pressure_surfaces, g_pressure_surfaces, sizeof(current->pressure_surfaces));
    current->analysis = g_analysis;
    current->solver_info = g_solver_info;
    current->num_nodes = g_num_nodes;
    current->num_elements = g_num_elements;
    current->num_materials = g_num_materials;
    current->total_dof = g_total_dof;
    memcpy(current->input_filename, g_input_filename, sizeof(current->input_filename));
    memcpy(current->output_filename, g_output_filename, sizeof(current->output_filename));
    current->num_threads = g_num_threads;
    current->t3_strict_orientation = g_t3_strict_orientation;

    err = fem_model_clone(dest, current);
    free(current);
    return err;
}

fem_error_t fem_model_restore_globals(const fem_model_t *src)
{
    fem_model_t *snapshot = NULL;
    fem_error_t err = FEM_SUCCESS;

    if (!src) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "fem_model_restore_globals requires a source snapshot");
    }

    snapshot = calloc(1, sizeof(*snapshot));
    if (!snapshot) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate FEM model restore snapshot");
    }

    fem_model_zero(snapshot);
    err = fem_model_clone(snapshot, src);
    if (err != FEM_SUCCESS) {
        free(snapshot);
        return err;
    }

    err = globals_finalize();
    if (err != FEM_SUCCESS) {
        fem_model_free(snapshot);
        free(snapshot);
        return err;
    }

    g_node_coords = snapshot->node_coords;
    g_node_displ = snapshot->node_displ;
    g_node_bc_values = snapshot->node_bc_values;
    g_node_force = snapshot->node_force;
    g_node_bc_flags = snapshot->node_bc_flags;
    g_node_ids = snapshot->node_ids;
    g_node_id_to_index = snapshot->node_id_to_index;
    g_element_nodes = snapshot->element_nodes;
    g_element_type = snapshot->element_type;
    g_element_material = snapshot->element_material;
    g_element_ids = snapshot->element_ids;
    g_element_id_to_index = snapshot->element_id_to_index;
    g_material_props = snapshot->material_props;
    g_material_type = snapshot->material_type;
    g_material_ids = snapshot->material_ids;
    g_material_id_to_index = snapshot->material_id_to_index;

    g_node_capacity = snapshot->node_capacity;
    g_element_capacity = snapshot->element_capacity;
    g_material_capacity = snapshot->material_capacity;
    g_node_id_capacity = snapshot->node_id_capacity;
    g_element_id_capacity = snapshot->element_id_capacity;
    g_material_id_capacity = snapshot->material_id_capacity;

    g_global_force = snapshot->global_force;
    g_global_displ = snapshot->global_displ;
    g_global_stiffness_values = snapshot->global_stiffness_values;
    g_stiffness_profile = snapshot->stiffness_profile;
    g_stiffness_offsets = snapshot->stiffness_offsets;
    g_stiffness_value_count = snapshot->stiffness_value_count;
    g_stiffness_bandwidth = snapshot->stiffness_bandwidth;

    memcpy(g_body_force, snapshot->body_force, sizeof(g_body_force));
    g_pressure_value = snapshot->pressure_value;
    g_has_body_force = snapshot->has_body_force;
    g_has_pressure = snapshot->has_pressure;
    g_num_tractions = snapshot->num_tractions;
    memcpy(g_traction_surfaces, snapshot->traction_surfaces, sizeof(g_traction_surfaces));
    memcpy(g_traction_values, snapshot->traction_values, sizeof(g_traction_values));
    g_num_pressure_surfaces = snapshot->num_pressure_surfaces;
    memcpy(g_pressure_surfaces, snapshot->pressure_surfaces, sizeof(g_pressure_surfaces));

    g_analysis = snapshot->analysis;
    g_solver_info = snapshot->solver_info;
    g_num_nodes = snapshot->num_nodes;
    g_num_elements = snapshot->num_elements;
    g_num_materials = snapshot->num_materials;
    g_total_dof = snapshot->total_dof;

    memcpy(g_input_filename, snapshot->input_filename, sizeof(g_input_filename));
    memcpy(g_output_filename, snapshot->output_filename, sizeof(g_output_filename));
    g_num_threads = snapshot->num_threads;
    g_t3_strict_orientation = snapshot->t3_strict_orientation;

    snapshot->node_coords = NULL;
    snapshot->node_displ = NULL;
    snapshot->node_bc_values = NULL;
    snapshot->node_force = NULL;
    snapshot->node_bc_flags = NULL;
    snapshot->node_ids = NULL;
    snapshot->node_id_to_index = NULL;
    snapshot->element_nodes = NULL;
    snapshot->element_type = NULL;
    snapshot->element_material = NULL;
    snapshot->element_ids = NULL;
    snapshot->element_id_to_index = NULL;
    snapshot->material_props = NULL;
    snapshot->material_type = NULL;
    snapshot->material_ids = NULL;
    snapshot->material_id_to_index = NULL;
    snapshot->global_force = NULL;
    snapshot->global_displ = NULL;
    snapshot->global_stiffness_values = NULL;
    snapshot->stiffness_profile = NULL;
    snapshot->stiffness_offsets = NULL;

    fem_model_free(snapshot);
    free(snapshot);
    return FEM_SUCCESS;
}
