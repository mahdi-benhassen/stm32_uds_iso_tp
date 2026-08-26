#include "aes_cmac.h"

#include "aes.h"

#include <string.h>

static void secure_clear(void *data, size_t length) {
    volatile uint8_t *bytes = (volatile uint8_t *)data;
    while ((bytes != NULL) && (length-- != 0U))
        *bytes++ = 0U;
}

static void xor_block(uint8_t destination[16], const uint8_t source[16]) {
    for (uint8_t index = 0U; index < 16U; ++index)
        destination[index] ^= source[index];
}

static void left_shift_block(uint8_t output[16], const uint8_t input[16]) {
    uint8_t carry = 0U;
    for (int index = 15; index >= 0; --index) {
        uint8_t value = input[index];
        output[index] = (uint8_t)((value << 1U) | carry);
        carry = (uint8_t)((value >> 7U) & 1U);
    }
    if (carry != 0U)
        output[15] ^= 0x87U;
}

static void generate_subkey(const Aes128Context *aes, uint8_t output[16], bool second) {
    uint8_t zero[16] = {0U};
    uint8_t intermediate[16];
    aes128_encrypt_block(aes, zero, intermediate);
    left_shift_block(output, intermediate);
    if (second)
        left_shift_block(output, output);
    secure_clear(zero, sizeof(zero));
    secure_clear(intermediate, sizeof(intermediate));
}

bool aes_cmac_128(const uint8_t key[AES_CMAC_128_KEY_SIZE], const uint8_t *message,
                  size_t message_length, uint8_t mac[AES_CMAC_128_TAG_SIZE]) {
    if ((key == NULL) || (mac == NULL) || ((message == NULL) && (message_length != 0U)))
        return false;

    Aes128Context aes;
    uint8_t k1[16];
    uint8_t k2[16];
    uint8_t state[16] = {0U};
    uint8_t block[16] = {0U};
    uint8_t last[16] = {0U};
    aes128_init(&aes, key);
    generate_subkey(&aes, k1, false);
    generate_subkey(&aes, k2, true);

    size_t block_count = (message_length == 0U) ? 1U : ((message_length + 15U) / 16U);
    size_t complete_count = block_count - 1U;
    for (size_t index = 0U; index < complete_count; ++index) {
        (void)memcpy(block, &message[index * 16U], 16U);
        xor_block(block, state);
        aes128_encrypt_block(&aes, block, state);
    }

    size_t last_offset = complete_count * 16U;
    size_t remaining = (message_length > last_offset) ? message_length - last_offset : 0U;
    if ((message_length != 0U) && (remaining == 16U)) {
        (void)memcpy(last, &message[last_offset], 16U);
        xor_block(last, k1);
    } else {
        if (remaining != 0U)
            (void)memcpy(last, &message[last_offset], remaining);
        last[remaining] = 0x80U;
        xor_block(last, k2);
    }
    xor_block(last, state);
    aes128_encrypt_block(&aes, last, mac);
    secure_clear(&aes, sizeof(aes));
    secure_clear(k1, sizeof(k1));
    secure_clear(k2, sizeof(k2));
    secure_clear(state, sizeof(state));
    secure_clear(block, sizeof(block));
    secure_clear(last, sizeof(last));
    return true;
}
