#include "bcm_uds_adapter.h"
#include "bcm_app.h"
#include <string.h>

static const char BCM_VIN_STRING[17] = "1HGCR2F83HA000001";
static const char BCM_PART_NUM_STRING[14] = "BCM-STM32-01A";

static UdsCallbackResult bcm_read_did(void *context, uint16_t did, uint8_t *data,
                                      uint16_t *length, uint16_t capacity)
{
    (void)context;
    const bcm_state_t *st = bcm_app_get_state();

    if (data == NULL || length == NULL) {
        return UDS_RESULT_ERROR;
    }

    switch (did) {
    case BCM_DID_STATE_SNAPSHOT:
        if (capacity < 6U) {
            return UDS_RESULT_RESPONSE_TOO_LONG;
        }
        data[0] = (uint8_t)st->headlights;
        data[1] = (uint8_t)(st->high_beam_active ? 1U : 0U);
        data[2] = (uint8_t)(st->hazard_lights_active ? 1U : 0U);
        data[3] = (uint8_t)(st->horn_active ? 1U : 0U);
        data[4] = (uint8_t)st->wipers;
        data[5] = (uint8_t)((st->driver_door_locked ? 1U : 0U) |
                            (st->passenger_door_locked ? 2U : 0U) |
                            (st->driver_door_ajar ? 4U : 0U) |
                            (st->passenger_door_ajar ? 8U : 0U));
        *length = 6U;
        return UDS_RESULT_OK;

    case BCM_DID_POWER_ENVIRONMENT:
        if (capacity < 4U) {
            return UDS_RESULT_RESPONSE_TOO_LONG;
        }
        data[0] = (uint8_t)((st->battery_voltage_mv >> 8) & 0xFFU);
        data[1] = (uint8_t)(st->battery_voltage_mv & 0xFFU);
        data[2] = (uint8_t)(((uint16_t)st->ambient_temp_deci_celsius >> 8) & 0xFFU);
        data[3] = (uint8_t)((uint16_t)st->ambient_temp_deci_celsius & 0xFFU);
        *length = 4U;
        return UDS_RESULT_OK;

    case BCM_DID_VIN:
        if (capacity < sizeof(BCM_VIN_STRING)) {
            return UDS_RESULT_RESPONSE_TOO_LONG;
        }
        memcpy(data, BCM_VIN_STRING, sizeof(BCM_VIN_STRING));
        *length = (uint16_t)sizeof(BCM_VIN_STRING);
        return UDS_RESULT_OK;

    case BCM_DID_PART_NUMBER:
        if (capacity < sizeof(BCM_PART_NUM_STRING)) {
            return UDS_RESULT_RESPONSE_TOO_LONG;
        }
        memcpy(data, BCM_PART_NUM_STRING, sizeof(BCM_PART_NUM_STRING));
        *length = (uint16_t)sizeof(BCM_PART_NUM_STRING);
        return UDS_RESULT_OK;

    default:
        return UDS_RESULT_NOT_SUPPORTED;
    }
}

