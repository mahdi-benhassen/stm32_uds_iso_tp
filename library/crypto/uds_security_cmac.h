#ifndef STM32_UDS_ISO_TP_UDS_SECURITY_CMAC_H
#define STM32_UDS_ISO_TP_UDS_SECURITY_CMAC_H

#include <stdbool.h>
#include <stdint.h>

bool uds_security_cmac_derive_key(const uint8_t master_key[16], const uint8_t seed[16],
                                  uint8_t derived_key[16]);
bool uds_security_cmac_constant_time_equal(const uint8_t left[16], const uint8_t right[16]);

#endif
