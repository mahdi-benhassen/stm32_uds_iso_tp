/**
 * @file crypto_aes_cmac.c
 * @brief AES-128 and AES-CMAC-128 Implementation (NIST SP 800-38B, FIPS 197)
 */

#include "crypto_aes_cmac.h"
#include <string.h>

static const uint8_t sbox[256] = {
    0x63U, 0x7cU, 0x77U, 0x7bU, 0xf2U, 0x6bU, 0x6fU, 0xc5U, 0x30U, 0x01U, 0x67U, 0x2bU, 0xfeU,
    0xd7U, 0xabU, 0x76U, 0xcaU, 0x82U, 0xc9U, 0x7dU, 0xfaU, 0x59U, 0x47U, 0xf0U, 0xadU, 0xd4U,
    0xa2U, 0xafU, 0x9cU, 0xa4U, 0x72U, 0xc0U, 0xb7U, 0xfdU, 0x93U, 0x26U, 0x36U, 0x3fU, 0xf7U,
    0xccU, 0x34U, 0xa5U, 0xe5U, 0xf1U, 0x71U, 0xd8U, 0x31U, 0x15U, 0x04U, 0xc7U, 0x23U, 0xc3U,
    0x18U, 0x96U, 0x05U, 0x9aU, 0x07U, 0x12U, 0x80U, 0xe2U, 0xebU, 0x27U, 0xb2U, 0x75U, 0x09U,
    0x83U, 0x2cU, 0x1aU, 0x1bU, 0x6eU, 0x5aU, 0xa0U, 0x52U, 0x3bU, 0xd6U, 0xb3U, 0x29U, 0xe3U,
    0x2fU, 0x84U, 0x53U, 0xd1U, 0x00U, 0xedU, 0x20U, 0xfcU, 0xb1U, 0x5bU, 0x6aU, 0xcbU, 0xbeU,
    0x39U, 0x4aU, 0x4cU, 0x58U, 0xcfU, 0xd0U, 0xefU, 0xaaU, 0xfbU, 0x43U, 0x4dU, 0x33U, 0x85U,
    0x45U, 0xf9U, 0x02U, 0x7fU, 0x50U, 0x3cU, 0x9fU, 0xa8U, 0x51U, 0xa3U, 0x40U, 0x8fU, 0x92U,
    0x9dU, 0x38U, 0xf5U, 0xbcU, 0xb6U, 0xdaU, 0x21U, 0x10U, 0xffU, 0xf3U, 0xd2U, 0xcdU, 0x0cU,
    0x13U, 0xecU, 0x5fU, 0x97U, 0x44U, 0x17U, 0xc4U, 0xa7U, 0x7eU, 0x3dU, 0x64U, 0x5dU, 0x19U,
    0x73U, 0x60U, 0x81U, 0x4fU, 0xdcU, 0x22U, 0x2aU, 0x90U, 0x88U, 0x46U, 0xeeU, 0xb8U, 0x14U,
    0xdeU, 0x5eU, 0x0bU, 0xdbU, 0xe0U, 0x32U, 0x3aU, 0x0aU, 0x49U, 0x06U, 0x24U, 0x5cU, 0xc2U,
    0xd3U, 0xacU, 0x62U, 0x91U, 0x95U, 0xe4U, 0x79U, 0xe7U, 0xc8U, 0x37U, 0x6dU, 0x8dU, 0xd5U,
    0x4eU, 0xa9U, 0x6cU, 0x56U, 0xf4U, 0xeaU, 0x65U, 0x7aU, 0xaeU, 0x08U, 0xbaU, 0x78U, 0x25U,
    0x2eU, 0x1cU, 0xa6U, 0xb4U, 0xc6U, 0xe8U, 0xddU, 0x74U, 0x1fU, 0x4bU, 0xbdU, 0x8bU, 0x8aU,
    0x70U, 0x3eU, 0xb5U, 0x66U, 0x48U, 0x03U, 0xf6U, 0x0eU, 0x61U, 0x35U, 0x57U, 0xb9U, 0x86U,
    0xc1U, 0x1dU, 0x9eU, 0xe1U, 0xf8U, 0x98U, 0x11U, 0x69U, 0xd9U, 0x8eU, 0x94U, 0x9bU, 0x1eU,
    0x87U, 0xe9U, 0xceU, 0x55U, 0x28U, 0xdfU, 0x8cU, 0xa1U, 0x89U, 0x0dU, 0xbfU, 0xe6U, 0x42U,
    0x68U, 0x41U, 0x99U, 0x2dU, 0x0fU, 0xb0U, 0x54U, 0xbbU, 0x16U
};

