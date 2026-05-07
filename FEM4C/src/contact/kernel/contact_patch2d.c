#include "contact_patch2d.h"
#include "../../common/error.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static fem_error_t contact_patch2d_validate_unit_basis(const double vector[2],
                                                       const char *label);
static fem_error_t contact_patch2d_validate_frame(const double normal_world[2],
                                                  const double tangent_world[2]);
static fem_error_t contact_patch2d_build_path(char path[MAX_FILENAME_LEN],
                                              const char *artifact_dir,
                                              const char *pattern,
                                              int pair_id,
                                              int body_id,
                                              int step);

void contact_patch2d_zero(contact_patch2d_t *patch)
{
    if (!patch) {
        return;
    }
    memset(patch, 0, sizeof(*patch));
    patch->body_id = -1;
    patch->pair_id = -1;
    patch->step = -1;
}

void contact_patch2d_macro_trace_row_zero(contact_patch2d_macro_trace_row_t *row)
{
    if (!row) {
        return;
    }
    memset(row, 0, sizeof(*row));
    row->step = -1;
    row->pair_id = -1;
}

fem_error_t contact_patch2d_parse_macro_trace_csv_row(
    const char *line,
    contact_patch2d_macro_trace_row_t *row)
{
    int scanned;
    double vn_dummy = 0.0;
    double f1x_dummy = 0.0;
    double f1y_dummy = 0.0;
    double m1_dummy = 0.0;
    double f2x_dummy = 0.0;
    double f2y_dummy = 0.0;
    double m2_dummy = 0.0;

    CHECK_NULL(line, "contact patch macro trace row");
    CHECK_NULL(row, "contact patch macro trace row output");

    contact_patch2d_macro_trace_row_zero(row);
    scanned = sscanf(line,
                     "%d,%lf,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
                     &row->step,
                     &row->time,
                     &row->pair_id,
                     &row->active,
                     &row->gap_macro,
                     &row->penetration,
                     &row->normal_world[0],
                     &row->normal_world[1],
                     &vn_dummy,
                     &row->fn_macro,
                     &row->cp1_world[0],
                     &row->cp1_world[1],
                     &row->cp2_world[0],
                     &row->cp2_world[1],
                     &f1x_dummy,
                     &f1y_dummy,
                     &m1_dummy,
                     &f2x_dummy,
                     &f2y_dummy,
                     &m2_dummy);
    if (scanned != 20) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch macro trace row requires 20 CSV fields");
    }
    if (row->step < 0 || row->pair_id < 0 || (row->active != 0 && row->active != 1)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch macro trace row has invalid step/pair_id/active");
    }
    if (!isfinite(row->time) ||
        !isfinite(row->gap_macro) ||
        !isfinite(row->penetration) ||
        !isfinite(row->normal_world[0]) ||
        !isfinite(row->normal_world[1]) ||
        !isfinite(row->fn_macro) ||
        !isfinite(row->cp1_world[0]) ||
        !isfinite(row->cp1_world[1]) ||
        !isfinite(row->cp2_world[0]) ||
        !isfinite(row->cp2_world[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch macro trace row values must be finite");
    }

    return contact_patch2d_validate_unit_basis(row->normal_world, "normal_world");
}

fem_error_t contact_patch2d_build(
    contact_patch2d_t *patch,
    int body_id,
    int pair_id,
    int step,
    double time,
    const double contact_point_world[2],
    const double normal_world[2],
    double radius_body,
    double gap_macro,
    double fn_macro,
    double thickness,
    double patch_size,
    const char *artifact_dir)
{
    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(contact_point_world, "contact patch contact point");
    CHECK_NULL(normal_world, "contact patch normal");
    CHECK_NULL(artifact_dir, "contact patch artifact_dir");

    if (body_id < 0 || pair_id < 0 || step < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch requires non-negative body_id/pair_id/step");
    }
    if (!isfinite(time) || time < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch time must be finite and non-negative");
    }
    if (!isfinite(contact_point_world[0]) || !isfinite(contact_point_world[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch contact_point_world must be finite");
    }
    if (!isfinite(radius_body) || radius_body <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch radius_body must be finite and positive");
    }
    if (!isfinite(gap_macro)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch gap_macro must be finite");
    }
    if (!isfinite(fn_macro) || fn_macro < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch fn_macro must be finite and non-negative");
    }
    if (!isfinite(thickness) || thickness <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch thickness must be finite and positive");
    }
    if (!isfinite(patch_size) || patch_size <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch patch_size must be finite and positive");
    }
    if (artifact_dir[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch artifact_dir must not be empty");
    }

    contact_patch2d_zero(patch);
    CHECK_ERROR(contact_patch2d_validate_unit_basis(normal_world, "normal_world"));

    patch->body_id = body_id;
    patch->pair_id = pair_id;
    patch->step = step;
    patch->time = time;
    patch->contact_point_world[0] = contact_point_world[0];
    patch->contact_point_world[1] = contact_point_world[1];
    patch->normal_world[0] = normal_world[0];
    patch->normal_world[1] = normal_world[1];
    patch->tangent_world[0] = -normal_world[1];
    patch->tangent_world[1] = normal_world[0];
    patch->radius_body = radius_body;
    patch->gap_macro = gap_macro;
    patch->fn_macro = fn_macro;
    patch->thickness = thickness;
    patch->patch_size = patch_size;

    CHECK_ERROR(contact_patch2d_build_path(patch->mesh_path,
                                           artifact_dir,
                                           "%s/patch_pair%d_body%d_step%04d.dat",
                                           pair_id,
                                           body_id,
                                           step));
    CHECK_ERROR(contact_patch2d_build_path(patch->output_path,
                                           artifact_dir,
                                           "%s/patch_pair%d_body%d_step%04d_out.json",
                                           pair_id,
                                           body_id,
                                           step));
    return FEM_SUCCESS;
}

fem_error_t contact_patch2d_build_pair_from_trace_row(
    const contact_patch2d_macro_trace_row_t *row,
    int body_small_id,
    int body_large_id,
    double radius_small,
    double radius_large,
    double thickness,
    double patch_size,
    const char *artifact_dir,
    contact_patch2d_t patches[2])
{
    CHECK_NULL(row, "contact patch macro trace row");
    CHECK_NULL(patches, "contact patch pair output");

    CHECK_ERROR(contact_patch2d_build(&patches[0],
                                      body_small_id,
                                      row->pair_id,
                                      row->step,
                                      row->time,
                                      row->cp1_world,
                                      row->normal_world,
                                      radius_small,
                                      row->gap_macro,
                                      row->fn_macro,
                                      thickness,
                                      patch_size,
                                      artifact_dir));
    CHECK_ERROR(contact_patch2d_build(&patches[1],
                                      body_large_id,
                                      row->pair_id,
                                      row->step,
                                      row->time,
                                      row->cp2_world,
                                      row->normal_world,
                                      radius_large,
                                      row->gap_macro,
                                      row->fn_macro,
                                      thickness,
                                      patch_size,
                                      artifact_dir));
    return FEM_SUCCESS;
}

fem_error_t contact_patch2d_build_metadata_path(
    const contact_patch2d_t *patch,
    const char *artifact_dir,
    char metadata_path[MAX_FILENAME_LEN])
{
    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(artifact_dir, "contact patch artifact_dir");
    CHECK_NULL(metadata_path, "contact patch metadata path");

    return contact_patch2d_build_path(metadata_path,
                                      artifact_dir,
                                      "%s/patch_pair%d_body%d_step%04d.json",
                                      patch->pair_id,
                                      patch->body_id,
                                      patch->step);
}

fem_error_t contact_patch2d_write_metadata_json(
    const contact_patch2d_t *patch,
    const char *metadata_path)
{
    FILE *out = NULL;

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(metadata_path, "contact patch metadata path");
    CHECK_ERROR(contact_patch2d_validate_frame(patch->normal_world, patch->tangent_world));

    out = fopen(metadata_path, "w");
    if (!out) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "cannot open contact patch metadata file: %s",
                         metadata_path);
    }

    if (fprintf(out,
                "{\n"
                "  \"body_id\": %d,\n"
                "  \"pair_id\": %d,\n"
                "  \"step\": %d,\n"
                "  \"time\": %.16e,\n"
                "  \"contact_point_world\": [%.16e, %.16e],\n"
                "  \"normal_world\": [%.16e, %.16e],\n"
                "  \"tangent_world\": [%.16e, %.16e],\n"
                "  \"radius_body\": %.16e,\n"
                "  \"gap_macro\": %.16e,\n"
                "  \"fn_macro\": %.16e,\n"
                "  \"thickness\": %.16e,\n"
                "  \"patch_size\": %.16e,\n"
                "  \"mesh_path\": \"%s\",\n"
                "  \"output_path\": \"%s\"\n"
                "}\n",
                patch->body_id,
                patch->pair_id,
                patch->step,
                patch->time,
                patch->contact_point_world[0],
                patch->contact_point_world[1],
                patch->normal_world[0],
                patch->normal_world[1],
                patch->tangent_world[0],
                patch->tangent_world[1],
                patch->radius_body,
                patch->gap_macro,
                patch->fn_macro,
                patch->thickness,
                patch->patch_size,
                patch->mesh_path,
                patch->output_path) < 0) {
        fclose(out);
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to write contact patch metadata file: %s",
                         metadata_path);
    }

    if (fclose(out) != 0) {
        return error_set(FEM_ERROR_FILE_WRITE,
                         "failed to close contact patch metadata file: %s",
                         metadata_path);
    }
    return FEM_SUCCESS;
}

fem_error_t contact_patch2d_world_to_local(
    const contact_patch2d_t *patch,
    const double point_world[2],
    double *s_out,
    double *u_out)
{
    double delta[2];

    CHECK_NULL(patch, "contact patch");
    CHECK_NULL(point_world, "contact patch world point");
    CHECK_NULL(s_out, "contact patch local s");
    CHECK_NULL(u_out, "contact patch local u");
    CHECK_ERROR(contact_patch2d_validate_frame(patch->normal_world, patch->tangent_world));

    if (!isfinite(point_world[0]) || !isfinite(point_world[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch world point must be finite");
    }

    delta[0] = point_world[0] - patch->contact_point_world[0];
    delta[1] = point_world[1] - patch->contact_point_world[1];
    *s_out = delta[0] * patch->tangent_world[0] + delta[1] * patch->tangent_world[1];
    *u_out = delta[0] * patch->normal_world[0] + delta[1] * patch->normal_world[1];
    return FEM_SUCCESS;
}

static fem_error_t contact_patch2d_validate_unit_basis(const double vector[2],
                                                       const char *label)
{
    const double norm = sqrt(vector[0] * vector[0] + vector[1] * vector[1]);

    CHECK_NULL(vector, "contact patch basis");
    CHECK_NULL(label, "contact patch basis label");

    if (!isfinite(vector[0]) || !isfinite(vector[1])) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch %s must be finite",
                         label);
    }
    if (!isfinite(norm) || norm <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch %s must have positive norm",
                         label);
    }
    if (fabs(norm - 1.0) > 1.0e-8) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch %s must be unit length (norm=%.16e)",
                         label,
                         norm);
    }
    return FEM_SUCCESS;
}

