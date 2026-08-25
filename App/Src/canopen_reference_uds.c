/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "canopen_reference_uds.h"

#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)

#include <stddef.h>
#include <string.h>

#include "canopen_reference_diagnostics.h"
#include "canopen_reference_timing.h"
#include "uds.h"
#include "uds_did.h"
#include "uds_download.h"
#include "uds_security_provider.h"

#define UDS_RUNTIME_MAX_RESPONSE UDS_MAX_RESPONSE_LENGTH
#define UDS_RUNTIME_ISOTP_BLOCK_SIZE 8U
#define UDS_RUNTIME_ISOTP_ST_MIN 0U

typedef struct {
    UdsStm32Can can;
    IsoTpConfig isotp_config;
    IsoTpRx isotp_rx;
    IsoTpTx isotp_tx;
    UdsServer server;
    UdsDidRegistry did_registry;
    UdsProjectDidSource did_source;
    UdsSecurityProvider security;
    UdsDownload download;
    uint8_t response[UDS_RUNTIME_MAX_RESPONSE];
    uint8_t software_version[10];
    uint8_t hardware_version[10];
    uint8_t bootloader_version[8];
    uint8_t serial_number[16];
    uint8_t device_identity[16];
    uint8_t canopen_node_id;
    uint8_t can_bitrate[4];
    uint8_t diagnostic_status[4];
    uint8_t firmware_status[4];
    uint8_t reset_cause[4];
    uint8_t watchdog_status[4];
    uint8_t can_error_counters[8];
    uint8_t canopen_error_state[4];
    uint16_t response_length;
    uint32_t now_ms;
    bool initialized;
} UdsRuntime;

static UdsRuntime s_uds;

static UdsCallbackResult map_did_result(UdsDidResult result) {
    switch (result) {
    case UDS_DID_OK:
        return UDS_RESULT_OK;
    case UDS_DID_SESSION_DENIED:
    case UDS_DID_SECURITY_DENIED:
        return UDS_RESULT_DENIED;
    case UDS_DID_NOT_FOUND:
    case UDS_DID_NOT_READABLE:
    case UDS_DID_NOT_WRITABLE:
    case UDS_DID_INVALID_WRITE:
        return UDS_RESULT_OUT_OF_RANGE;
    case UDS_DID_RESPONSE_TOO_LONG:
        return UDS_RESULT_RESPONSE_TOO_LONG;
    default:
        return UDS_RESULT_ERROR;
    }
}

static UdsCallbackResult uds_read_did(void *context, uint16_t did, uint8_t *data, uint16_t *length,
                                      uint16_t capacity) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if (runtime == NULL) {
        return UDS_RESULT_ERROR;
    }
    return map_did_result(uds_did_registry_read(
        &runtime->did_registry, did, (uint8_t)uds_server_session(&runtime->server),
        (uint8_t)uds_server_security_level(&runtime->server), data, length, capacity));
}

static UdsCallbackResult uds_write_did(void *context, uint16_t did, const uint8_t *data,
                                       uint16_t length) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if (runtime == NULL) {
        return UDS_RESULT_ERROR;
    }
    return map_did_result(uds_did_registry_write(
        &runtime->did_registry, did, (uint8_t)uds_server_session(&runtime->server),
        (uint8_t)uds_server_security_level(&runtime->server), data, length));
}

