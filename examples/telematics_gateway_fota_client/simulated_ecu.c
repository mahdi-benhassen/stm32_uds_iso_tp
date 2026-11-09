#include "simulated_ecu.h"
#include <string.h>

static UdsCallbackResult ecu_security_seed(void *context, uint8_t level, uint8_t *seed,
                                           uint16_t *length, uint16_t capacity)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    if (level != 1U) return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    if (seed == NULL || length == NULL || capacity < 4U) return UDS_RESULT_ERROR;

    ecu->active_seed = 0x11223344U;
    seed[0] = (uint8_t)((ecu->active_seed >> 24) & 0xFFU);
    seed[1] = (uint8_t)((ecu->active_seed >> 16) & 0xFFU);
    seed[2] = (uint8_t)((ecu->active_seed >> 8) & 0xFFU);
    seed[3] = (uint8_t)(ecu->active_seed & 0xFFU);
    *length = 4U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_security_key(void *context, uint8_t level, const uint8_t *key,
                                          uint16_t length)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    if (level != 1U) return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    if (key == NULL || length < 4U) return UDS_RESULT_INVALID_FORMAT;

    uint32_t expected = ecu->active_seed ^ 0xDEAD1234U;
    uint32_t got = ((uint32_t)key[0] << 24) | ((uint32_t)key[1] << 16) |
                   ((uint32_t)key[2] << 8)  | (uint32_t)key[3];
    return (got == expected) ? UDS_RESULT_OK : UDS_RESULT_INVALID_KEY;
}

static UdsCallbackResult ecu_comm_control(void *context, uint8_t subfunction,
                                          uint8_t communication_type)
{
    (void)context;
    (void)subfunction;
    (void)communication_type;
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_control_dtc_setting(void *context, uint8_t subfunction)
{
    (void)context;
    (void)subfunction;
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_routine_control(void *context, uint8_t subfunction,
                                             uint16_t routine_id, const uint8_t *request,
                                             uint16_t request_len, uint8_t *response,
                                             uint16_t *response_len, uint16_t capacity)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    (void)request;
    (void)request_len;

    if (subfunction != 0x01U) { /* startRoutine */
        return UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED;
    }

    if (routine_id == 0xFF00U) { /* EraseMemory */
        ecu->memory_erased = true;
        if (response && response_len && capacity >= 2U) {
            response[0] = subfunction;
            response[1] = 0x00U;
            *response_len = 2U;
        }
        return UDS_RESULT_OK;
    }

    if (routine_id == 0x0202U) { /* CheckProgrammingDependencies */
        ecu->flash_verified = true;
        if (response && response_len && capacity >= 2U) {
            response[0] = subfunction;
            response[1] = 0x00U;
            *response_len = 2U;
        }
        return UDS_RESULT_OK;
    }

    return UDS_RESULT_NOT_SUPPORTED;
}

static UdsCallbackResult ecu_request_download(void *context, uint32_t address,
                                              uint32_t length, uint16_t *max_block_length)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    ecu->download_address = address;
    ecu->download_size = length;
    ecu->bytes_received = 0U;
    if (max_block_length) {
        *max_block_length = 256U;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_transfer_data(void *context, uint8_t block_sequence_counter,
                                           const uint8_t *data, uint16_t length)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    (void)block_sequence_counter;

    if (data && length > 0U && (ecu->bytes_received + length <= sizeof(ecu->flash_buffer))) {
        memcpy(&ecu->flash_buffer[ecu->bytes_received], data, length);
        ecu->bytes_received += length;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_request_transfer_exit(void *context, const uint8_t *request,
                                                   uint16_t request_len, uint8_t *response,
                                                   uint16_t *response_len, uint16_t capacity)
{
    (void)context;
    (void)request;
    (void)request_len;
    (void)response;
    (void)response_len;
    (void)capacity;
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_reset(void *context, uint8_t subfunction)
{
    simulated_ecu_t *ecu = (simulated_ecu_t *)context;
    (void)subfunction;
    ecu->reset_triggered = true;
    return UDS_RESULT_OK;
}

static UdsCallbacks s_ecu_callbacks = {
    .read_did = NULL,
    .write_did = NULL,
    .read_dtc = NULL,
    .dtc_backend = NULL,
    .security_seed = ecu_security_seed,
    .security_key = ecu_security_key,
    .communication_control = ecu_comm_control,
    .io_control = NULL,
    .routine_control = ecu_routine_control,
    .request_download = ecu_request_download,
    .transfer_data = ecu_transfer_data,
    .request_transfer_exit = ecu_request_transfer_exit,
    .ecu_reset = ecu_reset,
    .ecu_reset_execute = NULL,
    .control_dtc_setting = ecu_control_dtc_setting,
    .clear_dtc = NULL,
    .service_backends = NULL
};

void simulated_ecu_init(simulated_ecu_t *ecu, uint32_t now_ms)
{
    if (ecu == NULL) return;
    memset(ecu, 0, sizeof(*ecu));
    uds_server_init(&ecu->server, &s_ecu_callbacks, (void *)ecu, now_ms);
}

UdsCallbackResult simulated_ecu_process_request(simulated_ecu_t *ecu,
                                                const uint8_t *req, uint16_t req_len,
                                                uint8_t *resp, uint16_t *resp_len,
                                                uint16_t resp_cap, uint32_t now_ms)
{
    if (ecu == NULL) return UDS_RESULT_ERROR;
    return uds_server_handle(&ecu->server, req, req_len, resp, resp_len, resp_cap, now_ms);
}
