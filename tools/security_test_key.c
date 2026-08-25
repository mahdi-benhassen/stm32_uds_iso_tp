/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_security_reference.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void usage(const char *program) {
    fprintf(stderr, "Usage: %s <1|5> <8-hex-digit-seed>\n", program);
    fprintf(stderr, "       seed bytes are parsed left-to-right as seed[0]..seed[3].\n");
    fprintf(stderr, "TEST/REFERENCE ONLY: not production ECU security.\n");
}

static bool parse_level(const char *text, uint8_t *level) {
    if ((text == NULL) || (level == NULL)) {
        return false;
    }
    if (strcmp(text, "1") == 0) {
        *level = UDS_SECURITY_REFERENCE_LEVEL_1;
        return true;
    }
    if (strcmp(text, "5") == 0) {
        *level = UDS_SECURITY_REFERENCE_LEVEL_5;
        return true;
    }
    return false;
}

static bool parse_hex_nibble(char value, uint8_t *nibble) {
    if (nibble == NULL) {
        return false;
    }
    if ((value >= '0') && (value <= '9')) {
        *nibble = (uint8_t)(value - '0');
        return true;
    }
    if ((value >= 'A') && (value <= 'F')) {
        *nibble = (uint8_t)(value - 'A' + 10);
        return true;
    }
    if ((value >= 'a') && (value <= 'f')) {
        *nibble = (uint8_t)(value - 'a' + 10);
        return true;
    }
    return false;
}

static bool parse_seed(const char *text, uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH]) {
    size_t offset = 0U;
    if ((text == NULL) || (seed == NULL)) {
        return false;
    }
    if ((strlen(text) >= 2U) && (text[0] == '0') && ((text[1] == 'x') || (text[1] == 'X'))) {
        offset = 2U;
    }
    if (strlen(text) - offset != (UDS_SECURITY_REFERENCE_SEED_LENGTH * 2U)) {
        return false;
    }
    for (size_t index = 0U; index < UDS_SECURITY_REFERENCE_SEED_LENGTH; ++index) {
        uint8_t high = 0U;
        uint8_t low = 0U;
        if (!parse_hex_nibble(text[offset + (index * 2U)], &high) ||
            !parse_hex_nibble(text[offset + (index * 2U) + 1U], &low)) {
            return false;
        }
        seed[index] = (uint8_t)((high << 4U) | low);
    }
    return true;
}

static void print_bytes(const char *label, const uint8_t *data, uint16_t length) {
    printf("%s", label);
    for (uint16_t index = 0U; index < length; ++index) {
        printf("%s%02X", (index == 0U) ? "" : " ", data[index]);
    }
    putchar('\n');
}

int main(int argc, char **argv) {
    uint8_t level = 0U;
    uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
    uint8_t key[UDS_SECURITY_REFERENCE_MAX_KEY_LENGTH] = {0U};
    uint16_t key_length = 0U;

    if ((argc != 3) || !parse_level(argv[1], &level) || !parse_seed(argv[2], seed) ||
        !uds_security_reference_calculate_key(level, seed, sizeof(seed), key, sizeof(key),
                                              &key_length)) {
        usage(argv[0]);
        return 2;
    }

    printf("level = %u\n", (unsigned int)level);
    print_bytes("seed = ", seed, (uint16_t)sizeof(seed));
    print_bytes("key  = ", key, key_length);
    puts("WARNING: TEST/REFERENCE ONLY; do not use as production security.");
    return 0;
}
