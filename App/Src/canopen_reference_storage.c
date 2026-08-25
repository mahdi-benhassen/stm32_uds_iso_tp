/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_storage.h"

#include "canopen_reference_config.h"

#include <string.h>

#include "canopen_reference_od.h"
#include "storage/CO_storage.h"

#if defined(STM32F767xx)
#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_flash_ex.h"
#endif

#define CANOPEN_REFERENCE_STORAGE_MAGIC       0x434F5354UL
#define CANOPEN_REFERENCE_STORAGE_VERSION     1UL
#define CANOPEN_REFERENCE_STORAGE_SLOT_A      0x08180000UL
#define CANOPEN_REFERENCE_STORAGE_SLOT_B      0x081C0000UL
#define CANOPEN_REFERENCE_STORAGE_SECTOR_A    FLASH_SECTOR_10
#define CANOPEN_REFERENCE_STORAGE_SECTOR_B    FLASH_SECTOR_11

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t sequence;
    uint32_t length;
    uint32_t crc32;
    OD_PERSIST_COMM_t payload;
} canopen_reference_storage_image_t;

_Static_assert((sizeof(canopen_reference_storage_image_t) % sizeof(uint32_t)) == 0U,
               "storage image must be word-aligned for STM32F7 Flash programming");
_Static_assert(sizeof(canopen_reference_storage_image_t) <= CANOPEN_REFERENCE_STORAGE_SLOT_SIZE,
               "storage image must fit inside one reserved Flash sector");

static canopen_reference_storage_image_t s_ram_image;
static OD_PERSIST_COMM_t s_factory_defaults;
static bool s_factory_defaults_valid;
static CO_storage_t s_storage;
static CO_storage_entry_t s_entries[1];
static uint32_t s_store_count;
#if defined(STM32F767xx)
static uint32_t s_last_store_tick;
static bool s_store_tick_valid;
#endif

static uint32_t
storage_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t i = 0U; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            crc = ((crc & 1U) != 0U) ? ((crc >> 1U) ^ 0xEDB88320UL) : (crc >> 1U);
        }
    }
    return ~crc;
}

#if defined(STM32F767xx)
static const canopen_reference_storage_image_t *
storage_flash_image(uint32_t address) {
    return (const canopen_reference_storage_image_t *)(uintptr_t)address;
}

static bool
storage_flash_valid(const canopen_reference_storage_image_t *image) {
    return image != NULL && image->magic == CANOPEN_REFERENCE_STORAGE_MAGIC
        && image->version == CANOPEN_REFERENCE_STORAGE_VERSION
        && image->length == sizeof(image->payload)
        && image->crc32 == storage_crc32((const uint8_t *)&image->payload, sizeof(image->payload));
}

static const canopen_reference_storage_image_t *
storage_flash_newest(void) {
    const canopen_reference_storage_image_t *slot_a = storage_flash_image(CANOPEN_REFERENCE_STORAGE_SLOT_A);
    const canopen_reference_storage_image_t *slot_b = storage_flash_image(CANOPEN_REFERENCE_STORAGE_SLOT_B);
    bool valid_a = storage_flash_valid(slot_a);
    bool valid_b = storage_flash_valid(slot_b);

    if (!valid_a) {
        return valid_b ? slot_b : NULL;
    }
    if (!valid_b) {
        return slot_a;
    }
    return ((int32_t)(slot_a->sequence - slot_b->sequence) >= 0) ? slot_a : slot_b;
}

static uint32_t
storage_flash_next_sequence(void) {
    const canopen_reference_storage_image_t *newest = storage_flash_newest();
    return newest == NULL ? 1U : newest->sequence + 1U;
}

static bool
storage_flash_program_words(uint32_t address, const void *data, size_t length) {
    const uint32_t *words = (const uint32_t *)data;

    if ((address & 3U) != 0U || (length & 3U) != 0U) {
        return false;
    }
    for (size_t offset = 0U; offset < length; offset += sizeof(uint32_t)) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + offset, words[offset / sizeof(uint32_t)])
            != HAL_OK) {
            return false;
        }
    }
    return true;
}

static bool
storage_flash_store(const void *data, size_t length) {
    canopen_reference_storage_image_t image = {0};
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t page_error = 0U;
    uint32_t target_address;
    uint32_t target_sector;
    const canopen_reference_storage_image_t *newest;
    bool ok = false;

    if (data == NULL || length != sizeof(image.payload)) {
        return false;
    }
    newest = storage_flash_newest();
    target_address = (newest != NULL && (uintptr_t)newest == CANOPEN_REFERENCE_STORAGE_SLOT_A)
                         ? CANOPEN_REFERENCE_STORAGE_SLOT_B
                         : CANOPEN_REFERENCE_STORAGE_SLOT_A;
    target_sector = target_address == CANOPEN_REFERENCE_STORAGE_SLOT_A ? CANOPEN_REFERENCE_STORAGE_SECTOR_A
                                                                        : CANOPEN_REFERENCE_STORAGE_SECTOR_B;

    image.version = CANOPEN_REFERENCE_STORAGE_VERSION;
    image.sequence = storage_flash_next_sequence();
    image.length = (uint32_t)length;
    (void)memcpy(&image.payload, data, length);
    image.crc32 = storage_crc32((const uint8_t *)&image.payload, length);

    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = target_sector;
    erase.NbSectors = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    if (HAL_FLASHEx_Erase(&erase, &page_error) == HAL_OK
        && storage_flash_program_words(target_address + sizeof(uint32_t), &image.version,
                                       sizeof(image) - sizeof(uint32_t))
        && storage_flash_program_words(target_address, &((const canopen_reference_storage_image_t){
                                                              .magic = CANOPEN_REFERENCE_STORAGE_MAGIC})
                                                       .magic,
                                       sizeof(uint32_t))) {
        ok = true;
    }
    (void)HAL_FLASH_Lock();
    return ok;
}

