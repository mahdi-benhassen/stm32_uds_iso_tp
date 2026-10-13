/**
 * @file dtc_store.c
 * @brief Unified Diagnostic Trouble Code (DTC) Storage and Fault Lifecycle Management
 */

#include "dtc_store.h"
#include <string.h>

static dtc_entry_t s_dtcs[DTC_MAX_ENTRIES];
static uint8_t     s_dtc_count = 0U;
static bool        s_dtc_setting_enabled = true;
static dtc_status_change_fn s_status_cb = NULL;

void dtc_store_init(void) {
    s_dtc_count = 0U;
    s_dtc_setting_enabled = true;
    memset(s_dtcs, 0, sizeof(s_dtcs));
}

void dtc_store_set_status_cb(dtc_status_change_fn cb) {
    s_status_cb = cb;
}

bool dtc_is_setting_enabled(void) {
    return s_dtc_setting_enabled;
}

void dtc_set_setting_enabled(bool enabled) {
    s_dtc_setting_enabled = enabled;
}

void dtc_store_add(uint32_t dtc, uint8_t status,
                   const uint8_t *snap, uint8_t snap_len,
                   const uint8_t *ext, uint8_t ext_len) {
    if (s_dtc_count >= DTC_MAX_ENTRIES) {
        return;
    }

    dtc_entry_t *e = &s_dtcs[s_dtc_count++];
    memset(e, 0, sizeof(*e));
    e->dtc = dtc & 0x00FFFFFFU;
    e->status = status;
    e->fault_detection_counter = 0;
    e->aging_counter = 0U;
    e->occurrence_counter = 0U;

    if ((snap != NULL) && (snap_len > 0U)) {
        e->has_snapshot = true;
        e->snapshot_len = (snap_len > DTC_SNAPSHOT_SIZE) ? DTC_SNAPSHOT_SIZE : snap_len;
        memcpy(e->snapshot, snap, e->snapshot_len);
    }

    if ((ext != NULL) && (ext_len > 0U)) {
        e->has_extdata = true;
        e->extdata_len = (ext_len > DTC_EXTDATA_SIZE) ? DTC_EXTDATA_SIZE : ext_len;
        memcpy(e->extdata, ext, e->extdata_len);
    }
}

uint8_t dtc_store_count(void) {
    return s_dtc_count;
}

uint8_t dtc_store_get_status_mask(void) {
    return DTC_STATUS_MASK_ALL;
}

uint16_t dtc_store_count_by_mask(uint8_t status_mask) {
    uint16_t count = 0U;
    for (uint8_t i = 0U; i < s_dtc_count; i++) {
        if ((s_dtcs[i].status & status_mask) != 0U) {
            count++;
        }
    }
    return count;
}

const dtc_entry_t *dtc_store_get_by_index(uint8_t index) {
    if (index >= s_dtc_count) {
        return NULL;
    }
    return &s_dtcs[index];
}

dtc_entry_t *dtc_store_find_mut(uint32_t dtc) {
    uint32_t d = dtc & 0x00FFFFFFU;
    for (uint8_t i = 0U; i < s_dtc_count; i++) {
        if (s_dtcs[i].dtc == d) {
            return &s_dtcs[i];
        }
    }
    return NULL;
}

const dtc_entry_t *dtc_store_find(uint32_t dtc) {
    return dtc_store_find_mut(dtc);
}

static void dtc_reset_entry(dtc_entry_t *e) {
    /* ISO 14229-1 status reset on 0x14 ClearDiagnosticInformation */
    e->status &= (uint8_t)~(DTC_STATUS_TEST_FAILED |
                            DTC_STATUS_TEST_FAILED_THIS_CYCLE |
                            DTC_STATUS_PENDING |
                            DTC_STATUS_CONFIRMED |
                            DTC_STATUS_FAILED_SINCE_CLEAR |
                            DTC_STATUS_WARNING_INDICATOR);
    e->status |= (DTC_STATUS_NOT_COMPLETED_CLEAR | DTC_STATUS_NOT_COMPLETED_CYCLE);
    e->fault_detection_counter = 0;
    e->aging_counter = 0U;
    e->occurrence_counter = 0U;
    e->has_snapshot = false;
    e->has_extdata = false;
    if (s_status_cb != NULL) {
        s_status_cb(e->dtc, e->status);
    }
}

