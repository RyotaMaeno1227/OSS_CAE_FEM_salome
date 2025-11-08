#ifndef CHRONO_ISLAND2D_TBB_H
#define CHRONO_ISLAND2D_TBB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chrono_island2d.h"

#define CHRONO_ISLAND_TBB_AVAILABLE 1

int chrono_island2d_run_tbb(const ChronoIsland2DWorkspace_C *workspace,
                            double dt,
                            const ChronoConstraint2DBatchConfig_C *config,
                            int max_threads);

#ifdef __cplusplus
}
#endif

#endif /* CHRONO_ISLAND2D_TBB_H */
