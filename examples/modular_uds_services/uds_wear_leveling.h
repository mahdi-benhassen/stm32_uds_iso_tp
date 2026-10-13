/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */

#ifndef STM32_UDS_ISO_TP_WEAR_LEVELING_H
#define STM32_UDS_ISO_TP_WEAR_LEVELING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UDS_PARAM_OK 0
#define UDS_PARAM_ERR (-1)
#define UDS_PARAM_INVALID_PARAM (-4)

#define UDS_PARAM_MAGIC 0xC0DEu

/**
 * @brief Flash port operations provided by platform HAL.
 */
typedef struct {
    int (*erase)(uint32_t addr);
    int (*read)(uint32_t addr, void *buf, size_t len);
    int (*write)(uint32_t addr, const void *buf, size_t len);
    uint32_t (*sector_size)(uint32_t addr);
    uint8_t program_granule; /**< Write granularity (2, 4, 8, 16, 32, 64; 0 defaults to 2) */
    uint8_t erased_byte;     /**< Default byte value of erased flash (usually 0xFF) */
} UdsFlashPort;

/**
 * @brief 16-byte slot header aligned for embedded NOR / doubleword flash.
 */
typedef struct {
    uint16_t magic;     /**< Magic number (0xC0DE) */
    uint16_t seq;       /**< Monotonic sequence number */
    uint16_t data_size; /**< Payload data size */
    uint16_t crc;       /**< CRC-16 CCITT over payload */
    uint8_t _pad[8];    /**< Alignment padding to 16 bytes */
} UdsParamSlotHeader;

/**
 * @brief Wear-leveled persistent parameter store instance.
 */
typedef struct {
    const UdsFlashPort *port;
    uint32_t flash_base;
    uint32_t sector_size;
    uint8_t sector_count;
    uint16_t data_size;
    uint16_t slot_size;
    uint16_t slots_per_sector;
    uint16_t header_size;
    uint16_t payload_size_aligned;
    uint8_t program_granule;
    uint8_t erased_byte;

    uint8_t active_sector;
    uint16_t active_slot;
    uint16_t next_seq;
    bool initialized;
    bool has_active_slot;
} UdsParamStore;

uint16_t uds_crc16_ccitt(const void *data, size_t len);
uint16_t uds_crc16_ccitt_update(uint16_t crc, const void *data, size_t len);

int uds_param_init(UdsParamStore *store, const UdsFlashPort *port, uint32_t flash_base,
                   uint8_t sector_count, uint16_t data_size);

int uds_param_load(const UdsParamStore *store, void *data);

int uds_param_save(UdsParamStore *store, const void *data);

int uds_param_erase_all(UdsParamStore *store);

uint16_t uds_param_write_count(const UdsParamStore *store);

#ifdef __cplusplus
}
#endif

#endif /* STM32_UDS_ISO_TP_WEAR_LEVELING_H */
