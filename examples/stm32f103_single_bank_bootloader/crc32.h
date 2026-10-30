/**
 * @file crc32.h
 * @brief IEEE 802.3 CRC32 Calculation Engine
 */

#ifndef BOOTLOADER_CRC32_H
#define BOOTLOADER_CRC32_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t bootloader_crc32(const uint8_t *data, size_t length);
uint32_t bootloader_crc32_update(uint32_t current_crc, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* BOOTLOADER_CRC32_H */
