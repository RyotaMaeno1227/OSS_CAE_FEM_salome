#ifndef FEM4C_ANALYSIS_RUNNER_H
#define FEM4C_ANALYSIS_RUNNER_H

#include "../common/types.h"

typedef enum {
    ANALYSIS_MODE_FEM = 0,
    ANALYSIS_MODE_MBD,
    ANALYSIS_MODE_COUPLED
} analysis_mode_t;

const char *analysis_mode_to_string(analysis_mode_t mode);
fem_error_t analysis_mode_parse(const char *text, analysis_mode_t *mode);
analysis_mode_t analysis_mode_from_env(void);

fem_error_t analysis_run(analysis_mode_t mode,
                         const char *input_filename,
                         const char *output_filename);

#endif /* FEM4C_ANALYSIS_RUNNER_H */
