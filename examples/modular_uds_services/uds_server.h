/**
 * @file uds_server.h
 * @brief Complete UDS Unified Server Framework (ISO 14229-1)
 * @details Complete drop-in replacement supporting session, security, and addressing gating.
 */

#ifndef UDS_SERVER_H
#define UDS_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "uds_types.h"
#include "uds_config.h"

/* Addressing Modes (ISO 14229-1) */
#define UDS_ADDR_PHYSICAL     (1U << 0)
#define UDS_ADDR_FUNCTIONAL   (1U << 1)
#define UDS_ADDR_ALL          (UDS_ADDR_PHYSICAL | UDS_ADDR_FUNCTIONAL)

/* Security Levels (ISO 14229-1) */
#define UDS_SEC_LEVEL_LOCKED  (1U << 0)
#define UDS_SEC_LEVEL_1       (1U << 1)
#define UDS_SEC_LEVEL_2       (1U << 2)
#define UDS_SEC_LEVEL_ALL     0xFFFFFFFFU

#ifndef UDS_MAX_SERVICES
#define UDS_MAX_SERVICES      32U
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t               sid;
    uds_service_handler_t handler;
    uint8_t               session_mask;      /**< Allowed sessions bitmask */
    bool                  requires_security; /**< Legacy boolean flag */
    uint32_t              addressMask;       /**< Physical / Functional mask */
    uint32_t              securityMask;      /**< Required security levels */
} uds_service_entry_t;

struct uds_app_config_s {
    void *user_data;
};

/**
 * @brief Initialize UDS Server with application configuration and register all 21 core services
 */
void uds_server_init(const uds_app_config_t *config);

/**
 * @brief Retrieve current registered application configuration
 */
const uds_app_config_t *uds_get_app_config(void);

/**
 * @brief Dynamically register or override a service entry
 */
bool uds_server_register(const uds_service_entry_t *entry);

/**
 * @brief Process an incoming diagnostic request frame
 * @param data Request payload bytes (including SID at data[0])
 * @param len Total length of data
 * @param functional True if functional addressing, false if physical
 * @param resp Output response structure
 * @return True if response generated, false if suppressed or ignored
 */
bool uds_server_process(const uint8_t *data, uint16_t len,
                        bool functional, uds_response_t *resp);

/**
 * @brief Periodic background task (handles S3 timeout, security lockout, and ECU reset)
 */
void uds_server_poll(void);

/**
 * @brief Set currently active security level bitmask
 */
void uds_server_set_security_level(uint32_t active_level_mask);

/**
 * @brief Get currently active security level bitmask
 */
uint32_t uds_server_get_security_level(void);

/**
 * @brief Set currently active diagnostic session bitmask
 */
void uds_server_set_session(uint8_t session_mask);

/**
 * @brief Get currently active diagnostic session bitmask
 */
uint8_t uds_server_get_session(void);

#ifdef __cplusplus
}
#endif

#endif /* UDS_SERVER_H */
