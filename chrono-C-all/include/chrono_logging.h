#ifndef CHRONO_LOGGING_H
#define CHRONO_LOGGING_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CHRONO_LOG_LEVEL_TRACE = 0,
    CHRONO_LOG_LEVEL_DEBUG = 1,
    CHRONO_LOG_LEVEL_INFO = 2,
    CHRONO_LOG_LEVEL_WARNING = 3,
    CHRONO_LOG_LEVEL_ERROR = 4
} ChronoLogLevel_C;

typedef enum {
    CHRONO_LOG_CATEGORY_GENERAL = 0,
    CHRONO_LOG_CATEGORY_CONSTRAINT = 1,
    CHRONO_LOG_CATEGORY_SOLVER = 2,
    CHRONO_LOG_CATEGORY_BENCHMARK = 3
} ChronoLogCategory_C;

typedef void (*ChronoLogHandler_C)(ChronoLogLevel_C level,
                                   ChronoLogCategory_C category,
                                   const char *message,
                                   void *user_data);

void chrono_log_set_handler(ChronoLogHandler_C handler, void *user_data);
void chrono_log_get_handler(ChronoLogHandler_C *out_handler, void **out_user_data);

void chrono_log_set_level(ChronoLogLevel_C level);
ChronoLogLevel_C chrono_log_get_level(void);
int chrono_log_is_enabled(ChronoLogLevel_C level, ChronoLogCategory_C category);

#if defined(__GNUC__) || defined(__clang__)
#define CHRONO_PRINTF_ATTR(fmt_index, arg_index) __attribute__((format(printf, fmt_index, arg_index)))
#else
#define CHRONO_PRINTF_ATTR(fmt_index, arg_index)
#endif

void chrono_log_write(ChronoLogLevel_C level,
                      ChronoLogCategory_C category,
                      const char *format,
                      ...) CHRONO_PRINTF_ATTR(3, 4);
void chrono_log_write_va(ChronoLogLevel_C level,
                         ChronoLogCategory_C category,
                         const char *format,
                         va_list args);

const char *chrono_log_level_name(ChronoLogLevel_C level);
const char *chrono_log_category_name(ChronoLogCategory_C category);

void chrono_log_flush(void);

#undef CHRONO_PRINTF_ATTR

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_LOGGING_H */
