#ifndef ERROR_H
#define ERROR_H

/* FEM4C - High Performance Finite Element Method in C
 * Error handling functions
 */

#include "types.h"
#include <stdio.h>

/* Error message buffer size */
#define ERROR_MSG_LEN 256

/* Global error state */
extern fem_error_t g_last_error;
extern char g_error_message[ERROR_MSG_LEN];

/* Error handling functions */
fem_error_t error_set(fem_error_t error_code, const char* format, ...);
fem_error_t error_get_last(void);
const char* error_get_message(void);
const char* error_get_string(fem_error_t error_code);
void error_clear(void);
void error_print(fem_error_t error_code);

/* Error checking macros */
#define CHECK_ERROR(call) \
    do { \
        fem_error_t _err = (call); \
        if (_err != FEM_SUCCESS) { \
            error_print(_err); \
            return _err; \
        } \
    } while(0)

#define CHECK_ERROR_CLEANUP(call, cleanup) \
    do { \
        fem_error_t _err = (call); \
        if (_err != FEM_SUCCESS) { \
            error_print(_err); \
            cleanup; \
            return _err; \
        } \
    } while(0)

#define CHECK_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            return error_set(FEM_ERROR_MEMORY_ALLOCATION, msg); \
        } \
    } while(0)

#define CHECK_BOUNDS(index, max_val, msg) \
    do { \
        if ((index) < 0 || (index) >= (max_val)) { \
            return error_set(FEM_ERROR_INVALID_INPUT, msg ": index %d out of bounds [0, %d)", (index), (max_val)); \
        } \
    } while(0)

#define CHECK_POSITIVE(value, msg) \
    do { \
        if ((value) <= 0.0) { \
            return error_set(FEM_ERROR_INVALID_INPUT, msg ": value %g must be positive", (double)(value)); \
        } \
    } while(0)

#define CHECK_FILE(file_ptr, filename) \
    do { \
        if ((file_ptr) == NULL) { \
            return error_set(FEM_ERROR_FILE_NOT_FOUND, "Cannot open file: %s", filename); \
        } \
    } while(0)

#endif /* ERROR_H */