static UdsCallbackResult uds_read_dtc(void *context, uint8_t subfunction, const uint8_t *request,
                                      uint16_t request_len, uint8_t *response,
                                      uint16_t *response_len, uint16_t capacity) {
    (void)context;
    (void)request;
    (void)request_len;
    if ((response == NULL) || (response_len == NULL) || (capacity < 3U)) {
        return UDS_RESULT_RESPONSE_TOO_LONG;
    }
    if ((subfunction != 0x01U) && (subfunction != 0x02U) && (subfunction != 0x0AU)) {
        return UDS_RESULT_NOT_SUPPORTED;
    }
    response[0] = 0U;
    response[1] = 0U;
    response[2] = 0U;
    *response_len = 3U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult uds_security_seed(void *context, uint8_t level, uint8_t *seed,
                                           uint16_t *length, uint16_t capacity) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if (runtime == NULL) {
        return UDS_RESULT_ERROR;
    }
    UdsSecurityResult result = uds_security_provider_generate_seed(
        &runtime->security, level, seed, length, capacity, runtime->now_ms);
    switch (result) {
    case UDS_SECURITY_OK:
        return UDS_RESULT_OK;
    case UDS_SECURITY_DELAY_ACTIVE:
        return UDS_RESULT_DELAY_ACTIVE;
    case UDS_SECURITY_BUFFER_TOO_SMALL:
        return UDS_RESULT_RESPONSE_TOO_LONG;
    default:
        return UDS_RESULT_DENIED;
    }
}

static UdsCallbackResult uds_security_key(void *context, uint8_t level, const uint8_t *key,
                                          uint16_t length) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if (runtime == NULL) {
        return UDS_RESULT_ERROR;
    }
    UdsSecurityResult result =
        uds_security_provider_verify_key(&runtime->security, level, key, length, runtime->now_ms);
    switch (result) {
    case UDS_SECURITY_OK:
        return UDS_RESULT_OK;
    case UDS_SECURITY_INVALID_KEY:
        return UDS_RESULT_INVALID_KEY;
    case UDS_SECURITY_ATTEMPTS_EXCEEDED:
        return UDS_RESULT_ATTEMPTS_EXCEEDED;
    case UDS_SECURITY_DELAY_ACTIVE:
        return UDS_RESULT_DELAY_ACTIVE;
    default:
        return UDS_RESULT_DENIED;
    }
}

static UdsCallbackResult uds_communication_control(void *context, uint8_t subfunction,
                                                   uint8_t communication_type) {
    (void)context;
    (void)subfunction;
    (void)communication_type;
    /* The reference application has no independent communication-control
     * gate. A product must replace this callback before enabling 0x28. */
    return UDS_RESULT_NOT_SUPPORTED;
}

static UdsCallbackResult uds_routine_control(void *context, uint8_t subfunction,
                                             uint16_t routine_id, const uint8_t *request,
                                             uint16_t request_len, uint8_t *response,
                                             uint16_t *response_len, uint16_t capacity) {
    (void)context;
    (void)subfunction;
    (void)routine_id;
    (void)request;
    (void)request_len;
    (void)response;
    (void)response_len;
    (void)capacity;
    return UDS_RESULT_NOT_SUPPORTED;
}

static UdsCallbackResult uds_ecu_reset(void *context, uint8_t subfunction) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if ((runtime == NULL) || (subfunction != 0x01U)) {
        return UDS_RESULT_NOT_SUPPORTED;
    }
    if (uds_server_security_level(&runtime->server) == 0U) {
        return UDS_RESULT_DENIED;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult uds_dtc_setting(void *context, uint8_t subfunction) {
    (void)context;
    return ((subfunction == 0x01U) || (subfunction == 0x02U)) ? UDS_RESULT_OK
                                                              : UDS_RESULT_NOT_SUPPORTED;
}

static UdsCallbackResult uds_request_download(void *context, uint32_t address, uint32_t length,
                                              uint16_t *max_block_length) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if ((runtime == NULL) || (max_block_length == NULL)) {
        return UDS_RESULT_ERROR;
    }
    UdsDownloadResult result =
        uds_download_begin(&runtime->download, address, length, runtime->now_ms);
    if (result != UDS_DOWNLOAD_OK) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    *max_block_length = runtime->download.memory.max_block_length;
    return UDS_RESULT_OK;
}

static UdsCallbackResult uds_transfer_data(void *context, uint8_t block_sequence_counter,
                                           const uint8_t *data, uint16_t length) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if (runtime == NULL) {
        return UDS_RESULT_ERROR;
    }
    UdsDownloadResult result = uds_download_write(&runtime->download, block_sequence_counter, data,
                                                  length, runtime->now_ms);
    return (result == UDS_DOWNLOAD_OK) ? UDS_RESULT_OK : UDS_RESULT_PROGRAMMING_FAILURE;
}