bool dtc_store_clear(uint32_t group_or_dtc) {
    uint32_t target = group_or_dtc & 0x00FFFFFFU;
    bool found = false;

    /* Case 1: 0xFFFFFF clears all DTCs */
    if (target == 0x00FFFFFFU) {
        for (uint8_t i = 0U; i < s_dtc_count; i++) {
            dtc_reset_entry(&s_dtcs[i]);
        }
        return (s_dtc_count > 0U);
    }

    /* Case 2: Specific 24-bit DTC match */
    for (uint8_t i = 0U; i < s_dtc_count; i++) {
        if (s_dtcs[i].dtc == target) {
            dtc_reset_entry(&s_dtcs[i]);
            return true;
        }
    }

    /* Case 3: Group prefix match if lower 16 bits are wildcards (0x0000 or 0xFFFF) */
    if (((target & 0x00FFFFU) == 0x000000U) || ((target & 0x00FFFFU) == 0x00FFFFU)) {
        uint8_t group_hi = (uint8_t)(target >> 16U);
        for (uint8_t i = 0U; i < s_dtc_count; i++) {
            if ((uint8_t)(s_dtcs[i].dtc >> 16U) == group_hi) {
                dtc_reset_entry(&s_dtcs[i]);
                found = true;
            }
        }
    }

    return found;
}

void dtc_store_clear_all(void) {
    (void)dtc_store_clear(0x00FFFFFFU);
}

void dtc_process_sample(dtc_runtime_item_t *item, bool sample_failed, int8_t step_fail, int8_t step_pass) {
    if (item == NULL) return;

    /* ISO 14229-1 Service 0x85 ControlDTCSetting: Freeze debouncing when setting is OFF */
    if (!s_dtc_setting_enabled) return;

    if (sample_failed) {
        if ((int32_t)item->fault_detection_counter + step_fail >= FDC_THRESHOLD_FAILED) {
            item->fault_detection_counter = FDC_THRESHOLD_FAILED;
            /* Qualify fault */
            item->status |= (DTC_STATUS_TEST_FAILED |
                             DTC_STATUS_TEST_FAILED_THIS_CYCLE |
                             DTC_STATUS_PENDING |
                             DTC_STATUS_CONFIRMED |
                             DTC_STATUS_FAILED_SINCE_CLEAR);
            item->status &= (uint8_t)~(DTC_STATUS_NOT_COMPLETED_CYCLE |
                                       DTC_STATUS_NOT_COMPLETED_CLEAR);
            item->aging_counter = 0U;
            if (item->occurrence_counter < 0xFFU) {
                item->occurrence_counter++;
            }
        } else {
            item->fault_detection_counter = (int8_t)(item->fault_detection_counter + step_fail);
        }
    } else {
        if ((int32_t)item->fault_detection_counter - step_pass <= FDC_THRESHOLD_PASSED) {
            item->fault_detection_counter = FDC_THRESHOLD_PASSED;
            /* Qualify pass */
            item->status &= (uint8_t)~DTC_STATUS_TEST_FAILED;
            item->status &= (uint8_t)~(DTC_STATUS_NOT_COMPLETED_CYCLE |
                                       DTC_STATUS_NOT_COMPLETED_CLEAR);
        } else {
            item->fault_detection_counter = (int8_t)(item->fault_detection_counter - step_pass);
        }
    }

    /* Update central store entry if present and notify persistence callback */
    dtc_entry_t *entry = dtc_store_find_mut(item->dtc);
    if (entry != NULL) {
        entry->status = item->status;
        entry->fault_detection_counter = item->fault_detection_counter;
        entry->aging_counter = item->aging_counter;
        entry->occurrence_counter = item->occurrence_counter;
    }
    if (s_status_cb != NULL) {
        s_status_cb(item->dtc, item->status);
    }
}

