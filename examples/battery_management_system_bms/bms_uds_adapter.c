#include "bms_uds_adapter.h"
#include "bms_app.h"
#include <string.h>

static uint32_t s_active_seed = 0x12345678U;

static UdsCallbackResult bms_read_did(void *context, uint16_t did, uint8_t *data,
                                      uint16_t *length, uint16_t capacity)
{
    (void)context;
    const bms_state_t *st = bms_app_get_state();

    if (data == NULL || length == NULL) {
        return UDS_RESULT_ERROR;
    }

    switch (did) {
    case BMS_DID_PACK_SUMMARY:
        if (capacity < 12U) return UDS_RESULT_RESPONSE_TOO_LONG;
        /* Pack Voltage (mV, uint32_t) */
        data[0] = (uint8_t)((st->pack_voltage_mv >> 24) & 0xFFU);
        data[1] = (uint8_t)((st->pack_voltage_mv >> 16) & 0xFFU);
        data[2] = (uint8_t)((st->pack_voltage_mv >> 8) & 0xFFU);
        data[3] = (uint8_t)(st->pack_voltage_mv & 0xFFU);
        /* Pack Current (mA, int32_t) */
        data[4] = (uint8_t)(((uint32_t)st->pack_current_ma >> 24) & 0xFFU);
        data[5] = (uint8_t)(((uint32_t)st->pack_current_ma >> 16) & 0xFFU);
        data[6] = (uint8_t)(((uint32_t)st->pack_current_ma >> 8) & 0xFFU);
        data[7] = (uint8_t)((uint32_t)st->pack_current_ma & 0xFFU);
        /* State of Charge (0.1%) */
        data[8] = (uint8_t)((st->state_of_charge_permille >> 8) & 0xFFU);
        data[9] = (uint8_t)(st->state_of_charge_permille & 0xFFU);
        /* State of Health (0.1%) */
        data[10] = (uint8_t)((st->state_of_health_permille >> 8) & 0xFFU);
        data[11] = (uint8_t)(st->state_of_health_permille & 0xFFU);
        *length = 12U;
        return UDS_RESULT_OK;

    case BMS_DID_CELL_EXTREMES:
        if (capacity < 8U) return UDS_RESULT_RESPONSE_TOO_LONG;
        data[0] = (uint8_t)((st->cell_min_voltage_mv >> 8) & 0xFFU);
        data[1] = (uint8_t)(st->cell_min_voltage_mv & 0xFFU);
        data[2] = (uint8_t)((st->cell_max_voltage_mv >> 8) & 0xFFU);
        data[3] = (uint8_t)(st->cell_max_voltage_mv & 0xFFU);
        data[4] = (uint8_t)(((uint16_t)st->temp_min_deci_celsius >> 8) & 0xFFU);
        data[5] = (uint8_t)((uint16_t)st->temp_min_deci_celsius & 0xFFU);
        data[6] = (uint8_t)(((uint16_t)st->temp_max_deci_celsius >> 8) & 0xFFU);
        data[7] = (uint8_t)((uint16_t)st->temp_max_deci_celsius & 0xFFU);
        *length = 8U;
        return UDS_RESULT_OK;

    case BMS_DID_CONTACTOR_STATUS:
        if (capacity < 2U) return UDS_RESULT_RESPONSE_TOO_LONG;
        data[0] = (uint8_t)st->contactor_state;
        data[1] = (uint8_t)(st->hvil_closed ? 1U : 0U);
        *length = 2U;
        return UDS_RESULT_OK;

    case BMS_DID_ISOLATION_STATUS:
        if (capacity < 2U) return UDS_RESULT_RESPONSE_TOO_LONG;
        data[0] = (uint8_t)((st->isolation_resistance_kohm >> 8) & 0xFFU);
        data[1] = (uint8_t)(st->isolation_resistance_kohm & 0xFFU);
        *length = 2U;
        return UDS_RESULT_OK;

    default:
        return UDS_RESULT_NOT_SUPPORTED;
    }
}