static UdsCallbackResult bcm_io_control(void *context, uint16_t did, const uint8_t *parameter,
                                        uint16_t parameter_len, uint8_t *response,
                                        uint16_t *response_len, uint16_t capacity)
{
    (void)context;
    if (parameter == NULL || parameter_len < 1U) {
        return UDS_RESULT_INVALID_FORMAT;
    }

    uint8_t subfunction = parameter[0];

    switch (did) {
    case BCM_DID_IO_HEADLIGHT:
        if (subfunction == 0x00U) { /* returnControlToECU */
            bcm_app_io_override_headlight(false, BCM_LIGHT_OFF);
        } else if (subfunction == 0x03U) { /* shortTermAdjustment */
            if (parameter_len < 2U) {
                return UDS_RESULT_INVALID_FORMAT;
            }
            bcm_app_io_override_headlight(true, (bcm_light_state_t)parameter[1]);
        } else {
            return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
        }
        break;

    case BCM_DID_IO_HORN:
        if (subfunction == 0x00U) {
            bcm_app_io_override_horn(false, false);
        } else if (subfunction == 0x03U) {
            if (parameter_len < 2U) {
                return UDS_RESULT_INVALID_FORMAT;
            }
            bcm_app_io_override_horn(true, parameter[1] != 0U);
        } else {
            return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
        }
        break;

    case BCM_DID_IO_HAZARD:
        if (subfunction == 0x00U) {
            bcm_app_io_override_hazard(false, false);
        } else if (subfunction == 0x03U) {
            if (parameter_len < 2U) {
                return UDS_RESULT_INVALID_FORMAT;
            }
            bcm_app_io_override_hazard(true, parameter[1] != 0U);
        } else {
            return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
        }
        break;

    default:
        return UDS_RESULT_NOT_SUPPORTED;
    }

    if (response != NULL && response_len != NULL && capacity >= 1U) {
        response[0] = subfunction;
        *response_len = 1U;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult bcm_routine_control(void *context, uint8_t subfunction, uint16_t routine_id,
                                             const uint8_t *request, uint16_t request_len,
                                             uint8_t *response, uint16_t *response_len,
                                             uint16_t capacity)
{
    (void)context;
    (void)request;
    (void)request_len;

    if (routine_id != BCM_ROUTINE_WIPER_SWEEP_TEST) {
        return UDS_RESULT_NOT_SUPPORTED;
    }

    if (subfunction != 0x01U) { /* startRoutine */
        return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    }

    if (response != NULL && response_len != NULL && capacity >= 2U) {
        response[0] = subfunction;
        response[1] = 0x00U; /* Routine started successfully */
        *response_len = 2U;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult bcm_read_dtc(void *context, uint8_t subfunction, const uint8_t *request,
                                      uint16_t request_len, uint8_t *response,
                                      uint16_t *response_len, uint16_t capacity)
{
    (void)context;
    (void)request;
    (void)request_len;

    if (response == NULL || response_len == NULL) {
        return UDS_RESULT_ERROR;
    }

    const bcm_state_t *st = bcm_app_get_state();

    if (subfunction == 0x01U) { /* reportNumberOfDTCByStatusMask */
        uint8_t count = 0U;
        if (st->fault_light_short_circuit) count++;
        if (st->fault_door_lock_stall) count++;
        if (st->fault_battery_low) count++;

        if (capacity < 4U) return UDS_RESULT_RESPONSE_TOO_LONG;
        response[0] = 0x09U; /* DTCStatusAvailabilityMask */
        response[1] = 0x01U; /* ISO 14229-1 DTC format */
        response[2] = 0x00U; /* count high */
        response[3] = count; /* count low */
        *response_len = 4U;
        return UDS_RESULT_OK;
    }

    if (subfunction == 0x02U) { /* reportDTCByStatusMask */
        uint16_t offset = 0U;
        if (capacity < 1U) return UDS_RESULT_RESPONSE_TOO_LONG;
        response[offset++] = 0x09U; /* availability mask */

        if (st->fault_light_short_circuit && (offset + 4U <= capacity)) {
            response[offset++] = (uint8_t)((BCM_DTC_LIGHT_CIRCUIT_FAULT >> 16) & 0xFFU);
            response[offset++] = (uint8_t)((BCM_DTC_LIGHT_CIRCUIT_FAULT >> 8) & 0xFFU);
            response[offset++] = (uint8_t)(BCM_DTC_LIGHT_CIRCUIT_FAULT & 0xFFU);
            response[offset++] = 0x29U; /* ConfirmedDTC | TestFailed */
        }
        if (st->fault_door_lock_stall && (offset + 4U <= capacity)) {
            response[offset++] = (uint8_t)((BCM_DTC_DOOR_ACTUATOR_STALL >> 16) & 0xFFU);
            response[offset++] = (uint8_t)((BCM_DTC_DOOR_ACTUATOR_STALL >> 8) & 0xFFU);
            response[offset++] = (uint8_t)(BCM_DTC_DOOR_ACTUATOR_STALL & 0xFFU);
            response[offset++] = 0x29U;
        }
        if (st->fault_battery_low && (offset + 4U <= capacity)) {
            response[offset++] = (uint8_t)((BCM_DTC_BATTERY_VOLTAGE_LOW >> 16) & 0xFFU);
            response[offset++] = (uint8_t)((BCM_DTC_BATTERY_VOLTAGE_LOW >> 8) & 0xFFU);
            response[offset++] = (uint8_t)(BCM_DTC_BATTERY_VOLTAGE_LOW & 0xFFU);
            response[offset++] = 0x29U;
        }
        *response_len = offset;
        return UDS_RESULT_OK;
    }

    return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
}

static UdsCallbackResult bcm_clear_dtc(void *context, uint32_t group_of_dtc)
{
    (void)context;
    (void)group_of_dtc;
    /* Clear active injected faults */
    bcm_app_inject_fault_light_short(false);
    bcm_app_inject_fault_door_stall(false);
    return UDS_RESULT_OK;
}

static UdsCallbackResult bcm_ecu_reset(void *context, uint8_t subfunction)
{
    (void)context;
    (void)subfunction;
    /* Reset BCM application state */
    bcm_app_init();
    return UDS_RESULT_OK;
}

static UdsCallbacks s_bcm_callbacks = {
    .read_did = bcm_read_did,
    .write_did = NULL,
    .read_dtc = bcm_read_dtc,
    .dtc_backend = NULL,
    .security_seed = NULL,
    .security_key = NULL,
    .communication_control = NULL,
    .io_control = bcm_io_control,
    .routine_control = bcm_routine_control,
    .request_download = NULL,
    .transfer_data = NULL,
    .request_transfer_exit = NULL,
    .ecu_reset = bcm_ecu_reset,
    .ecu_reset_execute = NULL,
    .control_dtc_setting = NULL,
    .clear_dtc = bcm_clear_dtc,
    .service_backends = NULL
};

void bcm_uds_adapter_init(UdsServer *server, uint32_t now_ms)
{
    uds_server_init(server, &s_bcm_callbacks, NULL, now_ms);
}

void bcm_uds_adapter_poll(UdsServer *server, uint32_t now_ms)
{
    uds_server_tick(server, now_ms);
}
