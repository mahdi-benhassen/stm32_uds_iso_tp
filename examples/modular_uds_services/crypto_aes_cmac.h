/**
 * @file crypto_aes_cmac.h
 * @brief AES-128 and AES-CMAC-128 Message Authentication Code (NIST SP 800-38B, FIPS 197)
 */

#ifndef CRYPTO_AES_CMAC_H
#define CRYPTO_AES_CMAC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AES_CMAC_KEY_SIZE 16U
#define AES_CMAC_TAG_SIZE 16U

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generate AES-CMAC-128 authentication tag
 * @param key 16-byte cryptographic key
 * @param message Message buffer to authenticate
 * @param message_length Length of message in bytes
 * @param mac 16-byte output buffer for authentication tag
 * @return True on success, false on invalid parameter
 */
bool crypto_aes_cmac_128(const uint8_t key[AES_CMAC_KEY_SIZE], const uint8_t *message,
                         size_t message_length, uint8_t mac[AES_CMAC_TAG_SIZE]);

/**
 * @brief Constant-time byte array comparison to protect against timing side-channel attacks
 * @param left First buffer
 * @param right Second buffer
 * @param length Number of bytes to compare
 * @return True if identical, false otherwise
 */
bool crypto_constant_time_equal(const uint8_t *left, const uint8_t *right, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_AES_CMAC_H */