static UdsCallbackResult bms_security_seed(void *context, uint8_t level, uint8_t *seed,
                                           uint16_t *length, uint16_t capacity)
{
    (void)context;
    if (level != 1U) {
        return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    }
    if (seed == NULL || length == NULL || capacity < 4U) {
        return UDS_RESULT_ERROR;
    }

    /* Fixed pseudo-random seed for demonstration */
    s_active_seed = 0x55AA1234U;
    seed[0] = (uint8_t)((s_active_seed >> 24) & 0xFFU);
    seed[1] = (uint8_t)((s_active_seed >> 16) & 0xFFU);
    seed[2] = (uint8_t)((s_active_seed >> 8) & 0xFFU);
    seed[3] = (uint8_t)(s_active_seed & 0xFFU);
    *length = 4U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult bms_security_key(void *context, uint8_t level, const uint8_t *key,
                                          uint16_t length)
{
    (void)context;
    if (level != 1U) {
        return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    }
    if (key == NULL || length < 4U) {
        return UDS_RESULT_INVALID_FORMAT;
    }

    uint32_t expected_key = s_active_seed ^ 0xA5A55A5AU;
    uint32_t received_key = ((uint32_t)key[0] << 24) |
                            ((uint32_t)key[1] << 16) |
                            ((uint32_t)key[2] << 8)  |
                            ((uint32_t)key[3]);

    if (received_key != expected_key) {
        return UDS_RESULT_INVALID_KEY;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult bms_routine_control(void *context, uint8_t subfunction, uint16_t routine_id,
                                             const uint8_t *request, uint16_t request_len,
                                             uint8_t *response, uint16_t *response_len,
                                             uint16_t capacity)
{
    UdsServer *srv = (UdsServer *)context;
    (void)request;
    (void)request_len;

    if (routine_id == BMS_ROUTINE_CONTACTOR_CYCLE) {
        /* Enforce security gate: must be unlocked at Security Level 1 */
        if (srv != NULL && uds_server_security_level(srv) < 1U) {
            return UDS_RESULT_SECURITY_DENIED;
        }

        if (subfunction != 0x01U) { /* startRoutine */
            return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
        }

        bool ok = bms_app_execute_contactor_cycle();
        if (!ok) {
            return UDS_RESULT_DENIED;
        }

        if (response != NULL && response_len != NULL && capacity >= 2U) {
            response[0] = subfunction;
            response[1] = 0x00U; /* Success */
            *response_len = 2U;
        }
        return UDS_RESULT_OK;
    }

    if (routine_id == BMS_ROUTINE_CALIBRATE_SOC) {
        if (subfunction != 0x01U) return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
        if (response != NULL && response_len != NULL && capacity >= 2U) {
            response[0] = subfunction;
            response[1] = 0x00U;
            *response_len = 2U;
        }
        return UDS_RESULT_OK;
    }

    return UDS_RESULT_NOT_SUPPORTED;
}

static UdsCallbackResult bms_read_dtc(void *context, uint8_t subfunction, const uint8_t *request,
                                      uint16_t request_len, uint8_t *response,
                                      uint16_t *response_len, uint16_t capacity)
{
    (void)context;
    (void)request;
    (void)request_len;

    if (response == NULL || response_len == NULL) {
        return UDS_RESULT_ERROR;
    }

    const bms_state_t *st = bms_app_get_state();

    if (subfunction == 0x01U) { /* reportNumberOfDTCByStatusMask */
        uint8_t count = 0U;
        if (st->fault_cell_overvoltage) count++;
        if (st->fault_cell_undervoltage) count++;
        if (st->fault_isolation_loss) count++;
        if (st->fault_contactor_welded) count++;

        if (capacity < 4U) return UDS_RESULT_RESPONSE_TOO_LONG;
        response[0] = 0x09U;
        response[1] = 0x01U;
        response[2] = 0x00U;
        response[3] = count;
        *response_len = 4U;
        return UDS_RESULT_OK;
    }

    if (subfunction == 0x02U) { /* reportDTCByStatusMask */
        uint16_t offset = 0U;
        if (capacity < 1U) return UDS_RESULT_RESPONSE_TOO_LONG;
        response[offset++] = 0x09U;

        if (st->fault_cell_overvoltage && (offset + 4U <= capacity)) {
            response[offset++] = (uint8_t)((BMS_DTC_CELL_OVERVOLTAGE >> 16) & 0xFFU);
            response[offset++] = (uint8_t)((BMS_DTC_CELL_OVERVOLTAGE >> 8) & 0xFFU);
            response[offset++] = (uint8_t)(BMS_DTC_CELL_OVERVOLTAGE & 0xFFU);
            response[offset++] = 0x2FU;
        }
        if (st->fault_isolation_loss && (offset + 4U <= capacity)) {
            response[offset++] = (uint8_t)((BMS_DTC_ISOLATION_LOSS >> 16) & 0xFFU);
            response[offset++] = (uint8_t)((BMS_DTC_ISOLATION_LOSS >> 8) & 0xFFU);
            response[offset++] = (uint8_t)(BMS_DTC_ISOLATION_LOSS & 0xFFU);
            response[offset++] = 0x2FU;
        }
        *response_len = offset;
        return UDS_RESULT_OK;
    }

    return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
}

static UdsCallbackResult bms_clear_dtc(void *context, uint32_t group_of_dtc)
{
    (void)context;
    (void)group_of_dtc;
    bms_app_inject_fault_ovp(false);
    bms_app_inject_fault_isolation(false);
    return UDS_RESULT_OK;
}

static UdsCallbacks s_bms_callbacks = {
    .read_did = bms_read_did,
    .write_did = NULL,
    .read_dtc = bms_read_dtc,
    .dtc_backend = NULL,
    .security_seed = bms_security_seed,
    .security_key = bms_security_key,
    .communication_control = NULL,
    .io_control = NULL,
    .routine_control = bms_routine_control,
    .request_download = NULL,
    .transfer_data = NULL,
    .request_transfer_exit = NULL,
    .ecu_reset = NULL,
    .ecu_reset_execute = NULL,
    .control_dtc_setting = NULL,
    .clear_dtc = bms_clear_dtc,
    .service_backends = NULL
};

void bms_uds_adapter_init(UdsServer *server, uint32_t now_ms)
{
    uds_server_init(server, &s_bms_callbacks, (void *)server, now_ms);
}

void bms_uds_adapter_poll(UdsServer *server, uint32_t now_ms)
{
    uds_server_tick(server, now_ms);
}
