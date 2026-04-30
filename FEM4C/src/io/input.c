/* FEM4C - Input module implementation
 * Data input functions
 */

#include "input.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include "../coupled/case2d.h"
#include <errno.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

#define MAX_NASTRAN_PROPERTIES 512
#define MBD_DIAG_E_BODY_PARSE "E_BODY_PARSE"
#define MBD_DIAG_E_BODY_DYN_PARSE "E_BODY_DYN_PARSE"
#define MBD_DIAG_E_BODY_GROUND_PARSE "E_BODY_GROUND_PARSE"
#define MBD_DIAG_E_BODY_CIRCLE_PARSE "E_BODY_CIRCLE_PARSE"
#define MBD_DIAG_E_CONTACT_HALFSPACE_PARSE "E_CONTACT_HALFSPACE_PARSE"
#define MBD_DIAG_E_CONTACT_SURFACE_POLYLINE_PARSE "E_CONTACT_SURFACE_POLYLINE_PARSE"
#define MBD_DIAG_E_BODY_RANGE "E_BODY_RANGE"
#define MBD_DIAG_E_DUP_BODY "E_DUP_BODY"
#define MBD_DIAG_E_DUP_BODY_GROUND "E_DUP_BODY_GROUND"
#define MBD_DIAG_E_GRAVITY_PARSE "E_GRAVITY_PARSE"
#define MBD_DIAG_E_FORCE_PARSE "E_FORCE_PARSE"
#define MBD_DIAG_E_FORCE_RANGE "E_FORCE_RANGE"
#define MBD_DIAG_E_CONTACT_COUPLING_MODE_PARSE "E_CONTACT_COUPLING_MODE_PARSE"
#define MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE "E_LOCAL_FEEDBACK_MODE_PARSE"
#define MBD_DIAG_E_LOCAL_FEEDBACK_FILE_PARSE "E_LOCAL_FEEDBACK_FILE_PARSE"
#define MBD_DIAG_E_LOCAL_CONTACT_FILE_PARSE "E_LOCAL_CONTACT_FILE_PARSE"
#define MBD_DIAG_E_EHL_FILE_PARSE "E_EHL_FILE_PARSE"
#define MBD_DIAG_E_MONOLITHIC_PROPER_MODE_PARSE "E_MONOLITHIC_PROPER_MODE_PARSE"
#define MBD_DIAG_E_MONOLITHIC_PROPER_CONTEXT_PARSE "E_MONOLITHIC_PROPER_CONTEXT_PARSE"
#define MBD_DIAG_E_CONTACT_PAIR_PARSE "E_CONTACT_PAIR_PARSE"
#define MBD_DIAG_E_CONTACT_PAIR_HALFSPACE_PARSE "E_CONTACT_PAIR_HALFSPACE_PARSE"
#define MBD_DIAG_E_CONTACT_PAIR_GENERIC_PARSE "E_CONTACT_PAIR_GENERIC_PARSE"
#define MBD_DIAG_E_CONTACT_PAIR_RANGE "E_CONTACT_PAIR_RANGE"
#define MBD_DIAG_E_UNDEFINED_BODY_REF "E_UNDEFINED_BODY_REF"

typedef enum {
    INPUT_MBD_BODY_INPUT_NONE = 0,
    INPUT_MBD_BODY_INPUT_LEGACY = 1,
    INPUT_MBD_BODY_INPUT_DYN = 2
} input_mbd_body_input_kind_t;

typedef struct {
    int capacity;
    int *body_seen;
    int *body_defined_line;
    input_mbd_body_input_kind_t *body_defined_kind;
    int *body_ground_seen;
    int *body_ground_line;
    int *circle_seen;
    int *circle_line;
    int *body_force_seen;
    int *body_force_line;
    double (*body_force_accum)[3];
} input_mbd_body_tracker_t;

typedef struct {
    int pid;
    int mid;
    double thickness;
    int material_index;
} nastran_pshell_t;

static nastran_pshell_t g_nastran_pshells[MAX_NASTRAN_PROPERTIES];
static int g_nastran_pshell_count = 0;
static int *g_nastran_element_property = NULL;
static int g_nastran_element_property_capacity = 0;

const char *input_mbd_primary_directives_csv(void)
{
    return "MBD_BODY|MBD_BODY_DYN|MBD_BODY_GROUND|MBD_GRAVITY|MBD_FORCE|MBD_BODY_CIRCLE|MBD_CONTACT_HALFSPACE|MBD_CONTACT_PAIR|MBD_CONTACT_PAIR_HALFSPACE|MBD_CONTACT_SURFACE_POLYLINE|MBD_CONTACT_PAIR_GENERIC|MBD_CONTACT_COUPLING_MODE|MBD_LOCAL_FEEDBACK_MODE|MBD_LOCAL_CONTACT_MONOLITHIC|MBD_MONOLITHIC_PROPER_MODE|MBD_MONOLITHIC_PROPER_CONTEXT|MBD_LOCAL_FEEDBACK_FILE|MBD_LOCAL_CONTACT_FILE|MBD_EHL_FILE";
}

const char *input_mbd_optional_geometry_directives_csv(void)
{
    return "COUPLED_FLEX_BODY|COUPLED_FLEX_ROOT_SET|COUPLED_FLEX_TIP_SET";
}

static void input_nastran_normalize_line(char *line);
static void input_nastran_trim(char *text);
static int input_nastran_card_is_exact(const char *line, const char *card);
static int input_nastran_line_has_continuation(const char *line);
static fem_error_t input_parse_nastran_grid_long(input_control_t *input, const char *line);
static fem_error_t input_parse_nastran_grid_short(input_control_t *input, const char *line);
static fem_error_t input_parse_nastran_pload(input_control_t *input, const char *line);
static fem_error_t input_nastran_finalize_properties(void);
static fem_error_t input_nastran_find_pshell_material(int pid, int *material_index);
static fem_error_t input_ensure_nastran_element_capacity(int required);
static int input_parser_is_directory(const char *path);
static int input_parser_has_mesh_root(const char *path);
static fem_error_t input_read_parser_mesh(const char *mesh_path);
static fem_error_t input_read_parser_material(const char *material_path);
static fem_error_t input_read_parser_boundary(const char *boundary_path, const char *base_dir);
static fem_error_t input_read_parser_pressure(const char *pressure_path);
static fem_error_t input_store_pressure_surface_from_node_ids(int surface_index,
                                                              int node_count,
                                                              const int *node_ids,
                                                              double pressure);
static fem_error_t input_parse_parser_legacy_spc(const char *line);
static fem_error_t input_parse_parser_legacy_force(const char *line);
static const char *input_mbd_skip_leading_spaces(const char *text);
static int input_mbd_line_starts_with_token(const char *line, const char *token);
static int input_mbd_line_starts_with_prefix(const char *line, const char *prefix);
static void input_mbd_line_to_excerpt(const char *line, char *out, size_t out_size);
static int input_parse_mbd_body_line(const char *line,
                                     mbd_body2d_t *body,
                                     int *body_index);
static int input_parse_mbd_body_dyn_line(const char *line,
                                         mbd_body2d_t *body,
                                         int *body_index);
static int input_parse_mbd_gravity_line(const char *line, double gravity[2]);
static int input_parse_mbd_body_ground_line(const char *line, int *body_index);
static int input_parse_mbd_force_line(const char *line,
                                      int *body_index,
                                      double force[3]);
static int input_parse_mbd_body_circle_line(const char *line,
                                            int *body_id,
                                            double *radius,
                                            double *thickness);
static int input_parse_mbd_contact_halfspace_line(const char *line,
                                                  int *halfspace_id,
                                                  double point[2],
                                                  double normal[2],
                                                  double *thickness);
static int input_parse_mbd_contact_surface_polyline_line(const char *line,
                                                         int *surface_id,
                                                         int *body_id,
                                                         char *csv_path,
                                                         size_t csv_path_size);
static int input_parse_mbd_contact_pair_line(const char *line,
                                             int *pair_id,
                                             int *body_i,
                                             int *body_j,
                                             double *k_n,
                                             double *c_n,
                                             double *mu_base);
static int input_parse_mbd_contact_pair_halfspace_line(const char *line,
                                                       int *pair_id,
                                                       int *body_circle,
                                                       int *halfspace_id,
                                                       double *k_n,
                                                       double *c_n,
                                                       double *mu_base);
static int input_parse_mbd_contact_pair_generic_line(const char *line,
                                                     int *pair_id,
                                                     int *surface_i,
                                                     int *surface_j,
                                                     double *k_n,
                                                     double *c_n,
                                                     double *mu_base,
                                                     double *mu_static,
                                                     double *mu_dynamic,
                                                     double *v_ref,
                                                     double *v_smooth);
static int input_parse_mbd_contact_coupling_mode_line(
    const char *line,
    mbd_contact_coupling_mode_t *mode_out);
static int input_parse_mbd_local_feedback_mode_line(
    const char *line,
    mbd_local_feedback_mode_t *mode_out);
static int input_parse_mbd_local_feedback_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size);
static int input_parse_mbd_monolithic_proper_mode_line(
    const char *line,
    mbd_monolithic_proper_mode_t *mode_out);
static int input_parse_mbd_monolithic_proper_context_line(
    const char *line,
    mbd_monolithic_proper_context2d_t *context_out);
static int input_parse_mbd_local_contact_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size);
static int input_parse_mbd_ehl_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size);
static void input_mbd_body_tracker_zero(input_mbd_body_tracker_t *tracker);
static void input_mbd_body_tracker_free(input_mbd_body_tracker_t *tracker);
static fem_error_t input_mbd_body_tracker_reserve(input_mbd_body_tracker_t *tracker,
                                                  int required_capacity);
static fem_error_t input_parse_coupled_flex_body_directive(coupled_case2d_t *case_data,
                                                           const char *line,
                                                           int line_number);
static fem_error_t input_parse_coupled_flex_set_directive(coupled_case2d_t *case_data,
                                                          const char *line,
                                                          const char *directive,
                                                          int is_root,
                                                          int line_number);
static int input_parse_strict_int_token(const char *text, int *value_out);
static int input_parse_strict_double_token(const char *text, double *value_out);
static void input_parser_trim(char *text);
static int input_parser_is_label(const char *line, const char *label);
static int input_parser_split_tokens(const char *line, char tokens[][64], int max_tokens);
static fem_error_t input_build_path_relative_to_file(const char *deck_path,
                                                     const char *raw_path,
                                                     char *resolved_path,
                                                     size_t resolved_path_size);
static int input_find_fem_contact_generic_surface_index(int surface_id);
static int input_find_fem_contact_generic_pair_index(int pair_id);
static int input_parse_fem_contact_surface_edge_set_line(const char *line,
                                                         int *surface_id,
                                                         int *part_id,
                                                         char *csv_path,
                                                         size_t csv_path_size);
static int input_parse_fem_contact_pair_generic_line(const char *line,
                                                     int *pair_id,
                                                     int *surface_i,
                                                     int *surface_j,
                                                     double *k_pen,
                                                     double *c_pen,
                                                     double *mu);
static int input_parse_fem_contact_adhesion_generic_line(const char *line,
                                                         int *pair_id,
                                                         double *k_adh_n,
                                                         double *gap_adh_max_m);
static int input_parse_fem_contact_friction_generic_line(const char *line,
                                                         int *pair_id,
                                                         double *mu_cap,
                                                         double *k_t_pen,
                                                         double *u_t_reg_m);
static int input_parse_fem_solver_mode_line(const char *line,
                                            fem_solver_mode_t *mode_out);
static int input_parse_fem_time_step_dt_line(const char *line,
                                             double *dt_out);
static int input_parse_fem_num_time_steps_line(const char *line,
                                               int *count_out);
static int input_parse_fem_outer_max_iter_line(const char *line,
                                               int *count_out);
static int input_parse_fem_outer_tol_line(const char *line,
                                          double *tol_out);
static int input_parse_fem_load_scale_step_line(const char *line,
                                                int *step_index_out,
                                                double *scale_out);
static fem_error_t input_validate_fem_contact_surface_edge_csv(const char *csv_path,
                                                               int surface_id,
                                                               int *edge_count_out);
static fem_error_t input_register_fem_contact_surface_edge_set(int surface_id,
                                                               int part_id,
                                                               const char *csv_path,
                                                               int edge_count);
static fem_error_t input_register_fem_contact_pair_generic(int pair_id,
                                                           int surface_i,
                                                           int surface_j,
                                                           double k_pen,
                                                           double c_pen,
                                                           double mu);
static fem_error_t input_register_fem_contact_adhesion_generic(int pair_id,
                                                               double k_adh_n,
                                                               double gap_adh_max_m);
static fem_error_t input_register_fem_contact_friction_generic(int pair_id,
                                                               double mu_cap,
                                                               double k_t_pen,
                                                               double u_t_reg_m);
static fem_error_t input_register_fem_solver_mode(fem_solver_mode_t mode);
static fem_error_t input_register_fem_time_step_dt(double dt);
static fem_error_t input_register_fem_num_time_steps(int count);
static fem_error_t input_register_fem_outer_max_iter(int count);
static fem_error_t input_register_fem_outer_tol(double tol);
static fem_error_t input_register_fem_load_scale_step(int step_index,
                                                      double scale);
static fem_error_t input_validate_fem_contact_pair_references(void);
static fem_error_t input_read_fem_generic_contact_directives(const char *filename);
static fem_error_t input_read_fem_solver_directives(const char *filename);
static int input_parse_param_k6rot_value(const char *line, double *value_out);
static void input_warn_unsupported_spc_component(char comp, const char *context, int line_no);
static fem_error_t input_apply_constraint_component(int node_index,
                                                    char comp,
                                                    double value,
                                                    const char *context,
                                                    int line_no);
static fem_error_t input_apply_constraint_mask(int node_index,
                                               const char *component_field,
                                               double value,
                                               const char *context,
                                               int line_no);

/* Utility helpers */
static int input_is_blank_or_comment(const char *line)
{
    if (line == NULL) {
        return 1;
    }
    while (*line != '\0') {
        if (*line == '#') return 1;
        if (!isspace((unsigned char)*line)) {
            return 0;
        }
        line++;
    }
    return 1;
}

static void input_trim(char *line)
{
    size_t len;
    char *start;

    if (line == NULL) return;

    len = strlen(line);
    while (len > 0 && isspace((unsigned char)line[len-1])) {
        line[len-1] = '\0';
        len--;
    }

    start = line;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != line) {
        memmove(line, start, strlen(start) + 1);
    }
}

static int input_parse_param_k6rot_value(const char *line, double *value_out)
{
    char fields[10][9];
    char normalized[256];
    char tokens[8][64];
    int nt = 0;
    char key[32];
    double value = 0.0;

    if (!line || !value_out) {
        return 0;
    }

    if (input_nastran_parse_fixed_format(line, fields, 10) == FEM_SUCCESS) {
        strncpy(key, fields[1], sizeof(key) - 1);
        key[sizeof(key) - 1] = '\0';
        input_nastran_trim(key);
        if (strcmp(key, "K6ROT") == 0 &&
            input_nastran_get_double(fields[2], &value) == FEM_SUCCESS) {
            *value_out = value;
            return 1;
        }
        if (strncmp(key, "K6ROT", 5) == 0 && sscanf(key + 5, "%lf", &value) == 1) {
            *value_out = value;
            return 1;
        }
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    for (size_t i = 0; normalized[i] != '\0'; ++i) {
        if (normalized[i] == ',' || normalized[i] == '=' || normalized[i] == '\t') {
            normalized[i] = ' ';
        }
    }
    nt = input_parser_split_tokens(normalized, tokens, 8);
    if (nt >= 3 && strcmp(tokens[0], "PARAM") == 0 && strcmp(tokens[1], "K6ROT") == 0 &&
        sscanf(tokens[2], "%lf", &value) == 1) {
        *value_out = value;
        return 1;
    }
    if (nt >= 2 && strcmp(tokens[0], "PARAM") == 0 && strncmp(tokens[1], "K6ROT", 5) == 0 &&
        sscanf(tokens[1] + 5, "%lf", &value) == 1) {
        *value_out = value;
        return 1;
    }

    return 0;
}

static void input_warn_unsupported_spc_component(char comp, const char *context, int line_no)
{
    static int warned_3 = 0;
    static int warned_4 = 0;
    static int warned_5 = 0;
    int *flag = NULL;

    switch (comp) {
        case '3':
            flag = &warned_3;
            break;
        case '4':
            flag = &warned_4;
            break;
        case '5':
            flag = &warned_5;
            break;
        default:
            return;
    }

    if (*flag) {
        return;
    }
    *flag = 1;

    if (context) {
        printf("  Warning: SPC/Fix component %c ignored for shell-2D mainline at %s:%d (supported: 1,2,6)\n",
               comp, context, line_no);
    } else {
        printf("  Warning: SPC/Fix component %c ignored for shell-2D mainline (supported: 1,2,6)\n",
               comp);
    }
}

static fem_error_t input_apply_constraint_component(int node_index,
                                                    char comp,
                                                    double value,
                                                    const char *context,
                                                    int line_no)
{
    CHECK_BOUNDS(node_index, g_num_nodes, "Node index");

    switch (comp) {
        case '1':
            g_node_bc_flags[node_index][0] = 1;
            g_node_displ[node_index][0] = value;
            g_node_bc_values[node_index][0] = value;
            return FEM_SUCCESS;
        case '2':
            g_node_bc_flags[node_index][1] = 1;
            g_node_displ[node_index][1] = value;
            g_node_bc_values[node_index][1] = value;
            return FEM_SUCCESS;
        case '6':
            g_fem_dof_per_node = 3;
            g_node_bc_flags[node_index][2] = 1;
            g_node_displ[node_index][2] = value;
            g_node_bc_values[node_index][2] = value;
            return FEM_SUCCESS;
        case '3':
        case '4':
        case '5':
            input_warn_unsupported_spc_component(comp, context, line_no);
            return FEM_SUCCESS;
        default:
            return FEM_SUCCESS;
    }
}

static fem_error_t input_apply_constraint_mask(int node_index,
                                               const char *component_field,
                                               double value,
                                               const char *context,
                                               int line_no)
{
    fem_error_t err;

    if (!component_field) {
        return FEM_SUCCESS;
    }

    for (size_t i = 0; i < strlen(component_field); ++i) {
        err = input_apply_constraint_component(node_index,
                                               component_field[i],
                                               value,
                                               context,
                                               line_no);
        CHECK_ERROR(err);
    }

    return FEM_SUCCESS;
}

static fem_error_t input_validate_map_node(int node_id, int node_index)
{
    fem_error_t err;

    if (node_id <= 0) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Node ID %d is outside supported range (must be > 0)", node_id);
    }
    if (node_index < 0) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Negative node index %d", node_index);
    }

    err = globals_reserve_node_ids(node_id + 1);
    CHECK_ERROR(err);

    if (node_index >= g_node_capacity) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Node index %d exceeds allocated capacity %d",
                         node_index, g_node_capacity);
    }

    if (g_node_id_to_index[node_id] != -1 && g_node_id_to_index[node_id] != node_index) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Duplicate definition for node ID %d", node_id);
    }

    g_node_id_to_index[node_id] = node_index;
    if (g_node_ids) {
        g_node_ids[node_index] = node_id;
    }
    return FEM_SUCCESS;
}

static fem_error_t input_get_node_index(int node_id, int *node_index)
{
    if (node_id <= 0) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Node ID %d referenced outside supported range", node_id);
    }
    if (node_id >= g_node_id_capacity || g_node_id_to_index[node_id] < 0) {
        return error_set(FEM_ERROR_INVALID_NODE,
                         "Node ID %d referenced before definition", node_id);
    }
    *node_index = g_node_id_to_index[node_id];
    return FEM_SUCCESS;
}

static fem_error_t input_validate_map_element(int element_id, int element_index)
{
    fem_error_t err;

    if (element_id <= 0) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Element ID %d is outside supported range (must be > 0)", element_id);
    }
    if (element_index < 0) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Negative element index %d", element_index);
    }

    err = globals_reserve_element_ids(element_id + 1);
    CHECK_ERROR(err);

    if (element_index >= g_element_capacity) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Element index %d exceeds allocated capacity %d",
                         element_index, g_element_capacity);
    }

    if (g_element_id_to_index[element_id] != -1 &&
        g_element_id_to_index[element_id] != element_index) {
        return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                         "Duplicate definition for element ID %d", element_id);
    }

    g_element_id_to_index[element_id] = element_index;
    if (g_element_ids) {
        g_element_ids[element_index] = element_id;
    }
    return FEM_SUCCESS;
}

static fem_error_t input_validate_map_material(int material_id, int material_index)
{
    fem_error_t err;

    if (material_id <= 0) {
        return error_set(FEM_ERROR_INVALID_MATERIAL,
                         "Material ID %d is outside supported range (must be > 0)", material_id);
    }
    if (material_index < 0) {
        return error_set(FEM_ERROR_INVALID_MATERIAL,
                         "Negative material index %d", material_index);
    }

    err = globals_reserve_material_ids(material_id + 1);
    CHECK_ERROR(err);

    if (material_index >= g_material_capacity) {
        return error_set(FEM_ERROR_INVALID_MATERIAL,
                         "Material index %d exceeds allocated capacity %d",
                         material_index, g_material_capacity);
    }

    if (g_material_id_to_index[material_id] != -1 &&
        g_material_id_to_index[material_id] != material_index) {
        return error_set(FEM_ERROR_INVALID_MATERIAL,
                         "Duplicate definition for material ID %d", material_id);
    }

    g_material_id_to_index[material_id] = material_index;
    if (g_material_ids) {
        g_material_ids[material_index] = material_id;
    }
    return FEM_SUCCESS;
}

static fem_error_t input_ensure_nastran_element_capacity(int required)
{
    if (required <= g_nastran_element_property_capacity) {
        return FEM_SUCCESS;
    }
    int new_capacity = g_nastran_element_property_capacity > 0
                           ? g_nastran_element_property_capacity
                           : INITIAL_ELEMENT_CAPACITY;
    while (new_capacity < required) {
        if (new_capacity > INT_MAX / 2) {
            new_capacity = required;
            break;
        }
        new_capacity *= 2;
    }
    if (new_capacity < required) {
        new_capacity = required;
    }

    int *tmp = realloc(g_nastran_element_property, (size_t)new_capacity * sizeof(int));
    if (!tmp) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to resize Nastran element property array");
    }
    for (int i = g_nastran_element_property_capacity; i < new_capacity; ++i) {
        tmp[i] = -1;
    }
    g_nastran_element_property = tmp;
    g_nastran_element_property_capacity = new_capacity;
    return FEM_SUCCESS;
}

