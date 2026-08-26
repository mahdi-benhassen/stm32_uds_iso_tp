#include "uds_security_cmac.h"

#include "aes_cmac.h"

bool uds_security_cmac_derive_key_for_seed(const uint8_t master_key[16], const uint8_t *seed,
                                           uint16_t seed_length, uint8_t derived_key[16]) {
    if ((seed_length > UDS_SECURITY_CMAC_MAX_SEED_LENGTH) || (derived_key == NULL))
        return false;
    return aes_cmac_128(master_key, seed, seed_length, derived_key);
}

bool uds_security_cmac_derive_key(const uint8_t master_key[16], const uint8_t seed[16],
                                  uint8_t derived_key[16]) {
    return uds_security_cmac_derive_key_for_seed(master_key, seed, 16U, derived_key);
}

bool uds_security_cmac_constant_time_equal(const uint8_t left[16], const uint8_t right[16]) {
    if ((left == NULL) || (right == NULL))
        return false;
    uint8_t difference = 0U;
    for (uint8_t index = 0U; index < 16U; ++index)
        difference |= (uint8_t)(left[index] ^ right[index]);
    return difference == 0U;
}