static UdsCallbackResult uds_transfer_exit(void *context, const uint8_t *request,
                                           uint16_t request_len, uint8_t *response,
                                           uint16_t *response_len, uint16_t capacity) {
    UdsRuntime *runtime = (UdsRuntime *)context;
    if ((runtime == NULL) || (response == NULL) || (response_len == NULL) || (capacity < 1U)) {
        return UDS_RESULT_ERROR;
    }
    uint32_t expected_crc32 = 0U;
    bool has_crc = false;
    if (request_len == 5U) {
        expected_crc32 = ((uint32_t)request[1] << 24U) | ((uint32_t)request[2] << 16U) |
                         ((uint32_t)request[3] << 8U) | request[4];
        has_crc = true;
    } else if (request_len != 1U) {
        return UDS_RESULT_ERROR;
    }
    UdsDownloadResult result =
        uds_download_finish(&runtime->download, expected_crc32, has_crc, runtime->now_ms);
    if (result != UDS_DOWNLOAD_OK) {
        return UDS_RESULT_PROGRAMMING_FAILURE;
    }
    response[0] = 0U;
    *response_len = 1U;
    return UDS_RESULT_OK;
}

static void fill_project_dids(UdsRuntime *runtime) {
    static const uint8_t software[] = "uds-ref-1";
    static const uint8_t hardware[] = "stm32f767";
    static const uint8_t bootloader[] = "none";
    static const uint8_t serial[] = "reference";
    static const uint8_t identity[] = "CANopen-UDS";
    (void)memcpy(runtime->software_version, software, sizeof(software) - 1U);
    (void)memcpy(runtime->hardware_version, hardware, sizeof(hardware) - 1U);
    (void)memcpy(runtime->bootloader_version, bootloader, sizeof(bootloader) - 1U);
    (void)memcpy(runtime->serial_number, serial, sizeof(serial) - 1U);
    (void)memcpy(runtime->device_identity, identity, sizeof(identity) - 1U);
    runtime->did_source.software_version =
        (UdsDidValue){runtime->software_version, (uint16_t)(sizeof(software) - 1U)};
    runtime->did_source.hardware_version =
        (UdsDidValue){runtime->hardware_version, (uint16_t)(sizeof(hardware) - 1U)};
    runtime->did_source.bootloader_version =
        (UdsDidValue){runtime->bootloader_version, (uint16_t)(sizeof(bootloader) - 1U)};
    runtime->did_source.serial_number =
        (UdsDidValue){runtime->serial_number, (uint16_t)(sizeof(serial) - 1U)};
    runtime->did_source.device_identity =
        (UdsDidValue){runtime->device_identity, (uint16_t)(sizeof(identity) - 1U)};
    runtime->did_source.canopen_node_id = (UdsDidValue){&runtime->canopen_node_id, 1U};
    runtime->did_source.can_bitrate =
        (UdsDidValue){runtime->can_bitrate, sizeof(runtime->can_bitrate)};
    runtime->did_source.diagnostic_status =
        (UdsDidValue){runtime->diagnostic_status, sizeof(runtime->diagnostic_status)};
    runtime->did_source.firmware_status =
        (UdsDidValue){runtime->firmware_status, sizeof(runtime->firmware_status)};
    runtime->did_source.reset_cause =
        (UdsDidValue){runtime->reset_cause, sizeof(runtime->reset_cause)};
    runtime->did_source.watchdog_status =
        (UdsDidValue){runtime->watchdog_status, sizeof(runtime->watchdog_status)};
    runtime->did_source.can_error_counters =
        (UdsDidValue){runtime->can_error_counters, sizeof(runtime->can_error_counters)};
    runtime->did_source.canopen_error_state =
        (UdsDidValue){runtime->canopen_error_state, sizeof(runtime->canopen_error_state)};
    runtime->canopen_node_id = 1U;
    runtime->can_bitrate[0] = 0x00U;
    runtime->can_bitrate[1] = 0x07U;
    runtime->can_bitrate[2] = 0xA1U;
    runtime->can_bitrate[3] = 0x20U;
    uds_did_registry_init(&runtime->did_registry, &runtime->did_source);
}

