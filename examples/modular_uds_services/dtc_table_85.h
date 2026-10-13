/**
 * @file dtc_table_85.h
 * @brief 85 Standard Diagnostic Trouble Codes (DTC) from Specification (DTC.uds.xlsx)
 */

#ifndef DTC_TABLE_85_H
#define DTC_TABLE_85_H

#include <stdint.h>
#include <stdbool.h>

#define DTC_CONFIGURED_COUNT 85U

typedef struct {
    uint32_t dtc;          /**< 24-bit DTC value (e.g. 0xD00616) */
    const char *name;      /**< Symbolic fault name */
    const char *desc;      /**< Description */
} dtc_config_entry_t;

extern const dtc_config_entry_t g_dtc_config_85[DTC_CONFIGURED_COUNT];

/**
 * @brief Initialize DTC store with all 85 DTCs
 */
void dtc_store_init_85_dtcs(void);
void dtc_table_85_init(void);

#endif /* DTC_TABLE_85_H */
