/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_SECURITY_REFERENCE_H
#define STM32_UDS_ISO_TP_SECURITY_REFERENCE_H

#include "uds_iso_tp/uds.h"

#include <stdbool.h>
#include <stdint.h>

#define UDS_SECURITY_REFERENCE_SEED_LENGTH 4U
#define UDS_SECURITY_REFERENCE_MAX_KEY_LENGTH 16U
#define UDS_SECURITY_REFERENCE_LEVEL_1 1U
#define UDS_SECURITY_REFERENCE_LEVEL_5 5U
#define UDS_SECURITY_REFERENCE_DEFAULT_MAX_ATTEMPTS 3U
#define UDS_SECURITY_REFERENCE_DEFAULT_INITIAL_DELAY_MS 10000U
#define UDS_SECURITY_REFERENCE_DEFAULT_LOCKOUT_MS 10000U
#define UDS_SECURITY_REFERENCE_DEFAULT_SEED_TIMEOUT_MS 10000U

typedef enum {
    UDS_SECURITY_REFERENCE_STATE_LOCKED = 0,
    UDS_SECURITY_REFERENCE_STATE_WAITING_FOR_KEY,
    UDS_SECURITY_REFERENCE_STATE_UNLOCKED,
    UDS_SECURITY_REFERENCE_STATE_DELAY
} UdsSecurityReferenceState;

typedef enum {
    UDS_SECURITY_REFERENCE_OK = 0,
    UDS_SECURITY_REFERENCE_INVALID_ARGUMENT,
    UDS_SECURITY_REFERENCE_BUFFER_TOO_SMALL,
    UDS_SECURITY_REFERENCE_DELAY_ACTIVE,
    UDS_SECURITY_REFERENCE_SEQUENCE_ERROR,
    UDS_SECURITY_REFERENCE_INVALID_KEY,
    UDS_SECURITY_REFERENCE_ATTEMPTS_EXCEEDED,
    UDS_SECURITY_REFERENCE_LEVEL_UNSUPPORTED
} UdsSecurityReferenceResult;

typedef struct {
    uint32_t deterministic_state;
    uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];
    uint8_t level;
    bool seed_valid;
} UdsSecurityReferenceCallbacks;

typedef struct {
    uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];
    uint8_t key[UDS_SECURITY_REFERENCE_MAX_KEY_LENGTH];
    uint8_t key_length;
    uint8_t security_level;
    uint8_t pending_level;
    uint8_t failed_attempts;
    uint8_t maximum_attempts;
    uint32_t lockout_ms;
    uint32_t lockout_until_ms;
    uint32_t initial_delay_until_ms;
    uint32_t seed_expiry_ms;
    uint32_t initial_delay_ms;
    uint32_t seed_timeout_ms;
    uint32_t deterministic_state;
    UdsSecurityReferenceState state;
    bool initial_delay_active;
    bool lockout_active;
    bool seed_timer_active;
    bool seed_valid;
} UdsSecurityReference;

/* This implementation is a NON-PRODUCTION deterministic reference/test algorithm. It is
 * intentionally replaceable: production code must inject an approved provider
 * without changing UDS core dispatch. */
void uds_security_reference_callbacks_init(UdsSecurityReferenceCallbacks *callbacks,
                                           uint32_t deterministic_seed);
UdsCallbackResult uds_security_reference_seed_callback(void *context, uint8_t level, uint8_t *seed,
                                                       uint16_t *length, uint16_t capacity);
UdsCallbackResult uds_security_reference_key_callback(void *context, uint8_t level,
                                                      const uint8_t *key, uint16_t length);
void uds_security_reference_init(UdsSecurityReference *provider, uint32_t deterministic_seed,
                                 uint8_t maximum_attempts, uint32_t lockout_ms);
bool uds_security_reference_calculate_key(uint8_t level, const uint8_t *seed, uint16_t seed_length,
                                          uint8_t *key, uint16_t key_capacity,
                                          uint16_t *key_length);
UdsSecurityReferenceResult uds_security_reference_generate_seed(UdsSecurityReference *provider,
                                                                uint8_t level, uint8_t *seed,
                                                                uint16_t *length, uint16_t capacity,
                                                                uint32_t now_ms);
UdsSecurityReferenceResult uds_security_reference_verify_key(UdsSecurityReference *provider,
                                                             uint8_t level, const uint8_t *key,
                                                             uint16_t length, uint32_t now_ms);
void uds_security_reference_session_reset(UdsSecurityReference *provider);
void uds_security_reference_tick(UdsSecurityReference *provider, uint32_t now_ms);
bool uds_security_reference_is_locked(const UdsSecurityReference *provider, uint32_t now_ms);
UdsSecurityReferenceState uds_security_reference_state(const UdsSecurityReference *provider);
uint8_t uds_security_reference_security_level(const UdsSecurityReference *provider);
uint8_t uds_security_reference_failed_attempts(const UdsSecurityReference *provider);

#endif