int CANopenReference_UDS_Init(CAN_HandleTypeDef *hcan, uint32_t now_ms) {
    if (hcan == NULL) {
        return -1;
    }
    (void)memset(&s_uds, 0, sizeof(s_uds));
    if (uds_stm32_can_bind(&s_uds.can, hcan, UDS_RX_CAN_ID, UDS_TX_CAN_ID) != 0) {
        return -2;
    }
    if (uds_stm32_can_attach(&s_uds.can) != 0) {
        return -3;
    }
    isotp_config_default(&s_uds.isotp_config);
    s_uds.isotp_config.block_size = UDS_RUNTIME_ISOTP_BLOCK_SIZE;
    s_uds.isotp_config.st_min = UDS_RUNTIME_ISOTP_ST_MIN;
    isotp_rx_init(&s_uds.isotp_rx, &s_uds.isotp_config, UDS_RX_CAN_ID, UDS_TX_CAN_ID);
    isotp_tx_init(&s_uds.isotp_tx, &s_uds.isotp_config, UDS_RX_CAN_ID, UDS_TX_CAN_ID);
    fill_project_dids(&s_uds);
    uds_security_provider_init(&s_uds.security, 0x13579BDFUL, 3U, 10000U);
    UdsDownloadMemoryMap memory = {
        .staging_image = {0x08080000UL, 0x080C0000UL},
        .bootloader = {0x08000000UL, 0x08040000UL},
        .active_application = {0x08040000UL, 0x08080000UL},
        .canopen_nvm = {0x08180000UL, 0x08200000UL},
        .diagnostic_storage = {0x08170000UL, 0x08180000UL},
        .erase_alignment = 4U,
        .program_alignment = 4U,
        .max_block_length = 256U,
        .activation_supported = false,
    };
    uds_download_init(&s_uds.download, &memory, NULL, &s_uds);
    UdsCallbacks callbacks = {
        .read_did = uds_read_did,
        .write_did = uds_write_did,
        .read_dtc = uds_read_dtc,
        .security_seed = uds_security_seed,
        .security_key = uds_security_key,
        .communication_control = uds_communication_control,
        .io_control = NULL,
        .routine_control = uds_routine_control,
        .request_download = uds_request_download,
        .transfer_data = uds_transfer_data,
        .request_transfer_exit = uds_transfer_exit,
        .ecu_reset = uds_ecu_reset,
        .control_dtc_setting = uds_dtc_setting,
    };
    uds_server_init(&s_uds.server, &callbacks, &s_uds, now_ms);
    s_uds.now_ms = now_ms;
    s_uds.initialized = true;
    return 0;
}

void CANopenReference_UDS_RxFromIsr(uint32_t id, const uint8_t *data, uint8_t dlc) {
    if (s_uds.initialized) {
        uds_stm32_can_rx_from_isr(&s_uds.can, id, data, dlc);
    }
}

static void queue_tx_frame(const IsoTpCanFrame *frame) {
    if ((frame != NULL) && (uds_stm32_can_tx_queue(&s_uds.can, frame) != 0)) {
        uds_stm32_can_note_tx_timeout(&s_uds.can);
    }
}

