/**
 * @file uds_did_table.c
 * @brief Unified Data Identifier (DID) Registry Implementation
 */

#include "uds_did_table.h"
#include "uds_session.h"
#include <string.h>

static uds_did_record_t s_dids[UDS_DID_MAX_ENTRIES];
static uint16_t s_did_count = 0U;
static bool s_did_initialized = false;

static void register_default_dids(void) {
    /* 0xF180: Boot Software Identification */
    static const uint8_t boot_id[] = "BOOT-V1.2.0";
    uds_did_table_register(0xF180U, boot_id, (uint16_t)(sizeof(boot_id) - 1U), false, false);

    /* 0xF186: Active Diagnostic Session (dynamic, updated at read) */
    static const uint8_t sess_id[] = { 0x01U };
    uds_did_table_register(0xF186U, sess_id, 1U, false, false);

    /* 0xF187: Vehicle Manufacturer Spare Part Number */
    static const uint8_t spare_part[] = "STM32-PART1";
    uds_did_table_register(0xF187U, spare_part, (uint16_t)(sizeof(spare_part) - 1U), false, false);

    /* 0xF188: Vehicle Manufacturer ECU Software Number */
    static const uint8_t sw_num[] = "SW-01.00";
    uds_did_table_register(0xF188U, sw_num, (uint16_t)(sizeof(sw_num) - 1U), false, false);

    /* 0xF189: Vehicle Manufacturer ECU Software Version */
    static const uint8_t sw_ver[] = "V1.00.00";
    uds_did_table_register(0xF189U, sw_ver, (uint16_t)(sizeof(sw_ver) - 1U), false, false);

    /* 0xF190: Vehicle Identification Number (VIN - 17 bytes, writable) */
    static const uint8_t vin[] = "WAUZZZ8V1KA123456";
    uds_did_table_register(0xF190U, vin, 17U, true, true);

    /* 0xF191: Vehicle Manufacturer ECU Hardware Number */
    static const uint8_t hw_num[] = "HW-STM32";
    uds_did_table_register(0xF191U, hw_num, (uint16_t)(sizeof(hw_num) - 1U), false, false);

    /* 0xF197: System Name Or Engine Type */
    static const uint8_t sys_name[] = "ECU_GATEWAY_";
    uds_did_table_register(0xF197U, sys_name, (uint16_t)(sizeof(sys_name) - 1U), false, false);

    /* 0xF199: Programming Date (4 bytes: YYYYMMDD BCD) */
    static const uint8_t prog_date[] = { 0x20U, 0x26U, 0x10U, 0x30U };
    uds_did_table_register(0xF199U, prog_date, 4U, true, true);
}

void uds_did_table_init(void) {
    if (!s_did_initialized) {
        s_did_count = 0U;
        memset(s_dids, 0, sizeof(s_dids));
        register_default_dids();
        s_did_initialized = true;
    }
}

bool uds_did_table_register(uint16_t id, const uint8_t *initial_data, uint16_t len,
                            bool writable, bool secured) {
    if (s_did_count >= UDS_DID_MAX_ENTRIES) {
        return false;
    }
    if ((initial_data == NULL) && (len > 0U)) {
        return false;
    }
    if (len > UDS_DID_MAX_DATA_LEN) {
        len = UDS_DID_MAX_DATA_LEN;
    }

    uds_did_record_t *rec = &s_dids[s_did_count++];
    rec->id = id;
    rec->len = len;
    rec->writable = writable;
    rec->secured = secured;
    if ((initial_data != NULL) && (len > 0U)) {
        memcpy(rec->data, initial_data, len);
    }
    return true;
}

const uds_did_record_t *uds_did_table_find(uint16_t id) {
    uds_did_table_init();
    for (uint16_t i = 0U; i < s_did_count; i++) {
        if (s_dids[i].id == id) {
            /* Update dynamic DIDs if applicable */
            if (id == 0xF186U) {
                s_dids[i].data[0] = (uint8_t)uds_session_get();
                s_dids[i].len = 1U;
            }
            return &s_dids[i];
        }
    }
    return NULL;
}

bool uds_did_table_write(uint16_t id, const uint8_t *data, uint16_t len) {
    uds_did_table_init();
    if ((data == NULL) || (len == 0U) || (len > UDS_DID_MAX_DATA_LEN)) {
        return false;
    }
    for (uint16_t i = 0U; i < s_did_count; i++) {
        if (s_dids[i].id == id) {
            if (!s_dids[i].writable) {
                return false;
            }
            memcpy(s_dids[i].data, data, len);
            s_dids[i].len = len;
            return true;
        }
    }
    return false;
}
