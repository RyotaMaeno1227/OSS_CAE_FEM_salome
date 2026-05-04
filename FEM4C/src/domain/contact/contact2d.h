#ifndef FEM4C_MBD_CONTACT2D_H
#define FEM4C_MBD_CONTACT2D_H

#include <stdint.h>

#define MBD_CONTACT2D_MAX_PAIRS 8
#define MBD_CONTACT2D_COINCIDENCE_EPS 1.0e-12
#define MBD_LOCAL_FEEDBACK2D_MAX_RECORDS 4096
#define MBD_SAME_TIME_CONTACT_REQUEST2D_MAX_ROWS 32768
#define MBD_CONTACT_FEEDBACK2D_VALID_MU_EFF (1u << 0)
#define MBD_CONTACT_FEEDBACK2D_VALID_GAMMA_N (1u << 1)
#define MBD_CONTACT_FEEDBACK2D_VALID_DELTA_G_EFF (1u << 2)
#define MBD_CONTACT_FEEDBACK2D_VALID_FN_REF (1u << 3)
#define MBD_CONTACT_FEEDBACK2D_VALID_P_MAX (1u << 4)
#define MBD_CONTACT_FEEDBACK2D_VALID_H_MIN (1u << 5)
#define MBD_CONTACT_FEEDBACK2D_VALID_REGIME_FLAG (1u << 6)

typedef struct {
    double mu_eff;
    double gamma_n;
    double delta_g_eff;
    double fn_ref;
    double p_max;
    double h_min;
    uint32_t regime_flag;
    uint32_t valid_flag;
} mbd_contact_feedback2d_t;

typedef struct {
    int is_defined;
    int body_id;
    double radius;
    double thickness;
} mbd_contact_circle2d_t;

typedef struct {
    int is_defined;
    int halfspace_id;
    double point[2];
    double normal[2];
    double thickness;
} mbd_contact_halfspace2d_t;

typedef enum {
    MBD_CONTACT_PROXY_CIRCLE_CIRCLE = 0,
    MBD_CONTACT_PROXY_CIRCLE_HALFSPACE = 1
} mbd_contact_proxy_geometry2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int body_i;
    int body_j;
    int halfspace_id;
    mbd_contact_proxy_geometry2d_t proxy_geometry;
    double base_k_n;
    double c_n;
    double base_mu;
    double k_prev;
    int has_k_prev;
    double last_normal[2];
    int has_last_normal;
} mbd_contact_pair2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int active;
    double gap;
    double penetration;
    double normal[2];
    double vn;
    double fn;
    double cp1[2];
    double cp2[2];
    double f1[2];
    double m1_z;
    double f2[2];
    double m2_z;
} mbd_contact_trace2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    int active;
    double gap;
    double penetration;
    double fn;
    double kn_base;
    double kn_used;
    double kn_out;
} mbd_contact_feedback_trace2d_t;

typedef struct {
    int is_defined;
    int is_valid;
    int status_ok;
    int step;
    int pair_id;
    mbd_contact_feedback2d_t reduced_data;
    double mu_eff;
    double gamma_n;
    char status[32];
} mbd_local_feedback_record2d_t;

typedef struct {
    int is_defined;
    int pair_id;
    mbd_contact_feedback2d_t reduced_data;
    double mu_base;
    double mu_used;
    double gamma_n_used;
    double k_base;
    double k_used;
    int record_step;
    int record_iter;
    int status_ok;
    char source_mode[32];
    char fallback_reason[64];
    char status[32];
} mbd_contact_feedback_use_trace2d_t;

typedef struct {
    int is_defined;
    int step;
    int iter;
    int pair_id;
    int body_i;
    int body_j;
    int request_valid;
    int contact_active;
    double x_cp;
    double y_cp;
    double n_x;
    double n_y;
    double t_x;
    double t_y;
    double gap;
    double penetration;
    double v_n;
    double v_t;
    char source_mode_before_lookup[32];
} mbd_same_time_contact_request2d_t;

#endif /* FEM4C_MBD_CONTACT2D_H */
