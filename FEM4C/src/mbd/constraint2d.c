#include "constraint2d.h"
#include "../common/error.h"

fem_error_t mbd_constraint_validate(const mbd_constraint2d_t *c)
{
    CHECK_NULL(c, "constraint");

    if (c->id <= 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD constraint id must be positive");
    }
    if (c->body_i < 0 || c->body_j < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD body index must be non-negative");
    }
    if (c->body_i == c->body_j) {
        return error_set(FEM_ERROR_INVALID_INPUT, "MBD constraint requires two distinct bodies");
    }
    if (c->type != MBD_CONSTRAINT_DISTANCE && c->type != MBD_CONSTRAINT_REVOLUTE) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Unknown MBD constraint type: %d", c->type);
    }
    if (c->type == MBD_CONSTRAINT_DISTANCE && c->target_value <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT, "Distance constraint target must be positive");
    }

    return FEM_SUCCESS;
}

fem_error_t mbd_constraint_init_distance(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2],
                                         double distance)
{
    CHECK_NULL(out, "out");
    CHECK_NULL(anchor_i, "anchor_i");
    CHECK_NULL(anchor_j, "anchor_j");

    out->id = id;
    out->type = MBD_CONSTRAINT_DISTANCE;
    out->body_i = body_i;
    out->body_j = body_j;
    out->anchor_i[0] = anchor_i[0];
    out->anchor_i[1] = anchor_i[1];
    out->anchor_j[0] = anchor_j[0];
    out->anchor_j[1] = anchor_j[1];
    out->target_value = distance;

    return mbd_constraint_validate(out);
}

fem_error_t mbd_constraint_init_revolute(mbd_constraint2d_t *out,
                                         int id,
                                         int body_i,
                                         int body_j,
                                         const double anchor_i[2],
                                         const double anchor_j[2])
{
    CHECK_NULL(out, "out");
    CHECK_NULL(anchor_i, "anchor_i");
    CHECK_NULL(anchor_j, "anchor_j");

    out->id = id;
    out->type = MBD_CONSTRAINT_REVOLUTE;
    out->body_i = body_i;
    out->body_j = body_j;
    out->anchor_i[0] = anchor_i[0];
    out->anchor_i[1] = anchor_i[1];
    out->anchor_j[0] = anchor_j[0];
    out->anchor_j[1] = anchor_j[1];
    out->target_value = 0.0;

    return mbd_constraint_validate(out);
}
