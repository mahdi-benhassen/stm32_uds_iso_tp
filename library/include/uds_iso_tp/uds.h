/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_UDS_H
#define STM32_UDS_ISO_TP_UDS_H

#include <stdbool.h>
#include <stdint.h>

#ifndef UDS_MAX_REQUEST_LENGTH
#define UDS_MAX_REQUEST_LENGTH 4095U
#endif

#ifndef UDS_MAX_RESPONSE_LENGTH
#define UDS_MAX_RESPONSE_LENGTH 4095U
#endif

#if (UDS_MAX_REQUEST_LENGTH > UINT16_MAX) || (UDS_MAX_RESPONSE_LENGTH > UINT16_MAX)
#error "UDS_MAX_*_LENGTH must fit the standalone UDS uint16_t callback API"
#endif
_Static_assert(UDS_MAX_REQUEST_LENGTH <= UINT16_MAX, "UDS request bound exceeds uint16_t API");
_Static_assert(UDS_MAX_RESPONSE_LENGTH <= UINT16_MAX, "UDS response bound exceeds uint16_t API");

#ifndef UDS_DEFAULT_P2_SERVER_MS
#define UDS_DEFAULT_P2_SERVER_MS 50U
#endif

#ifndef UDS_DEFAULT_P2_STAR_SERVER_MS
#define UDS_DEFAULT_P2_STAR_SERVER_MS 5000U
#endif

#ifndef UDS_DEFAULT_S3_SERVER_MS
#define UDS_DEFAULT_S3_SERVER_MS 5000U
#endif

#define UDS_SESSION_DEFAULT 0x01U
#define UDS_SESSION_PROGRAMMING 0x02U
#define UDS_SESSION_EXTENDED 0x03U

#define UDS_POSITIVE_RESPONSE_MASK 0x40U
#define UDS_SUPPRESS_POSITIVE_RESPONSE 0x80U

#define UDS_NRC_SERVICE_NOT_SUPPORTED 0x11U
#define UDS_NRC_SUBFUNCTION_NOT_SUPPORTED 0x12U
#define UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT 0x13U
#define UDS_NRC_RESPONSE_TOO_LONG 0x14U
#define UDS_NRC_BUSY_REPEAT_REQUEST 0x21U
#define UDS_NRC_CONDITIONS_NOT_CORRECT 0x22U
#define UDS_NRC_REQUEST_SEQUENCE_ERROR 0x24U
#define UDS_NRC_REQUEST_OUT_OF_RANGE 0x31U
#define UDS_NRC_SECURITY_ACCESS_DENIED 0x33U
#define UDS_NRC_INVALID_KEY 0x35U
#define UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS 0x36U
#define UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED 0x37U
#define UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED 0x70U
#define UDS_NRC_TRANSFER_DATA_SUSPENDED 0x71U
#define UDS_NRC_GENERAL_PROGRAMMING_FAILURE 0x72U
#define UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER 0x73U
#define UDS_NRC_REQUEST_CORRECTLY_RECEIVED_RESPONSE_PENDING 0x78U
#define UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION 0x7FU

#ifndef UDS_ENABLE_SESSION_CONTROL
#define UDS_ENABLE_SESSION_CONTROL 1U
#endif
#ifndef UDS_ENABLE_ECU_RESET
#define UDS_ENABLE_ECU_RESET 1U
#endif
#ifndef UDS_ENABLE_READ_DTC_INFORMATION
#define UDS_ENABLE_READ_DTC_INFORMATION 1U
#endif
#ifndef UDS_ENABLE_READ_DATA_BY_IDENTIFIER
#define UDS_ENABLE_READ_DATA_BY_IDENTIFIER 1U
#endif
#ifndef UDS_ENABLE_SECURITY_ACCESS
#define UDS_ENABLE_SECURITY_ACCESS 1U
#endif
#ifndef UDS_ENABLE_COMMUNICATION_CONTROL
#define UDS_ENABLE_COMMUNICATION_CONTROL 1U
#endif
#ifndef UDS_ENABLE_IO_CONTROL_BY_IDENTIFIER
#define UDS_ENABLE_IO_CONTROL_BY_IDENTIFIER 0U
#endif
#ifndef UDS_ENABLE_ROUTINE_CONTROL
#define UDS_ENABLE_ROUTINE_CONTROL 1U
#endif
#ifndef UDS_ENABLE_REQUEST_DOWNLOAD
#define UDS_ENABLE_REQUEST_DOWNLOAD 1U
#endif
#ifndef UDS_ENABLE_TRANSFER_DATA
#define UDS_ENABLE_TRANSFER_DATA 1U
#endif
#ifndef UDS_ENABLE_REQUEST_TRANSFER_EXIT
#define UDS_ENABLE_REQUEST_TRANSFER_EXIT 1U
#endif
#ifndef UDS_ENABLE_TESTER_PRESENT
#define UDS_ENABLE_TESTER_PRESENT 1U
#endif
#ifndef UDS_ENABLE_CONTROL_DTC_SETTING
#define UDS_ENABLE_CONTROL_DTC_SETTING 1U
#endif

