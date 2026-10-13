/**
 * @file uds_server.c
 * @brief Complete UDS Unified Server Framework Implementation (ISO 14229-1)
 * @details Complete drop-in replacement with 21 service dispatch table, session, addressing, and security gates.
 */

#include <string.h>
#include "uds_server.h"
#include "uds_session.h"
#include "uds_config.h"

/* Service handler headers (library-provided) */
#include "svc_diag_session.h"
#include "svc_ecu_reset.h"
#include "svc_tester_present.h"
#include "svc_security_access.h"
#include "svc_read_did.h"
#include "svc_write_did.h"
#include "svc_read_dtc.h"
#include "svc_clear_dtc.h"
#include "svc_download.h"
#include "svc_upload.h"
#include "svc_periodic_did.h"
#include "svc_dyn_did.h"
#include "svc_io_control.h"
#include "svc_link_ctrl.h"
#include "svc_routine_control.h"
#include "svc_comm_control.h"
#include "svc_control_dtc.h"
#include "svc_read_address.h"
#include "svc_write_address.h"

/* ── Weak fallback declarations for standalone compilation ───────── */
#if defined(__GNUC__) || defined(__clang__)
#define UDS_WEAK __attribute__((weak))
#else
#define UDS_WEAK
#endif

UDS_WEAK void uds_session_refresh(void) { }
UDS_WEAK bool uds_session_check_timeout(void) { return false; }
UDS_WEAK void uds_reset_poll(void) { }
UDS_WEAK void uds_state_init(void) { }
UDS_WEAK void svc_diag_session_control(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_ecu_reset(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_tester_present(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_read_data_by_id(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_write_data_by_id(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_request_download(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_request_upload(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_transfer_data(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_transfer_exit(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_read_periodic_id(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_dyn_define_did(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_io_control(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }
UDS_WEAK void svc_link_control(const uds_request_t *r, uds_response_t *p) { (void)r; (void)p; }

/* ── App config storage ───────────────────────────────────────────── */

static const uds_app_config_t *s_app_config = NULL;

const uds_app_config_t *uds_get_app_config(void) {
    return s_app_config;
}

/* ── Service Dispatch Table ───────────────────────────────────────── */

static uds_service_entry_t s_service_table[UDS_MAX_SERVICES];
static uint8_t s_service_count = 0U;
static uint32_t s_active_security_mask = UDS_SEC_LEVEL_LOCKED;
static uint8_t  s_active_session_mask  = SESSION_MASK_DEFAULT;

static void register_service_ext(uint8_t sid, uds_service_handler_t handler,
                                 uint8_t session_mask, bool requires_security,
                                 uint32_t address_mask, uint32_t security_mask) {
    if (s_service_count >= UDS_MAX_SERVICES) return;
    uds_service_entry_t *e = &s_service_table[s_service_count++];
    e->sid = sid;
    e->handler = handler;
    e->session_mask = session_mask;
    e->requires_security = requires_security;
    e->addressMask = address_mask;
    e->securityMask = security_mask;
}

static void register_service(uint8_t sid, uds_service_handler_t handler,
                             uint8_t session_mask, bool requires_security) {
    uint32_t sec_mask = requires_security ? (UDS_SEC_LEVEL_1 | UDS_SEC_LEVEL_2) : UDS_SEC_LEVEL_ALL;
    uint32_t addr_mask = UDS_ADDR_ALL;
    if ((sid == 0x23U) || (sid == 0x27U) || (sid == 0x2AU) || (sid == 0x2CU) ||
        (sid == 0x2EU) || (sid == 0x2FU) || (sid == 0x31U) || (sid == 0x34U) ||
        (sid == 0x35U) || (sid == 0x36U) || (sid == 0x37U) || (sid == 0x3DU) || (sid == 0x87U)) {
        addr_mask = UDS_ADDR_PHYSICAL;
    }
    register_service_ext(sid, handler, session_mask, requires_security, addr_mask, sec_mask);
}

static void build_nrc(uds_response_t *resp, uint8_t sid, uint8_t nrc) {
    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
    resp->data[1] = sid;
    resp->data[2] = nrc;
    resp->len = 3U;
    resp->suppress = false;
}

static const uds_service_entry_t *find_service(uint8_t sid) {
    for (int32_t i = (int32_t)s_service_count - 1; i >= 0; i--) {
        if (s_service_table[i].sid == sid) {
            return &s_service_table[i];
        }
    }
    return NULL;
}

/* ── Public API ───────────────────────────────────────────────────── */

void uds_server_init(const uds_app_config_t *config) {
    s_app_config = config;
    s_service_count = 0U;
    s_active_security_mask = UDS_SEC_LEVEL_LOCKED;
    s_active_session_mask  = SESSION_MASK_DEFAULT;

    /* Install all 21 UDS base services (ISO 14229-1) */
    /*               SID   handler                     session_mask                                   security */
    register_service(0x10, svc_diag_session_control,    SESSION_MASK_ALL,                                 false);
    register_service(0x11, svc_ecu_reset,               SESSION_MASK_ALL,                                 false);
    register_service(0x14, svc_clear_dtc,               SESSION_MASK_NON_DEFAULT,                         false);
    register_service(0x19, svc_read_dtc_info,           SESSION_MASK_ALL,                                 false);
    register_service(0x22, svc_read_data_by_id,         SESSION_MASK_ALL,                                 false);
    register_service(0x23, svc_read_memory_by_address,  SESSION_MASK_ALL,                                 false);
    register_service(0x27, svc_security_access,         SESSION_MASK_NON_DEFAULT,                         false);
    register_service(0x28, svc_comm_control,            SESSION_MASK_ALL,                                 false);
    register_service(0x2A, svc_read_periodic_id,        SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x2C, svc_dyn_define_did,          SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x2E, svc_write_data_by_id,        SESSION_MASK_NON_DEFAULT,                         true);
    register_service(0x2F, svc_io_control,              SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);
    register_service(0x31, svc_routine_control,         SESSION_MASK_ALL,                                 false);
    register_service(0x34, svc_request_download,        SESSION_MASK_PROGRAMMING,                         true);
    register_service(0x35, svc_request_upload,          SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x36, svc_transfer_data,           SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x37, svc_transfer_exit,           SESSION_MASK_PROGRAMMING | SESSION_MASK_EXTENDED, true);
    register_service(0x3E, svc_tester_present,          SESSION_MASK_ALL,                                 false);
    register_service(0x3D, svc_write_memory_by_address, SESSION_MASK_ALL,                                 false);
    register_service(0x85, svc_control_dtc,             SESSION_MASK_ALL,                                 false);
    register_service(0x87, svc_link_control,            SESSION_MASK_EXTENDED | SESSION_MASK_ENGINEERING, false);

    uds_state_init();
}

bool uds_server_register(const uds_service_entry_t *entry) {
    if ((entry == NULL) || (s_service_count >= UDS_MAX_SERVICES)) {
        return false;
    }
    s_service_table[s_service_count++] = *entry;
    return true;
}

void uds_server_set_security_level(uint32_t active_level_mask) {
    s_active_security_mask = active_level_mask;
}

uint32_t uds_server_get_security_level(void) {
    return s_active_security_mask;
}

void uds_server_set_session(uint8_t session_mask) {
    s_active_session_mask = session_mask;
}

uint8_t uds_server_get_session(void) {
    return s_active_session_mask;
}

bool uds_server_process(const uint8_t *data, uint16_t len,
                        bool functional, uds_response_t *resp) {
    if ((data == NULL) || (len < 1U) || (resp == NULL)) {
        return false;
    }

    uint8_t sid = data[0];
    memset(resp, 0, sizeof(*resp));

    /* Refresh S3 timer on any valid request (ISO 14229-1 S3server) */
    uds_session_refresh();

    /* Find service handler */
    const uds_service_entry_t *entry = find_service(sid);
    if (entry == NULL) {
        if (functional) return false; /* No NRC for unknown SID on functional addressing */
        build_nrc(resp, sid, NRC_SERVICE_NOT_SUPPORTED);
        return true;
    }

    /* 1. Addressing Mode Gate (ISO 14229-1) */
    if (functional) {
        if ((entry->addressMask & UDS_ADDR_FUNCTIONAL) == 0U) {
            /* Functional request to physical-only service: Drop silently without response! */
            return false;
        }
    } else {
        if ((entry->addressMask & UDS_ADDR_PHYSICAL) == 0U) {
            build_nrc(resp, sid, NRC_SERVICE_NOT_SUPPORTED);
            return true;
        }
    }

    /* 2. Diagnostic Session Gate */
    if ((entry->session_mask & s_active_session_mask) == 0U) {
        if (functional) return false;
        build_nrc(resp, sid, NRC_SERVICE_NOT_SUPPORTED_IN_SESSION);
        return true;
    }

    /* 3. Security Access Gate */
    if (entry->requires_security && (s_active_security_mask == UDS_SEC_LEVEL_LOCKED)) {
        if (functional) return false;
        build_nrc(resp, sid, NRC_SECURITY_ACCESS_DENIED);
        return true;
    }
    if ((entry->securityMask & s_active_security_mask) == 0U) {
        if (functional) return false;
        build_nrc(resp, sid, NRC_SECURITY_ACCESS_DENIED);
        return true;
    }

    /* Build request and dispatch */
    uds_request_t req;
    req.sid        = sid;
    req.data       = (len > 1U) ? &data[1] : NULL;
    req.data_len   = (len > 1U) ? (len - 1U) : 0U;
    req.functional = functional;

    entry->handler(&req, resp);

    /* Check suppress-positive-response bit (sub-function bit 7) */
    if (resp->suppress) {
        return false;
    }

    return (resp->len > 0U);
}

void uds_server_poll(void) {
    (void)uds_session_check_timeout();
    uds_security_poll();
    uds_reset_poll();
}

/* Dispatch alias functions for backward compatibility */
void uds_server_dispatch_set_security_level(uint32_t active_level_mask) {
    uds_server_set_security_level(active_level_mask);
}

uint32_t uds_server_dispatch_get_security_level(void) {
    return uds_server_get_security_level();
}

void uds_server_dispatch_set_session(uint8_t session_mask) {
    uds_server_set_session(session_mask);
}

uint8_t uds_server_dispatch_get_session(void) {
    return uds_server_get_session();
}

bool uds_server_dispatch_process(const uint8_t *data, uint16_t len, bool functional, uds_response_t *resp) {
    return uds_server_process(data, len, functional, resp);
}

