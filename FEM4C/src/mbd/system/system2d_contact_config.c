#include "system2d.h"
#include "../../common/error.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define MBD_GENERIC_CONTACT_FRICTION_VREF_DEFAULT 1.0e-1
#define MBD_GENERIC_CONTACT_FRICTION_VSMOOTH_DEFAULT 1.0e-3

/*
 * Owns narrow contact registration/config helper implementations for
 * mbd_system2d_t. Inputs are the system object, contact/body ids, contact
 * geometry/config data, and contact-pair data; outputs and side effects are
 * contact registration, contact-pair append/id assignment, count updates, and
 * related error handling. Lifecycle/default config, body/constraint storage,
 * solver orchestration, contact runtime refresh, input parsing, contact-load
 * bridge, output bridge, static/coupled bridge, and file IO remain in other
 * files. This extraction is for readability and maintainability, not behavior
 * change.
 */

static int mbd_system2d_contact_pair_id_exists(const mbd_system2d_t *system,
                                               int pair_id)
{
    int i = 0;

    if (!system) {
        return 0;
    }
    for (i = 0; i < system->num_contact_pairs; ++i) {
        if (system->contact_pairs[i].is_defined &&
            system->contact_pairs[i].pair_id == pair_id) {
            return 1;
        }
    }
    for (i = 0; i < system->num_generic_contact_pairs; ++i) {
        if (system->generic_contact_pairs[i].is_defined &&
            system->generic_contact_pairs[i].pair_id == pair_id) {
            return 1;
        }
    }
    return 0;
}