static void input_nastran_normalize_line(char *line)
{
    if (line == NULL) return;

    char *src = line;
    char *dst = line;

    while (*src) {
        unsigned char c = (unsigned char)*src;
        if (c == 0xC2 && (unsigned char)src[1] == 0xA0) {
            *dst++ = ' ';
            src += 2;
            continue;
        }
        if (c == '\r' || c == '\n') {
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

static void input_nastran_trim(char *text)
{
    size_t len;
    size_t start = 0;

    if (text == NULL) return;

    len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len-1])) {
        text[len-1] = '\0';
        len--;
    }
    while (text[start] != '\0' && isspace((unsigned char)text[start])) {
        start++;
    }
    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

static int input_nastran_card_is_exact(const char *line, const char *card)
{
    size_t card_len;

    if (line == NULL || card == NULL) {
        return 0;
    }

    card_len = strlen(card);
    if (strncmp(line, card, card_len) != 0) {
        return 0;
    }

    switch (line[card_len]) {
        case '\0':
        case ' ':
        case '\t':
        case ',':
            return 1;
        default:
            return 0;
    }
}

static int input_nastran_line_has_continuation(const char *line)
{
    if (line == NULL) return 0;
    for (int i = (int)strlen(line) - 1; i >= 0; --i) {
        char c = line[i];
        if (c == ' ') continue;
        return c == '+';
    }
    return 0;
}

/* Main data reading function */
fem_error_t input_read_data(const char *filename)
{
    input_control_t input;
    fem_error_t err;

    coupled_case2d_reset_current();

    /* If the argument is a directory that contains parser outputs, shortcut here */
    if (input_parser_is_directory(filename) && input_parser_has_mesh_root(filename)) {
        printf("Detected parser output package in directory: %s\n", filename);
        err = input_read_parser_package(filename);
        return err;
    }
    
    /* Open input file */
    err = input_open_file(&input, filename);
    CHECK_ERROR(err);
    
    /* Detect file format */
    err = input_detect_format(&input);
    CHECK_ERROR_CLEANUP(err, input_close_file(&input));

    printf("Detected file format: %s\n",
           input.format == INPUT_FORMAT_NASTRAN ? "Nastran" : "Native");
    
    /* Read data based on format */
    switch (input.format) {
        case INPUT_FORMAT_NATIVE:
            err = input_read_header(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            
            err = input_read_nodes(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            
            err = input_read_elements(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            
            err = input_read_materials(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            
            err = input_read_boundary_conditions(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            
            err = input_read_loads(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            break;
            
        case INPUT_FORMAT_NASTRAN:
            err = input_read_nastran_bulk(&input);
            CHECK_ERROR_CLEANUP(err, input_close_file(&input));
            break;

        default:
            input_close_file(&input);
            return error_set(FEM_ERROR_INVALID_INPUT, "Unknown input format");
    }
    
    /* Close file */
    input_close_file(&input);
    
    /* Validate input data */
    err = input_validate_nodes();
    CHECK_ERROR(err);
    
    err = input_validate_elements();
    CHECK_ERROR(err);
    
    err = input_validate_materials();
    CHECK_ERROR(err);
    
    /* Update global analysis control */
    g_analysis.num_nodes = g_num_nodes;
    g_analysis.num_elements = g_num_elements;
    g_analysis.num_materials = g_num_materials;
    g_total_dof = g_num_nodes * g_fem_dof_per_node;

    err = input_read_fem_generic_contact_directives(filename);
    CHECK_ERROR(err);

    err = input_read_fem_solver_directives(filename);
    CHECK_ERROR(err);

    err = input_read_coupled_directives(filename);
    CHECK_ERROR(err);

    return FEM_SUCCESS;
}

/* Open input file */
fem_error_t input_open_file(input_control_t *input, const char *filename)
{
    if (input == NULL || filename == NULL) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Null pointer in input_open_file");
    }
    
    strncpy(input->filename, filename, MAX_FILENAME_LEN-1);
    input->filename[MAX_FILENAME_LEN-1] = '\0';
    
    input->file_ptr = fopen(filename, "r");
    CHECK_FILE(input->file_ptr, filename);
    
    input->line_number = 0;
    input->current_line[0] = '\0';
    
    return FEM_SUCCESS;
}

/* Close input file */
fem_error_t input_close_file(input_control_t *input)
{
    if (input != NULL && input->file_ptr != NULL) {
        fclose(input->file_ptr);
        input->file_ptr = NULL;
    }
    return FEM_SUCCESS;
}

/* Detect input format */
fem_error_t input_detect_format(input_control_t *input)
{
    char line[256];
    long file_pos = ftell(input->file_ptr);
    
    /* Read first few lines to detect format */
    while (fgets(line, sizeof(line), input->file_ptr)) {
        input_nastran_normalize_line(line);

        /* Skip blank lines and comments */
        if (line[0] == '\0' || line[0] == '#' || line[0] == '$') {
            continue;
        }
        
        /* Check for Nastran keywords */
        if (strncmp(line, "BEGIN BULK", 10) == 0 ||
            strncmp(line, "GRID", 4) == 0 ||
            strncmp(line, "CTRIA3", 6) == 0 ||
            strncmp(line, "CQUAD4", 6) == 0 ||
            strncmp(line, "CTRIA6", 6) == 0 ||
            strncmp(line, "SOL", 3) == 0 ||
            strncmp(line, "ID", 2) == 0 ||
            strncmp(line, "MAT1", 4) == 0) {
            input->format = INPUT_FORMAT_NASTRAN;
            fseek(input->file_ptr, file_pos, SEEK_SET);
            return FEM_SUCCESS;
        }
        
        /* Assume native format if no Nastran keywords found */
        input->format = INPUT_FORMAT_NATIVE;
        fseek(input->file_ptr, file_pos, SEEK_SET);
        return FEM_SUCCESS;
    }
    
    /* Default to native format */
    input->format = INPUT_FORMAT_NATIVE;
    fseek(input->file_ptr, file_pos, SEEK_SET);
    return FEM_SUCCESS;
}

/* Read header information */
fem_error_t input_read_header(input_control_t *input)
{
    fem_error_t err;
    
    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);
    
    /* Read title */
    err = input_read_line(input);
    CHECK_ERROR(err);
    strncpy(g_analysis.title, input->current_line, MAX_TITLE_LEN-1);
    g_analysis.title[MAX_TITLE_LEN-1] = '\0';
    
    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);
    
    /* Read problem size */
    err = input_read_line(input);
    CHECK_ERROR(err);
    
    if (sscanf(input->current_line, "%d %d", &g_num_nodes, &g_num_elements) != 2) {
        return error_set(FEM_ERROR_FILE_READ, 
                        "Error reading problem size at line %d", input->line_number);
    }
    
    CHECK_POSITIVE(g_num_nodes, "Number of nodes");
    CHECK_POSITIVE(g_num_elements, "Number of elements");

    err = globals_reserve_nodes(g_num_nodes);
    CHECK_ERROR(err);
    err = globals_reserve_elements(g_num_elements);
    CHECK_ERROR(err);
    err = globals_reserve_node_ids(g_num_nodes + 1);
    CHECK_ERROR(err);
    err = globals_reserve_element_ids(g_num_elements + 1);
    CHECK_ERROR(err);
    
    return FEM_SUCCESS;
}

/* Read node coordinates */
fem_error_t input_read_nodes(input_control_t *input)
{
    fem_error_t err;
    int node_id;

    err = globals_reserve_nodes(g_num_nodes);
    CHECK_ERROR(err);

    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);

    for (int i = 0; i < g_num_nodes; i++) {
        err = input_read_line(input);
        CHECK_ERROR(err);

        double x, y;
        if (sscanf(input->current_line, "%d %lf %lf", &node_id, &x, &y) != 3) {
            return error_set(FEM_ERROR_FILE_READ,
                             "Error reading node %d at line %d", i + 1, input->line_number);
        }

        globals_initialize_node_entry(i);
        g_node_coords[i][0] = x;
        g_node_coords[i][1] = y;
        g_node_coords[i][2] = 0.0; /* 2D */

        err = input_validate_map_node(node_id, i);
        CHECK_ERROR(err);
    }

    return FEM_SUCCESS;
}

/* Read element connectivity */
fem_error_t input_read_elements(input_control_t *input)
{
    fem_error_t err;
    int element_id = 0;

    err = globals_reserve_elements(g_num_elements);
    CHECK_ERROR(err);

    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);

    for (int i = 0; i < g_num_elements; i++) {
        err = input_read_line(input);
        CHECK_ERROR(err);

        /* Parse line to determine element type automatically */
        char *token;
        char line_copy[1024];
        strcpy(line_copy, input->current_line);

        /* Count tokens to determine element type */
        int token_count = 0;
        int nodes[MAX_NODES_PER_ELEMENT];

        token = strtok(line_copy, " \t");
        if (token == NULL) {
            return error_set(FEM_ERROR_FILE_READ,
                             "Missing element identifier at line %d", input->line_number);
        }
        element_id = atoi(token); /* First tokenは要素ID */

        globals_initialize_element_entry(i);
        err = input_validate_map_element(element_id, i);
        CHECK_ERROR(err);

        /* Read node IDs */
        while ((token = strtok(NULL, " \t")) && token_count < MAX_NODES_PER_ELEMENT) {
            nodes[token_count] = atoi(token);
            token_count++;
        }

        /* Determine element type based on node count */
        int element_type;
        int nodes_per_element;
        switch (token_count) {
            case 3:
                element_type = ELEMENT_T3;
                nodes_per_element = 3;
                break;
            case 4:
                element_type = ELEMENT_Q4;
                nodes_per_element = 4;
                break;
            case 6:
                element_type = ELEMENT_T6;
                nodes_per_element = 6;
                break;
            case 9:
                element_type = ELEMENT_Q9;
                nodes_per_element = 9;
                break;
            default:
                return error_set(FEM_ERROR_FILE_READ,
                                "Unsupported element type with %d nodes at line %d",
                                token_count, input->line_number);
        }

        /* Store element data */
        for (int j = 0; j < nodes_per_element; j++) {
            int node_index = -1;
            err = input_get_node_index(nodes[j], &node_index);
            CHECK_ERROR(err);
            g_element_nodes[i][j] = node_index;
        }

        /* Set element properties */
        g_element_type[i] = element_type;
        g_element_material[i] = 0; /* Default material */

        /* Initialize unused nodes */
        for (int j = nodes_per_element; j < MAX_NODES_PER_ELEMENT; j++) {
            g_element_nodes[i][j] = -1;
        }
    }
    
    return FEM_SUCCESS;
}

/* Read material properties */
fem_error_t input_read_materials(input_control_t *input)
{
    fem_error_t err;

    err = globals_reserve_materials(1);
    CHECK_ERROR(err);
    err = globals_reserve_material_ids(2);
    CHECK_ERROR(err);
    globals_initialize_material_entry(0);

    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);

    /* Read material properties (E, nu) */
    err = input_read_line(input);
    CHECK_ERROR(err);

    if (sscanf(input->current_line, "%lf %lf",
               &g_material_props[0][0], &g_material_props[0][1]) != 2) {
        return error_set(FEM_ERROR_FILE_READ,
                        "Error reading material properties at line %d", input->line_number);
    }

    /* Set default values */
    g_material_props[0][2] = 1.0;    /* thickness */
    g_material_props[0][3] = 1.0;    /* density */
    g_material_type[0] = MATERIAL_PLANE_STRESS;
    g_num_materials = 1;
    err = input_validate_map_material(1, 0); /* Native format: assign default material ID = 1 */
    CHECK_ERROR(err);

    CHECK_POSITIVE(g_material_props[0][0], "Young's modulus");
    if (g_material_props[0][1] >= 0.5) {
        return error_set(FEM_ERROR_INVALID_MATERIAL, "Poisson's ratio must be < 0.5");
    }

    return FEM_SUCCESS;
}

/* Read boundary conditions */
fem_error_t input_read_boundary_conditions(input_control_t *input)
{
    fem_error_t err;
    char line[256];
    int node_id, bc_flags[3];
    double prescribed_values[3];
    
    /* Skip blank lines */
    err = input_skip_blank_lines(input);
    CHECK_ERROR(err);
    
    while (1) {
        long pos = ftell(input->file_ptr);
        if (!fgets(line, sizeof(line), input->file_ptr)) {
            break;
        }
        
        /* Check for end of boundary conditions */
        if (strncmp(line, "point", 5) == 0 || strncmp(line, "load", 4) == 0 || strncmp(line, "end", 3) == 0) {
            fseek(input->file_ptr, pos, SEEK_SET);
            input->line_number--;
            break;
        }
        
        input->line_number++;
        
        if (sscanf(line, "%d %d %d %d %lf %lf %lf", 
                   &node_id, &bc_flags[0], &bc_flags[1], &bc_flags[2],
                   &prescribed_values[0], &prescribed_values[1], &prescribed_values[2]) >= 4) {
            
            int node_index = -1;
            err = input_get_node_index(node_id, &node_index);
            CHECK_ERROR(err);
            
            g_node_bc_flags[node_index][0] = bc_flags[0];
            g_node_bc_flags[node_index][1] = bc_flags[1];
            g_node_bc_flags[node_index][2] = bc_flags[2];
            if (bc_flags[0]) {
                g_node_displ[node_index][0] = prescribed_values[0];
                g_node_bc_values[node_index][0] = prescribed_values[0];
            }
            if (bc_flags[1]) {
                g_node_displ[node_index][1] = prescribed_values[1];
                g_node_bc_values[node_index][1] = prescribed_values[1];
            }
            if (bc_flags[2]) {
                g_fem_dof_per_node = 3;
                g_node_displ[node_index][2] = prescribed_values[2];
                g_node_bc_values[node_index][2] = prescribed_values[2];
            }
        }
    }
    
    return FEM_SUCCESS;
}

/* Read load conditions */
fem_error_t input_read_loads(input_control_t *input)
{
    fem_error_t err;
    int done = 0;

    while (!done) {
        err = input_read_line(input);
        if (err != FEM_SUCCESS) {
            /* No explicit load section is acceptable */
            return FEM_SUCCESS;
        }

        input_trim(input->current_line);
        if (input_is_blank_or_comment(input->current_line)) {
            continue;
        }

        if (strncmp(input->current_line, "end", 3) == 0) {
            break;
        } else if (strncmp(input->current_line, "body", 4) == 0) {
            double fx = 0.0, fy = 0.0, fz = 0.0;
            err = input_read_line(input);
            CHECK_ERROR(err);
            if (sscanf(input->current_line, "%lf %lf %lf", &fx, &fy, &fz) < 2) {
                return error_set(FEM_ERROR_FILE_READ,
                                 "Invalid body force specification at line %d",
                                 input->line_number);
            }
            g_body_force[0] = fx;
            g_body_force[1] = fy;
            g_body_force[2] = fz;
            g_has_body_force = 1;
        } else if (strncmp(input->current_line, "press", 5) == 0) {
            double pressure;

            err = input_read_line(input);
            CHECK_ERROR(err);
            if (sscanf(input->current_line, "%lf", &pressure) != 1) {
                return error_set(FEM_ERROR_FILE_READ,
                                 "Invalid pressure specification at line %d",
                                 input->line_number);
            }
            g_pressure_value = pressure;
            g_has_pressure = 1;
            g_num_pressure_surfaces = 0;

            /* Attempt to read optional surface definitions */
            while (1) {
                long current_pos = ftell(input->file_ptr);
                int current_line = input->line_number;

                fem_error_t line_err = input_read_line(input);
                if (line_err != FEM_SUCCESS) {
                    /* Reached EOF or next section */
                    break;
                }

                input_trim(input->current_line);
                if (input_is_blank_or_comment(input->current_line)) {
                    continue;
                }

                if (strncmp(input->current_line, "end", 3) == 0 ||
                    strncmp(input->current_line, "body", 4) == 0 ||
                    strncmp(input->current_line, "tract", 5) == 0 ||
                    strncmp(input->current_line, "point", 5) == 0 ||
                    strncmp(input->current_line, "load", 4) == 0) {
                    /* Next section begins here */
                    fseek(input->file_ptr, current_pos, SEEK_SET);
                    input->line_number = current_line;
                    break;
                }

                if (g_num_pressure_surfaces >= MAX_TRACTION_SURFACES) {
                    return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                                     "Exceeded maximum pressure surfaces (%d)",
                                     MAX_TRACTION_SURFACES);
                }

                int node_ids[MAX_SURFACE_NODES] = {0};
                int parsed = sscanf(input->current_line,
                                    "%d %d %d",
                                    &node_ids[0], &node_ids[1], &node_ids[2]);
                if (parsed != MAX_SURFACE_NODES) {
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Invalid pressure surface definition at line %d",
                                     input->line_number);
                }

                err = input_store_pressure_surface_from_node_ids(
                    g_num_pressure_surfaces,
                    MAX_SURFACE_NODES,
                    node_ids,
                    g_pressure_value);
                CHECK_ERROR(err);
                g_num_pressure_surfaces++;
            }
        } else if (strncmp(input->current_line, "tract", 5) == 0) {
            int ntrs = 0;
            err = input_read_line(input);
            CHECK_ERROR(err);
            if (sscanf(input->current_line, "%d", &ntrs) != 1 || ntrs < 0) {
                return error_set(FEM_ERROR_FILE_READ,
                                 "Invalid traction count at line %d",
                                 input->line_number);
            }

            for (int t = 0; t < ntrs; t++) {
                int nodes[MAX_SURFACE_NODES] = {0};
                double traction[3] = {0.0, 0.0, 0.0};
                int parsed;

                err = input_read_line(input);
                CHECK_ERROR(err);
                parsed = sscanf(input->current_line,
                                "%d %d %d %lf %lf %lf",
                                &nodes[0], &nodes[1], &nodes[2],
                                &traction[0], &traction[1], &traction[2]);
                if (parsed < 5) {
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Invalid traction entry at line %d",
                                     input->line_number);
                }
                if (g_num_tractions >= MAX_TRACTION_SURFACES) {
                    return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                                     "Exceeded maximum traction surfaces (%d)",
                                     MAX_TRACTION_SURFACES);
                }

                for (int k = 0; k < MAX_SURFACE_NODES; k++) {
                    int node_index = -1;
                    err = input_get_node_index(nodes[k], &node_index);
                    CHECK_ERROR(err);
                    g_traction_surfaces[g_num_tractions][k] = node_index;
                }
                g_traction_values[g_num_tractions][0] = traction[0];
                g_traction_values[g_num_tractions][1] = traction[1];
                g_traction_values[g_num_tractions][2] = traction[2];
                g_num_tractions++;
            }
        } else if (strncmp(input->current_line, "point", 5) == 0 ||
                   strncmp(input->current_line, "load", 4) == 0) {
            while (1) {
                fem_error_t inner_err = input_read_line(input);
                if (inner_err != FEM_SUCCESS) {
                    done = 1;
                    break;
                }
                input_trim(input->current_line);
                if (input_is_blank_or_comment(input->current_line)) {
                    continue;
                }
                if (strncmp(input->current_line, "end", 3) == 0) {
                    done = 1;
                    break;
                }

                int node_id;
                double fx = 0.0, fy = 0.0, fz = 0.0;
                int values = sscanf(input->current_line,
                                    "%d %lf %lf %lf",
                                    &node_id, &fx, &fy, &fz);
                if (values < 3) {
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Invalid point load specification at line %d",
                                     input->line_number);
                }
                int node_index = -1;
                err = input_get_node_index(node_id, &node_index);
                CHECK_ERROR(err);
                g_node_force[node_index][0] += fx;
                g_node_force[node_index][1] += fy;
                if (values >= 4) {
                    g_node_force[node_index][2] += fz;
                }
            }
        } else {
            /* Unknown token; ignore to remain permissive */
            continue;
        }
    }

    return FEM_SUCCESS;
}

/* Utility functions implementation */
fem_error_t input_skip_blank_lines(input_control_t *input)
{
    char line[256];
    
    while (fgets(line, sizeof(line), input->file_ptr)) {
        input->line_number++;
        
        /* Skip blank lines and comments */
        if (strlen(line) > 1 && line[0] != '#' && line[0] != '\n') {
            /* Put back the non-blank line */
            fseek(input->file_ptr, -(long)strlen(line), SEEK_CUR);
            input->line_number--;
            break;
        }
    }
    
    return FEM_SUCCESS;
}

fem_error_t input_read_line(input_control_t *input)
{
    if (!fgets(input->current_line, sizeof(input->current_line), input->file_ptr)) {
        return error_set(FEM_ERROR_FILE_READ, "Unexpected end of file at line %d", input->line_number);
    }
    
    input->line_number++;
    
    /* Remove newline character */
    int len = strlen(input->current_line);
    if (len > 0 && input->current_line[len-1] == '\n') {
        input->current_line[len-1] = '\0';
    }
    
    return FEM_SUCCESS;
}

/* Validation functions */
fem_error_t input_validate_nodes(void)
{
    if (g_num_nodes <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "No nodes defined");
    }
    return FEM_SUCCESS;
}

fem_error_t input_validate_elements(void)
{
    if (g_num_elements <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "No elements defined");
    }
    return FEM_SUCCESS;
}

fem_error_t input_validate_materials(void)
{
    if (g_num_materials <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "No materials defined");
    }
    return FEM_SUCCESS;
}

/* Nastran bulk data reader */
fem_error_t input_read_nastran_bulk(input_control_t *input)
{
    char line[256];
    fem_error_t err;
    int found_begin_bulk = 0;

    printf("Reading Nastran bulk data format...\n");

    /* Find BEGIN BULK section */
    while (fgets(line, sizeof(line), input->file_ptr)) {
        input->line_number++;
        input_nastran_normalize_line(line);

        /* Skip comments and empty lines */
        if (line[0] == '$' || line[0] == '\0') {
            continue;
        }

        /* Check for BEGIN BULK */
        if (strncmp(line, "BEGIN BULK", 10) == 0) {
            found_begin_bulk = 1;
            printf("  Found BEGIN BULK at line %d\n", input->line_number);
            break;
        }
    }

    if (!found_begin_bulk) {
        return error_set(FEM_ERROR_FILE_READ, "BEGIN BULK not found in Nastran file");
    }

    /* Initialize counters */
    g_num_nodes = 0;
    g_num_elements = 0;
    g_num_materials = 0;
    g_nastran_pshell_count = 0;
    err = input_ensure_nastran_element_capacity(INITIAL_ELEMENT_CAPACITY);
    CHECK_ERROR(err);
    for (int i = 0; i < g_nastran_element_property_capacity; ++i) {
        g_nastran_element_property[i] = -1;
    }

    /* Parse bulk data */
    while (fgets(line, sizeof(line), input->file_ptr)) {
        input->line_number++;
        input_nastran_normalize_line(line);

        /* Skip comments and empty lines */
        if (line[0] == '$' || line[0] == '\0') {
            continue;
        }

        /* Check for end of bulk data */
        if (strncmp(line, "ENDDATA", 7) == 0) {
            printf("  Found ENDDATA at line %d\n", input->line_number);
            break;
        }

        /* Parse different card types */
        if (strncmp(line, "GRID", 4) == 0) {
            err = input_parse_nastran_grid(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "CTRIA3", 6) == 0) {
            err = input_parse_nastran_ctria3(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "CQUAD4", 6) == 0) {
            err = input_parse_nastran_cquad4(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "CTRIA6", 6) == 0) {
            err = input_parse_nastran_ctria6(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "PARAM", 5) == 0) {
            double k6rot = 0.0;
            if (input_parse_param_k6rot_value(line, &k6rot)) {
                g_shell_k6rot = k6rot;
                g_fem_dof_per_node = 3;
            }
        } else if (strncmp(line, "MAT1", 4) == 0) {
            err = input_parse_nastran_mat1(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "PSHELL", 6) == 0) {
            err = input_parse_nastran_pshell(input, line);
            CHECK_ERROR(err);
        } else if (input_nastran_card_is_exact(line, "PLOAD")) {
            err = input_parse_nastran_pload(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "SPC", 3) == 0) {
            err = input_parse_nastran_spc(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "FORCE", 5) == 0) {
            err = input_parse_nastran_force(input, line);
            CHECK_ERROR(err);
        }
    }

    err = input_nastran_finalize_properties();
    CHECK_ERROR(err);

    /* Set total DOF */
    g_total_dof = g_num_nodes * g_fem_dof_per_node;

    printf("  Nastran bulk data parsing complete:\n");
    printf("    Nodes: %d\n", g_num_nodes);
    printf("    Elements: %d\n", g_num_elements);
    printf("    Materials: %d\n", g_num_materials);
    return FEM_SUCCESS;
}

/* -------- Parser package reader (mesh/material/boundary) -------- */
static int input_parser_is_directory(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

static int input_parser_has_mesh_root(const char *path)
{
    char test_path[1024];
    snprintf(test_path, sizeof(test_path), "%s/mesh/mesh.dat", path);
    struct stat st;
    return stat(test_path, &st) == 0 && S_ISREG(st.st_mode);
}

static const char *input_mbd_skip_leading_spaces(const char *text)
{
    const char *p = text;

    while (p && *p != '\0' && isspace((unsigned char)*p)) {
        ++p;
    }
    return p;
}

static int input_mbd_line_starts_with_token(const char *line, const char *token)
{
    const char *p;
    size_t token_len;

    if (!line || !token) {
        return 0;
    }

    p = input_mbd_skip_leading_spaces(line);
    token_len = strlen(token);
    if (strncmp(p, token, token_len) != 0) {
        return 0;
    }
    return p[token_len] == '\0' || isspace((unsigned char)p[token_len]);
}

static int input_mbd_line_starts_with_prefix(const char *line, const char *prefix)
{
    const char *p;
    size_t prefix_len;

    if (!line || !prefix) {
        return 0;
    }

    p = input_mbd_skip_leading_spaces(line);
    prefix_len = strlen(prefix);
    return strncmp(p, prefix, prefix_len) == 0;
}

static void input_mbd_line_to_excerpt(const char *line, char *out, size_t out_size)
{
    size_t i = 0;

    if (!line || !out || out_size == 0) {
        return;
    }

    while (line[i] != '\0' && line[i] != '\n' && line[i] != '\r' && i + 1 < out_size) {
        out[i] = line[i];
        ++i;
    }
    out[i] = '\0';
}

static int input_parse_mbd_body_line(const char *line,
                                     mbd_body2d_t *body,
                                     int *body_index)
{
    int idx = -1;
    double x = 0.0;
    double y = 0.0;
    double theta = 0.0;
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_BODY")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_BODY %d %lf %lf %lf", &idx, &x, &y, &theta);
    if (scanned != 4 || idx < 0) {
        return -1;
    }
    if (!isfinite(x) || !isfinite(y) || !isfinite(theta)) {
        return -1;
    }

    *body_index = idx;
    if (mbd_body2d_init_pose(body, idx, x, y, theta) != FEM_SUCCESS) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_body_dyn_line(const char *line,
                                         mbd_body2d_t *body,
                                         int *body_index)
{
    int idx = -1;
    double mass = 0.0;
    double inertia = 0.0;
    double q[3];
    double v[3];
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_BODY_DYN")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_BODY_DYN %d %lf %lf %lf %lf %lf %lf %lf %lf",
                     &idx, &mass, &inertia,
                     &q[0], &q[1], &q[2],
                     &v[0], &v[1], &v[2]);
    if (scanned != 9 || idx < 0) {
        return -1;
    }

    *body_index = idx;
    if (mbd_body2d_init_dyn(body, idx, mass, inertia, q, v) != FEM_SUCCESS) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_gravity_line(const char *line, double gravity[2])
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_GRAVITY")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_GRAVITY %lf %lf", &gravity[0], &gravity[1]);
    if (scanned != 2) {
        return -1;
    }
    if (!isfinite(gravity[0]) || !isfinite(gravity[1])) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_body_ground_line(const char *line, int *body_index)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_BODY_GROUND")) {
        return 0;
    }
    if (!body_index) {
        return -1;
    }

    scanned = sscanf(line, "MBD_BODY_GROUND %d", body_index);
    if (scanned != 1 || *body_index < 0) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_force_line(const char *line,
                                      int *body_index,
                                      double force[3])
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_FORCE")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_FORCE %d %lf %lf %lf",
                     body_index, &force[0], &force[1], &force[2]);
    if (scanned != 4 || *body_index < 0) {
        return -1;
    }
    if (!isfinite(force[0]) || !isfinite(force[1]) || !isfinite(force[2])) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_body_circle_line(const char *line,
                                            int *body_id,
                                            double *radius,
                                            double *thickness)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_BODY_CIRCLE")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_BODY_CIRCLE %d %lf %lf",
                     body_id, radius, thickness);
    if (scanned != 3 || *body_id < 0) {
        return -1;
    }
    if (!isfinite(*radius) || *radius <= 0.0 ||
        !isfinite(*thickness) || *thickness <= 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_contact_halfspace_line(const char *line,
                                                  int *halfspace_id,
                                                  double point[2],
                                                  double normal[2],
                                                  double *thickness)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_HALFSPACE")) {
        return 0;
    }

    scanned = sscanf(line, "MBD_CONTACT_HALFSPACE %d %lf %lf %lf %lf %lf",
                     halfspace_id,
                     &point[0],
                     &point[1],
                     &normal[0],
                     &normal[1],
                     thickness);
    if (scanned != 6 || *halfspace_id < 0) {
        return -1;
    }
    if (!isfinite(point[0]) || !isfinite(point[1]) ||
        !isfinite(normal[0]) || !isfinite(normal[1]) ||
        !isfinite(*thickness) || *thickness <= 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_contact_surface_polyline_line(const char *line,
                                                         int *surface_id,
                                                         int *body_id,
                                                         char *csv_path,
                                                         size_t csv_path_size)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_SURFACE_POLYLINE")) {
        return 0;
    }
    if (!csv_path || csv_path_size == 0) {
        return -1;
    }

    csv_path[0] = '\0';
    scanned = sscanf(line, "MBD_CONTACT_SURFACE_POLYLINE %d %d %1023s",
                     surface_id, body_id, csv_path);
    if (scanned != 3 || *surface_id < 0 || *body_id < 0 || csv_path[0] == '\0') {
        return -1;
    }
    csv_path[csv_path_size - 1] = '\0';
    return 1;
}

