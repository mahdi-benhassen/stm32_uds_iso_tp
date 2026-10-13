/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */

#include "uds_wear_leveling.h"

#include <string.h>

#define UDS_CRC16_CCITT_INIT 0xFFFFu
#define UDS_CRC16_CCITT_POLY 0x1021u

uint16_t uds_crc16_ccitt_update(uint16_t crc, const void *data, size_t len) {
    if (data == NULL) {
        return crc;
    }
    const uint8_t *ptr = (const uint8_t *)data;
    uint32_t val = (uint32_t)crc;
    for (size_t i = 0U; i < len; ++i) {
        val ^= ((uint32_t)ptr[i] << 8U);
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            if ((val & 0x8000U) != 0U) {
                val = ((val << 1U) ^ (uint32_t)UDS_CRC16_CCITT_POLY) & 0xFFFFU;
            } else {
                val = (val << 1U) & 0xFFFFU;
            }
        }
    }
    return (uint16_t)(val & 0xFFFFU);
}

uint16_t uds_crc16_ccitt(const void *data, size_t len) {
    return uds_crc16_ccitt_update(UDS_CRC16_CCITT_INIT, data, len);
}

static uint32_t slot_addr(const UdsParamStore *store, uint8_t sector, uint16_t slot) {
    return store->flash_base + ((uint32_t)sector * store->sector_size) +
           ((uint32_t)slot * store->slot_size);
}

static bool verify_slot_crc(const UdsParamStore *store, uint8_t sector, uint16_t slot,
                            uint16_t expected_crc) {
    uint32_t addr = slot_addr(store, sector, slot) + (uint32_t)store->header_size;
    uint16_t crc = UDS_CRC16_CCITT_INIT;
    uint16_t remaining = store->data_size;
    uint8_t chunk[32];

    while (remaining > 0U) {
        uint16_t chunk_len =
            (remaining > (uint16_t)sizeof(chunk)) ? (uint16_t)sizeof(chunk) : remaining;
        if (store->port->read(addr, chunk, chunk_len) != 0) {
            return false;
        }
        crc = uds_crc16_ccitt_update(crc, chunk, chunk_len);
        addr += chunk_len;
        remaining = (uint16_t)(remaining - chunk_len);
    }
    return (crc == expected_crc);
}

static bool read_slot_header(const UdsParamStore *store, uint8_t sector, uint16_t slot,
                             UdsParamSlotHeader *hdr) {
    uint32_t addr = slot_addr(store, sector, slot);
    if (store->port->read(addr, hdr, sizeof(UdsParamSlotHeader)) != 0) {
        return false;
    }
    if ((hdr->magic != UDS_PARAM_MAGIC) || (hdr->data_size != store->data_size)) {
        return false;
    }
    return verify_slot_crc(store, sector, slot, hdr->crc);
}

static void scan_for_active_slot(UdsParamStore *store) {
    uint16_t best_seq = 0U;
    uint8_t best_sec = 0U;
    uint16_t best_slot = 0U;
    bool found_valid = false;

    for (uint8_t sec = 0U; sec < store->sector_count; ++sec) {
        for (uint16_t sl = 0U; sl < store->slots_per_sector; ++sl) {
            UdsParamSlotHeader hdr;
            if (read_slot_header(store, sec, sl, &hdr)) {
                if (!found_valid || ((int16_t)(hdr.seq - best_seq) > 0)) {
                    best_seq = hdr.seq;
                    best_sec = sec;
                    best_slot = sl;
                    found_valid = true;
                }
            }
        }
    }

    if (found_valid) {
        store->active_sector = best_sec;
        store->active_slot = best_slot;
        store->next_seq = (uint16_t)(best_seq + 1U);
        if (store->next_seq == 0U) {
            store->next_seq = 1U;
        }
        store->has_active_slot = true;
    }
}

static bool validate_init_args(const UdsParamStore *store, const UdsFlashPort *port,
                               uint8_t sector_count, uint16_t data_size) {
    return (store != NULL) && (port != NULL) && (port->erase != NULL) && (port->read != NULL) &&
           (port->write != NULL) && (port->sector_size != NULL) && (sector_count != 0U) &&
           (data_size != 0U);
}

