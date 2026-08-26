#include "aes_cmac.h"
#include "uds_security_cmac.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static const uint8_t key[16] = {0x2BU, 0x7EU, 0x15U, 0x16U, 0x28U, 0xAEU, 0xD2U, 0xA6U,
                                0xABU, 0xF7U, 0x15U, 0x88U, 0x09U, 0xCFU, 0x4FU, 0x3CU};

static void check_tag(const uint8_t *message, size_t length, const uint8_t expected[16]) {
    uint8_t actual[16];
    assert(aes_cmac_128(key, message, length, actual));
    assert(memcmp(actual, expected, sizeof(actual)) == 0);
}

int main(void) {
    static const uint8_t empty_tag[16] = {0xBBU, 0x1DU, 0x69U, 0x29U, 0xE9U, 0x59U, 0x37U, 0x28U,
                                          0x7FU, 0xA3U, 0x7DU, 0x12U, 0x9BU, 0x75U, 0x67U, 0x46U};
    static const uint8_t tag_16[16] = {0x07U, 0x0AU, 0x16U, 0xB4U, 0x6BU, 0x4DU, 0x41U, 0x44U,
                                       0xF7U, 0x9BU, 0xDDU, 0x9DU, 0xD0U, 0x4AU, 0x28U, 0x7CU};
    static const uint8_t tag_40[16] = {0xDFU, 0xA6U, 0x67U, 0x47U, 0xDEU, 0x9AU, 0xE6U, 0x30U,
                                       0x30U, 0xCAU, 0x32U, 0x61U, 0x14U, 0x97U, 0xC8U, 0x27U};
    static const uint8_t tag_64[16] = {0x51U, 0xF0U, 0xBEU, 0xBFU, 0x7EU, 0x3BU, 0x9DU, 0x92U,
                                       0xFCU, 0x49U, 0x74U, 0x17U, 0x79U, 0x36U, 0x3CU, 0xFEU};
    static const uint8_t message[64] = {
        0x6BU, 0xC1U, 0xBEU, 0xE2U, 0x2EU, 0x40U, 0x9FU, 0x96U, 0xE9U, 0x3DU, 0x7EU, 0x11U, 0x73U,
        0x93U, 0x17U, 0x2AU, 0xAEU, 0x2DU, 0x8AU, 0x57U, 0x1EU, 0x03U, 0xACU, 0x9CU, 0x9EU, 0xB7U,
        0x6FU, 0xACU, 0x45U, 0xAFU, 0x8EU, 0x51U, 0x30U, 0xC8U, 0x1CU, 0x46U, 0xA3U, 0x5CU, 0xE4U,
        0x11U, 0xE5U, 0xFBU, 0xC1U, 0x19U, 0x1AU, 0x0AU, 0x52U, 0xEFU, 0xF6U, 0x9FU, 0x24U, 0x45U,
        0xDFU, 0x4FU, 0x9BU, 0x17U, 0xADU, 0x2BU, 0x41U, 0x7BU, 0xE6U, 0x6CU, 0x37U, 0x10U};
    static const uint8_t seed[16] = {0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U, 0x77U,
                                     0x88U, 0x99U, 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU};
    uint8_t seed_mac[16];
    static const uint8_t seed_expected[16] = {0x10U, 0xFEU, 0xD7U, 0x80U, 0x5BU, 0x23U,
                                              0x63U, 0x3FU, 0x6CU, 0x22U, 0xDEU, 0xC5U,
                                              0xA3U, 0x8EU, 0x07U, 0x0BU};

    check_tag(NULL, 0U, empty_tag);
    check_tag(message, 16U, tag_16);
    check_tag(message, 40U, tag_40);
    check_tag(message, 64U, tag_64);
    assert(aes_cmac_128(key, seed, sizeof(seed), seed_mac));
    assert(memcmp(seed_mac, seed_expected, sizeof(seed_mac)) == 0);
    assert(uds_security_cmac_derive_key(key, seed, seed_mac));
    assert(memcmp(seed_mac, seed_expected, sizeof(seed_mac)) == 0);

    static const uint8_t arbitrary_seed[7] = {0x01U, 0xA2U, 0x03U, 0xB4U, 0x05U, 0xC6U, 0x07U};
    uint8_t arbitrary_expected[16];
    uint8_t arbitrary_actual[16];
    assert(aes_cmac_128(key, arbitrary_seed, sizeof(arbitrary_seed), arbitrary_expected));
    assert(uds_security_cmac_derive_key_for_seed(key, arbitrary_seed, sizeof(arbitrary_seed),
                                                 arbitrary_actual));
    assert(memcmp(arbitrary_actual, arbitrary_expected, sizeof(arbitrary_actual)) == 0);

    static const uint8_t zero_key[16] = {0U};
    static const uint8_t zero_seed[16] = {0U};
    assert(aes_cmac_128(zero_key, zero_seed, sizeof(zero_seed), arbitrary_expected));
    assert(uds_security_cmac_derive_key_for_seed(zero_key, zero_seed, sizeof(zero_seed),
                                                 arbitrary_actual));
    assert(memcmp(arbitrary_actual, arbitrary_expected, sizeof(arbitrary_actual)) == 0);

    static uint8_t maximum_seed[UDS_SECURITY_CMAC_MAX_SEED_LENGTH];
    for (uint16_t index = 0U; index < sizeof(maximum_seed); ++index)
        maximum_seed[index] = (uint8_t)(index * 13U + 7U);
    assert(aes_cmac_128(key, maximum_seed, sizeof(maximum_seed), arbitrary_expected));
    assert(uds_security_cmac_derive_key_for_seed(key, maximum_seed, sizeof(maximum_seed),
                                                 arbitrary_actual));
    assert(memcmp(arbitrary_actual, arbitrary_expected, sizeof(arbitrary_actual)) == 0);
    assert(!uds_security_cmac_derive_key_for_seed(
        key, maximum_seed, (uint16_t)(UDS_SECURITY_CMAC_MAX_SEED_LENGTH + 1U), arbitrary_actual));
    assert(uds_security_cmac_constant_time_equal(seed_mac, seed_expected));
    seed_mac[0] ^= 0x01U;
    assert(!uds_security_cmac_constant_time_equal(seed_mac, seed_expected));
    assert(uds_security_cmac_constant_time_equal(NULL, seed_expected) == false);
    assert(aes_cmac_128(NULL, seed, sizeof(seed), seed_mac) == false);
    assert(aes_cmac_128(key, NULL, 1U, seed_mac) == false);
    assert(uds_security_cmac_derive_key_for_seed(key, NULL, 1U, seed_mac) == false);
    return 0;
}