static int input_parse_mbd_contact_pair_line(const char *line,
                                             int *pair_id,
                                             int *body_i,
                                             int *body_j,
                                             double *k_n,
                                             double *c_n,
                                             double *mu_base)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_PAIR")) {
        return 0;
    }

    if (!mu_base) {
        return -1;
    }

    *mu_base = 0.0;
    scanned = sscanf(line, "MBD_CONTACT_PAIR %d %d %d %lf %lf %lf",
                     pair_id, body_i, body_j, k_n, c_n, mu_base);
    if ((scanned != 5 && scanned != 6) || *pair_id < 0 || *body_i < 0 || *body_j < 0) {
        return -1;
    }
    if (!isfinite(*k_n) || *k_n <= 0.0 ||
        !isfinite(*c_n) || *c_n < 0.0 ||
        !isfinite(*mu_base) || *mu_base < 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_contact_pair_generic_line(const char *line,
                                                     int *pair_id,
                                                     int *surface_i,
                                                     int *surface_j,
                                                     double *k_n,
                                                     double *c_n,
                                                     double *mu_base,
                                                     double *mu_static,
                                                     double *mu_dynamic,
                                                     double *v_ref,
                                                     double *v_smooth)
{
    char buffer[1024];
    char *tokens[16];
    char *token = NULL;
    char *endptr = NULL;
    long parsed_long = 0;
    int token_count = 0;
    int i = 0;

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_PAIR_GENERIC")) {
        return 0;
    }
    if (!mu_base || !mu_static || !mu_dynamic || !v_ref || !v_smooth) {
        return -1;
    }
    if (strlen(line) >= sizeof(buffer)) {
        return -1;
    }

    strncpy(buffer, line, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    for (token = strtok(buffer, " \t\r\n");
         token && token_count < (int)(sizeof(tokens) / sizeof(tokens[0]));
         token = strtok(NULL, " \t\r\n")) {
        tokens[token_count++] = token;
    }
    if (token_count != 6 && token_count != 7 && token_count != 10) {
        return -1;
    }
    if (strcmp(tokens[0], "MBD_CONTACT_PAIR_GENERIC") != 0) {
        return -1;
    }

    for (i = 1; i <= 3; ++i) {
        errno = 0;
        endptr = NULL;
        parsed_long = strtol(tokens[i], &endptr, 10);
        if (errno != 0 || !endptr || *endptr != '\0' || parsed_long < 0 || parsed_long > INT_MAX) {
            return -1;
        }
        if (i == 1) {
            *pair_id = (int)parsed_long;
        } else if (i == 2) {
            *surface_i = (int)parsed_long;
        } else {
            *surface_j = (int)parsed_long;
        }
    }
    for (i = 4; i <= 5; ++i) {
        errno = 0;
        endptr = NULL;
        if (i == 4) {
            *k_n = strtod(tokens[i], &endptr);
        } else {
            *c_n = strtod(tokens[i], &endptr);
        }
        if (errno != 0 || !endptr || *endptr != '\0') {
            return -1;
        }
    }
    *mu_base = 0.0;
    *mu_static = 0.0;
    *mu_dynamic = 0.0;
    *v_ref = 1.0e-1;
    *v_smooth = 1.0e-3;
    if (!isfinite(*k_n) || *k_n <= 0.0 ||
        !isfinite(*c_n) || *c_n < 0.0) {
        return -1;
    }
    if (token_count == 7) {
        errno = 0;
        endptr = NULL;
        *mu_base = strtod(tokens[6], &endptr);
        if (errno != 0 || !endptr || *endptr != '\0' ||
            !isfinite(*mu_base) || *mu_base < 0.0) {
            return -1;
        }
        *mu_static = *mu_base;
        *mu_dynamic = *mu_base;
    }
    if (token_count == 10) {
        errno = 0;
        endptr = NULL;
        *mu_static = strtod(tokens[6], &endptr);
        if (errno != 0 || !endptr || *endptr != '\0' ||
            !isfinite(*mu_static) || *mu_static < 0.0) {
            return -1;
        }
        errno = 0;
        endptr = NULL;
        *mu_dynamic = strtod(tokens[7], &endptr);
        if (errno != 0 || !endptr || *endptr != '\0' ||
            !isfinite(*mu_dynamic) || *mu_dynamic < 0.0) {
            return -1;
        }
        errno = 0;
        endptr = NULL;
        *v_ref = strtod(tokens[8], &endptr);
        if (errno != 0 || !endptr || *endptr != '\0' ||
            !isfinite(*v_ref) || *v_ref < 0.0) {
            return -1;
        }
        errno = 0;
        endptr = NULL;
        *v_smooth = strtod(tokens[9], &endptr);
        if (errno != 0 || !endptr || *endptr != '\0' ||
            !isfinite(*v_smooth) || *v_smooth < 0.0) {
            return -1;
        }
    }
    *mu_base = *mu_dynamic;
    return 1;
}

static int input_parse_mbd_contact_pair_halfspace_line(const char *line,
                                                       int *pair_id,
                                                       int *body_circle,
                                                       int *halfspace_id,
                                                       double *k_n,
                                                       double *c_n,
                                                       double *mu_base)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_PAIR_HALFSPACE")) {
        return 0;
    }

    if (!mu_base) {
        return -1;
    }

    *mu_base = 0.0;
    scanned = sscanf(line, "MBD_CONTACT_PAIR_HALFSPACE %d %d %d %lf %lf %lf",
                     pair_id, body_circle, halfspace_id, k_n, c_n, mu_base);
    if ((scanned != 5 && scanned != 6) || *pair_id < 0 || *body_circle < 0 || *halfspace_id < 0) {
        return -1;
    }
    if (!isfinite(*k_n) || *k_n <= 0.0 ||
        !isfinite(*c_n) || *c_n < 0.0 ||
        !isfinite(*mu_base) || *mu_base < 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_contact_coupling_mode_line(
    const char *line,
    mbd_contact_coupling_mode_t *mode_out)
{
    char mode_text[64];

    if (!input_mbd_line_starts_with_token(line, "MBD_CONTACT_COUPLING_MODE")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    if (sscanf(line, "MBD_CONTACT_COUPLING_MODE %63s", mode_text) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "ONE_WAY") == 0) {
        *mode_out = MBD_CONTACT_COUPLING_MODE_ONE_WAY;
        return 1;
    }
    if (strcmp(mode_text, "LAGGED_STIFFNESS") == 0) {
        *mode_out = MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS;
        return 1;
    }
    return -1;
}

static int input_parse_mbd_local_feedback_mode_line(
    const char *line,
    mbd_local_feedback_mode_t *mode_out)
{
    char mode_text[64];

    if (!input_mbd_line_starts_with_token(line, "MBD_LOCAL_FEEDBACK_MODE")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    if (sscanf(line, "MBD_LOCAL_FEEDBACK_MODE %63s", mode_text) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "NONE") == 0) {
        *mode_out = MBD_LOCAL_FEEDBACK_MODE_NONE;
        return 1;
    }
    if (strcmp(mode_text, "LAGGED_REDUCED") == 0) {
        *mode_out = MBD_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED;
        return 1;
    }
    if (strcmp(mode_text, "SAME_TIME_REDUCED") == 0) {
        *mode_out = MBD_LOCAL_FEEDBACK_MODE_SAME_TIME_REDUCED;
        return 1;
    }
    return -1;
}

static int input_parse_mbd_local_contact_monolithic_line(
    const char *line,
    mbd_local_contact_monolithic_mode_t *mode_out)
{
    char mode_text[64];

    if (!input_mbd_line_starts_with_token(line, "MBD_LOCAL_CONTACT_MONOLITHIC")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    if (sscanf(line, "MBD_LOCAL_CONTACT_MONOLITHIC %63s", mode_text) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "NONE") == 0) {
        *mode_out = MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE;
        return 1;
    }
    if (strcmp(mode_text, "PATCH_MVP_CIRCLE") == 0) {
        *mode_out = MBD_LOCAL_CONTACT_MONOLITHIC_MODE_PATCH_MVP_CIRCLE;
        return 1;
    }
    return -1;
}

static int input_parse_mbd_monolithic_proper_mode_line(
    const char *line,
    mbd_monolithic_proper_mode_t *mode_out)
{
    char mode_text[64];

    if (!input_mbd_line_starts_with_token(line, "MBD_MONOLITHIC_PROPER_MODE")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    if (sscanf(line, "MBD_MONOLITHIC_PROPER_MODE %63s", mode_text) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "NONE") == 0) {
        *mode_out = MBD_MONOLITHIC_PROPER_MODE_NONE;
        return 1;
    }
    if (strcmp(mode_text, "BLOCK_NEWTON_V1") == 0) {
        *mode_out = MBD_MONOLITHIC_PROPER_MODE_BLOCK_NEWTON_V1;
        return 1;
    }
    return -1;
}

static int input_parse_mbd_monolithic_proper_context_line(
    const char *line,
    mbd_monolithic_proper_context2d_t *context_out)
{
    int converged_flag = 0;
    int scanned = 0;

    if (!input_mbd_line_starts_with_token(line, "MBD_MONOLITHIC_PROPER_CONTEXT")) {
        return 0;
    }
    if (!context_out) {
        return -1;
    }

    memset(context_out, 0, sizeof(*context_out));
    scanned = sscanf(line,
                     "MBD_MONOLITHIC_PROPER_CONTEXT %d %d %lf %lf %lf %lf %lf %lf %d",
                     &context_out->context_step,
                     &context_out->iter_index,
                     &context_out->k_contact_eff,
                     &context_out->mu_eff,
                     &context_out->stress_residual,
                     &context_out->displacement_residual,
                     &context_out->contact_parameter_residual,
                     &context_out->fem_residual,
                     &converged_flag);
    if (scanned != 9) {
        return -1;
    }
    context_out->converged_flag = converged_flag;
    context_out->is_defined = 1;
    if (context_out->context_step < 0 || context_out->iter_index < 0 ||
        !isfinite(context_out->k_contact_eff) || context_out->k_contact_eff < 0.0 ||
        !isfinite(context_out->mu_eff) || context_out->mu_eff < 0.0 ||
        !isfinite(context_out->stress_residual) || context_out->stress_residual < 0.0 ||
        !isfinite(context_out->displacement_residual) ||
        context_out->displacement_residual < 0.0 ||
        !isfinite(context_out->contact_parameter_residual) ||
        context_out->contact_parameter_residual < 0.0 ||
        !isfinite(context_out->fem_residual) || context_out->fem_residual < 0.0 ||
        (context_out->converged_flag != 0 && context_out->converged_flag != 1)) {
        return -1;
    }
    return 1;
}

static int input_parse_mbd_local_feedback_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_LOCAL_FEEDBACK_FILE")) {
        return 0;
    }
    if (!path_out || path_out_size == 0) {
        return -1;
    }

    path_out[0] = '\0';
    scanned = sscanf(line, "MBD_LOCAL_FEEDBACK_FILE %1023s", path_out);
    if (scanned != 1 || path_out[0] == '\0') {
        return -1;
    }
    path_out[path_out_size - 1] = '\0';
    return 1;
}

static int input_parse_mbd_local_contact_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_LOCAL_CONTACT_FILE")) {
        return 0;
    }
    if (!path_out || path_out_size == 0) {
        return -1;
    }

    path_out[0] = '\0';
    scanned = sscanf(line, "MBD_LOCAL_CONTACT_FILE %1023s", path_out);
    if (scanned != 1 || path_out[0] == '\0') {
        return -1;
    }
    path_out[path_out_size - 1] = '\0';
    return 1;
}

static int input_parse_mbd_ehl_file_line(
    const char *line,
    char *path_out,
    size_t path_out_size)
{
    int scanned;

    if (!input_mbd_line_starts_with_token(line, "MBD_EHL_FILE")) {
        return 0;
    }
    if (!path_out || path_out_size == 0) {
        return -1;
    }

    path_out[0] = '\0';
    scanned = sscanf(line, "MBD_EHL_FILE %1023s", path_out);
    if (scanned != 1 || path_out[0] == '\0') {
        return -1;
    }
    path_out[path_out_size - 1] = '\0';
    return 1;
}

static void input_mbd_body_tracker_zero(input_mbd_body_tracker_t *tracker)
{
    if (!tracker) {
        return;
    }
    memset(tracker, 0, sizeof(*tracker));
}

static void input_mbd_body_tracker_free(input_mbd_body_tracker_t *tracker)
{
    if (!tracker) {
        return;
    }

    free(tracker->body_seen);
    free(tracker->body_defined_line);
    free(tracker->body_defined_kind);
    free(tracker->body_ground_seen);
    free(tracker->body_ground_line);
    free(tracker->circle_seen);
    free(tracker->circle_line);
    free(tracker->body_force_seen);
    free(tracker->body_force_line);
    free(tracker->body_force_accum);
    input_mbd_body_tracker_zero(tracker);
}

static fem_error_t input_mbd_body_tracker_reserve(input_mbd_body_tracker_t *tracker,
                                                  int required_capacity)
{
    int *body_seen = NULL;
    int *body_defined_line = NULL;
    input_mbd_body_input_kind_t *body_defined_kind = NULL;
    int *body_ground_seen = NULL;
    int *body_ground_line = NULL;
    int *circle_seen = NULL;
    int *circle_line = NULL;
    int *body_force_seen = NULL;
    int *body_force_line = NULL;
    double (*body_force_accum)[3] = NULL;
    int index = 0;

    CHECK_NULL(tracker, "input mbd body tracker");

    if (required_capacity < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "input body tracker capacity %d must be non-negative",
                         required_capacity);
    }
    if (required_capacity <= tracker->capacity) {
        return FEM_SUCCESS;
    }
    if (required_capacity == 0) {
        return FEM_SUCCESS;
    }

    body_seen = (int *) calloc((size_t) required_capacity, sizeof(*body_seen));
    body_defined_line = (int *) calloc((size_t) required_capacity, sizeof(*body_defined_line));
    body_defined_kind = (input_mbd_body_input_kind_t *) calloc((size_t) required_capacity,
                                                               sizeof(*body_defined_kind));
    body_ground_seen = (int *) calloc((size_t) required_capacity, sizeof(*body_ground_seen));
    body_ground_line = (int *) calloc((size_t) required_capacity, sizeof(*body_ground_line));
    circle_seen = (int *) calloc((size_t) required_capacity, sizeof(*circle_seen));
    circle_line = (int *) calloc((size_t) required_capacity, sizeof(*circle_line));
    body_force_seen = (int *) calloc((size_t) required_capacity, sizeof(*body_force_seen));
    body_force_line = (int *) calloc((size_t) required_capacity, sizeof(*body_force_line));
    body_force_accum = (double (*)[3]) calloc((size_t) required_capacity, sizeof(*body_force_accum));
    if (!body_seen || !body_defined_line || !body_defined_kind || !body_ground_seen ||
        !body_ground_line || !circle_seen || !circle_line || !body_force_seen ||
        !body_force_line || !body_force_accum) {
        free(body_seen);
        free(body_defined_line);
        free(body_defined_kind);
        free(body_ground_seen);
        free(body_ground_line);
        free(circle_seen);
        free(circle_line);
        free(body_force_seen);
        free(body_force_line);
        free(body_force_accum);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Failed to allocate input body tracker for capacity %d",
                         required_capacity);
    }

    if (tracker->capacity > 0) {
        memcpy(body_seen, tracker->body_seen, (size_t) tracker->capacity * sizeof(*body_seen));
        memcpy(body_defined_line,
               tracker->body_defined_line,
               (size_t) tracker->capacity * sizeof(*body_defined_line));
        memcpy(body_defined_kind,
               tracker->body_defined_kind,
               (size_t) tracker->capacity * sizeof(*body_defined_kind));
        memcpy(body_ground_seen,
               tracker->body_ground_seen,
               (size_t) tracker->capacity * sizeof(*body_ground_seen));
        memcpy(body_ground_line,
               tracker->body_ground_line,
               (size_t) tracker->capacity * sizeof(*body_ground_line));
        memcpy(circle_seen, tracker->circle_seen, (size_t) tracker->capacity * sizeof(*circle_seen));
        memcpy(circle_line, tracker->circle_line, (size_t) tracker->capacity * sizeof(*circle_line));
        memcpy(body_force_seen,
               tracker->body_force_seen,
               (size_t) tracker->capacity * sizeof(*body_force_seen));
        memcpy(body_force_line,
               tracker->body_force_line,
               (size_t) tracker->capacity * sizeof(*body_force_line));
        memcpy(body_force_accum,
               tracker->body_force_accum,
               (size_t) tracker->capacity * sizeof(*body_force_accum));
    }
    for (index = tracker->capacity; index < required_capacity; ++index) {
        body_defined_line[index] = -1;
        body_defined_kind[index] = INPUT_MBD_BODY_INPUT_NONE;
        body_ground_line[index] = -1;
        circle_line[index] = -1;
        body_force_line[index] = -1;
    }

    free(tracker->body_seen);
    free(tracker->body_defined_line);
    free(tracker->body_defined_kind);
    free(tracker->body_ground_seen);
    free(tracker->body_ground_line);
    free(tracker->circle_seen);
    free(tracker->circle_line);
    free(tracker->body_force_seen);
    free(tracker->body_force_line);
    free(tracker->body_force_accum);
    tracker->body_seen = body_seen;
    tracker->body_defined_line = body_defined_line;
    tracker->body_defined_kind = body_defined_kind;
    tracker->body_ground_seen = body_ground_seen;
    tracker->body_ground_line = body_ground_line;
    tracker->circle_seen = circle_seen;
    tracker->circle_line = circle_line;
    tracker->body_force_seen = body_force_seen;
    tracker->body_force_line = body_force_line;
    tracker->body_force_accum = body_force_accum;
    tracker->capacity = required_capacity;
    return FEM_SUCCESS;
}