typedef struct {
    uint8_t round_keys[176];
} aes128_ctx_t;

static uint8_t xtime(uint8_t value) {
    uint32_t wide = (uint32_t)value;
    return (uint8_t)((wide << 1U) ^ (((wide >> 7U) & 1U) * 0x1BU));
}

static void add_round_key(uint8_t state[16], const uint8_t *round_key) {
    for (uint8_t i = 0U; i < 16U; ++i) {
        state[i] ^= round_key[i];
    }
}

static void sub_bytes(uint8_t state[16]) {
    for (uint8_t i = 0U; i < 16U; ++i) {
        state[i] = sbox[state[i]];
    }
}

static void shift_rows(uint8_t state[16]) {
    uint8_t copy[16];
    memcpy(copy, state, 16U);
    state[0]  = copy[0];  state[1]  = copy[5];  state[2]  = copy[10]; state[3]  = copy[15];
    state[4]  = copy[4];  state[5]  = copy[9];  state[6]  = copy[14]; state[7]  = copy[3];
    state[8]  = copy[8];  state[9]  = copy[13]; state[10] = copy[2];  state[11] = copy[7];
    state[12] = copy[12]; state[13] = copy[1];  state[14] = copy[6];  state[15] = copy[11];
}

static void mix_columns(uint8_t state[16]) {
    for (uint8_t col = 0U; col < 4U; ++col) {
        uint8_t *v = &state[(size_t)col * 4U];
        uint8_t sum = (uint8_t)(v[0] ^ v[1] ^ v[2] ^ v[3]);
        uint8_t first = v[0];
        v[0] = (uint8_t)(v[0] ^ (uint8_t)(sum ^ xtime((uint8_t)(v[0] ^ v[1]))));
        v[1] = (uint8_t)(v[1] ^ (uint8_t)(sum ^ xtime((uint8_t)(v[1] ^ v[2]))));
        v[2] = (uint8_t)(v[2] ^ (uint8_t)(sum ^ xtime((uint8_t)(v[2] ^ v[3]))));
        v[3] = (uint8_t)(v[3] ^ (uint8_t)(sum ^ xtime((uint8_t)(v[3] ^ first))));
    }
}

static uint8_t rcon(uint8_t round) {
    uint8_t val = 1U;
    for (uint8_t i = 1U; i < round; ++i) {
        val = xtime(val);
    }
    return val;
}

static void aes128_init(aes128_ctx_t *ctx, const uint8_t key[16]) {
    for (uint8_t i = 0U; i < 16U; ++i) {
        ctx->round_keys[i] = key[i];
    }
    for (uint8_t round = 1U; round <= 10U; ++round) {
        uint8_t offset = (uint8_t)(round * 16U);
        uint8_t prev   = (uint8_t)((round - 1U) * 16U);
        uint8_t word[4];
        word[0] = sbox[ctx->round_keys[prev + 13U]] ^ rcon(round);
        word[1] = sbox[ctx->round_keys[prev + 14U]];
        word[2] = sbox[ctx->round_keys[prev + 15U]];
        word[3] = sbox[ctx->round_keys[prev + 12U]];

        for (uint8_t i = 0U; i < 4U; ++i) {
            ctx->round_keys[offset + i] = (uint8_t)(ctx->round_keys[prev + i] ^ word[i]);
        }
        for (uint8_t i = 4U; i < 16U; ++i) {
            ctx->round_keys[offset + i] = (uint8_t)(ctx->round_keys[prev + i] ^ ctx->round_keys[offset + i - 4U]);
        }
    }
}

