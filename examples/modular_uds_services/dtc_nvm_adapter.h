/**
 * @file dtc_nvm_adapter.h
 * @brief NVM Flash Wear-Leveling Adapter for 85 DTCs Storage (ISO 14229-1, Issue #120)
 * @details Bridges dtc_store memory state with flash wear leveling on STM32F103/F7.
 */

#ifndef DTC_NVM_ADAPTER_H
#define DTC_NVM_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "dtc_store.h"
#include "uds_wear_leveling.h"

/* Top 2 pages of 64KB STM32F103 Flash (Pages 62 & 63: 0x0800F800 - 0x0800FFFF) */
#define DTC_NVM_STORAGE_BASE 0x0800F800U
#define DTC_NVM_SECTOR_COUNT 2U
#define DTC_NVM_MAGIC        0x44544331U /* 'DTC1' */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize DTC NVM adapter, binding dtc_store to Flash wear-leveling storage
 * @details Reads saved DTC states from Flash. If valid, restores them into dtc_store.
 *          If flash is blank or invalid, populates cache with current DTC statuses.
 */
void dtc_nvm_init(void);

/**
 * @brief Initialize DTC NVM adapter with custom flash base and port
 */
void dtc_nvm_init_custom(const UdsFlashPort *port, uint32_t flash_base, uint8_t sector_count);

/**
 * @brief Save/update a single DTC status in RAM cache (marks dirty)
 */
void dtc_nvm_save(uint32_t dtc, uint8_t status);

/**
 * @brief Flush any pending dirty DTC modifications to Flash via wear-leveling algorithm
 * @return True if flash write succeeded or no changes, false on flash error
 */
bool dtc_nvm_flush(void);

/**
 * @brief Check if there are unflushed pending modifications in cache
 */
bool dtc_nvm_is_dirty(void);

/**
 * @brief Clear/erase all stored DTC statuses in NVM
 */
void dtc_nvm_erase_all(void);

#ifdef __cplusplus
}
#endif

#endif /* DTC_NVM_ADAPTER_H */