fem_error_t input_read_mbd_body_directives(const char *filename,
                                           mbd_system2d_t *system,
                                           int *saw_mbd_entry,
                                           int *first_mbd_line)
{
    FILE *fp = NULL;
    char line[512];
    char excerpt[160];
    input_mbd_body_tracker_t body_tracker __attribute__((cleanup(input_mbd_body_tracker_free)));
    int halfspace_seen[MBD_CONTACT2D_MAX_PAIRS];
    int halfspace_line[MBD_CONTACT2D_MAX_PAIRS];
    int line_no = 0;
    int contact_coupling_mode_seen = 0;
    int contact_coupling_mode_line = -1;
    int local_feedback_mode_seen = 0;
    int local_feedback_mode_line = -1;
    int local_contact_monolithic_seen = 0;
    int local_contact_monolithic_line = -1;
    int monolithic_proper_mode_seen = 0;
    int monolithic_proper_mode_line = -1;
    int monolithic_proper_context_seen = 0;
    int monolithic_proper_context_line = -1;
    int local_feedback_file_seen = 0;
    int local_feedback_file_line = -1;
    int local_contact_file_seen = 0;
    int local_contact_file_line = -1;
    int ehl_file_seen = 0;
    int ehl_file_line = -1;
    int i;

    CHECK_NULL(filename, "mbd input filename");
    CHECK_NULL(system, "mbd_system2d");

    if (saw_mbd_entry) {
        *saw_mbd_entry = 0;
    }
    if (first_mbd_line) {
        *first_mbd_line = 0;
    }

    input_mbd_body_tracker_zero(&body_tracker);
    memset(halfspace_seen, 0, sizeof(halfspace_seen));
    for (i = 0; i < MBD_CONTACT2D_MAX_PAIRS; ++i) {
        halfspace_line[i] = -1;
    }

    fp = fopen(filename, "r");
    if (!fp) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND,
                         "Cannot open MBD input file: %s",
                         filename);
    }

    while (fgets(line, sizeof(line), fp) != NULL) {
        mbd_body2d_t parsed_body;
        int body_index = -1;
        int parsed = 0;

        ++line_no;

        if (input_mbd_line_starts_with_prefix(line, "MBD_")) {
            if (saw_mbd_entry) {
                *saw_mbd_entry = 1;
            }
            if (first_mbd_line && *first_mbd_line == 0) {
                *first_mbd_line = line_no;
            }
        }

        parsed = input_parse_mbd_body_dyn_line(line, &parsed_body, &body_index);
        if (parsed == -1) {
            input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_BODY_DYN at line %d: '%s'",
                             MBD_DIAG_E_BODY_DYN_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            if (body_index < 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_BODY_DYN index %d at line %d is invalid",
                                 MBD_DIAG_E_BODY_RANGE,
                                 body_index,
                                 line_no);
            }
            CHECK_ERROR(input_mbd_body_tracker_reserve(&body_tracker, body_index + 1));
            CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));
            if (body_tracker.body_seen[body_index] &&
                body_tracker.body_defined_kind[body_index] == INPUT_MBD_BODY_INPUT_DYN) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Duplicate MBD_BODY_DYN id %d at line %d (first defined at line %d)",
                                 MBD_DIAG_E_DUP_BODY,
                                 body_index,
                                 line_no,
                                 body_tracker.body_defined_line[body_index]);
            }
            if (body_tracker.body_force_seen[body_index]) {
                parsed_body.force[0] = body_tracker.body_force_accum[body_index][0];
                parsed_body.force[1] = body_tracker.body_force_accum[body_index][1];
                parsed_body.force[2] = body_tracker.body_force_accum[body_index][2];
            }
            if (mbd_system2d_add_body(system, body_index, &parsed_body) != FEM_SUCCESS) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Failed to register MBD_BODY_DYN id %d at line %d",
                                 MBD_DIAG_E_BODY_RANGE,
                                 body_index,
                                 line_no);
            }
            body_tracker.body_seen[body_index] = 1;
            body_tracker.body_defined_line[body_index] = line_no;
            body_tracker.body_defined_kind[body_index] = INPUT_MBD_BODY_INPUT_DYN;
            continue;
        }

        parsed = input_parse_mbd_body_line(line, &parsed_body, &body_index);
        if (parsed == -1) {
            input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Invalid MBD_BODY at line %d: '%s'",
                             MBD_DIAG_E_BODY_PARSE, line_no, excerpt);
        }
        if (parsed == 1) {
            if (body_index < 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_BODY index %d at line %d is invalid",
                                 MBD_DIAG_E_BODY_RANGE,
                                 body_index,
                                 line_no);
            }
            CHECK_ERROR(input_mbd_body_tracker_reserve(&body_tracker, body_index + 1));
            CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));
            if (body_tracker.body_seen[body_index] &&
                body_tracker.body_defined_kind[body_index] == INPUT_MBD_BODY_INPUT_DYN) {
                continue;
            }
            if (body_tracker.body_seen[body_index]) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Duplicate MBD_BODY id %d at line %d (first defined at line %d)",
                                 MBD_DIAG_E_DUP_BODY,
                                 body_index,
                                 line_no,
                                 body_tracker.body_defined_line[body_index]);
            }
            if (body_tracker.body_force_seen[body_index]) {
                parsed_body.force[0] = body_tracker.body_force_accum[body_index][0];
                parsed_body.force[1] = body_tracker.body_force_accum[body_index][1];
                parsed_body.force[2] = body_tracker.body_force_accum[body_index][2];
            }
            if (mbd_system2d_add_body(system, body_index, &parsed_body) != FEM_SUCCESS) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Failed to register MBD_BODY id %d at line %d",
                                 MBD_DIAG_E_BODY_RANGE,
                                 body_index,
                                 line_no);
            }
            body_tracker.body_seen[body_index] = 1;
            body_tracker.body_defined_line[body_index] = line_no;
            body_tracker.body_defined_kind[body_index] = INPUT_MBD_BODY_INPUT_LEGACY;
            continue;
        }

        {
            parsed = input_parse_mbd_body_ground_line(line, &body_index);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_BODY_GROUND at line %d: '%s'",
                                 MBD_DIAG_E_BODY_GROUND_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (body_index < 0) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_BODY_GROUND body %d at line %d is invalid",
                                     MBD_DIAG_E_BODY_RANGE,
                                     body_index,
                                     line_no);
                }
                CHECK_ERROR(input_mbd_body_tracker_reserve(&body_tracker, body_index + 1));
                CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));
                for (i = 0; i < body_tracker.capacity; ++i) {
                    if (body_tracker.body_ground_seen[i]) {
                        fclose(fp);
                        return error_set(FEM_ERROR_INVALID_INPUT,
                                         "MBD_INPUT_ERROR[%s] Duplicate MBD_BODY_GROUND at line %d (first defined for body %d at line %d)",
                                         MBD_DIAG_E_DUP_BODY_GROUND,
                                         line_no,
                                         i,
                                         body_tracker.body_ground_line[i]);
                    }
                }
                body_tracker.body_ground_seen[body_index] = 1;
                body_tracker.body_ground_line[body_index] = line_no;
                if (mbd_body2d_is_defined(&system->bodies[body_index])) {
                    system->bodies[body_index].is_ground = 1;
                }
                continue;
            }
        }

        {
            double gravity[2];

            parsed = input_parse_mbd_gravity_line(line, gravity);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_GRAVITY at line %d: '%s'",
                                 MBD_DIAG_E_GRAVITY_PARSE, line_no, excerpt);
            }
            if (parsed == 1) {
                if (mbd_system2d_set_gravity(system, gravity[0], gravity[1]) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_GRAVITY at line %d",
                                     MBD_DIAG_E_GRAVITY_PARSE,
                                     line_no);
                }
                continue;
            }
        }

        {
            double force[3];

            parsed = input_parse_mbd_force_line(line, &body_index, force);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_FORCE at line %d: '%s'",
                                 MBD_DIAG_E_FORCE_PARSE, line_no, excerpt);
            }
            if (parsed == 1) {
                if (body_index < 0) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_FORCE at line %d references invalid body %d",
                                     MBD_DIAG_E_FORCE_RANGE,
                                     line_no,
                                     body_index);
                }
                CHECK_ERROR(input_mbd_body_tracker_reserve(&body_tracker, body_index + 1));
                CHECK_ERROR(mbd_system2d_reserve_body_storage(system, body_index + 1));
                body_tracker.body_force_seen[body_index] = 1;
                if (body_tracker.body_force_line[body_index] < 0) {
                    body_tracker.body_force_line[body_index] = line_no;
                }
                body_tracker.body_force_accum[body_index][0] += force[0];
                body_tracker.body_force_accum[body_index][1] += force[1];
                body_tracker.body_force_accum[body_index][2] += force[2];
                if (mbd_body2d_is_defined(&system->bodies[body_index])) {
                    system->bodies[body_index].force[0] = body_tracker.body_force_accum[body_index][0];
                    system->bodies[body_index].force[1] = body_tracker.body_force_accum[body_index][1];
                    system->bodies[body_index].force[2] = body_tracker.body_force_accum[body_index][2];
                }
            }
        }

        {
            mbd_contact_coupling_mode_t coupling_mode = MBD_CONTACT_COUPLING_MODE_ONE_WAY;

            parsed = input_parse_mbd_contact_coupling_mode_line(line, &coupling_mode);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_COUPLING_MODE at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_COUPLING_MODE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (contact_coupling_mode_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_CONTACT_COUPLING_MODE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_CONTACT_COUPLING_MODE_PARSE,
                                     line_no,
                                     contact_coupling_mode_line);
                }
                if (mbd_system2d_set_contact_coupling_mode(system, coupling_mode) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_COUPLING_MODE at line %d",
                                     MBD_DIAG_E_CONTACT_COUPLING_MODE_PARSE,
                                     line_no);
                }
                contact_coupling_mode_seen = 1;
                contact_coupling_mode_line = line_no;
                continue;
            }
        }

        {
            mbd_local_feedback_mode_t local_feedback_mode = MBD_LOCAL_FEEDBACK_MODE_NONE;

            parsed = input_parse_mbd_local_feedback_mode_line(line, &local_feedback_mode);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_LOCAL_FEEDBACK_MODE at line %d: '%s'",
                                 MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (local_feedback_mode_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_LOCAL_FEEDBACK_MODE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                     line_no,
                                     local_feedback_mode_line);
                }
                if (mbd_system2d_set_local_feedback_mode(system, local_feedback_mode) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_LOCAL_FEEDBACK_MODE at line %d",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                     line_no);
                }
                local_feedback_mode_seen = 1;
                local_feedback_mode_line = line_no;
                continue;
            }
        }

        {
            mbd_local_contact_monolithic_mode_t local_contact_monolithic_mode =
                MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE;

            parsed = input_parse_mbd_local_contact_monolithic_line(line,
                                                                   &local_contact_monolithic_mode);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_LOCAL_CONTACT_MONOLITHIC at line %d: '%s'",
                                 MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (local_contact_monolithic_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_LOCAL_CONTACT_MONOLITHIC at line %d (first defined at line %d)",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                     line_no,
                                     local_contact_monolithic_line);
                }
                if (mbd_system2d_set_local_contact_monolithic_mode(system,
                                                                   local_contact_monolithic_mode) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_LOCAL_CONTACT_MONOLITHIC at line %d",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_MODE_PARSE,
                                     line_no);
                }
                local_contact_monolithic_seen = 1;
                local_contact_monolithic_line = line_no;
                continue;
            }
        }

        {
            mbd_monolithic_proper_mode_t monolithic_proper_mode =
                MBD_MONOLITHIC_PROPER_MODE_NONE;

            parsed = input_parse_mbd_monolithic_proper_mode_line(line,
                                                                 &monolithic_proper_mode);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_MONOLITHIC_PROPER_MODE at line %d: '%s'",
                                 MBD_DIAG_E_MONOLITHIC_PROPER_MODE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (monolithic_proper_mode_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_MONOLITHIC_PROPER_MODE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_MONOLITHIC_PROPER_MODE_PARSE,
                                     line_no,
                                     monolithic_proper_mode_line);
                }
                if (mbd_system2d_set_monolithic_proper_mode(system,
                                                            monolithic_proper_mode) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_MONOLITHIC_PROPER_MODE at line %d",
                                     MBD_DIAG_E_MONOLITHIC_PROPER_MODE_PARSE,
                                     line_no);
                }
                monolithic_proper_mode_seen = 1;
                monolithic_proper_mode_line = line_no;
                continue;
            }
        }

        {
            mbd_monolithic_proper_context2d_t monolithic_proper_context;

            memset(&monolithic_proper_context, 0, sizeof(monolithic_proper_context));
            parsed = input_parse_mbd_monolithic_proper_context_line(line,
                                                                    &monolithic_proper_context);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_MONOLITHIC_PROPER_CONTEXT at line %d: '%s'",
                                 MBD_DIAG_E_MONOLITHIC_PROPER_CONTEXT_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (monolithic_proper_context_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_MONOLITHIC_PROPER_CONTEXT at line %d (first defined at line %d)",
                                     MBD_DIAG_E_MONOLITHIC_PROPER_CONTEXT_PARSE,
                                     line_no,
                                     monolithic_proper_context_line);
                }
                if (mbd_system2d_set_monolithic_proper_context(system,
                                                               &monolithic_proper_context) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_MONOLITHIC_PROPER_CONTEXT at line %d",
                                     MBD_DIAG_E_MONOLITHIC_PROPER_CONTEXT_PARSE,
                                     line_no);
                }
                monolithic_proper_context_seen = 1;
                monolithic_proper_context_line = line_no;
                continue;
            }
        }

        {
            char local_feedback_path[1024];

            local_feedback_path[0] = '\0';
            parsed = input_parse_mbd_local_feedback_file_line(line,
                                                              local_feedback_path,
                                                              sizeof(local_feedback_path));
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_LOCAL_FEEDBACK_FILE at line %d: '%s'",
                                 MBD_DIAG_E_LOCAL_FEEDBACK_FILE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (local_feedback_file_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_LOCAL_FEEDBACK_FILE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_FILE_PARSE,
                                     line_no,
                                     local_feedback_file_line);
                }
                if (mbd_system2d_set_local_feedback_file(system, local_feedback_path) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_LOCAL_FEEDBACK_FILE at line %d",
                                     MBD_DIAG_E_LOCAL_FEEDBACK_FILE_PARSE,
                                     line_no);
                }
                local_feedback_file_seen = 1;
                local_feedback_file_line = line_no;
                continue;
            }
        }

        {
            char local_contact_path[1024];

            local_contact_path[0] = '\0';
            parsed = input_parse_mbd_local_contact_file_line(line,
                                                             local_contact_path,
                                                             sizeof(local_contact_path));
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_LOCAL_CONTACT_FILE at line %d: '%s'",
                                 MBD_DIAG_E_LOCAL_CONTACT_FILE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (local_contact_file_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_LOCAL_CONTACT_FILE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_LOCAL_CONTACT_FILE_PARSE,
                                     line_no,
                                     local_contact_file_line);
                }
                if (mbd_system2d_set_local_contact_file(system, local_contact_path) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_LOCAL_CONTACT_FILE at line %d",
                                     MBD_DIAG_E_LOCAL_CONTACT_FILE_PARSE,
                                     line_no);
                }
                local_contact_file_seen = 1;
                local_contact_file_line = line_no;
                continue;
            }
        }

        {
            char ehl_path[1024];

            ehl_path[0] = '\0';
            parsed = input_parse_mbd_ehl_file_line(line, ehl_path, sizeof(ehl_path));
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_EHL_FILE at line %d: '%s'",
                                 MBD_DIAG_E_EHL_FILE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                if (ehl_file_seen) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_EHL_FILE at line %d (first defined at line %d)",
                                     MBD_DIAG_E_EHL_FILE_PARSE,
                                     line_no,
                                     ehl_file_line);
                }
                if (mbd_system2d_set_ehl_file(system, ehl_path) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_EHL_FILE at line %d",
                                     MBD_DIAG_E_EHL_FILE_PARSE,
                                     line_no);
                }
                ehl_file_seen = 1;
                ehl_file_line = line_no;
                continue;
            }
        }

        {
            double radius = 0.0;
            double thickness = 0.0;

            parsed = input_parse_mbd_body_circle_line(line, &body_index, &radius, &thickness);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_BODY_CIRCLE at line %d: '%s'",
                                 MBD_DIAG_E_BODY_CIRCLE_PARSE, line_no, excerpt);
            }
            if (parsed == 1) {
                if (body_index < 0) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_BODY_CIRCLE body %d at line %d is invalid",
                                     MBD_DIAG_E_BODY_RANGE,
                                     body_index,
                                     line_no);
                }
                CHECK_ERROR(input_mbd_body_tracker_reserve(&body_tracker, body_index + 1));
                if (body_index >= MBD_SYSTEM2D_MAX_BODIES) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_BODY_CIRCLE body %d at line %d exceeds supported range [0,%d)",
                                     MBD_DIAG_E_BODY_RANGE,
                                     body_index,
                                     line_no,
                                     MBD_SYSTEM2D_MAX_BODIES);
                }
                if (body_tracker.circle_seen[body_index]) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_BODY_CIRCLE body %d at line %d (first defined at line %d)",
                                     MBD_DIAG_E_DUP_BODY,
                                     body_index,
                                     line_no,
                                     body_tracker.circle_line[body_index]);
                }
                if (mbd_system2d_register_body_circle(system,
                                                      body_index,
                                                      radius,
                                                      thickness) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_BODY_CIRCLE body %d at line %d",
                                     MBD_DIAG_E_BODY_CIRCLE_PARSE,
                                     body_index,
                                     line_no);
                }
                body_tracker.circle_seen[body_index] = 1;
                body_tracker.circle_line[body_index] = line_no;
                continue;
            }
        }

        {
            int halfspace_id = -1;
            double point[2] = {0.0, 0.0};
            double normal[2] = {0.0, 0.0};
            double thickness = 0.0;

            parsed = input_parse_mbd_contact_halfspace_line(line,
                                                            &halfspace_id,
                                                            point,
                                                            normal,
                                                            &thickness);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_HALFSPACE at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_HALFSPACE_PARSE, line_no, excerpt);
            }
            if (parsed == 1) {
                if (halfspace_id >= MBD_CONTACT2D_MAX_PAIRS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_CONTACT_HALFSPACE id %d at line %d exceeds supported range [0,%d)",
                                     MBD_DIAG_E_CONTACT_HALFSPACE_PARSE,
                                     halfspace_id,
                                     line_no,
                                     MBD_CONTACT2D_MAX_PAIRS);
                }
                if (halfspace_seen[halfspace_id]) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Duplicate MBD_CONTACT_HALFSPACE id %d at line %d (first defined at line %d)",
                                     MBD_DIAG_E_CONTACT_HALFSPACE_PARSE,
                                     halfspace_id,
                                     line_no,
                                     halfspace_line[halfspace_id]);
                }
                if (mbd_system2d_register_contact_halfspace(system,
                                                            halfspace_id,
                                                            point,
                                                            normal,
                                                            thickness) != FEM_SUCCESS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_HALFSPACE id %d at line %d",
                                     MBD_DIAG_E_CONTACT_HALFSPACE_PARSE,
                                     halfspace_id,
                                     line_no);
                }
                halfspace_seen[halfspace_id] = 1;
                halfspace_line[halfspace_id] = line_no;
                continue;
            }
        }

        {
            int surface_id = -1;
            int surface_body = -1;
            char surface_csv_path[1024];

            surface_csv_path[0] = '\0';
            parsed = input_parse_mbd_contact_surface_polyline_line(line,
                                                                   &surface_id,
                                                                   &surface_body,
                                                                   surface_csv_path,
                                                                   sizeof(surface_csv_path));
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_SURFACE_POLYLINE at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_SURFACE_POLYLINE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                char register_error_buffer[ERROR_MSG_LEN];

                register_error_buffer[0] = '\0';
                if (mbd_system2d_register_contact_surface_polyline(system,
                                                                   surface_id,
                                                                   surface_body,
                                                                   surface_csv_path) != FEM_SUCCESS) {
                    const char *register_error = error_get_message();
                    if (register_error && register_error[0] != '\0') {
                        strncpy(register_error_buffer,
                                register_error,
                                sizeof(register_error_buffer) - 1);
                        register_error_buffer[sizeof(register_error_buffer) - 1] = '\0';
                    }
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_SURFACE_POLYLINE id %d at line %d: %s",
                                     MBD_DIAG_E_CONTACT_SURFACE_POLYLINE_PARSE,
                                     surface_id,
                                     line_no,
                                     register_error_buffer[0] != '\0'
                                         ? register_error_buffer
                                         : "unspecified generic contact surface registration error");
                }
                continue;
            }
        }

        {
            int pair_id = -1;
            int body_i = -1;
            int body_j = -1;
            double k_n = 0.0;
            double c_n = 0.0;
            double mu_base = 0.0;

            parsed = input_parse_mbd_contact_pair_line(line,
                                                       &pair_id,
                                                       &body_i,
                                                       &body_j,
                                                       &k_n,
                                                       &c_n,
                                                       &mu_base);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_PAIR at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_PAIR_PARSE, line_no, excerpt);
            }
            if (parsed == 1) {
                char register_error_buffer[ERROR_MSG_LEN];

                register_error_buffer[0] = '\0';
                if (body_i >= MBD_SYSTEM2D_MAX_BODIES || body_j >= MBD_SYSTEM2D_MAX_BODIES) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_CONTACT_PAIR at line %d references body outside supported range [0,%d)",
                                     MBD_DIAG_E_CONTACT_PAIR_RANGE,
                                     line_no,
                                     MBD_SYSTEM2D_MAX_BODIES);
                }
                if (mbd_system2d_append_contact_pair(system,
                                                     pair_id,
                                                     body_i,
                                                     body_j,
                                                     k_n,
                                                     c_n,
                                                     mu_base) != FEM_SUCCESS) {
                    const char *register_error = error_get_message();
                    if (register_error && register_error[0] != '\0') {
                        strncpy(register_error_buffer,
                                register_error,
                                sizeof(register_error_buffer) - 1);
                        register_error_buffer[sizeof(register_error_buffer) - 1] = '\0';
                    }
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_PAIR id %d at line %d: %s",
                                     MBD_DIAG_E_CONTACT_PAIR_PARSE,
                                     pair_id,
                                     line_no,
                                     register_error_buffer[0] != '\0'
                                         ? register_error_buffer
                                         : "unspecified contact pair registration error");
                }
                continue;
            }
        }

        {
            int pair_id = -1;
            int body_circle = -1;
            int halfspace_id = -1;
            double k_n = 0.0;
            double c_n = 0.0;
            double mu_base = 0.0;

            parsed = input_parse_mbd_contact_pair_halfspace_line(line,
                                                                 &pair_id,
                                                                 &body_circle,
                                                                 &halfspace_id,
                                                                 &k_n,
                                                                 &c_n,
                                                                 &mu_base);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_PAIR_HALFSPACE at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_PAIR_HALFSPACE_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                char register_error_buffer[ERROR_MSG_LEN];

                register_error_buffer[0] = '\0';
                if (body_circle >= MBD_SYSTEM2D_MAX_BODIES || halfspace_id >= MBD_CONTACT2D_MAX_PAIRS) {
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] MBD_CONTACT_PAIR_HALFSPACE at line %d references body/halfspace outside supported range",
                                     MBD_DIAG_E_CONTACT_PAIR_RANGE,
                                     line_no);
                }
                if (mbd_system2d_append_contact_halfspace_pair(system,
                                                               pair_id,
                                                               body_circle,
                                                               halfspace_id,
                                                               k_n,
                                                               c_n,
                                                               mu_base) != FEM_SUCCESS) {
                    const char *register_error = error_get_message();
                    if (register_error && register_error[0] != '\0') {
                        strncpy(register_error_buffer,
                                register_error,
                                sizeof(register_error_buffer) - 1);
                        register_error_buffer[sizeof(register_error_buffer) - 1] = '\0';
                    }
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_PAIR_HALFSPACE id %d at line %d: %s",
                                     MBD_DIAG_E_CONTACT_PAIR_HALFSPACE_PARSE,
                                     pair_id,
                                     line_no,
                                     register_error_buffer[0] != '\0'
                                         ? register_error_buffer
                                         : "unspecified contact pair registration error");
                }
                continue;
            }
        }

        {
            int pair_id = -1;
            int surface_i = -1;
            int surface_j = -1;
            double k_n = 0.0;
            double c_n = 0.0;
            double mu_base = 0.0;
            double mu_static = 0.0;
            double mu_dynamic = 0.0;
            double v_ref = 0.0;
            double v_smooth = 0.0;

            parsed = input_parse_mbd_contact_pair_generic_line(line,
                                                               &pair_id,
                                                               &surface_i,
                                                               &surface_j,
                                                               &k_n,
                                                               &c_n,
                                                               &mu_base,
                                                               &mu_static,
                                                               &mu_dynamic,
                                                               &v_ref,
                                                               &v_smooth);
            if (parsed == -1) {
                input_mbd_line_to_excerpt(line, excerpt, sizeof(excerpt));
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Invalid MBD_CONTACT_PAIR_GENERIC at line %d: '%s'",
                                 MBD_DIAG_E_CONTACT_PAIR_GENERIC_PARSE,
                                 line_no,
                                 excerpt);
            }
            if (parsed == 1) {
                char register_error_buffer[ERROR_MSG_LEN];

                register_error_buffer[0] = '\0';
                if (mbd_system2d_append_generic_contact_pair(system,
                                                             pair_id,
                                                             surface_i,
                                                             surface_j,
                                                             k_n,
                                                             c_n,
                                                             mu_base,
                                                             mu_static,
                                                             mu_dynamic,
                                                             v_ref,
                                                             v_smooth) != FEM_SUCCESS) {
                    const char *register_error = error_get_message();
                    if (register_error && register_error[0] != '\0') {
                        strncpy(register_error_buffer,
                                register_error,
                                sizeof(register_error_buffer) - 1);
                        register_error_buffer[sizeof(register_error_buffer) - 1] = '\0';
                    }
                    fclose(fp);
                    return error_set(FEM_ERROR_INVALID_INPUT,
                                     "MBD_INPUT_ERROR[%s] Failed to register MBD_CONTACT_PAIR_GENERIC id %d at line %d: %s",
                                     MBD_DIAG_E_CONTACT_PAIR_GENERIC_PARSE,
                                     pair_id,
                                     line_no,
                                     register_error_buffer[0] != '\0'
                                         ? register_error_buffer
                                         : "unspecified generic contact pair registration error");
                }
                continue;
            }
        }
    }

    if (fclose(fp) != 0) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot close MBD input file after read: %s",
                         filename);
    }

    for (i = 0; i < body_tracker.capacity; ++i) {
        if (body_tracker.body_ground_seen[i] && !body_tracker.body_seen[i]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_BODY_GROUND at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             i,
                             body_tracker.body_ground_line[i]);
        }
        if (body_tracker.body_ground_seen[i]) {
            system->bodies[i].is_ground = 1;
        }
        if (body_tracker.circle_seen[i] && !body_tracker.body_seen[i]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_BODY_CIRCLE at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             i,
                             body_tracker.circle_line[i]);
        }
        if (body_tracker.body_force_seen[i] && !body_tracker.body_seen[i]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_FORCE at line %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             i,
                             body_tracker.body_force_line[i]);
        }
    }

    for (i = 0; i < system->num_contact_pairs; ++i) {
        const mbd_contact_pair2d_t *pair = &system->contact_pairs[i];
        if (pair->body_i < 0 || pair->body_i >= body_tracker.capacity ||
            !body_tracker.body_seen[pair->body_i]) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_CONTACT_PAIR id %d",
                             MBD_DIAG_E_UNDEFINED_BODY_REF,
                             pair->body_i,
                             pair->pair_id);
        }
        if (pair->proxy_geometry == MBD_CONTACT_PROXY_CIRCLE_CIRCLE) {
            if (pair->body_j < 0 || pair->body_j >= body_tracker.capacity ||
                !body_tracker.body_seen[pair->body_j]) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_CONTACT_PAIR id %d",
                                 MBD_DIAG_E_UNDEFINED_BODY_REF,
                                 pair->body_j,
                                 pair->pair_id);
            }
            if (!body_tracker.circle_seen[pair->body_i] ||
                !body_tracker.circle_seen[pair->body_j]) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_CONTACT_PAIR id %d requires MBD_BODY_CIRCLE for both bodies",
                                 MBD_DIAG_E_CONTACT_PAIR_PARSE,
                                 pair->pair_id);
            }
        } else if (pair->proxy_geometry == MBD_CONTACT_PROXY_CIRCLE_HALFSPACE) {
            if (!body_tracker.circle_seen[pair->body_i]) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] MBD_CONTACT_PAIR_HALFSPACE id %d requires MBD_BODY_CIRCLE for body %d",
                                 MBD_DIAG_E_CONTACT_PAIR_HALFSPACE_PARSE,
                                 pair->pair_id,
                                 pair->body_i);
            }
            if (pair->halfspace_id < 0 || pair->halfspace_id >= MBD_CONTACT2D_MAX_PAIRS ||
                !halfspace_seen[pair->halfspace_id]) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "MBD_INPUT_ERROR[%s] Undefined MBD_CONTACT_HALFSPACE %d referenced by MBD_CONTACT_PAIR_HALFSPACE id %d",
                                 MBD_DIAG_E_CONTACT_PAIR_HALFSPACE_PARSE,
                                 pair->halfspace_id,
                                 pair->pair_id);
            }
        }
    }

    for (i = 0; i < system->num_contact_surface_polylines; ++i) {
        const mbd_contact_surface_polyline2d_t *surface =
            &system->contact_surface_polylines[i];
        if (!surface->is_defined) {
            continue;
        }
        if (surface->body_id < 0 || surface->body_id >= MBD_SYSTEM2D_MAX_BODIES ||
            surface->body_id >= body_tracker.capacity ||
            !body_tracker.body_seen[surface->body_id]) {
            return error_set(
                FEM_ERROR_INVALID_INPUT,
                "MBD_INPUT_ERROR[%s] Undefined MBD_BODY %d referenced by MBD_CONTACT_SURFACE_POLYLINE id %d",
                MBD_DIAG_E_UNDEFINED_BODY_REF,
                surface->body_id,
                surface->surface_id);
        }
    }

    for (i = 0; i < system->num_generic_contact_pairs; ++i) {
        const mbd_contact_generic_pair2d_t *pair = &system->generic_contact_pairs[i];
        int found_surface_i = 0;
        int found_surface_j = 0;
        int j = 0;

        if (!pair->is_defined) {
            continue;
        }
        for (j = 0; j < system->num_contact_surface_polylines; ++j) {
            const mbd_contact_surface_polyline2d_t *surface =
                &system->contact_surface_polylines[j];
            if (!surface->is_defined) {
                continue;
            }
            if (surface->surface_id == pair->surface_i) {
                found_surface_i = 1;
            }
            if (surface->surface_id == pair->surface_j) {
                found_surface_j = 1;
            }
        }
        if (!found_surface_i || !found_surface_j) {
            return error_set(
                FEM_ERROR_INVALID_INPUT,
                "MBD_INPUT_ERROR[%s] MBD_CONTACT_PAIR_GENERIC id %d references undefined surface(s) %d/%d",
                MBD_DIAG_E_CONTACT_PAIR_GENERIC_PARSE,
                pair->pair_id,
                pair->surface_i,
                pair->surface_j);
        }
    }

    if (system->contact_coupling_mode == MBD_CONTACT_COUPLING_MODE_LAGGED_STIFFNESS &&
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_NONE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_CONTACT_COUPLING_MODE LAGGED_STIFFNESS cannot be combined with non-NONE MBD_LOCAL_FEEDBACK_MODE");
    }
    if (system->local_contact_monolithic_mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE &&
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_NONE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_LOCAL_CONTACT_MONOLITHIC cannot be combined with non-NONE MBD_LOCAL_FEEDBACK_MODE");
    }
    if (system->local_contact_monolithic_mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE &&
        system->contact_coupling_mode != MBD_CONTACT_COUPLING_MODE_ONE_WAY) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_LOCAL_CONTACT_MONOLITHIC requires MBD_CONTACT_COUPLING_MODE ONE_WAY");
    }
    if (system->monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
        system->contact_coupling_mode != MBD_CONTACT_COUPLING_MODE_ONE_WAY) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_MONOLITHIC_PROPER_MODE currently requires MBD_CONTACT_COUPLING_MODE ONE_WAY");
    }
    if (system->monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
        system->local_feedback_mode != MBD_LOCAL_FEEDBACK_MODE_NONE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_MONOLITHIC_PROPER_MODE cannot be combined with non-NONE MBD_LOCAL_FEEDBACK_MODE");
    }
    if (system->monolithic_proper_mode != MBD_MONOLITHIC_PROPER_MODE_NONE &&
        system->local_contact_monolithic_mode != MBD_LOCAL_CONTACT_MONOLITHIC_MODE_NONE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_MONOLITHIC_PROPER_MODE cannot be combined with MBD_LOCAL_CONTACT_MONOLITHIC");
    }
    if (system->monolithic_proper_context.is_defined &&
        system->monolithic_proper_mode == MBD_MONOLITHIC_PROPER_MODE_NONE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_CONTACT_MODE_COMBINATION] MBD_MONOLITHIC_PROPER_CONTEXT requires MBD_MONOLITHIC_PROPER_MODE");
    }

    if (system->local_feedback_filename[0] != '\0' &&
        (system->local_contact_filename[0] != '\0' || system->ehl_filename[0] != '\0')) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "MBD_INPUT_ERROR[E_LOCAL_FEEDBACK_SOURCE_COMBINATION] MBD_LOCAL_FEEDBACK_FILE cannot be combined with MBD_LOCAL_CONTACT_FILE or MBD_EHL_FILE");
    }

    return FEM_SUCCESS;
}

