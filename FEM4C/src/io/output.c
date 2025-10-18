/* FEM4C - Output module implementation
 * Result output functions
 */

#include "output.h"
#include "../common/constants.h"
#include "../common/globals.h"
#include "../common/error.h"
#include "../elements/t6/t6_stiffness.h"
#include "../elements/t3/t3_element.h"
#include "../elements/q4/q4_element.h"
#include "../solver/cg_solver.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/* Main result writing function */
fem_error_t output_write_results(const char *filename)
{
    output_control_t output;
    fem_error_t err;
    
    /* Open output file */
    err = output_open_file(&output, filename, OUTPUT_FORMAT_NATIVE);
    CHECK_ERROR(err);
    
    /* Enable all output types */
    output.write_displacements = 1;
    output.write_stresses = 1;
    output.write_reactions = 1;
    
    /* Write results */
    err = output_write_header(&output);
    CHECK_ERROR_CLEANUP(err, output_close_file(&output));
    
    err = output_write_displacements(&output);
    CHECK_ERROR_CLEANUP(err, output_close_file(&output));
    
    /* Calculate and write stresses (if solver completed) */
    if (g_total_dof > 0) {
        err = output_calculate_element_stresses();
        if (err == FEM_SUCCESS) {
            err = output_write_stresses(&output);
            CHECK_ERROR_CLEANUP(err, output_close_file(&output));
        }
    }
    
    /* Calculate and write reactions */
    err = output_calculate_reactions();
    if (err == FEM_SUCCESS) {
        err = output_write_reactions(&output);
        CHECK_ERROR_CLEANUP(err, output_close_file(&output));
    }
    
    err = output_write_summary(&output);
    CHECK_ERROR_CLEANUP(err, output_close_file(&output));
    
    /* Close file */
    output_close_file(&output);
    
    return FEM_SUCCESS;
}

fem_error_t output_export_csv(const char *filename)
{
    FILE *csv = fopen(filename, "w");
    if (!csv) {
        return error_set(FEM_ERROR_FILE_WRITE, "Cannot create CSV file: %s", filename);
    }

    fprintf(csv, "type,id,x,y,z,ux,uy,uz,disp_mag,n1,n2,n3,n4,n5,n6,sigma_x,sigma_y,tau_xy,von_mises,sigma_max,sigma_min\n");

    /* Write nodal displacement results */
    for (int i = 0; i < g_num_nodes; ++i) {
        double ux = g_node_displ[i][0];
        double uy = g_node_displ[i][1];
        double uz = g_node_displ[i][2];
        double mag = sqrt(ux * ux + uy * uy + uz * uz);
        fprintf(csv, "NODE,%d,%.10f,%.10f,%.10f,%.6e,%.6e,%.6e,%.6e,,,,,,,,,,,,\n",
                g_node_ids ? g_node_ids[i] : (i + 1),
                g_node_coords[i][0],
                g_node_coords[i][1],
                g_node_coords[i][2],
                ux, uy, uz, mag);
    }

    /* Write element stress results */
    for (int elem = 0; elem < g_num_elements; ++elem) {
        double stress[T6_STRESS_COMPONENTS] = {0.0, 0.0, 0.0};
        fem_error_t err = FEM_SUCCESS;

        if (g_element_type[elem] == ELEMENT_T6) {
            err = t6_calculate_element_stress(elem, stress);
        } else if (g_element_type[elem] == ELEMENT_T3) {
            err = t3_element_stress(elem, stress);
        } else if (g_element_type[elem] == ELEMENT_Q4) {
            err = q4_element_stress(elem, stress);
        } else {
            continue;
        }

        if (err != FEM_SUCCESS) {
            stress[0] = stress[1] = stress[2] = NAN;
        }

        double sx = stress[0];
        double sy = stress[1];
        double txy = stress[2];
        double von_mises = NAN;
        double sigma_max = NAN;
        double sigma_min = NAN;

        if (!isnan(sx) && !isnan(sy) && !isnan(txy)) {
            von_mises = sqrt(sx * sx + sy * sy - sx * sy + 3.0 * txy * txy);
            double avg = 0.5 * (sx + sy);
            double diff = 0.5 * (sx - sy);
            double radius = sqrt(diff * diff + txy * txy);
            sigma_max = avg + radius;
            sigma_min = avg - radius;
        }

        int n1 = -1, n2 = -1, n3 = -1, n4 = -1, n5 = -1, n6 = -1;
        if (g_element_type[elem] == ELEMENT_T6) {
            n1 = g_element_nodes[elem][0];
            n2 = g_element_nodes[elem][1];
            n3 = g_element_nodes[elem][2];
            n4 = g_element_nodes[elem][3];
            n5 = g_element_nodes[elem][4];
            n6 = g_element_nodes[elem][5];
        } else if (g_element_type[elem] == ELEMENT_T3) {
            n1 = g_element_nodes[elem][0];
            n2 = g_element_nodes[elem][1];
            n3 = g_element_nodes[elem][2];
        } else if (g_element_type[elem] == ELEMENT_Q4) {
            n1 = g_element_nodes[elem][0];
            n2 = g_element_nodes[elem][1];
            n3 = g_element_nodes[elem][2];
            n4 = g_element_nodes[elem][3];
        }

        int node_ids[6] = {n1, n2, n3, n4, n5, n6};
        for (int j = 0; j < 6; ++j) {
            if (node_ids[j] >= 0) {
                node_ids[j] = g_node_ids ? g_node_ids[node_ids[j]] : (node_ids[j] + 1);
            } else {
                node_ids[j] = 0;
            }
        }

        fprintf(csv, "ELEMENT,%d,,,,,,,,%d,%d,%d,%d,%d,%d,%.6e,%.6e,%.6e,%.6e,%.6e,%.6e\n",
                g_element_ids ? g_element_ids[elem] : (elem + 1),
                node_ids[0], node_ids[1], node_ids[2],
                node_ids[3], node_ids[4], node_ids[5],
                sx, sy, txy, von_mises, sigma_max, sigma_min);
    }

    fclose(csv);
    return FEM_SUCCESS;
}