static bool
storage_flash_restore(void *data, size_t length) {
    const canopen_reference_storage_image_t *image = storage_flash_newest();

    if (data == NULL || length != sizeof(s_ram_image.payload) || image == NULL) {
        return false;
    }
    (void)memcpy(data, &image->payload, length);
    return true;
}
#endif

__attribute__((weak)) bool
CANopenReferenceStorage_BoardStore(const void *data, size_t length) {
#if defined(STM32F767xx)
    return storage_flash_store(data, length);
#else
    if (data == NULL || length != sizeof(s_ram_image.payload)) {
        return false;
    }
    s_ram_image.magic = CANOPEN_REFERENCE_STORAGE_MAGIC;
    s_ram_image.version = CANOPEN_REFERENCE_STORAGE_VERSION;
    s_ram_image.sequence++;
    s_ram_image.length = (uint32_t)length;
    (void)memcpy(&s_ram_image.payload, data, length);
    s_ram_image.crc32 = storage_crc32((const uint8_t *)&s_ram_image.payload, length);
    return true;
#endif
}

__attribute__((weak)) bool
CANopenReferenceStorage_BoardRestore(void *data, size_t length) {
#if defined(STM32F767xx)
    return storage_flash_restore(data, length);
#else
    if (data == NULL || length != sizeof(s_ram_image.payload)
        || s_ram_image.magic != CANOPEN_REFERENCE_STORAGE_MAGIC
        || s_ram_image.version != CANOPEN_REFERENCE_STORAGE_VERSION
        || s_ram_image.length != (uint32_t)length
        || s_ram_image.crc32 != storage_crc32((const uint8_t *)&s_ram_image.payload, length)) {
        return false;
    }
    (void)memcpy(data, &s_ram_image.payload, length);
    return true;
#endif
}

static bool
storage_store_rate_allowed(void) {
#if defined(STM32F767xx)
#if CANOPEN_REFERENCE_STORAGE_MIN_STORE_INTERVAL_MS == 0U
    return true;
#else
    uint32_t now;

    if (!s_store_tick_valid) {
        return true;
    }
    now = HAL_GetTick();
    return (uint32_t)(now - s_last_store_tick) >= CANOPEN_REFERENCE_STORAGE_MIN_STORE_INTERVAL_MS;
#endif
#else
    return true;
#endif
}

static ODR_t
storage_store(CO_storage_entry_t *entry, CO_CANmodule_t *can_module) {
    bool stored;

    (void)can_module;
    if (entry == NULL || entry->addr == NULL || entry->len != sizeof(OD_PERSIST_COMM_t)) {
        return ODR_DEV_INCOMPAT;
    }
    if (!storage_store_rate_allowed()) {
        return ODR_HW;
    }
    stored = CANopenReferenceStorage_BoardStore(entry->addr, entry->len);
    if (!stored) {
        return ODR_HW;
    }
    ++s_store_count;
#if defined(STM32F767xx)
    s_last_store_tick = HAL_GetTick();
    s_store_tick_valid = true;
#endif
    return ODR_OK;
}

static ODR_t
storage_restore(CO_storage_entry_t *entry, CO_CANmodule_t *can_module) {
    (void)can_module;
    if (entry == NULL || entry->addr == NULL || entry->len != sizeof(OD_PERSIST_COMM_t)) {
        return ODR_DEV_INCOMPAT;
    }
    if (!CANopenReferenceStorage_BoardRestore(entry->addr, entry->len)) {
        if (!s_factory_defaults_valid) {
            return ODR_HW;
        }
        (void)memcpy(entry->addr, &s_factory_defaults, entry->len);
    }
    return ODR_OK;
}

uint32_t
CANopenReferenceStorage_StoreCount(void) {
    return s_store_count;
}

CO_ReturnError_t
CANopenReferenceStorage_Init(CO_t *co) {
    OD_entry_t *store_entry;
    OD_entry_t *restore_entry;
    CO_ReturnError_t result;

    if (co == NULL || co->CANmodule == NULL || OD == NULL) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }
    if (!s_factory_defaults_valid) {
        (void)memcpy(&s_factory_defaults, &OD_PERSIST_COMM, sizeof(s_factory_defaults));
        s_factory_defaults_valid = true;
    }
    (void)CANopenReferenceStorage_BoardRestore(&OD_PERSIST_COMM, sizeof(OD_PERSIST_COMM));

    store_entry = OD_find(OD, 0x1010U);
    restore_entry = OD_find(OD, 0x1011U);
    s_entries[0].addr = &OD_PERSIST_COMM;
    s_entries[0].len = sizeof(OD_PERSIST_COMM);
    s_entries[0].subIndexOD = 2U;
    s_entries[0].attr = (uint8_t)CO_storage_cmd | (uint8_t)CO_storage_restore;
    s_entries[0].addrNV = &s_ram_image;

    result = CO_storage_init(&s_storage, co->CANmodule, store_entry, restore_entry,
                             storage_store, storage_restore, s_entries, 1U);
    if (result == CO_ERROR_NO) {
        s_storage.enabled = true;
    }
    return result;
}
