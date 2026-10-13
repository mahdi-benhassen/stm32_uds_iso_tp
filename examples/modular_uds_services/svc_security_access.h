/**
 * @file svc_security_access.h
 * @brief UDS Service 0x27: SecurityAccess with Dual-Level Authentication (ISO 14229-1)
 * @details Level 1: 4-byte key (Basic OEM), Level 2: 128-bit / 16-byte cryptographic AES-CMAC-128.
 */

#ifndef SVC_SECURITY_ACCESS_H
#define SVC_SECURITY_ACCESS_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_types.h"
#include "uds_server_dispatch.h"

#define UDS_SEC_SUB_REQUEST_SEED_L1   0x01U
#define UDS_SEC_SUB_SEND_KEY_L1       0x02U
#define UDS_SEC_SUB_REQUEST_SEED_L2   0x03U
#define UDS_SEC_SUB_SEND_KEY_L2       0x04U

#define SECURITY_SEED_LEN_L1          4U
#define SECURITY_KEY_LEN_L1           4U
#define SECURITY_SEED_LEN_L2          16U
#define SECURITY_KEY_LEN_L2           16U
#define SECURITY_MAX_ATTEMPTS         3U
#define SECURITY_LOCKOUT_MS           10000U

typedef enum {
    SECURITY_LOCKED = 0,
    SECURITY_SEED_SENT_L1,
    SECURITY_UNLOCKED_L1,
    SECURITY_SEED_SENT_L2,
    SECURITY_UNLOCKED_L2
} security_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Main handler for UDS Service 0x27 SecurityAccess
 */
void svc_security_access(const uds_request_t *req, uds_response_t *resp);

/**
 * @brief Initialize security access subsystem
 */
void uds_security_init(void);

/**
 * @brief Reset security state to LOCKED (called on session timeout or transition to default session)
 */
void uds_security_reset(void);

/**
 * @brief Check if any security level is currently unlocked
 */
bool uds_security_is_unlocked(void);

/**
 * @brief Check if a specific level (1 or 2) is unlocked
 */
bool uds_security_is_level_unlocked(uint8_t level);

/**
 * @brief Get current active security bitmask
 */
uint32_t uds_security_get_active_mask(void);

/**
 * @brief Get current internal security state
 */
security_state_t uds_security_get_state(void);

/**
 * @brief Periodic poll routine for managing lockout delay countdown
 */
void uds_security_poll(void);

/**
 * @brief Configure 128-bit AES master key used for Level 2 CMAC authentication
 */
void uds_security_set_aes_key(const uint8_t key[16]);

/**
 * @brief Configure secret mask used for Level 1 authentication
 */
void uds_security_set_secret_l1(const uint8_t *secret, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* SVC_SECURITY_ACCESS_H */