void dtc_operation_cycle_start(dtc_runtime_item_t *items, uint8_t count) {
    if (items == NULL) return;
    for (uint8_t i = 0U; i < count; i++) {
        dtc_runtime_item_t *it = &items[i];
        it->status &= (uint8_t)~(DTC_STATUS_TEST_FAILED | DTC_STATUS_TEST_FAILED_THIS_CYCLE);
        it->status |= DTC_STATUS_NOT_COMPLETED_CYCLE;
        it->fault_detection_counter = 0;
    }
}

void dtc_operation_cycle_end(dtc_runtime_item_t *items, uint8_t count) {
    if (items == NULL) return;
    for (uint8_t i = 0U; i < count; i++) {
        dtc_runtime_item_t *it = &items[i];
        /* Unlearning / Aging: increment aging if confirmed and didn't fail this cycle */
        if ((it->status & DTC_STATUS_CONFIRMED) != 0U) {
            if ((it->status & DTC_STATUS_TEST_FAILED_THIS_CYCLE) == 0U) {
                it->aging_counter++;
                if (it->aging_counter >= DTC_AGING_CYCLES_MAX) {
                    it->status &= (uint8_t)~DTC_STATUS_CONFIRMED;
                    it->aging_counter = 0U;
                }
            }
        }
    }
}

uint8_t dtc_pack_standard_snapshot(const dtc_standard_snapshot_t *snap, uint8_t *out_buf, uint8_t max_len) {
    if ((snap == NULL) || (out_buf == NULL) || (max_len < 25U)) {
        return 0U;
    }

    uint8_t pos = 0U;
    out_buf[pos++] = 0x01U; /* Record number 0x01 */
    out_buf[pos++] = 0x06U; /* 6 DIDs included */

    /* DID 0xDF00: ECU Voltage */
    out_buf[pos++] = 0xDFU; out_buf[pos++] = 0x00U;
    out_buf[pos++] = (uint8_t)(snap->voltage_mv >> 8U);
    out_buf[pos++] = (uint8_t)(snap->voltage_mv & 0xFFU);

    /* DID 0xDF01: Vehicle Speed */
    out_buf[pos++] = 0xDFU; out_buf[pos++] = 0x01U;
    out_buf[pos++] = (uint8_t)(snap->speed_scaled >> 8U);
    out_buf[pos++] = (uint8_t)(snap->speed_scaled & 0xFFU);

    /* DID 0xDF02: Occurrence Counter */
    out_buf[pos++] = 0xDFU; out_buf[pos++] = 0x02U;
    out_buf[pos++] = snap->occurrence_counter;

    /* DID 0xDF03: First Odometer */
    out_buf[pos++] = 0xDFU; out_buf[pos++] = 0x03U;
    out_buf[pos++] = (uint8_t)(snap->first_odometer >> 24U);
    out_buf[pos++] = (uint8_t)(snap->first_odometer >> 16U);
    out_buf[pos++] = (uint8_t)(snap->first_odometer >> 8U);
    out_buf[pos++] = (uint8_t)(snap->first_odometer & 0xFFU);

    /* DID 0xDF04: Last Odometer */
    out_buf[pos++] = 0xDFU; out_buf[pos++] = 0x04U;
    out_buf[pos++] = (uint8_t)(snap->last_odometer >> 24U);
    out_buf[pos++] = (uint8_t)(snap->last_odometer >> 16U);
    out_buf[pos++] = (uint8_t)(snap->last_odometer >> 8U);
    out_buf[pos++] = (uint8_t)(snap->last_odometer & 0xFFU);

    /* DID 0xDD00: Time */
    out_buf[pos++] = 0xDDU; out_buf[pos++] = 0x00U;
    memcpy(&out_buf[pos], snap->timestamp, 6U);
    pos = (uint8_t)(pos + 6U);

    return pos;
}
