#ifndef FEM4C_COUPLED_FLEX_SNAPSHOT2D_H
#define FEM4C_COUPLED_FLEX_SNAPSHOT2D_H

#include "fem_model_copy.h"

fem_error_t flex_snapshot2d_build_output_path(char path[MAX_FILENAME_LEN],
                                              const char *output_filename,
                                              int body_id,
                                              int step_index,
                                              int iteration_index,
                                              double time);

fem_error_t flex_snapshot2d_write_csv(const fem_model_t *model,
                                      int body_id,
                                      int step_index,
                                      int iteration_index,
                                      double time,
                                      const double marker_pose[3],
                                      const char *output_filename);
fem_error_t flex_snapshot2d_write_csv_with_interface_centers(
    const fem_model_t *model,
    int body_id,
    int step_index,
    int iteration_index,
    double time,
    const double marker_pose[3],
    const double root_center_local[2],
    const double tip_center_local[2],
    const double root_center_world[2],
    const double tip_center_world[2],
    const double root_reaction_local[3],
    const double tip_reaction_local[3],
    const double root_body_force[3],
    const double tip_body_force[3],
    const double total_body_force[3],
    const char *output_filename);

#endif /* FEM4C_COUPLED_FLEX_SNAPSHOT2D_H */
