#ifndef FEM_MODEL_COPY_H
#define FEM_MODEL_COPY_H

/* FEM4C - Deep-copyable snapshot of the globals-based FE model.
 * Keep the public type distinct from D-team's temporary fem_model2d_t
 * until the coupled model headers are unified.
 */

#include "../common/globals.h"

typedef struct {
    double translational_accel[2];
    double angular_accel;
    double angular_velocity;
    double density;
} flex_inertial_load2d_t;

typedef struct {
    double (*node_coords)[3];
    double (*node_displ)[3];
    double (*node_bc_values)[3];
    double (*node_force)[3];
    int (*node_bc_flags)[3];
    int *node_ids;
    int *node_id_to_index;

    int (*element_nodes)[MAX_NODES_PER_ELEMENT];
    int *element_type;
    int *element_material;
    int *element_ids;
    int *element_id_to_index;

    double (*material_props)[6];
    int *material_type;
    int *material_ids;
    int *material_id_to_index;

    int node_capacity;
    int element_capacity;
    int material_capacity;
    int node_id_capacity;
    int element_id_capacity;
    int material_id_capacity;

    double *global_force;
    double *global_displ;
    double *global_stiffness_values;
    int *stiffness_profile;
    int *stiffness_offsets;
    int stiffness_value_count;
    int stiffness_bandwidth;

    double body_force[3];
    double base_body_force[3];
    double pressure_value;
    int has_body_force;
    int has_base_body_force;
    int has_pressure;
    int has_inertial_load;
    flex_inertial_load2d_t inertial_load;
    int num_tractions;
    int traction_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];
    double traction_values[MAX_TRACTION_SURFACES][3];
    int num_pressure_surfaces;
    int pressure_surfaces[MAX_TRACTION_SURFACES][MAX_SURFACE_NODES];

    analysis_control_t analysis;
    solver_info_t solver_info;
    int num_nodes;
    int num_elements;
    int num_materials;
    int total_dof;
    int full_reassembly_count;
    int static_solve_count;

    char input_filename[MAX_FILENAME_LEN];
    char output_filename[MAX_FILENAME_LEN];
    int num_threads;
    int t3_strict_orientation;
} fem_model_t;

void fem_model_zero(fem_model_t *model);
void fem_model_free(fem_model_t *model);
fem_error_t fem_model_clone(fem_model_t *dest, const fem_model_t *src);
fem_error_t fem_model_clone_from_globals(fem_model_t *dest);
fem_error_t fem_model_restore_globals(const fem_model_t *src);

#endif /* FEM_MODEL_COPY_H */