int uds_param_init(UdsParamStore *store, const UdsFlashPort *port, uint32_t flash_base,
                   uint8_t sector_count, uint16_t data_size) {
    if (!validate_init_args(store, port, sector_count, data_size)) {
        return UDS_PARAM_INVALID_PARAM;
    }

    uint32_t sec_size = port->sector_size(flash_base);
    if (sec_size == 0U) {
        return UDS_PARAM_INVALID_PARAM;
    }

    uint8_t granule = port->program_granule;
    if (granule == 0U) {
        granule = 2U;
    }
    /* Ensure power of 2 */
    if ((granule & (uint8_t)(granule - 1U)) != 0U) {
        return UDS_PARAM_INVALID_PARAM;
    }

    uint8_t erased_byte = (port->erased_byte != 0U) ? port->erased_byte : 0xFFU;

    uint16_t min_hdr = (sizeof(UdsParamSlotHeader) > (size_t)granule)
                           ? (uint16_t)sizeof(UdsParamSlotHeader)
                           : (uint16_t)granule;
    uint16_t header_size =
        (uint16_t)((((uint32_t)min_hdr + (uint32_t)granule - 1U) / (uint32_t)granule) *
                   (uint32_t)granule);
    uint16_t payload_aligned =
        (uint16_t)((((uint32_t)data_size + (uint32_t)granule - 1U) / (uint32_t)granule) *
                   (uint32_t)granule);
    uint16_t slot_size = (uint16_t)(header_size + payload_aligned);

    /* Keep slot_size aligned to at least 16 for backwards compatibility */
    if ((slot_size % 16U != 0U) && (granule < 16U)) {
        slot_size = (uint16_t)(((slot_size + 15U) / 16U) * 16U);
    }

    if ((uint32_t)slot_size > sec_size) {
        return UDS_PARAM_INVALID_PARAM;
    }

    store->port = port;
    store->flash_base = flash_base;
    store->sector_size = sec_size;
    store->sector_count = sector_count;
    store->data_size = data_size;
    store->slot_size = slot_size;
    store->header_size = header_size;
    store->payload_size_aligned = payload_aligned;
    store->program_granule = granule;
    store->erased_byte = erased_byte;
    store->slots_per_sector = (uint16_t)(sec_size / slot_size);
    store->active_sector = 0U;
    store->active_slot = 0U;
    store->next_seq = 1U;
    store->initialized = false;
    store->has_active_slot = false;

    scan_for_active_slot(store);

    store->initialized = true;
    return UDS_PARAM_OK;
}

int uds_param_load(const UdsParamStore *store, void *data) {
    if ((store == NULL) || (!store->initialized) || (data == NULL)) {
        return UDS_PARAM_INVALID_PARAM;
    }
    if (!store->has_active_slot) {
        return UDS_PARAM_ERR;
    }

    uint32_t addr =
        slot_addr(store, store->active_sector, store->active_slot) + (uint32_t)store->header_size;
    if (store->port->read(addr, data, store->data_size) != 0) {
        return UDS_PARAM_ERR;
    }

    uint16_t crc = uds_crc16_ccitt(data, store->data_size);
    UdsParamSlotHeader hdr;
    uint32_t hdr_addr = slot_addr(store, store->active_sector, store->active_slot);
    if (store->port->read(hdr_addr, &hdr, sizeof(hdr)) != 0) {
        return UDS_PARAM_ERR;
    }
    if (hdr.crc != crc) {
        return UDS_PARAM_ERR;
    }

    return UDS_PARAM_OK;
}

