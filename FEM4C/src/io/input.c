/* FEM4C - Input module implementation
 * Data input functions
 */

#include "input.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>

#define MAX_NASTRAN_PROPERTIES 512

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

static void input_nastran_normalize_line(char *line);
static void input_nastran_trim(char *text);
static int input_nastran_line_has_continuation(const char *line);
static fem_error_t input_parse_nastran_grid_long(input_control_t *input, const char *line);
static fem_error_t input_parse_nastran_grid_short(input_control_t *input, const char *line);
static fem_error_t input_nastran_finalize_properties(void);
static fem_error_t input_nastran_find_pshell_material(int pid, int *material_index);
static fem_error_t input_ensure_nastran_element_capacity(int required);
static int input_parser_is_directory(const char *path);
static int input_parser_has_mesh_root(const char *path);
static fem_error_t input_read_parser_mesh(const char *mesh_path);
static fem_error_t input_read_parser_material(const char *material_path);
static fem_error_t input_read_parser_boundary(const char *boundary_path);
static void input_parser_trim(char *text);
static int input_parser_is_label(const char *line, const char *label);
static int input_parser_split_tokens(const char *line, char tokens[][64], int max_tokens);

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
    g_total_dof = g_num_nodes * 2; /* 2D analysis for T6 */
    
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
            if (bc_flags[0]) g_node_displ[node_index][0] = prescribed_values[0];
            if (bc_flags[1]) g_node_displ[node_index][1] = prescribed_values[1];
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

                for (int k = 0; k < MAX_SURFACE_NODES; k++) {
                    if (k >= parsed) {
                        /* Require explicit specification for quadratic edges */
                        node_ids[k] = 0;
                    }
                    int node_index = -1;
                    err = input_get_node_index(node_ids[k], &node_index);
                    CHECK_ERROR(err);
                    g_pressure_surfaces[g_num_pressure_surfaces][k] = node_index;
                }
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
        } else if (strncmp(line, "MAT1", 4) == 0) {
            err = input_parse_nastran_mat1(input, line);
            CHECK_ERROR(err);
        } else if (strncmp(line, "PSHELL", 6) == 0) {
            err = input_parse_nastran_pshell(input, line);
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
    g_total_dof = g_num_nodes * 2; /* 2D analysis */

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

static fem_error_t input_read_parser_boundary(const char *boundary_path)
{
    fem_error_t err;
    FILE *fp = fopen(boundary_path, "r");
    CHECK_FILE(fp, boundary_path);

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        input_parser_trim(line);
        if (line[0] == '\0') continue;

        if (strncmp(line, "Total number of Boundary Conditions", 35) == 0) {
            fgets(line, sizeof(line), fp);
            continue;
        }
        if (strncmp(line, "UNITSYS", 7) == 0) {
            printf("  Info: boundary.dat declares UNITSYS (forces already in N)\n");
            continue;
        }

        if (strncmp(line, "SPC", 3) == 0) {
            char tok[12][64];
            int nt = input_parser_split_tokens(line, tok, 12);
            int gid = 0;
            char comp[32] = "";
            double disp = 0.0;
            for (int i = 0; i < nt; ++i) {
                if (strncmp(tok[i], "G=", 2) == 0) {
                    gid = atoi(tok[i] + 2);
                } else if (strncmp(tok[i], "C=", 2) == 0) {
                    snprintf(comp, sizeof(comp), "%s", tok[i] + 2);
                } else if (strncmp(tok[i], "D=", 2) == 0) {
                    disp = atof(tok[i] + 2);
                }
            }
            if (gid <= 0 || comp[0] == '\0') {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Malformed SPC entry in %s", boundary_path);
            }
            int node_index = -1;
            err = input_get_node_index(gid, &node_index);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            for (size_t k = 0; k < strlen(comp); ++k) {
                int c = comp[k] - '0';
                if (c == 1 || c == 2) {
                    g_node_bc_flags[node_index][c - 1] = 1;
                    g_node_displ[node_index][c - 1] = disp;
                } else if (c == 3) {
                    printf("  Warning: SPC with z-direction constraint ignored for G=%d\n", gid);
                }
            }
            continue;
        }

        if (strncmp(line, "FORCE", 5) == 0) {
            char tok[12][64];
            int nt = input_parser_split_tokens(line, tok, 12);
            int gid = 0;
            double F = 0.0, n1 = 0.0, n2 = 0.0, n3 = 0.0;
            for (int i = 0; i < nt; ++i) {
                if (strncmp(tok[i], "G=", 2) == 0) {
                    gid = atoi(tok[i] + 2);
                } else if (strncmp(tok[i], "F=", 2) == 0) {
                    F = atof(tok[i] + 2);
                } else if (tok[i][0] == 'N' && tok[i][1] == '=') {
                    char buf[192];
                    buf[0] = '\0';
                    for (int j = i; j < nt && j < i + 3; ++j) {
                        if (buf[0]) {
                            strncat(buf, " ", sizeof(buf) - strlen(buf) - 1);
                        }
                        strncat(buf, tok[j], sizeof(buf) - strlen(buf) - 1);
                        if (strchr(tok[j], ')')) {
                            i = j; /* consume up to j */
                            break;
                        }
                    }
                    for (char *p = buf; *p; ++p) {
                        if (*p == ',') *p = ' ';
                    }
                    double a = 0.0, b = 0.0, c = 0.0;
                    if (sscanf(buf, "N=(%lf %lf %lf)", &a, &b, &c) == 3 ||
                        sscanf(buf, "N=%lf %lf %lf", &a, &b, &c) == 3) {
                        n1 = a; n2 = b; n3 = c;
                    }
                }
            }
            if (gid <= 0) {
                fclose(fp);
                return error_set(FEM_ERROR_INVALID_INPUT,
                                 "Malformed FORCE entry in %s", boundary_path);
            }
            int node_index = -1;
            err = input_get_node_index(gid, &node_index);
            CHECK_ERROR_CLEANUP(err, fclose(fp));
            g_node_force[node_index][0] += F * n1;
            g_node_force[node_index][1] += F * n2;
            g_node_force[node_index][2] += F * n3;
            continue;
        }
    }

    fclose(fp);
    return FEM_SUCCESS;
}