static fem_error_t contact_patch2d_validate_frame(const double normal_world[2],
                                                  const double tangent_world[2])
{
    const double dot = normal_world[0] * tangent_world[0]
                       + normal_world[1] * tangent_world[1];

    CHECK_ERROR(contact_patch2d_validate_unit_basis(normal_world, "normal_world"));
    CHECK_ERROR(contact_patch2d_validate_unit_basis(tangent_world, "tangent_world"));
    if (!isfinite(dot) || fabs(dot) > 1.0e-8) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch normal_world and tangent_world must be orthogonal (dot=%.16e)",
                         dot);
    }
    return FEM_SUCCESS;
}

static fem_error_t contact_patch2d_build_path(char path[MAX_FILENAME_LEN],
                                              const char *artifact_dir,
                                              const char *pattern,
                                              int pair_id,
                                              int body_id,
                                              int step)
{
    int written;

    CHECK_NULL(path, "contact patch path");
    CHECK_NULL(artifact_dir, "contact patch artifact_dir");
    CHECK_NULL(pattern, "contact patch path pattern");

    written = snprintf(path,
                       MAX_FILENAME_LEN,
                       pattern,
                       artifact_dir,
                       pair_id,
                       body_id,
                       step);
    if (written < 0 || written >= MAX_FILENAME_LEN) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact patch path exceeds %d characters",
                         MAX_FILENAME_LEN - 1);
    }
    return FEM_SUCCESS;
}
