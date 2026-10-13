/**
 * @file dtc_store.h
 * @brief Unified Diagnostic Trouble Code (DTC) Storage and Fault Lifecycle Management
 * @details Conforms to ISO 14229-1 (UDS Service 0x14, 0x19) and ISO 14229-2.
 */

#ifndef DTC_STORE_H
#define DTC_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef DTC_MAX_ENTRIES
#define DTC_MAX_ENTRIES     100U
#endif

#ifndef DTC_SNAPSHOT_SIZE
#define DTC_SNAPSHOT_SIZE   32U
#endif

#ifndef DTC_EXTDATA_SIZE
#define DTC_EXTDATA_SIZE    16U
#endif

#define FDC_THRESHOLD_FAILED 127
#define FDC_THRESHOLD_PASSED -128
#define DTC_AGING_CYCLES_MAX 40U

/*
 * DTC status byte bit definitions (ISO 14229-1):
 *   bit 0: testFailed
 *   bit 1: testFailedThisOperationCycle
 *   bit 2: pendingDTC
 *   bit 3: confirmedDTC
 *   bit 4: testNotCompletedSinceLastClear
 *   bit 5: testFailedSinceLastClear
 *   bit 6: testNotCompletedThisOperationCycle
 *   bit 7: warningIndicatorRequested
 */
#define DTC_STATUS_TEST_FAILED              0x01U
#define DTC_STATUS_TEST_FAILED_THIS_CYCLE   0x02U
#define DTC_STATUS_PENDING                  0x04U
#define DTC_STATUS_CONFIRMED                0x08U
#define DTC_STATUS_NOT_COMPLETED_CLEAR      0x10U
#define DTC_STATUS_FAILED_SINCE_CLEAR       0x20U
#define DTC_STATUS_NOT_COMPLETED_CYCLE      0x40U
#define DTC_STATUS_WARNING_INDICATOR        0x80U

#define DTC_STATUS_MASK_ALL                 0xFFU

/**
 * @brief Runtime item used by fault monitoring debouncing tasks
 */
typedef struct {
    uint32_t dtc;                     /**< 24-bit DTC identifier */
    uint8_t  status;                  /**< ISO 14229-1 status byte */
    int8_t   fault_detection_counter; /**< Counter between -128 and +127 */
    uint8_t  aging_counter;           /**< Number of fault-free driving cycles (0..40) */
    uint8_t  occurrence_counter;      /**< Total failure occurrences since last clear */
} dtc_runtime_item_t;

/**
 * @brief Standardized Snapshot DIDs matching OEM Specification (Issue #108)
 */
typedef struct {
    uint16_t voltage_mv;        /**< DID 0xDF00: ECU power supply voltage (mV, 0..36000 mV) */
    uint16_t speed_scaled;      /**< DID 0xDF01: Vehicle speed (speed * 256, 0..250 km/h) */
    uint8_t  occurrence_counter;/**< DID 0xDF02: Fault occurrence count (0..255) */
    uint32_t first_odometer;    /**< DID 0xDF03: First failure odometer in 5m steps */
    uint32_t last_odometer;     /**< DID 0xDF04: Last failure odometer in 5m steps */
    uint8_t  timestamp[6];      /**< DID 0xDD00: [0]=sec*4, [1]=min, [2]=hr, [3]=month, [4]=day*4, [5]=yr-1985 */
} dtc_standard_snapshot_t;

/**
 * @brief Full DTC entry stored in memory
 */
typedef struct {
    uint32_t dtc;                           /**< 3-byte DTC (lower 24 bits) */
    uint8_t  status;                        /**< DTC status byte */
    int8_t   fault_detection_counter;       /**< Debouncing counter */
    uint8_t  aging_counter;                 /**< Driving cycles passed */
    uint8_t  occurrence_counter;            /**< Fault occurrences */
    bool     has_snapshot;                  /**< Freeze frame available */
    uint8_t  snapshot[DTC_SNAPSHOT_SIZE];   /**< Freeze frame raw payload */
    uint8_t  snapshot_len;
    bool     has_extdata;                   /**< Extended data available */
    uint8_t  extdata[DTC_EXTDATA_SIZE];     /**< Extended data payload */
    uint8_t  extdata_len;
} dtc_entry_t;

/* ── Core Management API ── */

void dtc_store_init(void);

void dtc_store_add(uint32_t dtc, uint8_t status,
                   const uint8_t *snap, uint8_t snap_len,
                   const uint8_t *ext, uint8_t ext_len);

uint8_t dtc_store_count(void);

uint8_t dtc_store_get_status_mask(void);

uint16_t dtc_store_count_by_mask(uint8_t status_mask);

const dtc_entry_t *dtc_store_get_by_index(uint8_t index);

dtc_entry_t *dtc_store_find_mut(uint32_t dtc);

const dtc_entry_t *dtc_store_find(uint32_t dtc);

/**
 * @brief Clear DTCs by 24-bit group mask or specific DTC code (Issue #115 fix)
 * @param group_or_dtc 0xFFFFFF for all DTCs, or specific 24-bit DTC, or group prefix
 * @return true if at least one matching DTC was cleared, false if not found
 */
bool dtc_store_clear(uint32_t group_or_dtc);

void dtc_store_clear_all(void);

/* ── Fault Debouncing & Lifecycle API (Issues #109, #110, #111, #112) ── */

/**
 * @brief Update DTC status on each sensor / diagnostic check sample
 * @param item Pointer to runtime item or entry
 * @param sample_failed True if sensor failed this sample, false if passed
 * @param step_fail FDC increment on failure (e.g. +5 to +10)
 * @param step_pass FDC decrement on pass (e.g. -2 to -5)
 */
void dtc_process_sample(dtc_runtime_item_t *item, bool sample_failed, int8_t step_fail, int8_t step_pass);

/**
 * @brief Call on driving cycle start (Ignition ON / KL15 active)
 */
void dtc_operation_cycle_start(dtc_runtime_item_t *items, uint8_t count);

/**
 * @brief Call on driving cycle end (Ignition OFF / KL15 inactive) - manages unlearning/aging
 */
void dtc_operation_cycle_end(dtc_runtime_item_t *items, uint8_t count);

/**
 * @brief Pack standard snapshot structure into raw buffer
 */
uint8_t dtc_pack_standard_snapshot(const dtc_standard_snapshot_t *snap, uint8_t *out_buf, uint8_t max_len);

/**
 * @brief Check if DTC setting/updating is enabled (controlled via Service 0x85 ControlDTCSetting)
 * @return True if DTC setting is enabled, false if disabled
 */
bool dtc_is_setting_enabled(void);

/**
 * @brief Set DTC setting/updating state (called by Service 0x85 ControlDTCSetting)
 * @param enabled True to enable DTC recording, false to freeze DTC debouncing
 */
void dtc_set_setting_enabled(bool enabled);

/**
 * @brief Optional callback invoked when a DTC status changes (for NVM persistence adapter)
 */
typedef void (*dtc_status_change_fn)(uint32_t dtc, uint8_t status);

/**
 * @brief Register a callback for DTC status modifications
 */
void dtc_store_set_status_cb(dtc_status_change_fn cb);

#endif /* DTC_STORE_H */
