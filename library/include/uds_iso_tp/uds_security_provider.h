/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_SECURITY_PROVIDER_H
#define STM32_UDS_ISO_TP_SECURITY_PROVIDER_H

#include <stdbool.h>
#include <stdint.h>

#define UDS_SECURITY_PROVIDER_SEED_LENGTH 4U
#define UDS_SECURITY_PROVIDER_MAX_KEY_LENGTH 16U
#define UDS_SECURITY_PROVIDER_DEFAULT_MAX_ATTEMPTS 3U
#define UDS_SECURITY_PROVIDER_DEFAULT_INITIAL_DELAY_MS 10000U
#define UDS_SECURITY_PROVIDER_DEFAULT_LOCKOUT_MS 10000U
#define UDS_SECURITY_PROVIDER_DEFAULT_SEED_TIMEOUT_MS 10000U

typedef enum {
    UDS_SECURITY_PROVIDER_STATE_LOCKED = 0,
    UDS_SECURITY_PROVIDER_STATE_WAITING_FOR_KEY,
    UDS_SECURITY_PROVIDER_STATE_UNLOCKED,
    UDS_SECURITY_PROVIDER_STATE_DELAY
} UdsSecurityProviderState;

typedef enum {
    UDS_SECURITY_OK = 0,
    UDS_SECURITY_INVALID_ARGUMENT,
    UDS_SECURITY_BUFFER_TOO_SMALL,
    UDS_SECURITY_DELAY_ACTIVE,
    UDS_SECURITY_SEQUENCE_ERROR,
    UDS_SECURITY_INVALID_KEY,
    UDS_SECURITY_ATTEMPTS_EXCEEDED,
    UDS_SECURITY_LEVEL_UNSUPPORTED
} UdsSecurityResult;

typedef struct {
    uint8_t seed[UDS_SECURITY_PROVIDER_SEED_LENGTH];
    uint8_t key[UDS_SECURITY_PROVIDER_MAX_KEY_LENGTH];
    uint8_t key_length;
    uint8_t security_level;
    uint8_t failed_attempts;
    uint8_t maximum_attempts;
    uint32_t lockout_ms;
    uint32_t lockout_until_ms;
    uint32_t initial_delay_until_ms;
    uint32_t seed_expiry_ms;
    uint32_t initial_delay_ms;
    uint32_t seed_timeout_ms;
    uint32_t deterministic_state;
    UdsSecurityProviderState state;
    bool initial_delay_active;
    bool lockout_active;
    bool seed_timer_active;
    bool seed_valid;
} UdsSecurityProvider;

/* This implementation is a NON-PRODUCTION deterministic test provider. It is
 * intentionally replaceable: production code must inject an approved provider
 * without changing UDS core dispatch. */
void uds_security_provider_init(UdsSecurityProvider *provider, uint32_t deterministic_seed,
                                uint8_t maximum_attempts, uint32_t lockout_ms);
UdsSecurityResult uds_security_provider_generate_seed(UdsSecurityProvider *provider, uint8_t level,
                                                      uint8_t *seed, uint16_t *length,
                                                      uint16_t capacity, uint32_t now_ms);
UdsSecurityResult uds_security_provider_verify_key(UdsSecurityProvider *provider, uint8_t level,
                                                   const uint8_t *key, uint16_t length,
                                                   uint32_t now_ms);
void uds_security_provider_session_reset(UdsSecurityProvider *provider);
void uds_security_provider_tick(UdsSecurityProvider *provider, uint32_t now_ms);
bool uds_security_provider_is_locked(const UdsSecurityProvider *provider, uint32_t now_ms);
UdsSecurityProviderState uds_security_provider_state(const UdsSecurityProvider *provider);
uint8_t uds_security_provider_security_level(const UdsSecurityProvider *provider);
uint8_t uds_security_provider_failed_attempts(const UdsSecurityProvider *provider);

#endif