fem_error_t input_read_coupled_directives(const char *filename)
{
    FILE *fp = NULL;
    char line[1024];
    int line_number = 0;
    coupled_case2d_t *case_data = coupled_case2d_current();

    CHECK_NULL(filename, "coupled directive filename");

    coupled_case2d_reset_current();
    fp = fopen(filename, "r");
    CHECK_FILE(fp, filename);

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *comment = NULL;
        fem_error_t err = FEM_SUCCESS;

        ++line_number;
        comment = strchr(line, '#');
        if (comment) {
            *comment = '\0';
        }
        input_parser_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        if (input_parser_is_label(line, "COUPLED_FLEX_BODY")) {
            err = input_parse_coupled_flex_body_directive(case_data,
                                                          line,
                                                          line_number);
        } else if (input_parser_is_label(line, "COUPLED_FLEX_ROOT_SET")) {
            err = input_parse_coupled_flex_set_directive(case_data,
                                                         line,
                                                         "COUPLED_FLEX_ROOT_SET",
                                                         1,
                                                         line_number);
        } else if (input_parser_is_label(line, "COUPLED_FLEX_TIP_SET")) {
            err = input_parse_coupled_flex_set_directive(case_data,
                                                         line,
                                                         "COUPLED_FLEX_TIP_SET",
                                                         0,
                                                         line_number);
        }

        if (err != FEM_SUCCESS) {
            fclose(fp);
            coupled_case2d_reset_current();
            return err;
        }
    }

    fclose(fp);
    return FEM_SUCCESS;
}

static fem_error_t input_parse_coupled_flex_body_directive(coupled_case2d_t *case_data,
                                                           const char *line,
                                                           int line_number)
{
    int body_id = -1;
    char fem_input_path[MAX_FILENAME_LEN];
    char extra[2];
    int scanned = 0;
    fem_error_t err;

    CHECK_NULL(case_data, "coupled case");
    CHECK_NULL(line, "coupled flex body line");

    fem_input_path[0] = '\0';
    extra[0] = '\0';
    scanned = sscanf(line, "COUPLED_FLEX_BODY %d %255s %1s",
                     &body_id, fem_input_path, extra);
    if (scanned != 2) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid COUPLED_FLEX_BODY at line %d",
                         line_number);
    }

    err = coupled_case2d_add_flex_body(case_data, body_id, fem_input_path);
    if (err != FEM_SUCCESS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid COUPLED_FLEX_BODY at line %d: %s",
                         line_number,
                         error_get_message());
    }

    return FEM_SUCCESS;
}

static fem_error_t input_parse_coupled_flex_set_directive(coupled_case2d_t *case_data,
                                                          const char *line,
                                                          const char *directive,
                                                          int is_root,
                                                          int line_number)
{
    char normalized[1024];
    char tok[1024][64];
    int *node_ids = NULL;
    int nt = 0;
    int body_id = -1;
    int count = 0;
    int i;
    fem_error_t err;

    CHECK_NULL(case_data, "coupled case");
    CHECK_NULL(line, "coupled flex set line");
    CHECK_NULL(directive, "coupled flex directive");

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    nt = input_parser_split_tokens(normalized, tok, 256);
    if (nt < 4 || strcmp(tok[0], directive) != 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid %s at line %d",
                         directive,
                         line_number);
    }
    if (!input_parse_strict_int_token(tok[1], &body_id) || body_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid %s body_id at line %d",
                         directive,
                         line_number);
    }
    if (!input_parse_strict_int_token(tok[2], &count) || count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid %s node count at line %d",
                         directive,
                         line_number);
    }
    if (count > 1021 || nt != count + 3) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid %s token count at line %d",
                         directive,
                         line_number);
    }

    node_ids = calloc((size_t)count, sizeof(*node_ids));
    CHECK_NULL(node_ids, directive);

    for (i = 0; i < count; ++i) {
        if (!input_parse_strict_int_token(tok[i + 3], &node_ids[i]) ||
            node_ids[i] <= 0) {
            free(node_ids);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid %s node id at line %d",
                             directive,
                             line_number);
        }
    }

    if (is_root) {
        err = coupled_case2d_set_root_set(case_data, body_id, node_ids, count);
    } else {
        err = coupled_case2d_set_tip_set(case_data, body_id, node_ids, count);
    }
    free(node_ids);
    if (err != FEM_SUCCESS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid %s at line %d: %s",
                         directive,
                         line_number,
                         error_get_message());
    }

    return FEM_SUCCESS;
}

static int input_parse_strict_int_token(const char *text, int *value_out)
{
    char *end_ptr = NULL;
    long value = 0;

    if (!text || !value_out || text[0] == '\0') {
        return 0;
    }

    errno = 0;
    value = strtol(text, &end_ptr, 10);
    if (end_ptr == text || *end_ptr != '\0' || errno == ERANGE) {
        return 0;
    }
    if (value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *value_out = (int)value;
    return 1;
}

static void input_parser_trim(char *text)
{
    if (!text) return;
    size_t len = strlen(text);
    while (len > 0 && isspace((unsigned char)text[len - 1])) {
        text[--len] = '\0';
    }
    size_t start = 0;
    while (text[start] && isspace((unsigned char)text[start])) {
        start++;
    }
    if (start > 0) {
        memmove(text, text + start, strlen(text + start) + 1);
    }
}

static int input_parser_is_label(const char *line, const char *label)
{
    if (!line || !label) return 0;
    size_t n = strlen(label);
    if (strlen(line) < n) return 0;
    for (size_t i = 0; i < n; ++i) {
        char a = (char)tolower((unsigned char)line[i]);
        char b = (char)tolower((unsigned char)label[i]);
        if (a != b) return 0;
    }
    return 1;
}

static int input_parser_split_tokens(const char *line, char tokens[][64], int max_tokens)
{
    int count = 0;
    const char *p = line;
    while (*p && count < max_tokens) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) {
            ++p;
        }
        if (!*p) break;
        const char *start = p;
        while (*p && !isspace((unsigned char)*p) && *p != ',') {
            ++p;
        }
        size_t len = (size_t)(p - start);
        if (len >= 64) len = 63;
        memcpy(tokens[count], start, len);
        tokens[count][len] = '\0';
        count++;
    }
    return count;
}

typedef struct {
    int *data;
    int size;
    int cap;
} parser_list_t;

static void parser_list_init(parser_list_t *list)
{
    list->data = NULL;
    list->size = 0;
    list->cap = 0;
}

static void parser_list_push(parser_list_t *list, int value)
{
    if (list->size == list->cap) {
        int new_cap = list->cap ? list->cap * 2 : 16;
        int *tmp = realloc(list->data, (size_t)new_cap * sizeof(int));
        if (!tmp) {
            perror("realloc");
            exit(1);
        }
        list->data = tmp;
        list->cap = new_cap;
    }
    list->data[list->size++] = value;
}

static void parser_list_free(parser_list_t *list)
{
    free(list->data);
    list->data = NULL;
    list->size = 0;
    list->cap = 0;
}

static int input_parse_strict_double_token(const char *text, double *value_out)
{
    char *end_ptr = NULL;
    double value = 0.0;

    if (!text || !value_out || text[0] == '\0') {
        return 0;
    }

    errno = 0;
    value = strtod(text, &end_ptr);
    if (end_ptr == text || *end_ptr != '\0' || errno == ERANGE || !isfinite(value)) {
        return 0;
    }

    *value_out = value;
    return 1;
}

static fem_error_t input_build_path_relative_to_file(const char *deck_path,
                                                     const char *raw_path,
                                                     char *resolved_path,
                                                     size_t resolved_path_size)
{
    const char *slash = NULL;
    size_t prefix_len = 0;

    CHECK_NULL(deck_path, "deck path");
    CHECK_NULL(raw_path, "raw path");
    CHECK_NULL(resolved_path, "resolved path");

    if (raw_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT, "Empty path token");
    }

    if (raw_path[0] == '/') {
        if (snprintf(resolved_path, resolved_path_size, "%s", raw_path) >= (int)resolved_path_size) {
            return error_set(FEM_ERROR_INVALID_INPUT, "Resolved path is too long: %s", raw_path);
        }
        return FEM_SUCCESS;
    }

    slash = strrchr(deck_path, '/');
    if (!slash) {
        if (snprintf(resolved_path, resolved_path_size, "%s", raw_path) >= (int)resolved_path_size) {
            return error_set(FEM_ERROR_INVALID_INPUT, "Resolved path is too long: %s", raw_path);
        }
        return FEM_SUCCESS;
    }

    prefix_len = (size_t)(slash - deck_path);
    if (prefix_len + 1 + strlen(raw_path) + 1 > resolved_path_size) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Resolved path is too long: %s", raw_path);
    }

    memcpy(resolved_path, deck_path, prefix_len);
    resolved_path[prefix_len] = '/';
    strcpy(resolved_path + prefix_len + 1, raw_path);
    return FEM_SUCCESS;
}

static int input_find_fem_contact_generic_surface_index(int surface_id)
{
    for (int i = 0; i < g_num_fem_contact_generic_surfaces; ++i) {
        if (g_fem_contact_generic_surface_ids[i] == surface_id) {
            return i;
        }
    }
    return -1;
}

static int input_find_fem_contact_generic_pair_index(int pair_id)
{
    for (int i = 0; i < g_num_fem_contact_generic_pairs; ++i) {
        if (g_fem_contact_generic_pair_ids[i] == pair_id) {
            return i;
        }
    }
    return -1;
}

static int input_parse_fem_contact_surface_edge_set_line(const char *line,
                                                         int *surface_id,
                                                         int *part_id,
                                                         char *csv_path,
                                                         size_t csv_path_size)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_CONTACT_SURFACE_EDGE_SET")) {
        return 0;
    }
    if (!surface_id || !part_id || !csv_path || csv_path_size == 0) {
        return -1;
    }

    csv_path[0] = '\0';
    extra[0] = '\0';
    scanned = sscanf(line,
                     "FEM_CONTACT_SURFACE_EDGE_SET %d %d %255s %1s",
                     surface_id,
                     part_id,
                     csv_path,
                     extra);
    if (scanned != 3 || *surface_id < 0 || *part_id < 0 || csv_path[0] == '\0') {
        return -1;
    }
    csv_path[csv_path_size - 1] = '\0';
    return 1;
}

static int input_parse_fem_contact_pair_generic_line(const char *line,
                                                     int *pair_id,
                                                     int *surface_i,
                                                     int *surface_j,
                                                     double *k_pen,
                                                     double *c_pen,
                                                     double *mu)
{
    char normalized[1024];
    char tok[16][64];
    int nt = 0;

    if (!input_parser_is_label(line, "FEM_CONTACT_PAIR_GENERIC")) {
        return 0;
    }
    if (!pair_id || !surface_i || !surface_j || !k_pen || !c_pen || !mu) {
        return -1;
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    nt = input_parser_split_tokens(normalized, tok, 16);
    if (nt != 6 && nt != 7) {
        return -1;
    }
    if (strcmp(tok[0], "FEM_CONTACT_PAIR_GENERIC") != 0) {
        return -1;
    }
    if (!input_parse_strict_int_token(tok[1], pair_id) || *pair_id < 0) {
        return -1;
    }
    if (!input_parse_strict_int_token(tok[2], surface_i) || *surface_i < 0) {
        return -1;
    }
    if (!input_parse_strict_int_token(tok[3], surface_j) || *surface_j < 0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[4], k_pen) || *k_pen <= 0.0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[5], c_pen) || *c_pen < 0.0) {
        return -1;
    }
    *mu = 0.0;
    if (nt == 7) {
        if (!input_parse_strict_double_token(tok[6], mu) || *mu < 0.0) {
            return -1;
        }
    }

    return 1;
}

static int input_parse_fem_contact_adhesion_generic_line(const char *line,
                                                         int *pair_id,
                                                         double *k_adh_n,
                                                         double *gap_adh_max_m)
{
    char normalized[1024];
    char tok[8][64];
    int nt = 0;

    if (!input_parser_is_label(line, "FEM_CONTACT_ADHESION_GENERIC")) {
        return 0;
    }
    if (!pair_id || !k_adh_n || !gap_adh_max_m) {
        return -1;
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    nt = input_parser_split_tokens(normalized, tok, 8);
    if (nt != 4) {
        return -1;
    }
    if (strcmp(tok[0], "FEM_CONTACT_ADHESION_GENERIC") != 0) {
        return -1;
    }
    if (!input_parse_strict_int_token(tok[1], pair_id) || *pair_id < 0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[2], k_adh_n) || *k_adh_n <= 0.0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[3], gap_adh_max_m) || *gap_adh_max_m <= 0.0) {
        return -1;
    }

    return 1;
}

static int input_parse_fem_contact_friction_generic_line(const char *line,
                                                         int *pair_id,
                                                         double *mu_cap,
                                                         double *k_t_pen,
                                                         double *u_t_reg_m)
{
    char normalized[1024];
    char tok[8][64];
    int nt = 0;

    if (!input_parser_is_label(line, "FEM_CONTACT_FRICTION_GENERIC")) {
        return 0;
    }
    if (!pair_id || !mu_cap || !k_t_pen || !u_t_reg_m) {
        return -1;
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    nt = input_parser_split_tokens(normalized, tok, 8);
    if (nt != 5) {
        return -1;
    }
    if (strcmp(tok[0], "FEM_CONTACT_FRICTION_GENERIC") != 0) {
        return -1;
    }
    if (!input_parse_strict_int_token(tok[1], pair_id) || *pair_id < 0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[2], mu_cap) || *mu_cap < 0.0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[3], k_t_pen) || *k_t_pen <= 0.0) {
        return -1;
    }
    if (!input_parse_strict_double_token(tok[4], u_t_reg_m) || *u_t_reg_m <= 0.0) {
        return -1;
    }

    return 1;
}

static int input_parse_fem_solver_mode_line(const char *line,
                                            fem_solver_mode_t *mode_out)
{
    char mode_text[64];
    char extra[2];

    if (!input_parser_is_label(line, "FEM_SOLVER_MODE")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    extra[0] = '\0';
    if (sscanf(line, "FEM_SOLVER_MODE %63s %1s", mode_text, extra) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "IMPLICIT_ONEWAY_NEWMARK") == 0) {
        *mode_out = FEM_SOLVER_MODE_IMPLICIT_ONEWAY_NEWMARK;
        return 1;
    }
    if (strcmp(mode_text, "EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE") == 0) {
        *mode_out = FEM_SOLVER_MODE_EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE;
        return 1;
    }
    return -1;
}

static int input_parse_fem_time_step_dt_line(const char *line,
                                             double *dt_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_TIME_STEP_DT")) {
        return 0;
    }
    if (!dt_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line, "FEM_TIME_STEP_DT %lf %1s", dt_out, extra);
    if (scanned != 1 || !isfinite(*dt_out) || *dt_out <= 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_fem_num_time_steps_line(const char *line,
                                               int *count_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_NUM_TIME_STEPS")) {
        return 0;
    }
    if (!count_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line, "FEM_NUM_TIME_STEPS %d %1s", count_out, extra);
    if (scanned != 1 || *count_out <= 0) {
        return -1;
    }
    return 1;
}

static int input_parse_fem_outer_max_iter_line(const char *line,
                                               int *count_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_OUTER_MAX_ITER")) {
        return 0;
    }
    if (!count_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line, "FEM_OUTER_MAX_ITER %d %1s", count_out, extra);
    if (scanned != 1 || *count_out <= 0) {
        return -1;
    }
    return 1;
}

static int input_parse_fem_outer_tol_line(const char *line,
                                          double *tol_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_OUTER_TOL")) {
        return 0;
    }
    if (!tol_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line, "FEM_OUTER_TOL %lf %1s", tol_out, extra);
    if (scanned != 1 || !isfinite(*tol_out) || *tol_out <= 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_fem_local_feedback_mode_line(const char *line,
                                                    int *mode_out)
{
    char mode_text[64];

    if (!input_parser_is_label(line, "FEM_LOCAL_FEEDBACK_MODE")) {
        return 0;
    }
    if (!mode_out) {
        return -1;
    }

    if (sscanf(line, "FEM_LOCAL_FEEDBACK_MODE %63s", mode_text) != 1) {
        return -1;
    }
    if (strcmp(mode_text, "NONE") == 0) {
        *mode_out = FEM_LOCAL_FEEDBACK_MODE_NONE;
        return 1;
    }
    if (strcmp(mode_text, "LAGGED_REDUCED") == 0) {
        *mode_out = FEM_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED;
        return 1;
    }
    return -1;
}

static int input_parse_fem_local_contact_file_line(const char *line,
                                                   char *path_out,
                                                   size_t path_out_size)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_LOCAL_CONTACT_FILE")) {
        return 0;
    }
    if (!path_out || path_out_size == 0) {
        return -1;
    }

    path_out[0] = '\0';
    extra[0] = '\0';
    scanned = sscanf(line,
                     "FEM_LOCAL_CONTACT_FILE %255s %1s",
                     path_out,
                     extra);
    if (scanned != 1 || path_out[0] == '\0') {
        return -1;
    }
    path_out[path_out_size - 1] = '\0';
    return 1;
}

static int input_parse_fem_static_load_step_line(const char *line,
                                                 int *load_step_out,
                                                 double *scale_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_STATIC_LOAD_STEP")) {
        return 0;
    }
    if (!load_step_out || !scale_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line,
                     "FEM_STATIC_LOAD_STEP %d %lf %1s",
                     load_step_out,
                     scale_out,
                     extra);
    if (scanned != 2 || *load_step_out < 0 || !isfinite(*scale_out) || *scale_out < 0.0) {
        return -1;
    }
    return 1;
}

static int input_parse_fem_load_scale_step_line(const char *line,
                                                int *step_index_out,
                                                double *scale_out)
{
    int scanned = 0;
    char extra[2];

    if (!input_parser_is_label(line, "FEM_LOAD_SCALE_STEP")) {
        return 0;
    }
    if (!step_index_out || !scale_out) {
        return -1;
    }

    extra[0] = '\0';
    scanned = sscanf(line,
                     "FEM_LOAD_SCALE_STEP %d %lf %1s",
                     step_index_out,
                     scale_out,
                     extra);
    if (scanned != 2 || *step_index_out < 0 || !isfinite(*scale_out) || *scale_out < 0.0) {
        return -1;
    }
    return 1;
}

