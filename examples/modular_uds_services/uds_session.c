/**
 * @file uds_session.c
 * @brief UDS Diagnostic Session Management Implementation (ISO 14229-1)
 */

#include "uds_session.h"
#include "uds_server.h"
#include "svc_security_access.h"

#define UDS_S3_SERVER_DEFAULT_TIMEOUT_MS  5000U

static uds_session_t s_current_session = UDS_SESSION_DEFAULT;
static uint32_t s_s3_timeout_ms = UDS_S3_SERVER_DEFAULT_TIMEOUT_MS;
static uint32_t s_s3_elapsed_ms = 0U;

void uds_state_init(void) {
    s_current_session = UDS_SESSION_DEFAULT;
    s_s3_elapsed_ms = 0U;
    uds_server_set_session(SESSION_MASK_DEFAULT);
    uds_security_init();
}

uds_session_t uds_session_get(void) {
    return s_current_session;
}

uint8_t uds_session_to_mask(uds_session_t session) {
    switch (session) {
    case UDS_SESSION_DEFAULT:
        return SESSION_MASK_DEFAULT;
    case UDS_SESSION_PROGRAMMING:
        return SESSION_MASK_PROGRAMMING;
    case UDS_SESSION_EXTENDED:
        return SESSION_MASK_EXTENDED;
    case UDS_SESSION_ENGINEERING:
        return SESSION_MASK_ENGINEERING;
    default:
        return SESSION_MASK_DEFAULT;
    }
}

void uds_session_set(uds_session_t session) {
    if (session != s_current_session) {
        s_current_session = session;
        uds_server_set_session(uds_session_to_mask(session));
        /* ISO 14229-1: Security state resets to locked on session transition */
        uds_security_reset();
    }
    uds_session_refresh();
}

void uds_session_reset(void) {
    s_current_session = UDS_SESSION_DEFAULT;
    s_s3_elapsed_ms = 0U;
    uds_server_set_session(SESSION_MASK_DEFAULT);
    uds_security_reset();
}

void uds_session_refresh(void) {
    s_s3_elapsed_ms = 0U;
}

bool uds_session_check_timeout(void) {
    if (s_current_session == UDS_SESSION_DEFAULT) {
        s_s3_elapsed_ms = 0U;
        return false;
    }

    s_s3_elapsed_ms++;
    if (s_s3_elapsed_ms >= s_s3_timeout_ms) {
        uds_session_reset();
        return true;
    }
    return false;
}
