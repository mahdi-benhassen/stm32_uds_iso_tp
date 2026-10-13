/**
 * @file dtc_nvm_adapter.c
 * @brief NVM Flash Wear-Leveling Adapter for 85 DTCs Storage Implementation (Issue #120)
 * @details Connects 85 DTCs store with UdsParamStore wear leveling on STM32 Flash.
 */

#include "dtc_nvm_adapter.h"
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define UDS_WEAK __attribute__((weak))
#else
#define UDS_WEAK
#endif

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  dtc_count;
    struct {
        uint32_t dtc;
        uint8_t  status;
    } entries[DTC_MAX_ENTRIES];
} DtcNvmPayload;

static UdsParamStore s_dtc_param_store;
static DtcNvmPayload s_dtc_nvm_cache;
static bool          s_dtc_nvm_dirty = false;
static bool          s_dtc_nvm_initialized = false;

/* Simulated memory buffer for fallback / testing */
static uint8_t s_simulated_flash[2048];
static bool    s_simulated_flash_inited = false;

static void ensure_simulated_flash(void) {
    if (!s_simulated_flash_inited) {
        memset(s_simulated_flash, 0xFF, sizeof(s_simulated_flash));
        s_simulated_flash_inited = true;
    }
}

/* Weak STM32F103 Flash port fallback handlers for standalone compilation */
UDS_WEAK int stm32f103_flash_erase(uint32_t addr) {
    ensure_simulated_flash();
    if ((addr >= DTC_NVM_STORAGE_BASE) && (addr < (DTC_NVM_STORAGE_BASE + 2048U))) {
        uint32_t offset = addr - DTC_NVM_STORAGE_BASE;
        uint32_t len = ((offset + 1024U) <= 2048U) ? 1024U : (2048U - offset);
        memset(&s_simulated_flash[offset], 0xFF, len);
    }
    return 0;
}

UDS_WEAK int stm32f103_flash_read(uint32_t addr, void *buf, size_t len) {
    if (buf == NULL) return -1;
    ensure_simulated_flash();
    if ((addr >= DTC_NVM_STORAGE_BASE) && (addr + len <= (DTC_NVM_STORAGE_BASE + 2048U))) {
        uint32_t offset = addr - DTC_NVM_STORAGE_BASE;
        memcpy(buf, &s_simulated_flash[offset], len);
        return 0;
    }
    memset(buf, 0xFF, len);
    return 0;
}

UDS_WEAK int stm32f103_flash_write(uint32_t addr, const void *buf, size_t len) {
    if (buf == NULL) return -1;
    ensure_simulated_flash();
    if ((addr >= DTC_NVM_STORAGE_BASE) && (addr + len <= (DTC_NVM_STORAGE_BASE + 2048U))) {
        uint32_t offset = addr - DTC_NVM_STORAGE_BASE;
        memcpy(&s_simulated_flash[offset], buf, len);
        return 0;
    }
    return 0;
}

UDS_WEAK uint32_t stm32f103_flash_sector_size(uint32_t addr) {
    (void)addr;
    return 1024U; /* 1KB sector size for STM32F103 Medium-Density */
}

static const UdsFlashPort s_default_f103_port = {
    .erase           = stm32f103_flash_erase,
    .read            = stm32f103_flash_read,
    .write           = stm32f103_flash_write,
    .sector_size     = stm32f103_flash_sector_size,
    .program_granule = 2U,
    .erased_byte     = 0xFFU
};

static void dtc_nvm_status_change_callback(uint32_t dtc, uint8_t status) {
    dtc_nvm_save(dtc, status);
}

void dtc_nvm_init_custom(const UdsFlashPort *port, uint32_t flash_base, uint8_t sector_count) {
    if (uds_param_init(&s_dtc_param_store, port, flash_base, sector_count, (uint16_t)sizeof(DtcNvmPayload)) == UDS_PARAM_OK) {
        s_dtc_nvm_initialized = true;
    }

    /* Attempt to load persisted state from flash */
    int load_res = uds_param_load(&s_dtc_param_store, &s_dtc_nvm_cache);
    if ((load_res == UDS_PARAM_OK) && (s_dtc_nvm_cache.magic == DTC_NVM_MAGIC)) {
        /* Valid NVM content restored: mirror status values into dtc_store */
        for (uint8_t i = 0U; i < s_dtc_nvm_cache.dtc_count; i++) {
            dtc_entry_t *entry = dtc_store_find_mut(s_dtc_nvm_cache.entries[i].dtc);
            if (entry != NULL) {
                entry->status = s_dtc_nvm_cache.entries[i].status;
            }
        }
        s_dtc_nvm_dirty = false;
    } else {
        /* Blank flash or invalid magic: populate cache from current dtc_store */
        s_dtc_nvm_cache.magic = DTC_NVM_MAGIC;
        uint8_t count = dtc_store_count();
        s_dtc_nvm_cache.dtc_count = count;
        for (uint8_t i = 0U; i < count; i++) {
            const dtc_entry_t *e = dtc_store_get_by_index(i);
            if (e != NULL) {
                s_dtc_nvm_cache.entries[i].dtc = e->dtc;
                s_dtc_nvm_cache.entries[i].status = e->status;
            }
        }
        s_dtc_nvm_dirty = true;
    }

    /* Register callback so dtc_store updates automatically mark dirty */
    dtc_store_set_status_cb(dtc_nvm_status_change_callback);
}

void dtc_nvm_init(void) {
    dtc_nvm_init_custom(&s_default_f103_port, DTC_NVM_STORAGE_BASE, DTC_NVM_SECTOR_COUNT);
}

void dtc_nvm_save(uint32_t dtc, uint8_t status) {
    uint32_t target = dtc & 0x00FFFFFFU;
    for (uint8_t i = 0U; i < s_dtc_nvm_cache.dtc_count; i++) {
        if (s_dtc_nvm_cache.entries[i].dtc == target) {
            if (s_dtc_nvm_cache.entries[i].status != status) {
                s_dtc_nvm_cache.entries[i].status = status;
                s_dtc_nvm_dirty = true;
            }
            return;
        }
    }

    /* If not found and space available, append new entry */
    if (s_dtc_nvm_cache.dtc_count < DTC_MAX_ENTRIES) {
        s_dtc_nvm_cache.entries[s_dtc_nvm_cache.dtc_count].dtc = target;
        s_dtc_nvm_cache.entries[s_dtc_nvm_cache.dtc_count].status = status;
        s_dtc_nvm_cache.dtc_count++;
        s_dtc_nvm_dirty = true;
    }
}

bool dtc_nvm_flush(void) {
    if (!s_dtc_nvm_dirty || !s_dtc_nvm_initialized) {
        return true;
    }
    int res = uds_param_save(&s_dtc_param_store, &s_dtc_nvm_cache);
    if (res == UDS_PARAM_OK) {
        s_dtc_nvm_dirty = false;
        return true;
    }
    return false;
}

bool dtc_nvm_is_dirty(void) {
    return s_dtc_nvm_dirty;
}

void dtc_nvm_erase_all(void) {
    if (s_dtc_nvm_initialized) {
        (void)uds_param_erase_all(&s_dtc_param_store);
        s_dtc_nvm_dirty = false;
    }
}