static fem_error_t input_validate_fem_contact_surface_edge_csv(const char *csv_path,
                                                               int surface_id,
                                                               int *edge_count_out)
{
    FILE *fp = NULL;
    char line[512];
    int line_number = 0;
    int edge_count = 0;

    CHECK_NULL(csv_path, "fem contact surface csv path");
    CHECK_NULL(edge_count_out, "fem contact surface edge count");

    fp = fopen(csv_path, "r");
    CHECK_FILE(fp, csv_path);

    while (fgets(line, sizeof(line), fp) != NULL) {
        char tokens[8][64];
        int node_ids[MAX_SURFACE_NODES] = {0, 0, 0};
        int token_count = 0;

        ++line_number;
        if (strchr(line, '#')) {
            *strchr(line, '#') = '\0';
        }
        input_parser_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        token_count = input_parser_split_tokens(line, tokens, 8);
        if (token_count != 2 && token_count != 3) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_CONTACT_SURFACE_EDGE_SET surface_id %d CSV row %d must have 2 or 3 node ids",
                             surface_id,
                             line_number);
        }

        for (int i = 0; i < token_count; ++i) {
            int node_index = -1;

            if (!input_parse_strict_int_token(tokens[i], &node_ids[i]) || node_ids[i] <= 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "FEM_CONTACT_SURFACE_EDGE_SET surface_id %d CSV row %d has invalid node id",
                                 surface_id,
                                 line_number);
            }
            if ((i > 0 && node_ids[i] == node_ids[0]) ||
                (i > 1 && node_ids[i] == node_ids[1])) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "FEM_CONTACT_SURFACE_EDGE_SET surface_id %d CSV row %d has duplicate node ids",
                                 surface_id,
                                 line_number);
            }
            if (input_get_node_index(node_ids[i], &node_index) != FEM_SUCCESS) {
                const char *detail = error_get_message();
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "FEM_CONTACT_SURFACE_EDGE_SET surface_id %d CSV row %d references undefined node %d: %s",
                                 surface_id,
                                 line_number,
                                 node_ids[i],
                                 detail ? detail : "unknown node lookup error");
            }
        }

        edge_count++;
    }

    fclose(fp);

    if (edge_count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_CONTACT_SURFACE_EDGE_SET surface_id %d has no edge rows",
                         surface_id);
    }

    *edge_count_out = edge_count;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_contact_surface_edge_set(int surface_id,
                                                               int part_id,
                                                               const char *csv_path,
                                                               int edge_count)
{
    int slot = -1;

    CHECK_NULL(csv_path, "fem contact surface path");

    if (surface_id < 0 || part_id < 0 || edge_count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_CONTACT_SURFACE_EDGE_SET values for surface_id %d",
                         surface_id);
    }
    if (g_num_fem_contact_generic_surfaces >= MAX_FEM_CONTACT_GENERIC_SURFACES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Too many FEM_CONTACT_SURFACE_EDGE_SET directives (max=%d)",
                         MAX_FEM_CONTACT_GENERIC_SURFACES);
    }
    if (input_find_fem_contact_generic_surface_index(surface_id) >= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate fem generic contact surface_id %d",
                         surface_id);
    }

    slot = g_num_fem_contact_generic_surfaces++;
    g_fem_contact_generic_surface_ids[slot] = surface_id;
    g_fem_contact_generic_surface_part_ids[slot] = part_id;
    g_fem_contact_generic_surface_edge_counts[slot] = edge_count;
    snprintf(g_fem_contact_generic_surface_paths[slot],
             sizeof(g_fem_contact_generic_surface_paths[slot]),
             "%s",
             csv_path);
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_contact_pair_generic(int pair_id,
                                                           int surface_i,
                                                           int surface_j,
                                                           double k_pen,
                                                           double c_pen,
                                                           double mu)
{
    int slot = -1;

    if (pair_id < 0 || surface_i < 0 || surface_j < 0 ||
        !isfinite(k_pen) || k_pen <= 0.0 ||
        !isfinite(c_pen) || c_pen < 0.0 ||
        !isfinite(mu) || mu < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_CONTACT_PAIR_GENERIC id %d",
                         pair_id);
    }
    if (g_num_fem_contact_generic_pairs >= MAX_FEM_CONTACT_GENERIC_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Too many FEM_CONTACT_PAIR_GENERIC directives (max=%d)",
                         MAX_FEM_CONTACT_GENERIC_PAIRS);
    }
    if (input_find_fem_contact_generic_pair_index(pair_id) >= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate fem generic contact pair_id %d",
                         pair_id);
    }

    slot = g_num_fem_contact_generic_pairs++;
    g_fem_contact_generic_pair_ids[slot] = pair_id;
    g_fem_contact_generic_pair_surface_i[slot] = surface_i;
    g_fem_contact_generic_pair_surface_j[slot] = surface_j;
    g_fem_contact_generic_pair_k_pen[slot] = k_pen;
    g_fem_contact_generic_pair_c_pen[slot] = c_pen;
    g_fem_contact_generic_pair_mu[slot] = mu;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_contact_adhesion_generic(int pair_id,
                                                               double k_adh_n,
                                                               double gap_adh_max_m)
{
    int slot = -1;

    if (pair_id < 0 ||
        !isfinite(k_adh_n) || k_adh_n <= 0.0 ||
        !isfinite(gap_adh_max_m) || gap_adh_max_m <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_CONTACT_ADHESION_GENERIC pair_id %d",
                         pair_id);
    }

    slot = input_find_fem_contact_generic_pair_index(pair_id);
    if (slot < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_CONTACT_ADHESION_GENERIC pair_id %d must appear after FEM_CONTACT_PAIR_GENERIC",
                         pair_id);
    }
    if (g_fem_contact_generic_pair_k_adh_n[slot] > 0.0 ||
        g_fem_contact_generic_pair_gap_adh_max_m[slot] > 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate FEM_CONTACT_ADHESION_GENERIC pair_id %d",
                         pair_id);
    }

    g_fem_contact_generic_pair_k_adh_n[slot] = k_adh_n;
    g_fem_contact_generic_pair_gap_adh_max_m[slot] = gap_adh_max_m;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_contact_friction_generic(int pair_id,
                                                               double mu_cap,
                                                               double k_t_pen,
                                                               double u_t_reg_m)
{
    int slot = -1;

    if (pair_id < 0 ||
        !isfinite(mu_cap) || mu_cap < 0.0 ||
        !isfinite(k_t_pen) || k_t_pen <= 0.0 ||
        !isfinite(u_t_reg_m) || u_t_reg_m <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_CONTACT_FRICTION_GENERIC pair_id %d",
                         pair_id);
    }

    slot = input_find_fem_contact_generic_pair_index(pair_id);
    if (slot < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_CONTACT_FRICTION_GENERIC pair_id %d must appear after FEM_CONTACT_PAIR_GENERIC",
                         pair_id);
    }
    if (g_fem_contact_generic_pair_k_t_pen[slot] > 0.0 ||
        g_fem_contact_generic_pair_u_t_reg_m[slot] > 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate FEM_CONTACT_FRICTION_GENERIC pair_id %d",
                         pair_id);
    }

    g_fem_contact_generic_pair_mu_cap[slot] = mu_cap;
    g_fem_contact_generic_pair_k_t_pen[slot] = k_t_pen;
    g_fem_contact_generic_pair_u_t_reg_m[slot] = u_t_reg_m;
    return FEM_SUCCESS;
}

static fem_error_t input_validate_fem_contact_pair_references(void)
{
    for (int i = 0; i < g_num_fem_contact_generic_pairs; ++i) {
        int surface_i = g_fem_contact_generic_pair_surface_i[i];
        int surface_j = g_fem_contact_generic_pair_surface_j[i];
        int pair_id = g_fem_contact_generic_pair_ids[i];

        if (input_find_fem_contact_generic_surface_index(surface_i) < 0 ||
            input_find_fem_contact_generic_surface_index(surface_j) < 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_CONTACT_PAIR_GENERIC id %d references undefined surface(s) %d/%d",
                             pair_id,
                             surface_i,
                             surface_j);
        }
    }

    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_local_feedback_mode(int mode)
{
    if (mode != FEM_LOCAL_FEEDBACK_MODE_NONE &&
        mode != FEM_LOCAL_FEEDBACK_MODE_LAGGED_REDUCED) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_LOCAL_FEEDBACK_MODE value %d",
                         mode);
    }
    g_fem_local_feedback_mode = mode;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_local_contact_file(const char *path)
{
    CHECK_NULL(path, "fem local feedback path");
    if (path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOCAL_CONTACT_FILE path must not be empty");
    }
    snprintf(g_fem_local_contact_file,
             sizeof(g_fem_local_contact_file),
             "%s",
             path);
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_solver_mode(fem_solver_mode_t mode)
{
    if (mode != FEM_SOLVER_MODE_IMPLICIT_ONEWAY_NEWMARK &&
        mode != FEM_SOLVER_MODE_EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_SOLVER_MODE value %d",
                         (int)mode);
    }
    g_analysis.fem_solver_mode = mode;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_time_step_dt(double dt)
{
    if (!isfinite(dt) || dt <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_TIME_STEP_DT value %g",
                         dt);
    }
    g_analysis.time_step_dt = dt;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_num_time_steps(int count)
{
    if (count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_NUM_TIME_STEPS value %d",
                         count);
    }
    g_analysis.num_time_steps = count;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_outer_max_iter(int count)
{
    if (count <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_OUTER_MAX_ITER value %d",
                         count);
    }
    g_analysis.outer_max_iterations = count;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_outer_tol(double tol)
{
    if (!isfinite(tol) || tol <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_OUTER_TOL value %g",
                         tol);
    }
    g_analysis.outer_tolerance = tol;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_static_load_step(int load_step,
                                                       double scale)
{
    if (load_step < 0 || !isfinite(scale) || scale < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_STATIC_LOAD_STEP load_step=%d scale=%g",
                         load_step,
                         scale);
    }
    if (g_num_fem_static_load_steps >= MAX_FEM_STATIC_LOAD_STEPS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Too many FEM_STATIC_LOAD_STEP directives (max=%d)",
                         MAX_FEM_STATIC_LOAD_STEPS);
    }
    for (int i = 0; i < g_num_fem_static_load_steps; ++i) {
        if (g_fem_static_load_step_ids[i] == load_step) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "duplicate FEM_STATIC_LOAD_STEP load_step %d",
                             load_step);
        }
    }

    g_fem_static_load_step_ids[g_num_fem_static_load_steps] = load_step;
    g_fem_static_load_step_scales[g_num_fem_static_load_steps] = scale;
    g_num_fem_static_load_steps++;
    return FEM_SUCCESS;
}

static fem_error_t input_register_fem_load_scale_step(int step_index,
                                                      double scale)
{
    int last_step = -1;

    if (step_index < 0 || !isfinite(scale) || scale < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid FEM_LOAD_SCALE_STEP step_index=%d scale=%g",
                         step_index,
                         scale);
    }
    if (g_num_fem_load_scale_steps >= MAX_FEM_LOAD_SCALE_STEPS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Too many FEM_LOAD_SCALE_STEP directives (max=%d)",
                         MAX_FEM_LOAD_SCALE_STEPS);
    }
    if (g_num_fem_load_scale_steps > 0) {
        last_step = g_fem_load_scale_step_ids[g_num_fem_load_scale_steps - 1];
        if (step_index == last_step) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "duplicate FEM_LOAD_SCALE_STEP step_index %d",
                             step_index);
        }
        if (step_index < last_step) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_LOAD_SCALE_STEP step_index %d must be in ascending order",
                             step_index);
        }
    }

    g_fem_load_scale_step_ids[g_num_fem_load_scale_steps] = step_index;
    g_fem_load_scale_step_scales[g_num_fem_load_scale_steps] = scale;
    g_num_fem_load_scale_steps++;
    return FEM_SUCCESS;
}

static fem_error_t input_read_fem_generic_contact_directives(const char *filename)
{
    FILE *fp = NULL;
    char line[1024];
    int line_number = 0;
    int local_feedback_mode_seen = 0;
    int local_feedback_mode_line = -1;
    int local_contact_file_seen = 0;
    int local_contact_file_line = -1;
    int static_load_step_seen = 0;

    CHECK_NULL(filename, "fem generic contact directive filename");

    fp = fopen(filename, "r");
    CHECK_FILE(fp, filename);

    while (fgets(line, sizeof(line), fp) != NULL) {
        char raw_surface_path[MAX_FILENAME_LEN];
        char resolved_path[MAX_FILENAME_LEN];
        int surface_id = -1;
        int part_id = -1;
        int pair_id = -1;
        int surface_i = -1;
        int surface_j = -1;
        int edge_count = 0;
        double k_pen = 0.0;
        double c_pen = 0.0;
        double mu = 0.0;
        double k_adh_n = 0.0;
        double gap_adh_max_m = 0.0;
        double mu_cap = 0.0;
        double k_t_pen = 0.0;
        double u_t_reg_m = 0.0;
        int local_feedback_mode = FEM_LOCAL_FEEDBACK_MODE_NONE;
        int load_step = -1;
        double load_scale = 0.0;
        int parsed = 0;

        ++line_number;
        if (strchr(line, '#')) {
            *strchr(line, '#') = '\0';
        }
        input_parser_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        raw_surface_path[0] = '\0';
        resolved_path[0] = '\0';
        parsed = input_parse_fem_contact_surface_edge_set_line(line,
                                                               &surface_id,
                                                               &part_id,
                                                               raw_surface_path,
                                                               sizeof(raw_surface_path));
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_CONTACT_SURFACE_EDGE_SET at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_build_path_relative_to_file(filename,
                                                                raw_surface_path,
                                                                resolved_path,
                                                                sizeof(resolved_path));
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            err = input_validate_fem_contact_surface_edge_csv(resolved_path,
                                                              surface_id,
                                                              &edge_count);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            err = input_register_fem_contact_surface_edge_set(surface_id,
                                                              part_id,
                                                              resolved_path,
                                                              edge_count);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            continue;
        }

        parsed = input_parse_fem_contact_pair_generic_line(line,
                                                           &pair_id,
                                                           &surface_i,
                                                           &surface_j,
                                                           &k_pen,
                                                           &c_pen,
                                                           &mu);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_CONTACT_PAIR_GENERIC at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_register_fem_contact_pair_generic(pair_id,
                                                                      surface_i,
                                                                      surface_j,
                                                                      k_pen,
                                                                      c_pen,
                                                                      mu);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            continue;
        }

        parsed = input_parse_fem_contact_adhesion_generic_line(line,
                                                               &pair_id,
                                                               &k_adh_n,
                                                               &gap_adh_max_m);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_CONTACT_ADHESION_GENERIC at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_register_fem_contact_adhesion_generic(pair_id,
                                                                          k_adh_n,
                                                                          gap_adh_max_m);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            continue;
        }

        parsed = input_parse_fem_contact_friction_generic_line(line,
                                                               &pair_id,
                                                               &mu_cap,
                                                               &k_t_pen,
                                                               &u_t_reg_m);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_CONTACT_FRICTION_GENERIC at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_register_fem_contact_friction_generic(pair_id,
                                                                          mu_cap,
                                                                          k_t_pen,
                                                                          u_t_reg_m);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            continue;
        }

        parsed = input_parse_fem_local_feedback_mode_line(line, &local_feedback_mode);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_LOCAL_FEEDBACK_MODE at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (local_feedback_mode_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_LOCAL_FEEDBACK_MODE at line %d (first defined at line %d)",
                                 line_number,
                                 local_feedback_mode_line);
            }
            err = input_register_fem_local_feedback_mode(local_feedback_mode);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            local_feedback_mode_seen = 1;
            local_feedback_mode_line = line_number;
            continue;
        }

        raw_surface_path[0] = '\0';
        resolved_path[0] = '\0';
        parsed = input_parse_fem_local_contact_file_line(line,
                                                         raw_surface_path,
                                                         sizeof(raw_surface_path));
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_LOCAL_CONTACT_FILE at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (local_contact_file_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_LOCAL_CONTACT_FILE at line %d (first defined at line %d)",
                                 line_number,
                                 local_contact_file_line);
            }
            err = input_build_path_relative_to_file(filename,
                                                    raw_surface_path,
                                                    resolved_path,
                                                    sizeof(resolved_path));
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            err = input_register_fem_local_contact_file(resolved_path);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            local_contact_file_seen = 1;
            local_contact_file_line = line_number;
            continue;
        }

        parsed = input_parse_fem_static_load_step_line(line, &load_step, &load_scale);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_STATIC_LOAD_STEP at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_register_fem_static_load_step(load_step, load_scale);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            static_load_step_seen = 1;
            continue;
        }
    }

    fclose(fp);
    if (g_fem_local_feedback_mode != FEM_LOCAL_FEEDBACK_MODE_NONE &&
        g_fem_local_contact_file[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOCAL_FEEDBACK_MODE requires FEM_LOCAL_CONTACT_FILE");
    }
    if (g_fem_local_feedback_mode == FEM_LOCAL_FEEDBACK_MODE_NONE &&
        g_fem_local_contact_file[0] != '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOCAL_CONTACT_FILE requires non-NONE FEM_LOCAL_FEEDBACK_MODE");
    }
    if (g_fem_local_feedback_mode != FEM_LOCAL_FEEDBACK_MODE_NONE &&
        g_num_fem_contact_generic_pairs <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOCAL_FEEDBACK_MODE requires FEM_CONTACT_PAIR_GENERIC");
    }
    if (static_load_step_seen && g_num_fem_contact_generic_pairs <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_STATIC_LOAD_STEP requires FEM_CONTACT_PAIR_GENERIC");
    }
    return input_validate_fem_contact_pair_references();
}

static fem_error_t input_read_fem_solver_directives(const char *filename)
{
    FILE *fp = NULL;
    char line[1024];
    int line_number = 0;
    int solver_mode_seen = 0;
    int solver_mode_line = -1;
    int time_step_dt_seen = 0;
    int time_step_dt_line = -1;
    int num_time_steps_seen = 0;
    int num_time_steps_line = -1;
    int outer_max_iter_seen = 0;
    int outer_max_iter_line = -1;
    int outer_tol_seen = 0;
    int outer_tol_line = -1;
    int load_scale_step_seen = 0;

    CHECK_NULL(filename, "fem solver directive filename");

    fp = fopen(filename, "r");
    CHECK_FILE(fp, filename);

    while (fgets(line, sizeof(line), fp) != NULL) {
        fem_solver_mode_t solver_mode = FEM_SOLVER_MODE_STATIC_DEFAULT;
        double dt = 0.0;
        int count = 0;
        double tol = 0.0;
        int step_index = -1;
        double load_scale = 0.0;
        int parsed = 0;

        ++line_number;
        if (strchr(line, '#')) {
            *strchr(line, '#') = '\0';
        }
        input_parser_trim(line);
        if (line[0] == '\0') {
            continue;
        }

        parsed = input_parse_fem_solver_mode_line(line, &solver_mode);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_SOLVER_MODE at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (solver_mode_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_SOLVER_MODE at line %d (first defined at line %d)",
                                 line_number,
                                 solver_mode_line);
            }
            err = input_register_fem_solver_mode(solver_mode);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            solver_mode_seen = 1;
            solver_mode_line = line_number;
            continue;
        }

        parsed = input_parse_fem_time_step_dt_line(line, &dt);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_TIME_STEP_DT at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (time_step_dt_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_TIME_STEP_DT at line %d (first defined at line %d)",
                                 line_number,
                                 time_step_dt_line);
            }
            err = input_register_fem_time_step_dt(dt);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            time_step_dt_seen = 1;
            time_step_dt_line = line_number;
            continue;
        }

        parsed = input_parse_fem_num_time_steps_line(line, &count);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_NUM_TIME_STEPS at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (num_time_steps_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_NUM_TIME_STEPS at line %d (first defined at line %d)",
                                 line_number,
                                 num_time_steps_line);
            }
            err = input_register_fem_num_time_steps(count);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            num_time_steps_seen = 1;
            num_time_steps_line = line_number;
            continue;
        }

        parsed = input_parse_fem_outer_max_iter_line(line, &count);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_OUTER_MAX_ITER at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (outer_max_iter_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_OUTER_MAX_ITER at line %d (first defined at line %d)",
                                 line_number,
                                 outer_max_iter_line);
            }
            err = input_register_fem_outer_max_iter(count);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            outer_max_iter_seen = 1;
            outer_max_iter_line = line_number;
            continue;
        }

        parsed = input_parse_fem_outer_tol_line(line, &tol);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_OUTER_TOL at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = FEM_SUCCESS;

            if (outer_tol_seen) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Duplicate FEM_OUTER_TOL at line %d (first defined at line %d)",
                                 line_number,
                                 outer_tol_line);
            }
            err = input_register_fem_outer_tol(tol);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            outer_tol_seen = 1;
            outer_tol_line = line_number;
            continue;
        }

        parsed = input_parse_fem_load_scale_step_line(line, &step_index, &load_scale);
        if (parsed == -1) {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid FEM_LOAD_SCALE_STEP at line %d",
                             line_number);
        }
        if (parsed == 1) {
            fem_error_t err = input_register_fem_load_scale_step(step_index, load_scale);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            load_scale_step_seen = 1;
            continue;
        }
    }

    fclose(fp);

    if (!solver_mode_seen) {
        if (time_step_dt_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_TIME_STEP_DT requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK or EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE");
        }
        if (num_time_steps_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_NUM_TIME_STEPS requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK or EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE");
        }
        if (outer_max_iter_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_OUTER_MAX_ITER requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        if (outer_tol_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_OUTER_TOL requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        if (load_scale_step_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_LOAD_SCALE_STEP requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        return FEM_SUCCESS;
    }

    if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_IMPLICIT_ONEWAY_NEWMARK) {
        if (!time_step_dt_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK requires FEM_TIME_STEP_DT");
        }
        if (!num_time_steps_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK requires FEM_NUM_TIME_STEPS");
        }
        if (!outer_max_iter_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK requires FEM_OUTER_MAX_ITER");
        }
        if (!outer_tol_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK requires FEM_OUTER_TOL");
        }
        if (g_num_fem_static_load_steps > 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK cannot be combined with FEM_STATIC_LOAD_STEP");
        }
    }

    if (g_analysis.fem_solver_mode == FEM_SOLVER_MODE_EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE) {
        if (!time_step_dt_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE requires FEM_TIME_STEP_DT");
        }
        if (!num_time_steps_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE requires FEM_NUM_TIME_STEPS");
        }
        if (outer_max_iter_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_OUTER_MAX_ITER requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        if (outer_tol_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_OUTER_TOL requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        if (load_scale_step_seen) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_LOAD_SCALE_STEP requires FEM_SOLVER_MODE IMPLICIT_ONEWAY_NEWMARK");
        }
        if (g_num_fem_static_load_steps > 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_SOLVER_MODE EXPLICIT_ONEWAY_CENTRAL_DIFFERENCE cannot be combined with FEM_STATIC_LOAD_STEP");
        }
    }

    if (load_scale_step_seen && g_num_fem_static_load_steps > 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "FEM_LOAD_SCALE_STEP cannot be combined with FEM_STATIC_LOAD_STEP");
    }

    if (load_scale_step_seen) {
        if (g_fem_load_scale_step_ids[0] != 0) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "FEM_LOAD_SCALE_STEP requires step_index 0 when any rows are present");
        }
        for (int i = 0; i < g_num_fem_load_scale_steps; ++i) {
            if (g_fem_load_scale_step_ids[i] >= g_analysis.num_time_steps) {
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "FEM_LOAD_SCALE_STEP step_index %d must be less than FEM_NUM_TIME_STEPS=%d",
                                 g_fem_load_scale_step_ids[i],
                                 g_analysis.num_time_steps);
            }
        }
    }

    return FEM_SUCCESS;
}

static int input_parser_element_node_count(int element_type)
{
    if (element_type == ELEMENT_T3) return 3;
    if (element_type == ELEMENT_T6) return 6;
    if (element_type == ELEMENT_Q4) return 4;
    return 0;
}

static fem_error_t input_parser_load_id_map(const char *path, parser_list_t **lists, int *list_count)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return FEM_SUCCESS; /* optional file */
    }

    char line[256];
    int max_id = *list_count - 1;

    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        if (strncmp(line, "angle=", 6) == 0) continue;
        int id = 0;
        int elem_id = 0;
        if (sscanf(line, "%d %d", &id, &elem_id) != 2) {
            continue;
        }
        if (id <= 0) continue;
        if (id > max_id) {
            int new_count = id + 1;
            parser_list_t *tmp = realloc(*lists, (size_t)new_count * sizeof(parser_list_t));
            if (!tmp) {
                fclose(fp);
                return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to resize id map");
            }
            for (int i = *list_count; i < new_count; ++i) {
                parser_list_init(&tmp[i]);
            }
            *lists = tmp;
            *list_count = new_count;
            max_id = new_count - 1;
        }

        int elem_index = -1;
        if (elem_id >= 0 && elem_id < g_element_id_capacity) {
            elem_index = g_element_id_to_index[elem_id];
        }
        if (elem_index < 0 || elem_index >= g_num_elements) {
            continue;
        }

        int node_count = input_parser_element_node_count(g_element_type[elem_index]);
        for (int k = 0; k < node_count; ++k) {
            int node_idx = g_element_nodes[elem_index][k];
            if (node_idx >= 0 && node_idx < g_num_nodes) {
                parser_list_push(&(*lists)[id], node_idx);
            }
        }
    }

    fclose(fp);
    return FEM_SUCCESS;
}

static fem_error_t input_parser_read_int(FILE *fp, const char *context, int *out)
{
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        if (sscanf(line, "%d", out) == 1) {
            return FEM_SUCCESS;
        }
        break;
    }
    return error_set(FEM_ERROR_FILE_READ, "Failed to read %s", context);
}