/* Open output file */
fem_error_t output_open_file(output_control_t *output, const char *filename, output_format_t format)
{
    if (output == NULL || filename == NULL) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Null pointer in output_open_file");
    }
    
    strncpy(output->filename, filename, MAX_FILENAME_LEN-1);
    output->filename[MAX_FILENAME_LEN-1] = '\0';
    output->format = format;
    
    output->file_ptr = fopen(filename, "w");
    if (output->file_ptr == NULL) {
        return error_set(FEM_ERROR_FILE_WRITE, "Cannot create output file: %s", filename);
    }
    
    return FEM_SUCCESS;
}

/* Close output file */
fem_error_t output_close_file(output_control_t *output)
{
    if (output != NULL && output->file_ptr != NULL) {
        fclose(output->file_ptr);
        output->file_ptr = NULL;
    }
    return FEM_SUCCESS;
}

/* Write header information */
fem_error_t output_write_header(output_control_t *output)
{
    time_t current_time;
    char time_str[64];
    
    current_time = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&current_time));
    
    fprintf(output->file_ptr, "FEM4C - High Performance Finite Element Method in C\n");
    fprintf(output->file_ptr, "Analysis Results\n");
    fprintf(output->file_ptr, "=====================================\n\n");
    
    fprintf(output->file_ptr, "Analysis Title: %s\n", g_analysis.title);
    fprintf(output->file_ptr, "Date/Time:      %s\n", time_str);
    fprintf(output->file_ptr, "Input File:     %s\n", g_input_filename);
    fprintf(output->file_ptr, "\n");
    
    fprintf(output->file_ptr, "Problem Size:\n");
    fprintf(output->file_ptr, "  Number of nodes:     %d\n", g_num_nodes);
    fprintf(output->file_ptr, "  Number of elements:  %d\n", g_num_elements);
    fprintf(output->file_ptr, "  Number of materials: %d\n", g_num_materials);
    fprintf(output->file_ptr, "  Total DOF:           %d\n", g_total_dof);
    fprintf(output->file_ptr, "\n");
    
    return FEM_SUCCESS;
}

