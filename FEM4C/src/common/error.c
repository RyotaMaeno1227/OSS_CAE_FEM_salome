/* FEM4C - High Performance Finite Element Method in C
 * Error handling implementation
 */

#include "error.h"
#include <stdarg.h>
#include <string.h>

/* Global error state */
fem_error_t g_last_error = FEM_SUCCESS;
char g_error_message[ERROR_MSG_LEN] = {0};

/* Set error with formatted message */
fem_error_t error_set(fem_error_t error_code, const char* format, ...)
{
    va_list args;
    
    g_last_error = error_code;
    
    if (format != NULL) {
        va_start(args, format);
        vsnprintf(g_error_message, ERROR_MSG_LEN, format, args);
        va_end(args);
    } else {
        strncpy(g_error_message, error_get_string(error_code), ERROR_MSG_LEN);
        g_error_message[ERROR_MSG_LEN-1] = '\0';
    }
    
    return error_code;
}

/* Get last error code */
fem_error_t error_get_last(void)
{
    return g_last_error;
}

/* Get error message */
const char* error_get_message(void)
{
    return g_error_message;
}

/* Convert error code to string */
const char* error_get_string(fem_error_t error_code)
{
    switch (error_code) {
        case FEM_SUCCESS:
            return "Success";
        case FEM_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case FEM_ERROR_FILE_READ:
            return "File read error";
        case FEM_ERROR_FILE_WRITE:
            return "File write error";
        case FEM_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation error";
        case FEM_ERROR_INVALID_INPUT:
            return "Invalid input";
        case FEM_ERROR_INVALID_ELEMENT_TYPE:
            return "Invalid element type";
        case FEM_ERROR_INVALID_MATERIAL:
            return "Invalid material properties";
        case FEM_ERROR_INVALID_NODE:
            return "Invalid node reference";
        case FEM_ERROR_CONVERGENCE_FAILED:
            return "Solver convergence failed";
        case FEM_ERROR_SINGULAR_MATRIX:
            return "Singular matrix encountered";
        case FEM_ERROR_MAX_ITERATIONS:
            return "Maximum iterations reached";
        case FEM_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

/* Clear error state */
void error_clear(void)
{
    g_last_error = FEM_SUCCESS;
    g_error_message[0] = '\0';
}

/* Print error to stderr */
void error_print(fem_error_t error_code)
{
    if (error_code != FEM_SUCCESS) {
        if (strlen(g_error_message) > 0) {
            fprintf(stderr, "FEM4C Error [%d]: %s\n", error_code, g_error_message);
        } else {
            fprintf(stderr, "FEM4C Error [%d]: %s\n", error_code, error_get_string(error_code));
        }
    }
}