static fem_error_t input_read_parser_mesh(const char *mesh_path)
{
    fem_error_t err;
    FILE *fp = fopen(mesh_path, "r");
    CHECK_FILE(fp, mesh_path);

    char line[256];
    int declared_nodes = 0;
    int declared_elements = 0;

    if (!fgets(line, sizeof(line), fp) ||
        FEM_SUCCESS != input_parser_read_int(fp, "node count", &declared_nodes) ||
        !fgets(line, sizeof(line), fp) ||
        FEM_SUCCESS != input_parser_read_int(fp, "element count", &declared_elements)) {
        fclose(fp);
        return error_set(FEM_ERROR_FILE_READ, "Failed to read mesh counts from %s", mesh_path);
    }

    if (declared_nodes <= 0) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT, "Invalid node count in %s", mesh_path);
    }

    err = globals_reserve_nodes(declared_nodes);
    CHECK_ERROR_CLEANUP(err, fclose(fp));
    err = globals_reserve_node_ids(declared_nodes + 1);
    CHECK_ERROR_CLEANUP(err, fclose(fp));
    err = globals_reserve_elements(declared_elements > 0 ? declared_elements : 1);
    CHECK_ERROR_CLEANUP(err, fclose(fp));
    err = globals_reserve_element_ids(declared_elements + 1);
    CHECK_ERROR_CLEANUP(err, fclose(fp));

    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (input_parser_is_label(line, "nodes")) {
            break;
        }
    }
    if (feof(fp)) {
        fclose(fp);
        return error_set(FEM_ERROR_FILE_READ, "nodes section not found in %s", mesh_path);
    }

    g_num_nodes = 0;
    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        if (input_parser_is_label(line, "elements")) {
            break;
        }
        char tok[8][64];
        int nt = input_parser_split_tokens(line, tok, 8);
        if (nt < 4) {
            fclose(fp);
            return error_set(FEM_ERROR_FILE_READ, "Malformed node entry in %s", mesh_path);
        }
        int node_id = atoi(tok[0]);
        double x = atof(tok[1]);
        double y = atof(tok[2]);
        double z = atof(tok[3]);

        err = globals_reserve_nodes(g_num_nodes + 1);
        CHECK_ERROR_CLEANUP(err, fclose(fp));
        globals_initialize_node_entry(g_num_nodes);
        g_node_coords[g_num_nodes][0] = x;
        g_node_coords[g_num_nodes][1] = y;
        g_node_coords[g_num_nodes][2] = z;
        err = input_validate_map_node(node_id, g_num_nodes);
        CHECK_ERROR_CLEANUP(err, fclose(fp));
        g_num_nodes++;
    }

    if (g_num_nodes <= 0) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT, "No nodes parsed from %s", mesh_path);
    }

    g_num_elements = 0;
    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        char tok[10][64];
        int nt = input_parser_split_tokens(line, tok, 10);
        if (nt < 4) {
            fclose(fp);
            return error_set(FEM_ERROR_FILE_READ, "Malformed element entry in %s", mesh_path);
        }
        int element_id = atoi(tok[0]);
        int node_count = nt - 1;
        int element_type;
        if (node_count == 3) {
            element_type = ELEMENT_T3;
        } else if (node_count == 6) {
            element_type = ELEMENT_T6;
        } else {
            fclose(fp);
            return error_set(FEM_ERROR_INVALID_ELEMENT_TYPE,
                             "Unsupported element node count %d in %s", node_count, mesh_path);
        }

        err = globals_reserve_elements(g_num_elements + 1);
        CHECK_ERROR_CLEANUP(err, fclose(fp));
        globals_initialize_element_entry(g_num_elements);
        err = input_validate_map_element(element_id, g_num_elements);
        CHECK_ERROR_CLEANUP(err, fclose(fp));

        for (int i = 0; i < node_count; ++i) {
            int node_id = atoi(tok[i + 1]);
            int node_index = -1;
            err = input_get_node_index(node_id, &node_index);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            g_element_nodes[g_num_elements][i] = node_index;
        }
        for (int i = node_count; i < MAX_NODES_PER_ELEMENT; ++i) {
            g_element_nodes[g_num_elements][i] = -1;
        }
        g_element_type[g_num_elements] = element_type;
        g_element_material[g_num_elements] = 0;
        g_num_elements++;
    }

    fclose(fp);
    return FEM_SUCCESS;
}

static fem_error_t input_read_parser_material(const char *material_path)
{
    fem_error_t err;
    FILE *fp = fopen(material_path, "r");
    CHECK_FILE(fp, material_path);

    char line[256];
    double E = 0.0, nu = 0.0, rho = 0.0;
    int got = 0;

    while (fgets(line, sizeof(line), fp) && got < 3) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        double v = 0.0;
        if (sscanf(line, "%lf", &v) == 1) {
            if (got == 0) E = v;
            else if (got == 1) nu = v;
            else if (got == 2) rho = v;
            got++;
        }
    }
    fclose(fp);

    if (got < 3) {
        return error_set(FEM_ERROR_FILE_READ, "material.dat is incomplete at %s", material_path);
    }

    err = globals_reserve_materials(1);
    CHECK_ERROR(err);
    err = globals_reserve_material_ids(2);
    CHECK_ERROR(err);
    globals_initialize_material_entry(0);
    g_material_props[0][0] = E;
    g_material_props[0][1] = nu;
    g_material_props[0][2] = 1.0;
    g_material_props[0][3] = rho;
    g_material_type[0] = MATERIAL_PLANE_STRESS;
    g_num_materials = 1;
    err = input_validate_map_material(1, 0);
    CHECK_ERROR(err);
    return FEM_SUCCESS;
}

static fem_error_t input_parse_parser_legacy_spc(const char *line)
{
    int node_id = -1;
    double disp = 0.0;
    char component_field[16] = {0};
    char normalized[512];
    char tok[32][64];
    int nt = 0;
    fem_error_t err;

    if (line == NULL) {
        return FEM_ERROR_INVALID_INPUT;
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    for (size_t i = 0; normalized[i] != '\0'; ++i) {
        if (normalized[i] == '=' || normalized[i] == ',' ||
            normalized[i] == '(' || normalized[i] == ')') {
            normalized[i] = ' ';
        }
    }

    nt = input_parser_split_tokens(normalized, tok, 32);
    if (nt < 2 || strcmp(tok[0], "SPC") != 0) {
        return FEM_ERROR_FILE_READ;
    }
    for (int i = 1; i < nt; ++i) {
        if (strcmp(tok[i], "G") == 0 && i + 1 < nt) {
            node_id = atoi(tok[++i]);
        } else if (strcmp(tok[i], "C") == 0 && i + 1 < nt) {
            strncpy(component_field, tok[++i], sizeof(component_field) - 1);
            component_field[sizeof(component_field) - 1] = '\0';
        } else if (strcmp(tok[i], "D") == 0 && i + 1 < nt) {
            disp = atof(tok[++i]);
        }
    }
    if (node_id <= 0 || component_field[0] == '\0') {
        return FEM_ERROR_FILE_READ;
    }

    int node_index = -1;
    err = input_get_node_index(node_id, &node_index);
    CHECK_ERROR(err);

    return input_apply_constraint_mask(node_index,
                                       component_field,
                                       disp,
                                       "legacy_spc",
                                       0);
}

static fem_error_t input_parse_parser_legacy_force(const char *line)
{
    int node_id = -1;
    double force = 0.0;
    double n1 = 1.0, n2 = 0.0, n3 = 0.0;
    int have_force = 0;
    int have_n = 0;
    char normalized[512];
    char tok[32][64];
    int nt = 0;
    fem_error_t err;

    if (line == NULL) {
        return FEM_ERROR_INVALID_INPUT;
    }

    strncpy(normalized, line, sizeof(normalized) - 1);
    normalized[sizeof(normalized) - 1] = '\0';
    for (size_t i = 0; normalized[i] != '\0'; ++i) {
        if (normalized[i] == '=' || normalized[i] == ',' ||
            normalized[i] == '(' || normalized[i] == ')') {
            normalized[i] = ' ';
        }
    }

    nt = input_parser_split_tokens(normalized, tok, 32);
    if (nt < 2 || strcmp(tok[0], "FORCE") != 0) {
        return FEM_ERROR_FILE_READ;
    }
    for (int i = 1; i < nt; ++i) {
        if (strcmp(tok[i], "G") == 0 && i + 1 < nt) {
            node_id = atoi(tok[++i]);
        } else if (strcmp(tok[i], "F") == 0 && i + 1 < nt) {
            force = atof(tok[++i]);
            have_force = 1;
        } else if (strcmp(tok[i], "N") == 0 && i + 3 < nt) {
            n1 = atof(tok[++i]);
            n2 = atof(tok[++i]);
            n3 = atof(tok[++i]);
            have_n = 1;
        }
    }
    if (node_id <= 0 || !have_force || !have_n) {
        return FEM_ERROR_FILE_READ;
    }

    int node_index = -1;
    err = input_get_node_index(node_id, &node_index);
    CHECK_ERROR(err);

    g_node_force[node_index][0] += force * n1;
    g_node_force[node_index][1] += force * n2;
    g_node_force[node_index][2] += force * n3;

    return FEM_SUCCESS;
}

static fem_error_t input_read_parser_boundary(const char *boundary_path, const char *base_dir)
{
    fem_error_t err = FEM_SUCCESS;
    char line[512];
    int line_no = 0;
    enum { MODE_NONE, MODE_FIX, MODE_FORCE } mode = MODE_NONE;
    int legacy_spc_count = 0;
    int legacy_force_count = 0;
    int fixed_spc_count = 0;
    int fixed_force_count = 0;
    FILE *fp = fopen(boundary_path, "r");
    CHECK_FILE(fp, boundary_path);

    parser_list_t *surface_lists = NULL;
    parser_list_t *ridge_lists = NULL;
    int surface_count = 0;
    int ridge_count = 0;

    char surface_path[1024];
    char ridge_path[1024];
    snprintf(surface_path, sizeof(surface_path), "%s/mesh/surface.dat", base_dir);
    snprintf(ridge_path, sizeof(ridge_path), "%s/mesh/ridgeline.dat", base_dir);
    err = input_parser_load_id_map(surface_path, &surface_lists, &surface_count);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }
    err = input_parser_load_id_map(ridge_path, &ridge_lists, &ridge_count);
    if (err != FEM_SUCCESS) {
        goto cleanup;
    }

    while (fgets(line, sizeof(line), fp)) {
        line_no++;
        input_parser_trim(line);
        if (line[0] == '\0') continue;

        if (strncmp(line, "Total number of Boundary Conditions", 35) == 0) {
            if (!fgets(line, sizeof(line), fp)) {
                break;
            }
            continue;
        }
        if (strncmp(line, "UNITSYS", 7) == 0) {
            printf("  Info: boundary.dat declares UNITSYS (forces already in N)\n");
            continue;
        }
        if (strncmp(line, "PARAM", 5) == 0) {
            double k6rot = 0.0;
            if (input_parse_param_k6rot_value(line, &k6rot)) {
                g_shell_k6rot = k6rot;
                g_fem_dof_per_node = 3;
            }
            continue;
        }
        if (strncmp(line, "SPC", 3) == 0) {
            if (strstr(line, "SID=") || strstr(line, "G=") || strstr(line, "C=")) {
                err = input_parse_parser_legacy_spc(line);
                if (err == FEM_SUCCESS) {
                    legacy_spc_count++;
                }
            } else {
                err = input_parse_nastran_spc(NULL, line);
                if (err == FEM_SUCCESS) {
                    fixed_spc_count++;
                }
            }
            if (err != FEM_SUCCESS) {
                err = input_parse_parser_legacy_spc(line);
                if (err != FEM_SUCCESS) {
                    err = error_set(FEM_ERROR_FILE_READ,
                                    "Failed to parse SPC card at %s:%d",
                                    boundary_path, line_no);
                    goto cleanup;
                }
                legacy_spc_count++;
            }
            continue;
        }
        if (strncmp(line, "FORCE", 5) == 0) {
            if (strstr(line, "SID=") || strstr(line, "G=") || strstr(line, "N=(")) {
                err = input_parse_parser_legacy_force(line);
                if (err == FEM_SUCCESS) {
                    legacy_force_count++;
                }
            } else {
                err = input_parse_nastran_force(NULL, line);
                if (err == FEM_SUCCESS) {
                    fixed_force_count++;
                }
            }
            if (err != FEM_SUCCESS) {
                err = input_parse_parser_legacy_force(line);
                if (err != FEM_SUCCESS) {
                    err = error_set(FEM_ERROR_FILE_READ,
                                    "Failed to parse FORCE card at %s:%d",
                                    boundary_path, line_no);
                    goto cleanup;
                }
                legacy_force_count++;
            }
            continue;
        }

        if (input_parser_is_label(line, "Fix")) {
            mode = MODE_FIX;
            continue;
        }
        if (input_parser_is_label(line, "Force")) {
            mode = MODE_FORCE;
            continue;
        }
        if (line[0] != '\0' && isalpha((unsigned char)line[0]) &&
            (strstr(line, "(") || input_parser_is_label(line, "Fixed"))) {
            mode = MODE_NONE;
            continue;
        }

        if (mode == MODE_FIX) {
            char tok[8][64];
            int nt = input_parser_split_tokens(line, tok, 8);
            if (nt < 4) continue;
            const char *target = tok[0];
            int tid = atoi(tok[1]);
            const char *mask = tok[2];
            double disp = atof(tok[3]);

            if (strcmp(target, "ridgeline") == 0) {
                err = error_set(FEM_ERROR_INVALID_INPUT,
                                "ridgeline boundary targets are not supported in parser packages: %s:%d",
                                boundary_path, line_no);
                goto cleanup;
            }

            int *mark = calloc((size_t)g_num_nodes, sizeof(int));
            if (!mark) {
                err = error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to allocate mark array");
                goto cleanup;
            }

            if (strcmp(target, "node") == 0) {
                int node_index = -1;
                err = input_get_node_index(tid, &node_index);
                if (err == FEM_SUCCESS) {
                    err = input_apply_constraint_mask(node_index,
                                                      mask,
                                                      disp,
                                                      boundary_path,
                                                      line_no);
                    CHECK_ERROR(err);
                }
            } else if (strcmp(target, "surface") == 0 && tid < surface_count) {
                parser_list_t *lst = &surface_lists[tid];
                for (int i = 0; i < lst->size; ++i) {
                    int node_index = lst->data[i];
                    if (node_index < 0 || node_index >= g_num_nodes || mark[node_index]) continue;
                    mark[node_index] = 1;
                    err = input_apply_constraint_mask(node_index,
                                                      mask,
                                                      disp,
                                                      boundary_path,
                                                      line_no);
                    CHECK_ERROR(err);
                }
            } else if (strcmp(target, "ridgeline") == 0 && tid < ridge_count) {
                parser_list_t *lst = &ridge_lists[tid];
                for (int i = 0; i < lst->size; ++i) {
                    int node_index = lst->data[i];
                    if (node_index < 0 || node_index >= g_num_nodes || mark[node_index]) continue;
                    mark[node_index] = 1;
                    err = input_apply_constraint_mask(node_index,
                                                      mask,
                                                      disp,
                                                      boundary_path,
                                                      line_no);
                    CHECK_ERROR(err);
                }
            }
            free(mark);
            continue;
        }

        if (mode == MODE_FORCE) {
            char tok[8][64];
            int nt = input_parser_split_tokens(line, tok, 8);
            if (nt < 5) continue;
            const char *target = tok[0];
            int tid = atoi(tok[1]);
            int axis = atoi(tok[3]);
            double value = atof(tok[4]);

            if (strcmp(target, "ridgeline") == 0) {
                err = error_set(FEM_ERROR_INVALID_INPUT,
                                "ridgeline load targets are not supported in parser packages: %s:%d",
                                boundary_path, line_no);
                goto cleanup;
            }

            int *mark = calloc((size_t)g_num_nodes, sizeof(int));
            if (!mark) {
                err = error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to allocate mark array");
                goto cleanup;
            }

            if (strcmp(target, "node") == 0) {
                int node_index = -1;
                err = input_get_node_index(tid, &node_index);
                if (err == FEM_SUCCESS) {
                    if (axis == 1) g_node_force[node_index][0] += value;
                    if (axis == 2) g_node_force[node_index][1] += value;
                    if (axis == 3) g_node_force[node_index][2] += value;
                }
            } else if (strcmp(target, "surface") == 0 && tid < surface_count) {
                parser_list_t *lst = &surface_lists[tid];
                for (int i = 0; i < lst->size; ++i) {
                    int node_index = lst->data[i];
                    if (node_index < 0 || node_index >= g_num_nodes || mark[node_index]) continue;
                    mark[node_index] = 1;
                    if (axis == 1) g_node_force[node_index][0] += value;
                    if (axis == 2) g_node_force[node_index][1] += value;
                    if (axis == 3) g_node_force[node_index][2] += value;
                }
            } else if (strcmp(target, "ridgeline") == 0 && tid < ridge_count) {
                parser_list_t *lst = &ridge_lists[tid];
                for (int i = 0; i < lst->size; ++i) {
                    int node_index = lst->data[i];
                    if (node_index < 0 || node_index >= g_num_nodes || mark[node_index]) continue;
                    mark[node_index] = 1;
                    if (axis == 1) g_node_force[node_index][0] += value;
                    if (axis == 2) g_node_force[node_index][1] += value;
                    if (axis == 3) g_node_force[node_index][2] += value;
                }
            }
            free(mark);
            continue;
        }
    }

cleanup:
    fclose(fp);
    for (int i = 0; i < surface_count; ++i) {
        parser_list_free(&surface_lists[i]);
    }
    for (int i = 0; i < ridge_count; ++i) {
        parser_list_free(&ridge_lists[i]);
    }
    free(surface_lists);
    free(ridge_lists);
    if (err == FEM_SUCCESS &&
        (legacy_spc_count > 0 || fixed_spc_count > 0 ||
         legacy_force_count > 0 || fixed_force_count > 0)) {
        printf("  parser boundary cards: SPC legacy=%d fixed=%d, FORCE legacy=%d fixed=%d\n",
               legacy_spc_count, fixed_spc_count, legacy_force_count, fixed_force_count);
    }
    return err;
}

static fem_error_t input_read_parser_pressure(const char *pressure_path)
{
    FILE *fp = fopen(pressure_path, "r");
    char line[256];
    char tokens[10][64];
    int declared_count = -1;
    int loaded_count = 0;
    int uses_shared_header = 0;
    int row_mode = 0;
    double header_pressure_value = 0.0;
    fem_error_t err = FEM_SUCCESS;

    if (!fp) {
        if (errno == ENOENT) {
            return FEM_SUCCESS;
        }
        return error_set(FEM_ERROR_FILE_READ,
                         "Cannot open parser pressure file: %s",
                         pressure_path);
    }

    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;
        int header_token_count = input_parser_split_tokens(line, tokens, 10);
        if (header_token_count == 1) {
            if (!input_parse_strict_int_token(tokens[0], &declared_count)) {
                fclose(fp);
                return error_set(FEM_ERROR_FILE_READ,
                                 "Malformed parser pressure header in %s",
                                 pressure_path);
            }
            uses_shared_header = 0;
        } else if (header_token_count == 2) {
            if (!input_parse_strict_int_token(tokens[0], &declared_count) ||
                !input_parse_strict_double_token(tokens[1], &header_pressure_value)) {
                fclose(fp);
                return error_set(FEM_ERROR_FILE_READ,
                                 "Malformed parser pressure header in %s",
                                 pressure_path);
            }
            uses_shared_header = 1;
        } else {
            fclose(fp);
            return error_set(FEM_ERROR_FILE_READ,
                             "Malformed parser pressure header in %s",
                             pressure_path);
        }
        break;
    }

    if (declared_count < 0) {
        fclose(fp);
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Invalid parser pressure count in %s",
                         pressure_path);
    }
    if (declared_count > MAX_TRACTION_SURFACES) {
        fclose(fp);
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Exceeded maximum pressure surfaces (%d)",
                         MAX_TRACTION_SURFACES);
    }

    g_pressure_value = uses_shared_header ? header_pressure_value : 0.0;
    g_has_pressure = declared_count > 0 ? 1 : 0;
    g_num_pressure_surfaces = 0;

    while (loaded_count < declared_count && fgets(line, sizeof(line), fp)) {
        int node_ids[T6_NODES_PER_ELEMENT] = {0, 0, 0, 0, 0, 0};
        int token_count = 0;
        int node_count = 0;
        double surface_pressure = 0.0;

        input_parser_trim(line);
        if (line[0] == '\0') continue;
        token_count = input_parser_split_tokens(line, tokens, 10);
        if (uses_shared_header) {
            if (token_count != 3 ||
                !input_parse_strict_int_token(tokens[0], &node_ids[0]) ||
                !input_parse_strict_int_token(tokens[1], &node_ids[1]) ||
                !input_parse_strict_int_token(tokens[2], &node_ids[2])) {
                fclose(fp);
                return error_set(FEM_ERROR_FILE_READ,
                                 "Malformed parser pressure surface in %s",
                                 pressure_path);
            }
            node_count = 3;
            surface_pressure = header_pressure_value;
        } else {
            if (row_mode == 0) {
                if (token_count == 4) {
                    row_mode = 1; /* current T3 multivalue */
                } else if (token_count == 5) {
                    row_mode = 2; /* mixed T3 */
                } else if (token_count == 8) {
                    row_mode = 3; /* mixed T6 */
                } else {
                    fclose(fp);
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Malformed parser pressure surface in %s",
                                     pressure_path);
                }
            }

            if (row_mode == 1) {
                if (token_count != 4 ||
                    !input_parse_strict_int_token(tokens[0], &node_ids[0]) ||
                    !input_parse_strict_int_token(tokens[1], &node_ids[1]) ||
                    !input_parse_strict_int_token(tokens[2], &node_ids[2]) ||
                    !input_parse_strict_double_token(tokens[3], &surface_pressure)) {
                    fclose(fp);
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Malformed parser pressure surface in %s",
                                     pressure_path);
                }
                node_count = 3;
            } else if (row_mode == 2) {
                if (token_count != 5 ||
                    !input_parse_strict_int_token(tokens[0], &node_count) ||
                    node_count != 3 ||
                    !input_parse_strict_int_token(tokens[1], &node_ids[0]) ||
                    !input_parse_strict_int_token(tokens[2], &node_ids[1]) ||
                    !input_parse_strict_int_token(tokens[3], &node_ids[2]) ||
                    !input_parse_strict_double_token(tokens[4], &surface_pressure)) {
                    fclose(fp);
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Malformed parser pressure surface in %s",
                                     pressure_path);
                }
            } else if (row_mode == 3) {
                if (token_count != 8 ||
                    !input_parse_strict_int_token(tokens[0], &node_count) ||
                    node_count != T6_NODES_PER_ELEMENT ||
                    !input_parse_strict_int_token(tokens[1], &node_ids[0]) ||
                    !input_parse_strict_int_token(tokens[2], &node_ids[1]) ||
                    !input_parse_strict_int_token(tokens[3], &node_ids[2]) ||
                    !input_parse_strict_int_token(tokens[4], &node_ids[3]) ||
                    !input_parse_strict_int_token(tokens[5], &node_ids[4]) ||
                    !input_parse_strict_int_token(tokens[6], &node_ids[5]) ||
                    !input_parse_strict_double_token(tokens[7], &surface_pressure)) {
                    fclose(fp);
                    return error_set(FEM_ERROR_FILE_READ,
                                     "Malformed parser pressure surface in %s",
                                     pressure_path);
                }
            } else {
                fclose(fp);
                return error_set(FEM_ERROR_FILE_READ,
                                 "Malformed parser pressure surface in %s",
                                 pressure_path);
            }
        }
        for (int i = 0; i < node_count; ++i) {
            if (node_ids[i] <= 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Parser pressure surface requires positive node ids in %s",
                                 pressure_path);
            }
        }

        err = input_store_pressure_surface_from_node_ids(
            loaded_count,
            node_count,
            node_ids,
            surface_pressure);
        CHECK_ERROR_CLEANUP(err, fclose(fp));
        if (!uses_shared_header && loaded_count == 0) {
            g_pressure_value = surface_pressure;
        }
        loaded_count++;
    }

    fclose(fp);

    if (loaded_count != declared_count) {
        return error_set(FEM_ERROR_FILE_READ,
                         "Parser pressure file %s ended early",
                         pressure_path);
    }

    g_num_pressure_surfaces = loaded_count;
    if (g_num_pressure_surfaces > 0) {
        printf("  parser pressure surfaces: count=%d value=%.16e\n",
               g_num_pressure_surfaces,
               g_pressure_value);
    }

    return FEM_SUCCESS;
}

static fem_error_t input_store_pressure_surface_from_node_ids(int surface_index,
                                                              int node_count,
                                                              const int *node_ids,
                                                              double pressure)
{
    fem_error_t err = FEM_SUCCESS;
    int node_index = -1;

    if (surface_index < 0 || surface_index >= MAX_TRACTION_SURFACES) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Exceeded maximum pressure surfaces (%d)",
                         MAX_TRACTION_SURFACES);
    }
    if (node_count != 3 && node_count != T6_NODES_PER_ELEMENT) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "Unsupported pressure surface node count %d",
                         node_count);
    }

    g_pressure_surface_node_counts[surface_index] = node_count;
    for (int i = 0; i < T6_NODES_PER_ELEMENT; ++i) {
        g_pressure_surface_nodes[surface_index][i] = -1;
    }
    for (int i = 0; i < MAX_SURFACE_NODES; ++i) {
        g_pressure_surfaces[surface_index][i] = -1;
    }

    for (int i = 0; i < node_count; ++i) {
        err = input_get_node_index(node_ids[i], &node_index);
        CHECK_ERROR(err);
        g_pressure_surface_nodes[surface_index][i] = node_index;
        if (node_count == 3 && i < MAX_SURFACE_NODES) {
            g_pressure_surfaces[surface_index][i] = node_index;
        }
    }
    g_pressure_surface_values[surface_index] = pressure;

    return FEM_SUCCESS;
}

fem_error_t input_read_parser_package(const char *directory)
{
    fem_error_t err;
    char mesh_path[1024];
    char material_path[1024];
    char boundary_path[1024];
    char pressure_path[1024];

    snprintf(mesh_path, sizeof(mesh_path), "%s/mesh/mesh.dat", directory);
    snprintf(material_path, sizeof(material_path), "%s/material/material.dat", directory);
    snprintf(boundary_path, sizeof(boundary_path), "%s/Boundary Conditions/boundary.dat", directory);
    snprintf(pressure_path, sizeof(pressure_path), "%s/Boundary Conditions/pressure.dat", directory);

    if (!input_parser_has_mesh_root(directory)) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND, "mesh/mesh.dat not found under %s", directory);
    }

    err = input_read_parser_mesh(mesh_path);
    CHECK_ERROR(err);
    err = input_read_parser_material(material_path);
    CHECK_ERROR(err);
    err = input_read_parser_boundary(boundary_path, directory);
    CHECK_ERROR(err);
    err = input_read_parser_pressure(pressure_path);
    CHECK_ERROR(err);

    g_analysis.num_nodes = g_num_nodes;
    g_analysis.num_elements = g_num_elements;
    g_analysis.num_materials = g_num_materials;
    g_total_dof = g_num_nodes * g_fem_dof_per_node;
    snprintf(g_analysis.title, sizeof(g_analysis.title), "Parser package: %s", directory);
    return FEM_SUCCESS;
}

