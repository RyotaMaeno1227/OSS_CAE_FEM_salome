#include "../include/chrono_logging.h"

#include <stdio.h>
#include <string.h>

#define CHRONO_LOG_MESSAGE_MAX 512

static ChronoLogHandler_C g_handler = NULL;
static void *g_handler_user = NULL;
static ChronoLogLevel_C g_min_level = CHRONO_LOG_LEVEL_INFO;

static const char *level_name_internal(ChronoLogLevel_C level) {
    switch (level) {
        case CHRONO_LOG_LEVEL_TRACE:
            return "TRACE";
        case CHRONO_LOG_LEVEL_DEBUG:
            return "DEBUG";
        case CHRONO_LOG_LEVEL_INFO:
            return "INFO";
        case CHRONO_LOG_LEVEL_WARNING:
            return "WARN";
        case CHRONO_LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

static const char *category_name_internal(ChronoLogCategory_C category) {
    switch (category) {
        case CHRONO_LOG_CATEGORY_GENERAL:
            return "general";
        case CHRONO_LOG_CATEGORY_CONSTRAINT:
            return "constraint";
        case CHRONO_LOG_CATEGORY_SOLVER:
            return "solver";
        case CHRONO_LOG_CATEGORY_BENCHMARK:
            return "benchmark";
        default:
            return "unknown";
    }
}

static void chrono_log_default_handler(ChronoLogLevel_C level,
                                       ChronoLogCategory_C category,
                                       const char *message,
                                       void *user_data) {
    (void)user_data;
    if (!message) {
        return;
    }
    fprintf(stderr, "[%s] (%s) %s\n", level_name_internal(level), category_name_internal(category), message);
    fflush(stderr);
}

void chrono_log_set_handler(ChronoLogHandler_C handler, void *user_data) {
    g_handler = handler;
    g_handler_user = handler ? user_data : NULL;
}

void chrono_log_get_handler(ChronoLogHandler_C *out_handler, void **out_user_data) {
    if (out_handler) {
        *out_handler = g_handler;
    }
    if (out_user_data) {
        *out_user_data = g_handler ? g_handler_user : NULL;
    }
}

void chrono_log_set_level(ChronoLogLevel_C level) {
    if (level < CHRONO_LOG_LEVEL_TRACE) {
        level = CHRONO_LOG_LEVEL_TRACE;
    } else if (level > CHRONO_LOG_LEVEL_ERROR) {
        level = CHRONO_LOG_LEVEL_ERROR;
    }
    g_min_level = level;
}

ChronoLogLevel_C chrono_log_get_level(void) {
    return g_min_level;
}

int chrono_log_is_enabled(ChronoLogLevel_C level, ChronoLogCategory_C category) {
    (void)category;
    return level >= g_min_level;
}

void chrono_log_write(ChronoLogLevel_C level,
                      ChronoLogCategory_C category,
                      const char *format,
                      ...) {
    if (!format || !chrono_log_is_enabled(level, category)) {
        return;
    }
    va_list args;
    va_start(args, format);
    chrono_log_write_va(level, category, format, args);
    va_end(args);
}

void chrono_log_write_va(ChronoLogLevel_C level,
                         ChronoLogCategory_C category,
                         const char *format,
                         va_list args) {
    if (!format || !chrono_log_is_enabled(level, category)) {
        return;
    }
    char buffer[CHRONO_LOG_MESSAGE_MAX];
    va_list args_copy;
    va_copy(args_copy, args);
    vsnprintf(buffer, sizeof(buffer), format, args_copy);
    va_end(args_copy);
    if (g_handler) {
        g_handler(level, category, buffer, g_handler_user);
    } else {
        chrono_log_default_handler(level, category, buffer, NULL);
    }
}

const char *chrono_log_level_name(ChronoLogLevel_C level) {
    return level_name_internal(level);
}

const char *chrono_log_category_name(ChronoLogCategory_C category) {
    return category_name_internal(category);
}

void chrono_log_flush(void) {
    if (!g_handler) {
        fflush(stderr);
    }
}