static void aes128_encrypt_block(const aes128_ctx_t *ctx, const uint8_t in[16], uint8_t out[16]) {
    memcpy(out, in, 16U);
    add_round_key(out, &ctx->round_keys[0]);
    for (uint8_t round = 1U; round < 10U; ++round) {
        sub_bytes(out);
        shift_rows(out);
        mix_columns(out);
        add_round_key(out, &ctx->round_keys[(size_t)round * 16U]);
    }
    sub_bytes(out);
    shift_rows(out);
    add_round_key(out, &ctx->round_keys[160]);
}

static void xor_block(uint8_t dst[16], const uint8_t src[16]) {
    for (uint8_t i = 0U; i < 16U; ++i) {
        dst[i] ^= src[i];
    }
}

static void left_shift_block(uint8_t out[16], const uint8_t in[16]) {
    uint8_t carry = 0U;
    for (int32_t i = 15; i >= 0; --i) {
        uint8_t val = in[i];
        out[i] = (uint8_t)((val << 1U) | carry);
        carry = (uint8_t)((val >> 7U) & 1U);
    }
    if (carry != 0U) {
        out[15] ^= 0x87U;
    }
}

static void generate_subkey(const aes128_ctx_t *ctx, uint8_t out[16], bool second) {
    uint8_t zero[16];
    uint8_t intermediate[16];
    memset(zero, 0, sizeof(zero));
    aes128_encrypt_block(ctx, zero, intermediate);
    left_shift_block(out, intermediate);
    if (second) {
        left_shift_block(out, out);
    }
}

bool crypto_aes_cmac_128(const uint8_t key[AES_CMAC_KEY_SIZE], const uint8_t *message,
                         size_t message_length, uint8_t mac[AES_CMAC_TAG_SIZE]) {
    if ((key == NULL) || (mac == NULL) || ((message == NULL) && (message_length != 0U))) {
        return false;
    }

    aes128_ctx_t aes;
    uint8_t k1[16];
    uint8_t k2[16];
    uint8_t state[16];
    uint8_t block[16];
    uint8_t last[16];

    memset(state, 0, sizeof(state));
    memset(block, 0, sizeof(block));
    memset(last, 0, sizeof(last));

    aes128_init(&aes, key);
    generate_subkey(&aes, k1, false);
    generate_subkey(&aes, k2, true);

    size_t block_count = (message_length == 0U) ? 1U : ((message_length + 15U) / 16U);
    size_t complete_count = block_count - 1U;

    for (size_t i = 0U; i < complete_count; ++i) {
        memcpy(block, &message[i * 16U], 16U);
        xor_block(block, state);
        aes128_encrypt_block(&aes, block, state);
    }

    size_t last_offset = complete_count * 16U;
    size_t remaining = (message_length > last_offset) ? (message_length - last_offset) : 0U;

    if ((message_length != 0U) && (remaining == 16U)) {
        memcpy(last, &message[last_offset], 16U);
        xor_block(last, k1);
    } else {
        if (remaining != 0U) {
            memcpy(last, &message[last_offset], remaining);
        }
        last[remaining] = 0x80U;
        xor_block(last, k2);
    }

    xor_block(last, state);
    aes128_encrypt_block(&aes, last, mac);

    /* Zero sensitive buffers */
    memset(&aes, 0, sizeof(aes));
    memset(k1, 0, sizeof(k1));
    memset(k2, 0, sizeof(k2));
    memset(state, 0, sizeof(state));
    memset(block, 0, sizeof(block));
    memset(last, 0, sizeof(last));

    return true;
}

bool crypto_constant_time_equal(const uint8_t *left, const uint8_t *right, size_t length) {
    if ((left == NULL) || (right == NULL)) {
        return false;
    }
    uint8_t diff = 0U;
    for (size_t i = 0U; i < length; ++i) {
        diff |= (uint8_t)(left[i] ^ right[i]);
    }
    return (diff == 0U);
}
