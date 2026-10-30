/**
 * @file app_metadata.h
 * @brief Application Metadata Structure for Atomic Anti-Bricking Verification (Issue #140)
 */

#ifndef STM32F103_APP_METADATA_H
#define STM32F103_APP_METADATA_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t app_size;        /**< Size of programmed application binary in bytes */
    uint32_t crc32;           /**< IEEE 802.3 CRC32 calculated over application flash */
    uint32_t entry_point;     /**< Application Reset_Handler address */
    uint32_t version;         /**< Application firmware version (e.g. 0x01000000 = 1.0.0) */
    uint32_t reserved;        /**< Reserved alignment word */
    uint32_t magic;           /**< Written LAST: APP_METADATA_MAGIC (0x55445331) */
} app_metadata_t;

#endif /* STM32F103_APP_METADATA_H */