fem_error_t mbd_system2d_register_body_circle(mbd_system2d_t *system,
                                              int body_id,
                                              double radius,
                                              double thickness)
{
    CHECK_NULL(system, "mbd_system2d");

    if (body_id < 0 || body_id >= MBD_SYSTEM2D_MAX_BODIES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact body_id %d outside supported range [0,%d)",
                         body_id,
                         MBD_SYSTEM2D_MAX_BODIES);
    }
    if (!isfinite(radius) || radius <= 0.0 ||
        !isfinite(thickness) || thickness <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact circle for body %d requires positive finite radius/thickness",
                         body_id);
    }

    if (!system->contact_circles[body_id].is_defined) {
        system->num_contact_circles += 1;
    }
    system->contact_circles[body_id].is_defined = 1;
    system->contact_circles[body_id].body_id = body_id;
    system->contact_circles[body_id].radius = radius;
    system->contact_circles[body_id].thickness = thickness;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_register_contact_halfspace(mbd_system2d_t *system,
                                                    int halfspace_id,
                                                    const double point[2],
                                                    const double normal[2],
                                                    double thickness)
{
    double norm = 0.0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(point, "contact halfspace point");
    CHECK_NULL(normal, "contact halfspace normal");

    if (halfspace_id < 0 || halfspace_id >= MBD_CONTACT2D_MAX_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace_id %d outside supported range [0,%d)",
                         halfspace_id,
                         MBD_CONTACT2D_MAX_PAIRS);
    }
    if (!isfinite(point[0]) || !isfinite(point[1]) ||
        !isfinite(normal[0]) || !isfinite(normal[1]) ||
        !isfinite(thickness) || thickness <= 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace %d requires finite point/normal and positive thickness",
                         halfspace_id);
    }

    norm = sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
    if (norm <= MBD_CONTACT2D_COINCIDENCE_EPS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace %d requires non-zero normal",
                         halfspace_id);
    }

    if (!system->contact_halfspaces[halfspace_id].is_defined) {
        system->num_contact_halfspaces += 1;
    }
    memset(&system->contact_halfspaces[halfspace_id], 0, sizeof(system->contact_halfspaces[halfspace_id]));
    system->contact_halfspaces[halfspace_id].is_defined = 1;
    system->contact_halfspaces[halfspace_id].halfspace_id = halfspace_id;
    system->contact_halfspaces[halfspace_id].point[0] = point[0];
    system->contact_halfspaces[halfspace_id].point[1] = point[1];
    system->contact_halfspaces[halfspace_id].normal[0] = normal[0] / norm;
    system->contact_halfspaces[halfspace_id].normal[1] = normal[1] / norm;
    system->contact_halfspaces[halfspace_id].thickness = thickness;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_register_contact_surface_polyline(
    mbd_system2d_t *system,
    int surface_id,
    int body_id,
    const char *csv_path)
{
    mbd_contact_surface_polyline2d_t *surface = NULL;
    int i = 0;

    CHECK_NULL(system, "mbd_system2d");
    CHECK_NULL(csv_path, "generic contact surface csv_path");

    if (surface_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact surface_id %d must be non-negative",
                         surface_id);
    }
    if (body_id < 0 || body_id >= MBD_SYSTEM2D_MAX_BODIES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact surface %d body %d outside supported range [0,%d)",
                         surface_id,
                         body_id,
                         MBD_SYSTEM2D_MAX_BODIES);
    }
    if (csv_path[0] == '\0') {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact surface %d requires non-empty csv path",
                         surface_id);
    }
    if (system->num_contact_surface_polylines >= MBD_GENERIC_CONTACT2D_MAX_SURFACES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact surface count exceeds supported range [0,%d)",
                         MBD_GENERIC_CONTACT2D_MAX_SURFACES);
    }
    for (i = 0; i < system->num_contact_surface_polylines; ++i) {
        if (system->contact_surface_polylines[i].is_defined &&
            system->contact_surface_polylines[i].surface_id == surface_id) {
            return error_set(FEM_ERROR_INVALID_INPUT,
                             "duplicate generic contact surface_id %d",
                             surface_id);
        }
    }

    surface = &system->contact_surface_polylines[system->num_contact_surface_polylines];
    memset(surface, 0, sizeof(*surface));
    surface->is_defined = 1;
    surface->surface_id = surface_id;
    surface->body_id = body_id;
    snprintf(surface->csv_path, sizeof(surface->csv_path), "%s", csv_path);
    system->num_contact_surface_polylines += 1;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_append_contact_pair(mbd_system2d_t *system,
                                             int pair_id,
                                             int body_i,
                                             int body_j,
                                             double k_n,
                                             double c_n,
                                             double mu_base)
{
    mbd_contact_pair2d_t *pair = NULL;

    CHECK_NULL(system, "mbd_system2d");

    if (pair_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair_id %d must be non-negative",
                         pair_id);
    }
    if (body_i < 0 || body_i >= MBD_SYSTEM2D_MAX_BODIES ||
        body_j < 0 || body_j >= MBD_SYSTEM2D_MAX_BODIES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair %d bodies (%d,%d) outside supported range [0,%d)",
                         pair_id,
                         body_i,
                         body_j,
                         MBD_SYSTEM2D_MAX_BODIES);
    }
    if (body_i == body_j) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair %d must reference distinct bodies (got %d,%d)",
                         pair_id,
                         body_i,
                         body_j);
    }
    if (system->num_contact_pairs >= MBD_CONTACT2D_MAX_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair count exceeds supported range [0,%d)",
                         MBD_CONTACT2D_MAX_PAIRS);
    }
    if (!isfinite(k_n) || k_n <= 0.0 ||
        !isfinite(c_n) || c_n < 0.0 ||
        !isfinite(mu_base) || mu_base < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair %d requires finite k_n>0, c_n>=0, mu_base>=0",
                         pair_id);
    }
    if (mbd_system2d_contact_pair_id_exists(system, pair_id)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate contact pair_id %d",
                         pair_id);
    }

    pair = &system->contact_pairs[system->num_contact_pairs];
    memset(pair, 0, sizeof(*pair));
    pair->is_defined = 1;
    pair->pair_id = pair_id;
    pair->body_i = body_i;
    pair->body_j = body_j;
    pair->halfspace_id = -1;
    pair->proxy_geometry = MBD_CONTACT_PROXY_CIRCLE_CIRCLE;
    pair->base_k_n = k_n;
    pair->c_n = c_n;
    pair->base_mu = mu_base;
    system->num_contact_pairs += 1;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_append_contact_halfspace_pair(mbd_system2d_t *system,
                                                       int pair_id,
                                                       int body_circle,
                                                       int halfspace_id,
                                                       double k_n,
                                                       double c_n,
                                                       double mu_base)
{
    mbd_contact_pair2d_t *pair = NULL;

    CHECK_NULL(system, "mbd_system2d");

    if (pair_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair_id %d must be non-negative",
                         pair_id);
    }
    if (body_circle < 0 || body_circle >= MBD_SYSTEM2D_MAX_BODIES) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace pair %d body %d outside supported range [0,%d)",
                         pair_id,
                         body_circle,
                         MBD_SYSTEM2D_MAX_BODIES);
    }
    if (halfspace_id < 0 || halfspace_id >= MBD_CONTACT2D_MAX_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace pair %d halfspace %d outside supported range [0,%d)",
                         pair_id,
                         halfspace_id,
                         MBD_CONTACT2D_MAX_PAIRS);
    }
    if (system->num_contact_pairs >= MBD_CONTACT2D_MAX_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact pair count exceeds supported range [0,%d)",
                         MBD_CONTACT2D_MAX_PAIRS);
    }
    if (!isfinite(k_n) || k_n <= 0.0 ||
        !isfinite(c_n) || c_n < 0.0 ||
        !isfinite(mu_base) || mu_base < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "contact halfspace pair %d requires finite k_n>0, c_n>=0, mu_base>=0",
                         pair_id);
    }
    if (mbd_system2d_contact_pair_id_exists(system, pair_id)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate contact pair_id %d",
                         pair_id);
    }

    pair = &system->contact_pairs[system->num_contact_pairs];
    memset(pair, 0, sizeof(*pair));
    pair->is_defined = 1;
    pair->pair_id = pair_id;
    pair->body_i = body_circle;
    pair->body_j = -1;
    pair->halfspace_id = halfspace_id;
    pair->proxy_geometry = MBD_CONTACT_PROXY_CIRCLE_HALFSPACE;
    pair->base_k_n = k_n;
    pair->c_n = c_n;
    pair->base_mu = mu_base;
    system->num_contact_pairs += 1;
    return FEM_SUCCESS;
}

