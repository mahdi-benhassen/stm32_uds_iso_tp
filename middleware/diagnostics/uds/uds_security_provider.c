/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_security_provider.h"

#include <stddef.h>

static bool deadline_active(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) < 0;
}

static uint32_t next_state(uint32_t value) {
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    return (value == 0U) ? 0xA5A5A5A5UL : value;
}

static uint8_t constant_time_equal(const uint8_t *left, const uint8_t *right, uint8_t length) {
    uint8_t difference = 0U;
    for (uint8_t index = 0U; index < length; ++index) {
        difference |= (uint8_t)(left[index] ^ right[index]);
    }
    return difference == 0U;
}

void uds_security_provider_init(UdsSecurityProvider *provider, uint32_t deterministic_seed,
                                uint8_t maximum_attempts, uint32_t lockout_ms) {
    if (provider == NULL) {
        return;
    }
    provider->key_length = 0U;
    provider->security_level = 0U;
    provider->failed_attempts = 0U;
    provider->maximum_attempts =
        (maximum_attempts == 0U) ? UDS_SECURITY_PROVIDER_DEFAULT_MAX_ATTEMPTS : maximum_attempts;
    provider->lockout_ms =
        (lockout_ms == 0U) ? UDS_SECURITY_PROVIDER_DEFAULT_LOCKOUT_MS : lockout_ms;
    provider->lockout_until_ms = 0U;
    provider->deterministic_state = (deterministic_seed == 0U) ? 0x13579BDFUL : deterministic_seed;
    provider->seed_valid = false;
    for (uint8_t index = 0U; index < UDS_SECURITY_PROVIDER_SEED_LENGTH; ++index) {
        provider->seed[index] = 0U;
    }
    for (uint8_t index = 0U; index < UDS_SECURITY_PROVIDER_MAX_KEY_LENGTH; ++index) {
        provider->key[index] = 0U;
    }
}

bool uds_security_provider_is_locked(const UdsSecurityProvider *provider, uint32_t now_ms) {
    return (provider != NULL) && deadline_active(now_ms, provider->lockout_until_ms);
}

UdsSecurityResult uds_security_provider_generate_seed(UdsSecurityProvider *provider, uint8_t level,
                                                      uint8_t *seed, uint16_t *length,
                                                      uint16_t capacity, uint32_t now_ms) {
    if ((provider == NULL) || (seed == NULL) || (length == NULL) || (level == 0U)) {
        return UDS_SECURITY_INVALID_ARGUMENT;
    }
    if (uds_security_provider_is_locked(provider, now_ms)) {
        return UDS_SECURITY_DELAY_ACTIVE;
    }
    if (capacity < UDS_SECURITY_PROVIDER_SEED_LENGTH) {
        return UDS_SECURITY_BUFFER_TOO_SMALL;
    }
    provider->deterministic_state = next_state(provider->deterministic_state ^ level);
    for (uint8_t index = 0U; index < UDS_SECURITY_PROVIDER_SEED_LENGTH; ++index) {
        provider->deterministic_state = next_state(provider->deterministic_state);
        provider->seed[index] = (uint8_t)(provider->deterministic_state >> 24U);
        seed[index] = provider->seed[index];
    }
    /* Test-only key derivation: XOR the seed with a fixed test constant. This
     * is deliberately not a production security algorithm. */
    static const uint8_t test_mask[UDS_SECURITY_PROVIDER_SEED_LENGTH] = {0xA5U, 0x5AU, 0xC3U,
                                                                         0x3CU};
    for (uint8_t index = 0U; index < UDS_SECURITY_PROVIDER_SEED_LENGTH; ++index) {
        provider->key[index] = (uint8_t)(provider->seed[index] ^ test_mask[index]);
    }
    provider->key_length = UDS_SECURITY_PROVIDER_SEED_LENGTH;
    provider->seed_valid = true;
    *length = UDS_SECURITY_PROVIDER_SEED_LENGTH;
    return UDS_SECURITY_OK;
}

UdsSecurityResult uds_security_provider_verify_key(UdsSecurityProvider *provider, uint8_t level,
                                                   const uint8_t *key, uint16_t length,
                                                   uint32_t now_ms) {
    if ((provider == NULL) || (key == NULL) || (level == 0U)) {
        return UDS_SECURITY_INVALID_ARGUMENT;
    }
    if (uds_security_provider_is_locked(provider, now_ms)) {
        return UDS_SECURITY_DELAY_ACTIVE;
    }
    if (!provider->seed_valid || (length != provider->key_length) ||
        (length > UDS_SECURITY_PROVIDER_MAX_KEY_LENGTH)) {
        return UDS_SECURITY_SEQUENCE_ERROR;
    }
    bool valid = constant_time_equal(key, provider->key, provider->key_length) != 0U;
    if (!valid) {
        provider->security_level = 0U;
        provider->failed_attempts = (uint8_t)(provider->failed_attempts + 1U);
        provider->seed_valid = false;
        if (provider->failed_attempts >= provider->maximum_attempts) {
            provider->lockout_until_ms = now_ms + provider->lockout_ms;
            provider->failed_attempts = 0U;
            return UDS_SECURITY_ATTEMPTS_EXCEEDED;
        }
        return UDS_SECURITY_INVALID_KEY;
    }
    provider->security_level = level;
    provider->failed_attempts = 0U;
    provider->seed_valid = false;
    return UDS_SECURITY_OK;
}

void uds_security_provider_session_reset(UdsSecurityProvider *provider) {
    if (provider == NULL) {
        return;
    }
    provider->security_level = 0U;
    provider->failed_attempts = 0U;
    provider->seed_valid = false;
}

uint8_t uds_security_provider_security_level(const UdsSecurityProvider *provider) {
    return (provider != NULL) ? provider->security_level : 0U;
}

uint8_t uds_security_provider_failed_attempts(const UdsSecurityProvider *provider) {
    return (provider != NULL) ? provider->failed_attempts : 0U;
}
