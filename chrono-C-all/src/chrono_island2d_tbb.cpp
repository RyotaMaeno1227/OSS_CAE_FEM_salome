#include "../include/chrono_island2d_tbb.h"

#include <exception>

#include "../include/chrono_logging.h"
#include "chrono_island2d_island_step.h"

#if __has_include(<oneapi/tbb/parallel_for.h>)
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>
namespace chrono_tbb = oneapi::tbb;
#define CHRONO_ONETBB_AVAILABLE 1
#elif __has_include(<tbb/parallel_for.h>)
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>
namespace chrono_tbb = tbb;
#define CHRONO_ONETBB_AVAILABLE 1
#else
#define CHRONO_ONETBB_AVAILABLE 0
static int g_tbb_stub_warned = 0;
#endif

int chrono_island2d_run_tbb(const ChronoIsland2DWorkspace_C *workspace,
                            double dt,
                            const ChronoConstraint2DBatchConfig_C *config,
                            int max_threads) {
#if !CHRONO_ONETBB_AVAILABLE
    (void)workspace;
    (void)dt;
    (void)config;
    (void)max_threads;
    if (!g_tbb_stub_warned) {
        chrono_log_write(CHRONO_LOG_LEVEL_INFO,
                         CHRONO_LOG_CATEGORY_SOLVER,
                         "oneTBB headers not found at build time; using fallback path");
        g_tbb_stub_warned = 1;
    }
    return 0;
#else
    if (!workspace || !config || workspace->island_count == 0) {
        return 0;
    }

    ChronoConstraint2DBatchConfig_C local_cfg = *config;
    local_cfg.enable_parallel = 0;

    const chrono_tbb::blocked_range<size_t> range(0, workspace->island_count);
    auto body = [&](const chrono_tbb::blocked_range<size_t> &chunk) {
        for (size_t idx = chunk.begin(); idx != chunk.end(); ++idx) {
            chrono_island2d_step_island(&workspace->islands[idx], dt, &local_cfg);
        }
    };

    try {
        if (max_threads > 0) {
            chrono_tbb::task_arena arena(max_threads);
            arena.execute([&]() { chrono_tbb::parallel_for(range, body); });
        } else {
            chrono_tbb::parallel_for(range, body);
        }
        return 1;
    } catch (const std::exception &ex) {
        chrono_log_write(CHRONO_LOG_LEVEL_ERROR,
                         CHRONO_LOG_CATEGORY_SOLVER,
                         "oneTBB backend error: %s",
                         ex.what());
    } catch (...) {
        chrono_log_write(CHRONO_LOG_LEVEL_ERROR,
                         CHRONO_LOG_CATEGORY_SOLVER,
                         "oneTBB backend error: unknown exception");
    }
    return 0;
#endif
}
