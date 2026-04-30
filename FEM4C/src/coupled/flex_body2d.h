#ifndef FEM4C_COUPLED_FLEX_BODY2D_H
#define FEM4C_COUPLED_FLEX_BODY2D_H

#include "fem_model_copy.h"
#include "flex_nodeset.h"

typedef struct flex_bc2d_list flex_bc2d_list_t;

typedef fem_model_t fem_model2d_t;

typedef struct {
    int body_id;
    fem_model2d_t model;
    node_set_t root_set;
    node_set_t tip_set;
    double *u_local;         /* Packed as [ux0, uy0, ux1, uy1, ...] */
    int u_local_size;
    double reaction_root[3];
    double reaction_tip[3];
} flex_body2d_t;

void fem_model2d_zero(fem_model2d_t *model);
fem_error_t fem_model2d_clone(fem_model2d_t *dst, const fem_model2d_t *src);
void fem_model2d_free(fem_model2d_t *model);

void flex_body2d_zero(flex_body2d_t *body);
fem_error_t flex_body2d_init(flex_body2d_t *body,
                             int body_id,
                             const fem_model2d_t *model,
                             const node_set_t *root_set,
                             const node_set_t *tip_set);
fem_error_t flex_body2d_build_root_rigid_bc(flex_body2d_t *body,
                                            const double marker_disp[3],
                                            flex_bc2d_list_t *bc_list);
fem_error_t flex_body2d_build_tip_rigid_bc(flex_body2d_t *body,
                                           const double marker_disp[3],
                                           flex_bc2d_list_t *bc_list);
fem_error_t flex_body2d_compute_root_marker_disp(
    const flex_body2d_t *body,
    const double reference_pose[3],
    const double current_pose[3],
    double marker_disp[3]);
fem_error_t flex_body2d_compute_tip_marker_disp(
    const flex_body2d_t *body,
    const double reference_pose[3],
    const double current_pose[3],
    double marker_disp[3]);
fem_error_t flex_body2d_compute_root_center_local(const flex_body2d_t *body,
                                                  double center_local[2]);
fem_error_t flex_body2d_compute_tip_center_local(const flex_body2d_t *body,
                                                 double center_local[2]);
fem_error_t flex_body2d_compute_root_center_world(const flex_body2d_t *body,
                                                  const double body_pose[3],
                                                  double center_world[2]);
fem_error_t flex_body2d_compute_tip_center_world(const flex_body2d_t *body,
                                                 const double body_pose[3],
                                                 double center_world[2]);
fem_error_t flex_body2d_compute_interface_body_force(
    const flex_body2d_t *body,
    double root_body_force[3],
    double tip_body_force[3],
    double total_body_force[3]);
/* inertial_load is optional and packed as {ax, ay, density}. */
fem_error_t flex_body2d_solve_snapshot(flex_body2d_t *body,
                                       const double root_marker_disp[3],
                                       const double tip_marker_disp[3],
                                       const double inertial_load[3]);
fem_error_t flex_body2d_apply_root_rigid_displacement(
    flex_body2d_t *body,
    const double marker_disp[3]);
fem_error_t flex_body2d_apply_tip_rigid_displacement(
    flex_body2d_t *body,
    const double marker_disp[3]);
void flex_body2d_free(flex_body2d_t *body);

#endif /* FEM4C_COUPLED_FLEX_BODY2D_H */
