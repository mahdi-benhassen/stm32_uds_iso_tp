#ifndef STM32_UDS_ISO_TP_AES_CMAC_H
#define STM32_UDS_ISO_TP_AES_CMAC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AES_CMAC_128_KEY_SIZE 16U
#define AES_CMAC_128_TAG_SIZE 16U

bool aes_cmac_128(const uint8_t key[AES_CMAC_128_KEY_SIZE], const uint8_t *message,
                  size_t message_length, uint8_t mac[AES_CMAC_128_TAG_SIZE]);

#endif