fem_error_t input_read_parser_package(const char *directory)
{
    fem_error_t err;
    char mesh_path[1024];
    char material_path[1024];
    char boundary_path[1024];

    snprintf(mesh_path, sizeof(mesh_path), "%s/mesh/mesh.dat", directory);
    snprintf(material_path, sizeof(material_path), "%s/material/material.dat", directory);
    snprintf(boundary_path, sizeof(boundary_path), "%s/Boundary Conditions/boundary.dat", directory);

    if (!input_parser_has_mesh_root(directory)) {
        return error_set(FEM_ERROR_FILE_NOT_FOUND, "mesh/mesh.dat not found under %s", directory);
    }

    err = input_read_parser_mesh(mesh_path);
    CHECK_ERROR(err);
    err = input_read_parser_material(material_path);
    CHECK_ERROR(err);
    err = input_read_parser_boundary(boundary_path);
    CHECK_ERROR(err);

    g_analysis.num_nodes = g_num_nodes;
    g_analysis.num_elements = g_num_elements;
    g_analysis.num_materials = g_num_materials;
    g_total_dof = g_num_nodes * 2;
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

    for (size_t i = 0; i < strlen(component_field); ++i) {
        char comp = component_field[i];
        if (comp == '1') {
            g_node_bc_flags[node_index][0] = 1;
            g_node_displ[node_index][0] = d;
        } else if (comp == '2') {
            g_node_bc_flags[node_index][1] = 1;
            g_node_displ[node_index][1] = d;
        } else if (comp == '3') {
            g_node_bc_flags[node_index][2] = 1;
            g_node_displ[node_index][2] = d;
        }
    }

    return FEM_SUCCESS;
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
