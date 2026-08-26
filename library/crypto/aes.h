#ifndef STM32_UDS_ISO_TP_AES_H
#define STM32_UDS_ISO_TP_AES_H

#include <stdint.h>

typedef struct {
    uint8_t round_keys[176];
} Aes128Context;

void aes128_init(Aes128Context *context, const uint8_t key[16]);
void aes128_encrypt_block(const Aes128Context *context, const uint8_t input[16],
                          uint8_t output[16]);

#endif