typedef enum {
    UDS_RESULT_OK = 0,
    UDS_RESULT_NO_RESPONSE,
    UDS_RESULT_NOT_SUPPORTED,
    UDS_RESULT_DENIED,
    UDS_RESULT_OUT_OF_RANGE,
    UDS_RESULT_BUSY,
    UDS_RESULT_SEQUENCE_ERROR,
    UDS_RESULT_INVALID_KEY,
    UDS_RESULT_ATTEMPTS_EXCEEDED,
    UDS_RESULT_DELAY_ACTIVE,
    UDS_RESULT_PROGRAMMING_FAILURE,
    UDS_RESULT_RESPONSE_TOO_LONG,
    UDS_RESULT_ERROR
} UdsCallbackResult;

typedef UdsCallbackResult (*UdsReadDidFn)(void *context, uint16_t did, uint8_t *data,
                                          uint16_t *length, uint16_t capacity);
typedef UdsCallbackResult (*UdsWriteDidFn)(void *context, uint16_t did, const uint8_t *data,
                                           uint16_t length);
typedef UdsCallbackResult (*UdsDtcFn)(void *context, uint8_t subfunction, const uint8_t *request,
                                      uint16_t request_len, uint8_t *response,
                                      uint16_t *response_len, uint16_t capacity);
typedef UdsCallbackResult (*UdsSecuritySeedFn)(void *context, uint8_t level, uint8_t *seed,
                                               uint16_t *length, uint16_t capacity);
typedef UdsCallbackResult (*UdsSecurityKeyFn)(void *context, uint8_t level, const uint8_t *key,
                                              uint16_t length);
typedef UdsCallbackResult (*UdsCommunicationControlFn)(void *context, uint8_t subfunction,
                                                       uint8_t communication_type);
typedef UdsCallbackResult (*UdsIoControlFn)(void *context, uint16_t did, const uint8_t *parameter,
                                            uint16_t parameter_len, uint8_t *response,
                                            uint16_t *response_len, uint16_t capacity);
typedef UdsCallbackResult (*UdsRoutineFn)(void *context, uint8_t subfunction, uint16_t routine_id,
                                          const uint8_t *request, uint16_t request_len,
                                          uint8_t *response, uint16_t *response_len,
                                          uint16_t capacity);
typedef UdsCallbackResult (*UdsDownloadRequestFn)(void *context, uint32_t address, uint32_t length,
                                                  uint16_t *max_block_length);
typedef UdsCallbackResult (*UdsTransferDataFn)(void *context, uint8_t block_sequence_counter,
                                               const uint8_t *data, uint16_t length);
typedef UdsCallbackResult (*UdsTransferExitFn)(void *context, const uint8_t *request,
                                               uint16_t request_len, uint8_t *response,
                                               uint16_t *response_len, uint16_t capacity);
typedef UdsCallbackResult (*UdsResetFn)(void *context, uint8_t subfunction);
typedef UdsCallbackResult (*UdsDtcSettingFn)(void *context, uint8_t subfunction);

typedef struct {
    UdsReadDidFn read_did;
    UdsWriteDidFn write_did;
    UdsDtcFn read_dtc;
    UdsSecuritySeedFn security_seed;
    UdsSecurityKeyFn security_key;
    UdsCommunicationControlFn communication_control;
    UdsIoControlFn io_control;
    UdsRoutineFn routine_control;
    UdsDownloadRequestFn request_download;
    UdsTransferDataFn transfer_data;
    UdsTransferExitFn request_transfer_exit;
    UdsResetFn ecu_reset;
    UdsDtcSettingFn control_dtc_setting;
} UdsCallbacks;

typedef struct {
    UdsCallbacks callbacks;
    void *context;
    uint8_t session;
    uint8_t security_level;
    uint8_t next_download_block;
    uint16_t max_download_block_length;
    uint16_t p2_server_ms;
    uint16_t p2_star_server_ms;
    uint32_t s3_server_timeout_ms;
    uint32_t last_activity_ms;
    bool download_active;
    bool reset_pending;
    bool dtc_setting_enabled;
} UdsServer;

void uds_server_init(UdsServer *server, const UdsCallbacks *callbacks, void *context,
                     uint32_t now_ms);
UdsCallbackResult uds_server_handle(UdsServer *server, const uint8_t *request, uint16_t request_len,
                                    uint8_t *response, uint16_t *response_len, uint16_t capacity,
                                    uint32_t now_ms);
UdsCallbackResult uds_server_tick(UdsServer *server, uint32_t now_ms);
void uds_server_reset_security(UdsServer *server);
bool uds_server_reset_pending(const UdsServer *server);
void uds_server_clear_reset(UdsServer *server);
uint8_t uds_server_session(const UdsServer *server);
uint8_t uds_server_security_level(const UdsServer *server);

#endif
