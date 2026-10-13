/**
 * @file dtc_lifecycle_example.c
 * @brief Complete Working Example Demonstrating DTC Storage and Lifecycle Management
 * @details Directly answers GitHub Issues #109, #110, #111, and #112:
 *          - How to call dtc_store_add()
 *          - How to call dtc_operation_cycle_start() on Ignition ON
 *          - How to call dtc_process_sample() during sensor polling with debouncing
 *          - How to call dtc_operation_cycle_end() on Ignition OFF for unlearning/aging
 */

#include "dtc_store.h"
#include "dtc_table_85.h"
#include "dtc_nvm_adapter.h"
#include <stdio.h>
#include <stdbool.h>

/* Global runtime items tracked by ASW (Application Software) */
static dtc_runtime_item_t s_runtime_dtcs[DTC_CONFIGURED_COUNT];

/**
 * @brief 1. System Startup Initialization (Issues #109, #120)
 * @details How to call dtc_store_add() and dtc_nvm_init():
 *          Populates DTC store with initial status, restores saved states from Flash NVM,
 *          and initializes runtime debouncing structures.
 */
void app_dtc_init(void) {
    /* Initialize storage and populate all 85 standard DTCs */
    dtc_store_init_85_dtcs();

    /* Initialize Flash NVM wear-leveling adapter (Issue #120) */
    dtc_nvm_init();

    /* Also initialize runtime monitoring structures */
    for (uint8_t i = 0; i < DTC_CONFIGURED_COUNT; i++) {
        s_runtime_dtcs[i].dtc = g_dtc_config_85[i].dtc;
        s_runtime_dtcs[i].status = 0x50U; /* testNotCompletedSinceLastClear | testNotCompletedThisCycle */
        s_runtime_dtcs[i].fault_detection_counter = 0;
        s_runtime_dtcs[i].aging_counter = 0U;
        s_runtime_dtcs[i].occurrence_counter = 0U;
    }

    /* Example of explicitly adding a single custom DTC with snapshot & extended data: */
    uint8_t sample_snapshot[8] = { 0x12, 0x34, 0x56, 0x78, 0x00, 0x00, 0x00, 0x00 };
    uint8_t sample_extdata[2]  = { 0x01, 0x00 }; /* Occurrence counter = 1, Aging = 0 */
    dtc_store_add(0xD00616U, 0x2FU, sample_snapshot, sizeof(sample_snapshot), sample_extdata, sizeof(sample_extdata));
}

/**
 * @brief 2. Ignition ON / KL15 Active Callback (Issue #111)
 * @details How to call dtc_operation_cycle_start():
 *          Clears testFailed & testFailedThisOperationCycle, sets testNotCompletedThisOperationCycle,
 *          and resets fault detection counters for the new driving cycle.
 */
void app_on_ignition_on(void) {
    printf("[ECU] Ignition ON: Starting new driving operation cycle.\n");
    dtc_operation_cycle_start(s_runtime_dtcs, DTC_CONFIGURED_COUNT);
}

/**
 * @brief 3. Periodic Sensor Monitoring Task (Issue #110)
 * @details How to call dtc_process_sample():
 *          Called every 10ms or 50ms task. Uses asymmetrical FDC debouncing:
 *          - If sensor read fails: step_fail (+10) moves FDC toward +127 (threshold failed)
 *          - If sensor read passes: step_pass (+5) moves FDC toward -128 (threshold passed)
 *          When +127 is reached, the DTC qualifies and status bits (Confirmed, Pending, etc.) are set!
 */
void app_sensor_task_10ms(uint16_t battery_mv) {
    /* DTC U300614: Battery Voltage High Warning (threshold: > 32000 mV) */
    dtc_runtime_item_t *item_batt_high = &s_runtime_dtcs[0];

    bool is_failed = (battery_mv > 32000U);
    int8_t step_fail = 10; /* Increments by +10 each failed sample */
    int8_t step_pass = 5;  /* Decrements by -5 each good sample */

    dtc_process_sample(item_batt_high, is_failed, step_fail, step_pass);

    /* Synchronize qualified status to the global UDS DTC store when state changes */
    dtc_entry_t *store_entry = dtc_store_find_mut(item_batt_high->dtc);
    if (store_entry != NULL) {
        store_entry->status = item_batt_high->status;
        store_entry->fault_detection_counter = item_batt_high->fault_detection_counter;
        store_entry->occurrence_counter = item_batt_high->occurrence_counter;

        /* If newly confirmed, capture standard snapshot */
        if ((item_batt_high->status & DTC_STATUS_CONFIRMED) && !store_entry->has_snapshot) {
            dtc_standard_snapshot_t snap;
            snap.voltage_mv = battery_mv;
            snap.speed_scaled = 0U;
            snap.occurrence_counter = item_batt_high->occurrence_counter;
            snap.first_odometer = 1000U; /* 5000 meters */
            snap.last_odometer  = 1000U;
            snap.timestamp[0] = 0; snap.timestamp[1] = 0; snap.timestamp[2] = 12;
            snap.timestamp[3] = 10; snap.timestamp[4] = 9; snap.timestamp[5] = 41; /* Year 2026 */

            store_entry->snapshot_len = dtc_pack_standard_snapshot(&snap, store_entry->snapshot, DTC_SNAPSHOT_SIZE);
            store_entry->has_snapshot = (store_entry->snapshot_len > 0);
        }
    }
}

/**
 * @brief 4. Ignition OFF / KL15 Inactive Callback (Issue #112)
 * @details How to call dtc_operation_cycle_end():
 *          Called when vehicle shuts down.
 *          If a confirmed fault did NOT fail during this operation cycle,
 *          its aging counter increments. When aging counter reaches 40 cycles,
 *          the confirmed fault is automatically unlearned/healed!
 */
void app_on_ignition_off(void) {
    printf("[ECU] Ignition OFF: Ending driving cycle, updating unlearning/aging counters.\n");
    dtc_operation_cycle_end(s_runtime_dtcs, DTC_CONFIGURED_COUNT);

    /* Update aging counter back to store */
    for (uint8_t i = 0; i < DTC_CONFIGURED_COUNT; i++) {
        const dtc_runtime_item_t *it = &s_runtime_dtcs[i];
        dtc_entry_t *entry = dtc_store_find_mut(it->dtc);
        if (entry != NULL) {
            entry->status = it->status;
            entry->aging_counter = it->aging_counter;
        }
    }

    /* Flush modified DTC statuses to Flash NVM wear-leveling storage (Issue #120) */
    dtc_nvm_flush();
}