fem_error_t mbd_system2d_append_generic_contact_pair(mbd_system2d_t *system,
                                                     int pair_id,
                                                     int surface_i,
                                                     int surface_j,
                                                     double k_n,
                                                     double c_n,
                                                     double mu_base,
                                                     double mu_static,
                                                     double mu_dynamic,
                                                     double v_ref,
                                                     double v_smooth)
{
    mbd_contact_generic_pair2d_t *pair = NULL;

    CHECK_NULL(system, "mbd_system2d");

    if (pair_id < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact pair_id %d must be non-negative",
                         pair_id);
    }
    if (surface_i < 0 || surface_j < 0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact pair %d requires non-negative surface ids",
                         pair_id);
    }
    if (surface_i == surface_j) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact pair %d must reference distinct surfaces (got %d,%d)",
                         pair_id,
                         surface_i,
                         surface_j);
    }
    if (system->num_generic_contact_pairs >= MBD_GENERIC_CONTACT2D_MAX_PAIRS) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact pair count exceeds supported range [0,%d)",
                         MBD_GENERIC_CONTACT2D_MAX_PAIRS);
    }
    if (!isfinite(k_n) || k_n <= 0.0 ||
        !isfinite(c_n) || c_n < 0.0 ||
        !isfinite(mu_base) || mu_base < 0.0 ||
        !isfinite(mu_static) || mu_static < 0.0 ||
        !isfinite(mu_dynamic) || mu_dynamic < 0.0 ||
        !isfinite(v_ref) || v_ref < 0.0 ||
        !isfinite(v_smooth) || v_smooth < 0.0) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "generic contact pair %d requires finite k_n>0, c_n>=0, mu>=0, v_ref>=0, v_smooth>=0",
                         pair_id);
    }
    if (mbd_system2d_contact_pair_id_exists(system, pair_id)) {
        return error_set(FEM_ERROR_INVALID_INPUT,
                         "duplicate contact pair_id %d",
                         pair_id);
    }

    pair = &system->generic_contact_pairs[system->num_generic_contact_pairs];
    memset(pair, 0, sizeof(*pair));
    pair->is_defined = 1;
    pair->pair_id = pair_id;
    pair->surface_i = surface_i;
    pair->surface_j = surface_j;
    pair->base_k_n = k_n;
    pair->c_n = c_n;
    pair->base_mu = mu_base;
    pair->mu_static = mu_static;
    pair->mu_dynamic = mu_dynamic;
    pair->v_ref = v_ref > 0.0 ? v_ref : MBD_GENERIC_CONTACT_FRICTION_VREF_DEFAULT;
    pair->v_smooth = v_smooth > 0.0 ? v_smooth : MBD_GENERIC_CONTACT_FRICTION_VSMOOTH_DEFAULT;
    system->num_generic_contact_pairs += 1;
    return FEM_SUCCESS;
}