void CANopenReference_UDS_Process(uint32_t now_ms) {
    if (!s_uds.initialized) {
        return;
    }
    uint32_t uds_mainline_start = CANopenReferenceTiming_PhaseEnter();
    s_uds.now_ms = now_ms;
    (void)uds_server_tick(&s_uds.server, now_ms);
    UdsDownloadResult download_state = uds_download_tick(&s_uds.download, now_ms);
    if (download_state == UDS_DOWNLOAD_TIMEOUT) {
        uds_stm32_can_note_rx_timeout(&s_uds.can);
    }

    for (uint32_t budget = 0U; budget < UDS_STM32_RX_BUDGET_PER_CALL; ++budget) {
        IsoTpCanFrame frame = {0};
        uint32_t rx_start = CANopenReferenceTiming_PhaseEnter();
        int rx_result = uds_stm32_can_rx_pop(&s_uds.can, &frame);
        CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.uds_rx_cycles_max, rx_start);
        if (rx_result != 1) {
            break;
        }
        if (frame.can_id == UDS_RX_CAN_ID) {
            IsoTpRxEvent event = {0};
            uint32_t isotp_start = CANopenReferenceTiming_PhaseEnter();
            IsoTpStatus status = isotp_rx_feed(&s_uds.isotp_rx, &frame, now_ms, &event);
            CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.isotp_cycles_max,
                                             isotp_start);
            if (status == ISOTP_NEED_FLOW_CONTROL && event.has_flow_control) {
                queue_tx_frame(&event.flow_control);
            }
            if (status == ISOTP_COMPLETE && (event.payload != NULL)) {
                s_uds.response_length = 0U;
                uint32_t dispatch_start = CANopenReferenceTiming_PhaseEnter();
                UdsCallbackResult result =
                    uds_server_handle(&s_uds.server, event.payload, event.length, s_uds.response,
                                      &s_uds.response_length, sizeof(s_uds.response), now_ms);
                CANopenReferenceTiming_PhaseExit(
                    &canopenReferenceTimingStats.uds_dispatch_cycles_max, dispatch_start);
                if ((result == UDS_RESULT_OK) && (s_uds.response_length != 0U)) {
                    IsoTpCanFrame tx_frame = {0};
                    if (isotp_tx_start(&s_uds.isotp_tx, s_uds.response, s_uds.response_length,
                                       now_ms, &tx_frame) == ISOTP_TX_FRAME_READY) {
                        uint32_t tx_start = CANopenReferenceTiming_PhaseEnter();
                        queue_tx_frame(&tx_frame);
                        CANopenReferenceTiming_PhaseExit(
                            &canopenReferenceTimingStats.uds_tx_cycles_max, tx_start);
                    }
                }
            }
        } else if (frame.can_id == UDS_TX_CAN_ID) {
            (void)isotp_tx_feed_flow_control(&s_uds.isotp_tx, &frame, now_ms);
        }
    }

    IsoTpCanFrame next_frame = {0};
    uint32_t isotp_tx_start = CANopenReferenceTiming_PhaseEnter();
    IsoTpStatus tx_status = isotp_tx_next(&s_uds.isotp_tx, now_ms, &next_frame);
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.isotp_cycles_max, isotp_tx_start);
    if (tx_status == ISOTP_TX_FRAME_READY) {
        uint32_t tx_start = CANopenReferenceTiming_PhaseEnter();
        queue_tx_frame(&next_frame);
        CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.uds_tx_cycles_max, tx_start);
    }
    if (isotp_rx_tick(&s_uds.isotp_rx, now_ms) == ISOTP_ERR_TIMEOUT) {
        uds_stm32_can_note_rx_timeout(&s_uds.can);
    }
    if (isotp_tx_tick(&s_uds.isotp_tx, now_ms) == ISOTP_ERR_TIMEOUT) {
        uds_stm32_can_note_tx_timeout(&s_uds.can);
    }
    uint32_t tx_start = CANopenReferenceTiming_PhaseEnter();
    (void)uds_stm32_can_process_tx(&s_uds.can, UDS_STM32_TX_BUDGET_PER_CALL);
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.uds_tx_cycles_max, tx_start);
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.uds_mainline_cycles_max,
                                     uds_mainline_start);
}

bool CANopenReference_UDS_ResetPending(void) {
    return s_uds.initialized && uds_server_reset_pending(&s_uds.server);
}

void CANopenReference_UDS_ClearReset(void) {
    if (s_uds.initialized) {
        uds_server_clear_reset(&s_uds.server);
    }
}

void CANopenReference_UDS_GetStats(UdsStm32CanStats *stats) {
    if (s_uds.initialized) {
        uds_stm32_can_get_stats(&s_uds.can, stats);
    }
}

#endif