/* Write nodal displacements */
fem_error_t output_write_displacements(output_control_t *output)
{
    int i;
    
    fprintf(output->file_ptr, "Nodal Displacements:\n");
    fprintf(output->file_ptr, "====================\n");
    fprintf(output->file_ptr, "Node      UX           UY           UZ\n");
    fprintf(output->file_ptr, "----  -----------  -----------  -----------\n");
    
    for (i = 0; i < g_num_nodes; i++) {
        fprintf(output->file_ptr, "%4d  %11.4e  %11.4e  %11.4e\n", 
                i+1, g_node_displ[i][0], g_node_displ[i][1], g_node_displ[i][2]);
    }
    
    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Write element stresses */
fem_error_t output_write_stresses(output_control_t *output)
{
    fprintf(output->file_ptr, "Element Stresses:\n");
    fprintf(output->file_ptr, "=================\n");
    fprintf(output->file_ptr, "Elem     SigmaX       SigmaY       TauXY\n");
    fprintf(output->file_ptr, "----  -----------  -----------  -----------\n");
    
    /* Note: Stress calculation will be implemented with T6 element */
    for (int i = 0; i < g_num_elements; i++) {
        fprintf(output->file_ptr, "%4d  %11.4e  %11.4e  %11.4e\n", 
                i+1, 0.0, 0.0, 0.0);
    }
    
    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Write reaction forces */
fem_error_t output_write_reactions(output_control_t *output)
{
    int i;
    
    fprintf(output->file_ptr, "Reaction Forces:\n");
    fprintf(output->file_ptr, "================\n");
    fprintf(output->file_ptr, "Node      RX           RY           RZ\n");
    fprintf(output->file_ptr, "----  -----------  -----------  -----------\n");
    
    for (i = 0; i < g_num_nodes; i++) {
        /* Only print reactions for constrained nodes */
        if (g_node_bc_flags[i][0] || g_node_bc_flags[i][1] || g_node_bc_flags[i][2]) {
            fprintf(output->file_ptr, "%4d  %11.4e  %11.4e  %11.4e\n", 
                    i+1, 0.0, 0.0, 0.0); /* Will be calculated by solver */
        }
    }
    
    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Write analysis summary */
fem_error_t output_write_summary(output_control_t *output)
{
    fprintf(output->file_ptr, "Analysis Summary:\n");
    fprintf(output->file_ptr, "=================\n");
    
    fprintf(output->file_ptr, "Solver Information:\n");
    fprintf(output->file_ptr, "  Iterations:     %d\n", g_solver_info.iterations);
    fprintf(output->file_ptr, "  Final residual: %e\n", g_solver_info.residual);
    fprintf(output->file_ptr, "  Elapsed time:   %.3f sec\n", g_solver_info.elapsed_time);
    fprintf(output->file_ptr, "  Status:         %s\n", 
            (g_solver_info.status == FEM_SUCCESS) ? "SUCCESS" : "ERROR");
    
    fprintf(output->file_ptr, "\nMaterial Properties:\n");
    for (int i = 0; i < g_num_materials; i++) {
        fprintf(output->file_ptr, "  Material %d:\n", i+1);
        fprintf(output->file_ptr, "    Young's modulus: %e\n", g_material_props[i][0]);
        fprintf(output->file_ptr, "    Poisson's ratio: %f\n", g_material_props[i][1]);
        fprintf(output->file_ptr, "    Thickness:       %f\n", g_material_props[i][2]);
    }
    
    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Calculate element stresses (placeholder) */
fem_error_t output_calculate_element_stresses(void)
{
    /* Will be implemented with T6 element stress calculation */
    return FEM_SUCCESS;
}

/* Calculate reaction forces (placeholder) */
fem_error_t output_calculate_reactions(void)
{
    /* Will be implemented with solver */
    return FEM_SUCCESS;
}

/* Console output functions */
void output_print_summary(void)
{
    printf("\nAnalysis Summary:\n");
    printf("=================\n");
    printf("Problem: %s\n", g_analysis.title);
    printf("Nodes:   %d\n", g_num_nodes);
    printf("Elements: %d\n", g_num_elements);
    printf("DOF:     %d\n", g_total_dof);
    
    if (g_solver_info.iterations > 0) {
        printf("Solver iterations: %d\n", g_solver_info.iterations);
        printf("Final residual:    %e\n", g_solver_info.residual);
        printf("Solution time:     %.3f sec\n", g_solver_info.elapsed_time);
    }
    
    printf("\n");
}

void output_print_convergence_info(void)
{
    if (g_solver_info.status == FEM_SUCCESS) {
        printf("Convergence achieved in %d iterations\n", g_solver_info.iterations);
        printf("Final residual: %e\n", g_solver_info.residual);
    } else {
        printf("Convergence failed: %s\n", error_get_string(g_solver_info.status));
    }
}

void output_print_timing_info(void)
{
    printf("Analysis completed in %.3f seconds\n", g_solver_info.elapsed_time);
}

/* Write VTK file for ParaView visualization */
fem_error_t output_write_vtk_file(const char *filename)
{
    FILE *vtk_file;
    int i, j;
    fem_error_t err;
    
    /* Open VTK file */
    vtk_file = fopen(filename, "w");
    if (vtk_file == NULL) {
        return error_set(FEM_ERROR_FILE_WRITE, "Cannot create VTK file: %s", filename);
    }
    
    /* Write VTK header */
    fprintf(vtk_file, "# vtk DataFile Version 3.0\n");
    fprintf(vtk_file, "FEM4C Analysis Results: %s\n", g_analysis.title);
    fprintf(vtk_file, "ASCII\n");
    fprintf(vtk_file, "DATASET UNSTRUCTURED_GRID\n\n");
    
    /* Write points (nodes) */
    fprintf(vtk_file, "POINTS %d float\n", g_num_nodes);
    for (i = 0; i < g_num_nodes; i++) {
        fprintf(vtk_file, "%.6f %.6f %.6f\n", 
                g_node_coords[i][0], g_node_coords[i][1], g_node_coords[i][2]);
    }
    fprintf(vtk_file, "\n");
    
    /* Write cells (elements) */
    fprintf(vtk_file, "CELLS %d %d\n", g_num_elements, g_num_elements * 7); /* 6 nodes + count */
    for (i = 0; i < g_num_elements; i++) {
        if (g_element_type[i] == ELEMENT_T6) {
            fprintf(vtk_file, "6");  /* Number of nodes */
            for (j = 0; j < T6_NODES_PER_ELEMENT; j++) {
                fprintf(vtk_file, " %d", g_element_nodes[i][j]);
            }
            fprintf(vtk_file, "\n");
        }
    }
    fprintf(vtk_file, "\n");
    
    /* Write cell types */
    fprintf(vtk_file, "CELL_TYPES %d\n", g_num_elements);
    for (i = 0; i < g_num_elements; i++) {
        if (g_element_type[i] == ELEMENT_T6) {
            fprintf(vtk_file, "22\n");  /* VTK_QUADRATIC_TRIANGLE */
        }
    }
    fprintf(vtk_file, "\n");
    
    /* Write point data (nodal results) */
    fprintf(vtk_file, "POINT_DATA %d\n", g_num_nodes);
    
    /* Write displacement vectors */
    fprintf(vtk_file, "VECTORS Displacement float\n");
    for (i = 0; i < g_num_nodes; i++) {
        fprintf(vtk_file, "%.6e %.6e %.6e\n", 
                g_node_displ[i][0], g_node_displ[i][1], g_node_displ[i][2]);
    }
    fprintf(vtk_file, "\n");
    
    /* Write displacement magnitudes */
    fprintf(vtk_file, "SCALARS Displacement_Magnitude float\n");
    fprintf(vtk_file, "LOOKUP_TABLE default\n");
    for (i = 0; i < g_num_nodes; i++) {
        double mag = sqrt(g_node_displ[i][0] * g_node_displ[i][0] + 
                         g_node_displ[i][1] * g_node_displ[i][1] + 
                         g_node_displ[i][2] * g_node_displ[i][2]);
        fprintf(vtk_file, "%.6e\n", mag);
    }
    fprintf(vtk_file, "\n");
    
    /* Write nodal forces */
    fprintf(vtk_file, "VECTORS Applied_Force float\n");
    for (i = 0; i < g_num_nodes; i++) {
        fprintf(vtk_file, "%.6e %.6e %.6e\n", 
                g_node_force[i][0], g_node_force[i][1], g_node_force[i][2]);
    }
    fprintf(vtk_file, "\n");
    
    /* Write cell data (element results) */
    fprintf(vtk_file, "CELL_DATA %d\n", g_num_elements);
    
    /* Write element stresses (placeholder - would calculate actual stresses) */
    fprintf(vtk_file, "SCALARS Von_Mises_Stress float\n");
    fprintf(vtk_file, "LOOKUP_TABLE default\n");
    for (i = 0; i < g_num_elements; i++) {
        double stress[T6_STRESS_COMPONENTS];
        double von_mises = 0.0;
        
        if (g_element_type[i] == ELEMENT_T6) {
            err = t6_calculate_element_stress(i, stress);
            if (err == FEM_SUCCESS) {
                /* Calculate von Mises stress: sqrt(sx^2 + sy^2 - sx*sy + 3*txy^2) */
                von_mises = sqrt(stress[0]*stress[0] + stress[1]*stress[1] 
                               - stress[0]*stress[1] + 3.0*stress[2]*stress[2]);
            }
        }
        fprintf(vtk_file, "%.6e\n", von_mises);
    }
    fprintf(vtk_file, "\n");
    
    /* Write element material IDs */
    fprintf(vtk_file, "SCALARS Material_ID int\n");
    fprintf(vtk_file, "LOOKUP_TABLE default\n");
    for (i = 0; i < g_num_elements; i++) {
        fprintf(vtk_file, "%d\n", g_element_material[i] + 1);  /* 1-based for visualization */
    }
    
    fclose(vtk_file);
    return FEM_SUCCESS;
}

/* Write Nastran F06 format results file */
fem_error_t output_write_nastran_f06_file(const char *filename)
{
    output_control_t output;
    fem_error_t err;

    /* Open F06 output file */
    err = output_open_file(&output, filename, OUTPUT_FORMAT_NASTRAN_F06);
    CHECK_ERROR(err);

    /* Enable all output types */
    output.write_displacements = 1;
    output.write_stresses = 1;
    output.write_reactions = 1;

    /* Write F06 format results */
    err = output_write_nastran_f06(&output);
    CHECK_ERROR_CLEANUP(err, output_close_file(&output));

    /* Close file */
    output_close_file(&output);
    return FEM_SUCCESS;
}

/* Write complete Nastran F06 format output */
fem_error_t output_write_nastran_f06(output_control_t *output)
{
    fem_error_t err;

    /* Write F06 header */
    err = output_write_nastran_f06_header(output);
    CHECK_ERROR(err);

    /* Write displacement results */
    err = output_write_nastran_f06_displacements(output);
    CHECK_ERROR(err);

    /* Calculate and write stresses */
    if (g_total_dof > 0) {
        err = output_calculate_element_stresses();
        if (err == FEM_SUCCESS) {
            err = output_write_nastran_f06_stresses(output);
            CHECK_ERROR(err);
        }
    }

    /* Calculate and write forces */
    err = output_calculate_reactions();
    if (err == FEM_SUCCESS) {
        err = output_write_nastran_f06_forces(output);
        CHECK_ERROR(err);
    }

    return FEM_SUCCESS;
}

/* Write Nastran F06 header */
fem_error_t output_write_nastran_f06_header(output_control_t *output)
{
    time_t current_time;
    char time_str[26];

    time(&current_time);
    strcpy(time_str, ctime(&current_time));
    time_str[24] = '\0'; /* Remove newline */

    fprintf(output->file_ptr,
        "1\n"
        "                                              N A S T R A N    F I L E    A N D    S Y S T E M    P A R A M E T E R    E C H O\n"
        "                                                                                                                                                          PAGE    1\n"
        "\n"
        "\n"
        "0                                                               * * * * * * * * * * * * * * * *\n"
        "                                                                 *                             *\n"
        "                                                                 *        FEM4C SOLUTION      *\n"
        "                                                                 *                             *\n"
        "                                                                 * * * * * * * * * * * * * * * *\n"
        "\n\n"
        "0SOLUTION SUMMARY:\n"
        "     PROBLEM TITLE........ FEM4C HIGH PERFORMANCE FINITE ELEMENT ANALYSIS\n"
        "     SOLUTION TYPE........ STATIC ANALYSIS (SOL 101)\n"
        "     ANALYSIS DATE........ %s\n"
        "     PROBLEM SIZE......... %d NODES, %d ELEMENTS, %d DOF\n"
        "\n",
        time_str, g_num_nodes, g_num_elements, g_total_dof);

    return FEM_SUCCESS;
}

/* Write Nastran F06 displacement results */
fem_error_t output_write_nastran_f06_displacements(output_control_t *output)
{
    int i;

    fprintf(output->file_ptr,
        "1                                                                          D I S P L A C E M E N T   V E C T O R\n"
        "                                                                                                                                                          PAGE    2\n"
        "0\n"
        "      POINT ID.   TYPE          T1             T2             T3             R1             R2             R3\n");

    for (i = 0; i < g_num_nodes; i++) {
        fprintf(output->file_ptr,
            "%14d      G      %13.6E  %13.6E  %13.6E  %13.6E  %13.6E  %13.6E\n",
            i + 1,                     /* Node ID (1-based) */
            g_node_displ[i][0],        /* T1 (X displacement) */
            g_node_displ[i][1],        /* T2 (Y displacement) */
            g_node_displ[i][2],        /* T3 (Z displacement) */
            0.0,                       /* R1 (X rotation) */
            0.0,                       /* R2 (Y rotation) */
            0.0);                      /* R3 (Z rotation) */
    }

    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Write Nastran F06 stress results */
fem_error_t output_write_nastran_f06_stresses(output_control_t *output)
{
    int i;
    double stress[3];
    fem_error_t err;

    fprintf(output->file_ptr,
        "1                                                            S T R E S S E S   I N   T R I A N G U L A R   E L E M E N T S    ( T R I A 3 )\n"
        "                                                                                                                                                          PAGE    3\n"
        "0\n"
        "  ELEMENT      FIBER               STRESSES IN ELEMENT COORD SYSTEM\n"
        "    ID.        DISTANCE           NORMAL-X       NORMAL-Y      SHEAR-XY\n");

    for (i = 0; i < g_num_elements; i++) {
        /* Calculate element stress (simplified) */
        if (g_element_type[i] == ELEMENT_T3) {
            err = t3_element_stress(i, stress);
        } else if (g_element_type[i] == ELEMENT_Q4) {
            err = q4_element_stress(i, stress);
        } else if (g_element_type[i] == ELEMENT_T6) {
            err = t6_calculate_element_stress(i, stress);
        } else {
            stress[0] = stress[1] = stress[2] = 0.0;
            err = FEM_SUCCESS;
        }

        if (err == FEM_SUCCESS) {
            fprintf(output->file_ptr,
                "%8d  Z1 = %8.3E    %13.6E  %13.6E  %13.6E\n",
                i + 1,                     /* Element ID (1-based) */
                0.5,                       /* Fiber distance */
                stress[0],                 /* Normal-X stress */
                stress[1],                 /* Normal-Y stress */
                stress[2]);                /* Shear-XY stress */
        }
    }

    fprintf(output->file_ptr, "\n");
    return FEM_SUCCESS;
}

/* Write Nastran F06 force results */
fem_error_t output_write_nastran_f06_forces(output_control_t *output)
{
    int i;

    fprintf(output->file_ptr,
        "1                                                            F O R C E S   A N D   M O M E N T S   I N   S P C   F O R C E S\n"
        "                                                                                                                                                          PAGE    4\n"
        "0\n"
        "      POINT ID.   TYPE          T1             T2             T3             R1             R2             R3\n");

    double *ku = NULL;
    if (g_total_dof > 0) {
        ku = malloc((size_t)g_total_dof * sizeof(double));
        if (ku == NULL) {
            return error_set(FEM_ERROR_MEMORY_ALLOCATION, "Failed to allocate reaction workspace");
        }

        fem_error_t err = cg_matrix_vector_multiply(NULL, g_global_displ, ku, g_total_dof);
        if (err != FEM_SUCCESS) {
            free(ku);
            return err;
        }
    }

    for (i = 0; i < g_num_nodes; i++) {
        if (g_node_bc_flags[i][0] || g_node_bc_flags[i][1] || g_node_bc_flags[i][2]) {
            double rx = 0.0, ry = 0.0, rz = 0.0;

            if (ku != NULL) {
                if (g_node_bc_flags[i][0]) {
                    rx = ku[i * 2] - g_global_force[i * 2];
                }
                if (g_node_bc_flags[i][1]) {
                    ry = ku[i * 2 + 1] - g_global_force[i * 2 + 1];
                }
            }

            fprintf(output->file_ptr,
                "%14d      G      %13.6E  %13.6E  %13.6E  %13.6E  %13.6E  %13.6E\n",
                i + 1,
                rx,
                ry,
                rz,
                0.0,
                0.0,
                0.0);
        }
    }

    if (ku) {
        free(ku);
    }

    fprintf(output->file_ptr, "\n1                                         * * * E N D   O F   J O B * * *\n");
    return FEM_SUCCESS;
}
