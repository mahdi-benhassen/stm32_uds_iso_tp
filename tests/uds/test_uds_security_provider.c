/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_security_provider.h"

#include <assert.h>
#include <stdint.h>

int main(void) {
    UdsSecurityProvider provider;
    uds_security_provider_init(&provider, 0x12345678UL, 3U, 100U);
    uint8_t seed[UDS_SECURITY_PROVIDER_SEED_LENGTH];
    uint16_t seed_length = 0U;
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               0U) == UDS_SECURITY_OK);
    assert(seed_length == 4U);
    uint8_t key[] = {0U, 0U, 0U, 0U};
    key[0] = (uint8_t)(seed[0] ^ 0xA5U);
    key[1] = (uint8_t)(seed[1] ^ 0x5AU);
    key[2] = (uint8_t)(seed[2] ^ 0xC3U);
    key[3] = (uint8_t)(seed[3] ^ 0x3CU);
    assert(uds_security_provider_verify_key(&provider, 1U, key, sizeof(key), 1U) ==
           UDS_SECURITY_OK);
    assert(uds_security_provider_security_level(&provider) == 1U);

    for (uint8_t attempt = 0U; attempt < 2U; ++attempt) {
        assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                   2U) == UDS_SECURITY_OK);
        key[0] ^= 0x01U;
        assert(uds_security_provider_verify_key(&provider, 1U, key, sizeof(key),
                                                (uint32_t)(3U + attempt)) ==
               UDS_SECURITY_INVALID_KEY);
        key[0] ^= 0x01U;
    }
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               5U) == UDS_SECURITY_OK);
    key[0] ^= 0x01U;
    assert(uds_security_provider_verify_key(&provider, 1U, key, sizeof(key), 6U) ==
           UDS_SECURITY_ATTEMPTS_EXCEEDED);
    assert(uds_security_provider_is_locked(&provider, 50U));
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               50U) == UDS_SECURITY_DELAY_ACTIVE);
    assert(!uds_security_provider_is_locked(&provider, 106U));
    uds_security_provider_session_reset(&provider);
    assert(uds_security_provider_security_level(&provider) == 0U);
    return 0;
}