static int write_padded_chunked(const UdsParamStore *store, uint32_t addr, const uint8_t *src,
                                uint16_t src_size, uint16_t total_size) {
    uint8_t chunk[64] = {0};
    uint16_t written = 0U;
    while (written < total_size) {
        uint16_t to_write = (uint16_t)(total_size - written);
        if (to_write > (uint16_t)sizeof(chunk)) {
            to_write = (uint16_t)sizeof(chunk);
        }
        memset(chunk, store->erased_byte, to_write);
        if (written < src_size) {
            uint16_t copy_len = (uint16_t)(src_size - written);
            if (copy_len > to_write) {
                copy_len = to_write;
            }
            memcpy(chunk, &src[written], copy_len);
        }
        if (store->port->write(addr + written, chunk, to_write) != 0) {
            return UDS_PARAM_ERR;
        }
        written = (uint16_t)(written + to_write);
    }
    return UDS_PARAM_OK;
}

static int prepare_next_slot(const UdsParamStore *store, uint8_t *out_sec, uint16_t *out_sl) {
    uint8_t sec = 0U;
    uint16_t sl = 0U;
    if (store->has_active_slot) {
        sl = (uint16_t)(store->active_slot + 1U);
        sec = store->active_sector;
        if (sl >= store->slots_per_sector) {
            sec = (uint8_t)((sec + 1U) % store->sector_count);
            sl = 0U;
            if (store->port->erase(store->flash_base + ((uint32_t)sec * store->sector_size)) != 0) {
                return UDS_PARAM_ERR;
            }
        }
    } else {
        if (store->port->erase(store->flash_base) != 0) {
            return UDS_PARAM_ERR;
        }
    }
    *out_sec = sec;
    *out_sl = sl;
    return UDS_PARAM_OK;
}

int uds_param_save(UdsParamStore *store, const void *data) {
    if ((store == NULL) || (!store->initialized) || (data == NULL)) {
        return UDS_PARAM_INVALID_PARAM;
    }

    uint8_t sec = 0U;
    uint16_t sl = 0U;
    if (prepare_next_slot(store, &sec, &sl) != UDS_PARAM_OK) {
        return UDS_PARAM_ERR;
    }

    uint32_t addr = slot_addr(store, sec, sl);

    /* 1. Write payload data first, aligned to program_granule */
    if (write_padded_chunked(store, addr + store->header_size, (const uint8_t *)data,
                             store->data_size, store->payload_size_aligned) != UDS_PARAM_OK) {
        return UDS_PARAM_ERR;
    }

    /* 2. Write header with magic number last (atomic two-phase commit) */
    UdsParamSlotHeader hdr;
    memset(&hdr, store->erased_byte, sizeof(hdr));
    hdr.magic = UDS_PARAM_MAGIC;
    hdr.seq = store->next_seq;
    hdr.data_size = store->data_size;
    hdr.crc = uds_crc16_ccitt(data, store->data_size);

    if (write_padded_chunked(store, addr, (const uint8_t *)&hdr, (uint16_t)sizeof(hdr),
                             store->header_size) != UDS_PARAM_OK) {
        return UDS_PARAM_ERR;
    }

    store->active_sector = sec;
    store->active_slot = sl;
    store->next_seq = (uint16_t)(store->next_seq + 1U);
    if (store->next_seq == 0U) {
        store->next_seq = 1U;
    }
    store->has_active_slot = true;

    return UDS_PARAM_OK;
}

int uds_param_erase_all(UdsParamStore *store) {
    if ((store == NULL) || (!store->initialized)) {
        return UDS_PARAM_INVALID_PARAM;
    }

    for (uint8_t sec = 0U; sec < store->sector_count; ++sec) {
        uint32_t addr = store->flash_base + ((uint32_t)sec * store->sector_size);
        if (store->port->erase(addr) != 0) {
            return UDS_PARAM_ERR;
        }
    }

    store->active_sector = 0U;
    store->active_slot = 0U;
    store->next_seq = 1U;
    store->has_active_slot = false;

    return UDS_PARAM_OK;
}

uint16_t uds_param_write_count(const UdsParamStore *store) {
    if ((store == NULL) || (!store->has_active_slot) || (store->next_seq == 0U)) {
        return 0U;
    }
    return (uint16_t)(store->next_seq - 1U);
}