/* Parse Nastran GRID card */
fem_error_t input_parse_nastran_grid(input_control_t *input, const char *line)
{
    if (strncmp(line, "GRID*", 5) == 0) {
        return input_parse_nastran_grid_long(input, line);
    }
    return input_parse_nastran_grid_short(input, line);
}

static fem_error_t input_parse_nastran_grid_short(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int grid_id;
    double x = 0.0, y = 0.0, z = 0.0;

    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[1], &grid_id);
    CHECK_ERROR(err);

    if (fields[3][0] != '\0') {
        err = input_nastran_get_double(fields[3], &x);
        CHECK_ERROR(err);
    }
    if (fields[4][0] != '\0') {
        err = input_nastran_get_double(fields[4], &y);
        CHECK_ERROR(err);
    }
    if (fields[5][0] != '\0') {
        err = input_nastran_get_double(fields[5], &z);
        if (err != FEM_SUCCESS) z = 0.0;
    }

    err = globals_reserve_nodes(g_num_nodes + 1);
    CHECK_ERROR(err);

    int node_index = g_num_nodes;
    globals_initialize_node_entry(node_index);

    err = input_validate_map_node(grid_id, node_index);
    CHECK_ERROR(err);

    g_node_coords[node_index][0] = x;
    g_node_coords[node_index][1] = y;
    g_node_coords[node_index][2] = z;

    g_num_nodes++;

    return FEM_SUCCESS;
}

static fem_error_t input_parse_nastran_grid_long(input_control_t *input, const char *first_line)
{
    char line[256];
    char cont_line[256];
    char fields[16][17];
    int field_count = 0;
    fem_error_t err;
    int grid_id = 0;
    int cp = 0;
    double x = 0.0, y = 0.0, z = 0.0;

    memset(fields, 0, sizeof(fields));
    snprintf(line, sizeof(line), "%s", first_line);

    /* Extract initial fields (columns 9-72, 16 chars each) */
    int len = strlen(line);
    int start = 8;
    while (start < len && field_count < 16) {
        int copy_len = ((start + 16) <= len) ? 16 : (len - start);
        if (copy_len <= 0) break;
        memcpy(fields[field_count], line + start, copy_len);
        fields[field_count][copy_len] = '\0';
        input_nastran_trim(fields[field_count]);
        field_count++;
        start += 16;
    }

    int has_more = input_nastran_line_has_continuation(line);

    while (has_more && field_count < 16) {
        if (!fgets(cont_line, sizeof(cont_line), input->file_ptr)) {
            return error_set(FEM_ERROR_FILE_READ, "Unexpected EOF in GRID* continuation line");
        }
        input->line_number++;
        input_nastran_normalize_line(cont_line);

        if (cont_line[0] == '\0' || cont_line[0] == '$') {
            continue;
        }

        if (cont_line[0] != '*' && cont_line[0] != '+') {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "Invalid continuation line for GRID* at line %d", input->line_number);
        }

        len = strlen(cont_line);
        start = 1;
        while (start < len && field_count < 16) {
            int copy_len = ((start + 16) <= len) ? 16 : (len - start);
            if (copy_len <= 0) break;
            memcpy(fields[field_count], cont_line + start, copy_len);
            fields[field_count][copy_len] = '\0';
            input_nastran_trim(fields[field_count]);
            field_count++;
            start += 16;
        }

        has_more = input_nastran_line_has_continuation(cont_line);
    }

    if (field_count < 1) {
        return error_set(FEM_ERROR_FILE_READ, "GRID* card missing fields");
    }

    err = input_nastran_get_integer(fields[0], &grid_id);
    CHECK_ERROR(err);

    if (field_count > 1 && fields[1][0] != '\0') {
        err = input_nastran_get_integer(fields[1], &cp);
        if (err != FEM_SUCCESS) cp = 0;
    }
    (void)cp; /* Currently unused */

    if (field_count > 2 && fields[2][0] != '\0') {
        err = input_nastran_get_double(fields[2], &x);
        CHECK_ERROR(err);
    }
    if (field_count > 3 && fields[3][0] != '\0') {
        err = input_nastran_get_double(fields[3], &y);
        CHECK_ERROR(err);
    }
    if (field_count > 4 && fields[4][0] != '\0') {
        err = input_nastran_get_double(fields[4], &z);
        if (err != FEM_SUCCESS) z = 0.0;
    }

    err = globals_reserve_nodes(g_num_nodes + 1);
    CHECK_ERROR(err);

    int node_index = g_num_nodes;
    globals_initialize_node_entry(node_index);

    err = input_validate_map_node(grid_id, node_index);
    CHECK_ERROR(err);
    g_node_coords[node_index][0] = x;
    g_node_coords[node_index][1] = y;
    g_node_coords[node_index][2] = z;

    g_num_nodes++;

    return FEM_SUCCESS;
}

/* Parse Nastran CTRIA3 card */
fem_error_t input_parse_nastran_ctria3(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int eid, pid, g1, g2, g3;

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract element data */
    err = input_nastran_get_integer(fields[1], &eid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[2], &pid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[3], &g1);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[4], &g2);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[5], &g3);
    CHECK_ERROR(err);

    err = globals_reserve_elements(g_num_elements + 1);
    CHECK_ERROR(err);
    err = input_ensure_nastran_element_capacity(g_num_elements + 1);
    CHECK_ERROR(err);

    int elem_index = g_num_elements;
    globals_initialize_element_entry(elem_index);
    err = input_validate_map_element(eid, elem_index);
    CHECK_ERROR(err);

    err = input_get_node_index(g1, &g_element_nodes[elem_index][0]);
    CHECK_ERROR(err);
    err = input_get_node_index(g2, &g_element_nodes[elem_index][1]);
    CHECK_ERROR(err);
    err = input_get_node_index(g3, &g_element_nodes[elem_index][2]);
    CHECK_ERROR(err);

    /* Fill unused nodes with -1 */
    for (int i = 3; i < MAX_NODES_PER_ELEMENT; i++) {
        g_element_nodes[elem_index][i] = -1;
    }

    g_element_type[elem_index] = ELEMENT_T3;
    g_element_material[elem_index] = -1;
    g_nastran_element_property[elem_index] = (pid > 0) ? pid : -1;

    g_num_elements++;

    return FEM_SUCCESS;
}

/* Parse Nastran CQUAD4 card */
fem_error_t input_parse_nastran_cquad4(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int eid, pid, g1, g2, g3, g4;

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract element data */
    err = input_nastran_get_integer(fields[1], &eid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[2], &pid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[3], &g1);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[4], &g2);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[5], &g3);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[6], &g4);
    CHECK_ERROR(err);

    err = globals_reserve_elements(g_num_elements + 1);
    CHECK_ERROR(err);
    err = input_ensure_nastran_element_capacity(g_num_elements + 1);
    CHECK_ERROR(err);

    int elem_index = g_num_elements;
    globals_initialize_element_entry(elem_index);
    err = input_validate_map_element(eid, elem_index);
    CHECK_ERROR(err);

    err = input_get_node_index(g1, &g_element_nodes[elem_index][0]);
    CHECK_ERROR(err);
    err = input_get_node_index(g2, &g_element_nodes[elem_index][1]);
    CHECK_ERROR(err);
    err = input_get_node_index(g3, &g_element_nodes[elem_index][2]);
    CHECK_ERROR(err);
    err = input_get_node_index(g4, &g_element_nodes[elem_index][3]);
    CHECK_ERROR(err);

    /* Fill unused nodes with -1 */
    for (int i = 4; i < MAX_NODES_PER_ELEMENT; i++) {
        g_element_nodes[elem_index][i] = -1;
    }

    g_element_type[elem_index] = ELEMENT_Q4;
    g_element_material[elem_index] = -1;
    g_nastran_element_property[elem_index] = (pid > 0) ? pid : -1;

    g_num_elements++;

    return FEM_SUCCESS;
}

/* Parse Nastran CTRIA6 card */
fem_error_t input_parse_nastran_ctria6(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int eid, pid, g1, g2, g3, g4, g5, g6;

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract element data */
    err = input_nastran_get_integer(fields[1], &eid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[2], &pid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[3], &g1);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[4], &g2);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[5], &g3);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[6], &g4);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[7], &g5);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[8], &g6);
    CHECK_ERROR(err);

    err = globals_reserve_elements(g_num_elements + 1);
    CHECK_ERROR(err);
    err = input_ensure_nastran_element_capacity(g_num_elements + 1);
    CHECK_ERROR(err);

    int elem_index = g_num_elements;
    globals_initialize_element_entry(elem_index);
    err = input_validate_map_element(eid, elem_index);
    CHECK_ERROR(err);

    err = input_get_node_index(g1, &g_element_nodes[elem_index][0]);
    CHECK_ERROR(err);
    err = input_get_node_index(g2, &g_element_nodes[elem_index][1]);
    CHECK_ERROR(err);
    err = input_get_node_index(g3, &g_element_nodes[elem_index][2]);
    CHECK_ERROR(err);
    err = input_get_node_index(g4, &g_element_nodes[elem_index][3]);
    CHECK_ERROR(err);
    err = input_get_node_index(g5, &g_element_nodes[elem_index][4]);
    CHECK_ERROR(err);
    err = input_get_node_index(g6, &g_element_nodes[elem_index][5]);
    CHECK_ERROR(err);

    /* Fill unused nodes with -1 */
    for (int i = 6; i < MAX_NODES_PER_ELEMENT; i++) {
        g_element_nodes[elem_index][i] = -1;
    }

    g_element_type[elem_index] = ELEMENT_T6;
    g_element_material[elem_index] = -1;
    g_nastran_element_property[elem_index] = (pid > 0) ? pid : -1;

    g_num_elements++;

    return FEM_SUCCESS;
}

fem_error_t input_parse_nastran_pshell(input_control_t *input, const char *line)
{
    (void)input;
    char fields[12][9];
    fem_error_t err;
    int pid;
    int mid1 = 0;
    double thickness = 0.0;

    err = input_nastran_parse_fixed_format(line, fields, 12);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[1], &pid);
    CHECK_ERROR(err);

    if (fields[2][0] != '\0') {
        err = input_nastran_get_integer(fields[2], &mid1);
        if (err != FEM_SUCCESS) mid1 = 0;
    }

    if (fields[3][0] != '\0') {
        err = input_nastran_get_double(fields[3], &thickness);
        if (err != FEM_SUCCESS) thickness = 0.0;
    }

    if (g_nastran_pshell_count >= MAX_NASTRAN_PROPERTIES) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Exceeded maximum supported PSHELL cards (%d)", MAX_NASTRAN_PROPERTIES);
    }

    g_nastran_pshells[g_nastran_pshell_count].pid = pid;
    g_nastran_pshells[g_nastran_pshell_count].mid = mid1;
    g_nastran_pshells[g_nastran_pshell_count].thickness = thickness;
    g_nastran_pshells[g_nastran_pshell_count].material_index = -1;
    g_nastran_pshell_count++;

    return FEM_SUCCESS;
}

/* Parse Nastran MAT1 card */
fem_error_t input_parse_nastran_mat1(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int mid;
    double E, G, nu, rho;

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract material data */
    err = input_nastran_get_integer(fields[1], &mid);
    CHECK_ERROR(err);

    err = input_nastran_get_double(fields[2], &E);
    CHECK_ERROR(err);

    err = input_nastran_get_double(fields[3], &G);
    if (err != FEM_SUCCESS) G = 0.0; /* Optional */

    err = input_nastran_get_double(fields[4], &nu);
    CHECK_ERROR(err);

    err = input_nastran_get_double(fields[5], &rho);
    if (err != FEM_SUCCESS) rho = 1.0; /* Default density */

    err = globals_reserve_materials(g_num_materials + 1);
    CHECK_ERROR(err);
    err = globals_reserve_material_ids(mid + 1);
    CHECK_ERROR(err);

    int mat_index = g_num_materials;
    globals_initialize_material_entry(mat_index);
    g_material_props[mat_index][0] = E;    /* Young's modulus */
    g_material_props[mat_index][1] = nu;   /* Poisson's ratio */
    g_material_props[mat_index][2] = 1.0;  /* thickness (default) */
    g_material_props[mat_index][3] = rho;  /* density */
    g_material_type[mat_index] = MATERIAL_PLANE_STRESS;
    err = input_validate_map_material(mid, mat_index);
    CHECK_ERROR(err);

    g_num_materials++;

    return FEM_SUCCESS;
}

/* Parse Nastran SPC card */
fem_error_t input_parse_nastran_spc(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int sid, g;
    double d;
    char component_field[16];

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract SPC data */
    err = input_nastran_get_integer(fields[1], &sid);
    CHECK_ERROR(err);
    (void)sid;

    err = input_nastran_get_integer(fields[2], &g);
    CHECK_ERROR(err);

    err = input_nastran_get_double(fields[4], &d);
    if (err != FEM_SUCCESS) d = 0.0; /* Default displacement */

    /* Apply constraint */
    int node_index = -1;
    err = input_get_node_index(g, &node_index);
    CHECK_ERROR(err);

    strncpy(component_field, fields[3], sizeof(component_field) - 1);
    component_field[sizeof(component_field) - 1] = '\0';
    input_nastran_trim(component_field);

    return input_apply_constraint_mask(node_index,
                                       component_field,
                                       d,
                                       "nastran_spc",
                                       0);
}

/* Parse Nastran FORCE card */
fem_error_t input_parse_nastran_force(input_control_t *input, const char *line)
{
    (void)input;
    char fields[10][9];
    fem_error_t err;
    int sid, g, cid;
    double f, n1, n2, n3;

    /* Parse fixed format fields */
    err = input_nastran_parse_fixed_format(line, fields, 10);
    CHECK_ERROR(err);

    /* Extract force data */
    err = input_nastran_get_integer(fields[1], &sid);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[2], &g);
    CHECK_ERROR(err);

    err = input_nastran_get_integer(fields[3], &cid);
    if (err != FEM_SUCCESS) cid = 0; /* Basic coordinate system */

    err = input_nastran_get_double(fields[4], &f);
    CHECK_ERROR(err);

    err = input_nastran_get_double(fields[5], &n1);
    if (err != FEM_SUCCESS) n1 = 1.0; /* Default X direction */

    err = input_nastran_get_double(fields[6], &n2);
    if (err != FEM_SUCCESS) n2 = 0.0; /* Default Y direction */

    err = input_nastran_get_double(fields[7], &n3);
    if (err != FEM_SUCCESS) n3 = 0.0; /* Default Z direction */

    /* Apply force */
    int node_index = -1;
    err = input_get_node_index(g, &node_index);
    CHECK_ERROR(err);

    g_node_force[node_index][0] += f * n1;
    g_node_force[node_index][1] += f * n2;
    g_node_force[node_index][2] += f * n3;

    return FEM_SUCCESS;
}

/* Year1 direct-bulk pressure bridge.
 * Supports exact PLOAD only for 3-node boundary surfaces with per-surface pressure values.
 * PLOAD2/PLOAD4/LOAD and 4-node faces remain unsupported.
 */
static fem_error_t input_parse_nastran_pload(input_control_t *input, const char *line)
{
    char fields[10][9];
    char normalized[256];
    char tokens[10][64];
    fem_error_t err = FEM_SUCCESS;
    int sid = 0;
    int g1 = 0;
    int g2 = 0;
    int g3 = 0;
    int g4 = 0;
    int g4_present = 0;
    int token_count = 0;
    double pressure = 0.0;

    (void)input;

    err = input_nastran_parse_fixed_format(line, fields, 10);
    if (err == FEM_SUCCESS &&
        input_nastran_get_integer(fields[1], &sid) == FEM_SUCCESS &&
        input_nastran_get_double(fields[2], &pressure) == FEM_SUCCESS &&
        input_nastran_get_integer(fields[3], &g1) == FEM_SUCCESS &&
        input_nastran_get_integer(fields[4], &g2) == FEM_SUCCESS &&
        input_nastran_get_integer(fields[5], &g3) == FEM_SUCCESS) {
        if (fields[6][0] != '\0') {
            g4_present = 1;
            err = input_nastran_get_integer(fields[6], &g4);
            CHECK_ERROR(err);
        }
    } else {
        strncpy(normalized, line, sizeof(normalized) - 1);
        normalized[sizeof(normalized) - 1] = '\0';
        token_count = input_parser_split_tokens(normalized, tokens, 10);
        if (token_count < 6 || strcmp(tokens[0], "PLOAD") != 0) {
            return error_set(FEM_ERROR_FILE_READ,
                             "Invalid PLOAD card");
        }
        if (!input_parse_strict_int_token(tokens[1], &sid) ||
            !input_parse_strict_double_token(tokens[2], &pressure) ||
            !input_parse_strict_int_token(tokens[3], &g1) ||
            !input_parse_strict_int_token(tokens[4], &g2) ||
            !input_parse_strict_int_token(tokens[5], &g3)) {
            return error_set(FEM_ERROR_FILE_READ,
                             "Invalid PLOAD card");
        }
        if (token_count >= 7) {
            g4_present = 1;
            if (!input_parse_strict_int_token(tokens[6], &g4)) {
                return error_set(FEM_ERROR_FILE_READ,
                                 "Invalid PLOAD G4 field");
            }
        }
    }

    if (sid <= 0 || g1 <= 0 || g2 <= 0 || g3 <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "PLOAD requires positive sid and node IDs");
    }

    if (g4_present && g4 != 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "PLOAD currently supports 3-node boundary surfaces only (G4 must be blank or 0)");
    }

    if (!g_has_pressure) {
        g_pressure_value = pressure;
        g_has_pressure = 1;
        g_num_pressure_surfaces = 0;
    }

    if (g_num_pressure_surfaces >= MAX_TRACTION_SURFACES) {
        return error_set(FEM_ERROR_MEMORY_ALLOCATION,
                         "Exceeded maximum pressure surfaces (%d)",
                         MAX_TRACTION_SURFACES);
    }

    {
        int node_ids[MAX_SURFACE_NODES] = {g1, g2, g3};
        err = input_store_pressure_surface_from_node_ids(
            g_num_pressure_surfaces,
            MAX_SURFACE_NODES,
            node_ids,
            pressure);
        CHECK_ERROR(err);
    }
    g_num_pressure_surfaces++;

    return FEM_SUCCESS;
}

static fem_error_t input_nastran_find_pshell_material(int pid, int *material_index)
{
    if (material_index == NULL) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Null pointer in PSHELL lookup");
    }
    for (int i = 0; i < g_nastran_pshell_count; ++i) {
        if (g_nastran_pshells[i].pid == pid) {
            if (g_nastran_pshells[i].material_index < 0) {
                return error_set(FEM_ERROR_INVALID_MATERIAL,
                                 "PSHELL property %d has no associated material", pid);
            }
            *material_index = g_nastran_pshells[i].material_index;
            return FEM_SUCCESS;
        }
    }
    return error_set(FEM_ERROR_INVALID_MATERIAL,
                     "PSHELL property %d is not defined", pid);
}

static fem_error_t input_nastran_finalize_properties(void)
{
    fem_error_t err;

    for (int i = 0; i < g_nastran_pshell_count; ++i) {
        nastran_pshell_t *prop = &g_nastran_pshells[i];
        if (prop->material_index >= 0) {
            continue;
        }

        int base_index = 0;
        if (prop->mid > 0) {
            if (prop->mid >= g_material_id_capacity || g_material_id_to_index[prop->mid] < 0) {
                return error_set(FEM_ERROR_INVALID_MATERIAL,
                                 "MAT1 %d referenced by PSHELL %d not found",
                                 prop->mid, prop->pid);
            }
            base_index = g_material_id_to_index[prop->mid];
        } else if (g_num_materials > 0) {
            base_index = 0;
        } else {
            return error_set(FEM_ERROR_INVALID_MATERIAL,
                             "No MAT1 defined for PSHELL %d", prop->pid);
        }

        err = globals_reserve_materials(g_num_materials + 1);
        CHECK_ERROR(err);
        err = globals_reserve_material_ids(prop->pid + 1);
        CHECK_ERROR(err);

        int new_index = g_num_materials++;
        globals_initialize_material_entry(new_index);
        g_material_props[new_index][0] = g_material_props[base_index][0];
        g_material_props[new_index][1] = g_material_props[base_index][1];
        g_material_props[new_index][2] = (prop->thickness > 0.0)
                                         ? prop->thickness
                                         : g_material_props[base_index][2];
        if (g_material_props[new_index][2] <= 0.0) {
            g_material_props[new_index][2] = 1.0;
        }
        g_material_props[new_index][3] = g_material_props[base_index][3];
        g_material_type[new_index] = g_material_type[base_index];
        if (prop->pid >= 0 && prop->pid < g_material_id_capacity &&
            g_material_id_to_index[prop->pid] < 0) {
            g_material_id_to_index[prop->pid] = new_index;
        }
        if (g_material_ids) {
            g_material_ids[new_index] = prop->pid;
        }
        prop->material_index = new_index;
    }

    for (int elem = 0; elem < g_num_elements; ++elem) {
        if (g_nastran_element_property[elem] >= 0) {
            int mat_index = -1;
            err = input_nastran_find_pshell_material(g_nastran_element_property[elem], &mat_index);
            if (err == FEM_SUCCESS) {
                g_element_material[elem] = mat_index;
            } else {
                int pid = g_nastran_element_property[elem];
                if (pid > 0 && pid < g_material_id_capacity && g_material_id_to_index[pid] >= 0) {
                    g_element_material[elem] = g_material_id_to_index[pid];
                } else {
                    return err;
                }
            }
        } else if (g_element_material[elem] < 0) {
            g_element_material[elem] = 0;
        }
    }

    return FEM_SUCCESS;
}

/* Parse Nastran fixed format fields */
fem_error_t input_nastran_parse_fixed_format(const char *line, char fields[][9], int max_fields)
{
    int len = strlen(line);
    int field_count = 0;

    /* Parse 8-character fixed format fields */
    for (int i = 0; i < len && field_count < max_fields; i += 8) {
        int field_len = (i + 8 <= len) ? 8 : len - i;
        strncpy(fields[field_count], &line[i], field_len);
        fields[field_count][field_len] = '\0';

        /* Trim trailing spaces and newlines */
        for (int j = field_len - 1; j >= 0 && (fields[field_count][j] == ' ' || fields[field_count][j] == '\n' || fields[field_count][j] == '\r'); j--) {
            fields[field_count][j] = '\0';
        }

        field_count++;
    }

    return FEM_SUCCESS;
}

/* Get integer from Nastran field */
fem_error_t input_nastran_get_integer(const char *field, int *value)
{
    if (strlen(field) == 0) {
        return FEM_ERROR_FILE_READ; /* Empty field */
    }

    char *endptr;
    *value = strtol(field, &endptr, 10);

    if (*endptr != '\0') {
        return FEM_ERROR_FILE_READ; /* Invalid integer */
    }

    return FEM_SUCCESS;
}

/* Get double from Nastran field */
fem_error_t input_nastran_get_double(const char *field, double *value)
{
    char buffer[64];
    size_t len;
    int has_exp = 0;

    if (field == NULL || value == NULL) {
        return FEM_ERROR_INVALID_INPUT;
    }

    if (strlen(field) >= sizeof(buffer)) {
        return FEM_ERROR_FILE_READ;
    }

    strncpy(buffer, field, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    input_nastran_trim(buffer);

    if (buffer[0] == '\0') {
        return FEM_ERROR_FILE_READ;
    }

    len = strlen(buffer);
    for (size_t i = 0; i < len; ++i) {
        if (buffer[i] == 'D' || buffer[i] == 'd') {
            buffer[i] = 'E';
            has_exp = 1;
        } else if (buffer[i] == 'E' || buffer[i] == 'e') {
            has_exp = 1;
        }
    }

    if (!has_exp) {
        for (size_t i = 1; i < len; ++i) {
            if ((buffer[i] == '+' || buffer[i] == '-') && isdigit((unsigned char)buffer[i-1])) {
                if (len + 1 >= sizeof(buffer)) {
                    return FEM_ERROR_FILE_READ;
                }
                memmove(buffer + i + 1, buffer + i, len - i + 1);
                buffer[i] = 'E';
                len++;
                has_exp = 1;
                break;
            }
        }
    }

    char *endptr;
    *value = strtod(buffer, &endptr);

    if (*endptr != '\0') {
        return FEM_ERROR_FILE_READ;
    }

    return FEM_SUCCESS;
}
