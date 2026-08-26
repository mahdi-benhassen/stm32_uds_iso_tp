#ifndef STM32_UDS_ISO_TP_UDS_SECURITY_CMAC_H
#define STM32_UDS_ISO_TP_UDS_SECURITY_CMAC_H

#include <stdbool.h>
#include <stdint.h>

#define UDS_SECURITY_CMAC_MAX_SEED_LENGTH 4095U

bool uds_security_cmac_derive_key_for_seed(const uint8_t master_key[16], const uint8_t *seed,
                                           uint16_t seed_length, uint8_t derived_key[16]);
bool uds_security_cmac_derive_key(const uint8_t master_key[16], const uint8_t seed[16],
                                  uint8_t derived_key[16]);
bool uds_security_cmac_constant_time_equal(const uint8_t left[16], const uint8_t right[16]);

#endif
