/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds.h"

#include <stddef.h>

static bool deadline_expired(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static uint16_t read_u16(const uint8_t *data) {
    return (uint16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static uint32_t read_be_u32(const uint8_t *data, uint8_t length) {
    uint32_t value = 0U;
    for (uint8_t index = 0U; index < length; ++index) {
        value = (value << 8U) | data[index];
    }
    return value;
}

static uint8_t result_to_nrc(UdsCallbackResult result) {
    switch (result) {
    case UDS_RESULT_NOT_SUPPORTED:
        return UDS_NRC_SERVICE_NOT_SUPPORTED;
    case UDS_RESULT_DENIED:
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    case UDS_RESULT_OUT_OF_RANGE:
        return UDS_NRC_REQUEST_OUT_OF_RANGE;
    case UDS_RESULT_BUSY:
        return UDS_NRC_BUSY_REPEAT_REQUEST;
    case UDS_RESULT_SEQUENCE_ERROR:
        return UDS_NRC_REQUEST_SEQUENCE_ERROR;
    case UDS_RESULT_INVALID_KEY:
        return UDS_NRC_INVALID_KEY;
    case UDS_RESULT_ATTEMPTS_EXCEEDED:
        return UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS;
    case UDS_RESULT_DELAY_ACTIVE:
        return UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED;
    case UDS_RESULT_PROGRAMMING_FAILURE:
        return UDS_NRC_GENERAL_PROGRAMMING_FAILURE;
    case UDS_RESULT_RESPONSE_TOO_LONG:
        return UDS_NRC_RESPONSE_TOO_LONG;
    case UDS_RESULT_OK:
    case UDS_RESULT_NO_RESPONSE:
    case UDS_RESULT_ERROR:
    default:
        return UDS_NRC_CONDITIONS_NOT_CORRECT;
    }
}

static UdsCallbackResult negative_response(const uint8_t *request, uint8_t nrc, uint8_t *response,
                                           uint16_t *response_len, uint16_t capacity) {
    if (capacity < 3U) {
        return UDS_RESULT_RESPONSE_TOO_LONG;
    }
    response[0] = 0x7FU;
    response[1] = request[0];
    response[2] = nrc;
    *response_len = 3U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult callback_result(UdsServer *server, UdsCallbackResult result,
                                         const uint8_t *request, uint8_t *response,
                                         uint16_t *response_len, uint16_t capacity) {
    (void)server;
    if (result == UDS_RESULT_OK) {
        return UDS_RESULT_OK;
    }
    return negative_response(request, result_to_nrc(result), response, response_len, capacity);
}

void uds_server_init(UdsServer *server, const UdsCallbacks *callbacks, void *context,
                     uint32_t now_ms) {
    if (server == NULL) {
        return;
    }
    if (callbacks != NULL) {
        server->callbacks = *callbacks;
    } else {
        server->callbacks.read_did = NULL;
        server->callbacks.write_did = NULL;
        server->callbacks.read_dtc = NULL;
        server->callbacks.security_seed = NULL;
        server->callbacks.security_key = NULL;
        server->callbacks.communication_control = NULL;
        server->callbacks.io_control = NULL;
        server->callbacks.routine_control = NULL;
        server->callbacks.request_download = NULL;
        server->callbacks.transfer_data = NULL;
        server->callbacks.request_transfer_exit = NULL;
        server->callbacks.ecu_reset = NULL;
        server->callbacks.control_dtc_setting = NULL;
    }
    server->context = context;
    server->session = UDS_SESSION_DEFAULT;
    server->security_level = 0U;
    server->next_download_block = 1U;
    server->max_download_block_length = 0U;
    server->p2_server_ms = UDS_DEFAULT_P2_SERVER_MS;
    server->p2_star_server_ms = UDS_DEFAULT_P2_STAR_SERVER_MS;
    server->s3_server_timeout_ms = UDS_DEFAULT_S3_SERVER_MS;
    server->last_activity_ms = now_ms;
    server->download_active = false;
    server->reset_pending = false;
    server->dtc_setting_enabled = true;
}

void uds_server_reset_security(UdsServer *server) {
    if (server == NULL) {
        return;
    }
    server->security_level = 0U;
}

static UdsCallbackResult service_session_control(UdsServer *server, const uint8_t *request,
                                                 uint16_t request_len, uint8_t *response,
                                                 uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_SESSION_CONTROL
    if (request_len != 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if ((subfunction != UDS_SESSION_DEFAULT) && (subfunction != UDS_SESSION_PROGRAMMING) &&
        (subfunction != UDS_SESSION_EXTENDED)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    server->session = subfunction;
    uds_server_reset_security(server);
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 6U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x50U;
    response[1] = subfunction;
    response[2] = (uint8_t)(server->p2_server_ms >> 8U);
    response[3] = (uint8_t)server->p2_server_ms;
    response[4] = (uint8_t)(server->p2_star_server_ms / 10U >> 8U);
    response[5] = (uint8_t)(server->p2_star_server_ms / 10U);
    *response_len = 6U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_ecu_reset(UdsServer *server, const uint8_t *request,
                                           uint16_t request_len, uint8_t *response,
                                           uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_ECU_RESET
    if (request_len != 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if ((subfunction != 0x01U) || (server->callbacks.ecu_reset == NULL)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    UdsCallbackResult result = server->callbacks.ecu_reset(server->context, subfunction);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    server->reset_pending = true;
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x51U;
    response[1] = subfunction;
    *response_len = 2U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_read_dtc(UdsServer *server, const uint8_t *request,
                                          uint16_t request_len, uint8_t *response,
                                          uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_READ_DTC_INFORMATION
    if ((request_len < 2U) || (server->callbacks.read_dtc == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    response[0] = 0x59U;
    *response_len = 1U;
    uint16_t extra_length = 0U;
    UdsCallbackResult result =
        server->callbacks.read_dtc(server->context, subfunction, request, request_len, &response[1],
                                   &extra_length, (capacity > 1U) ? (uint16_t)(capacity - 1U) : 0U);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((uint16_t)(capacity - *response_len) < extra_length) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    *response_len = (uint16_t)(*response_len + extra_length);
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_read_data(UdsServer *server, const uint8_t *request,
                                           uint16_t request_len, uint8_t *response,
                                           uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_READ_DATA_BY_IDENTIFIER
    if ((request_len < 3U) || (((request_len - 1U) % 2U) != 0U) ||
        (server->callbacks.read_did == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    response[0] = 0x62U;
    *response_len = 1U;
    for (uint16_t offset = 1U; offset < request_len; offset = (uint16_t)(offset + 2U)) {
        if ((uint16_t)(capacity - *response_len) < 2U) {
            return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                     capacity);
        }
        uint16_t did = read_u16(&request[offset]);
        response[*response_len] = request[offset];
        response[*response_len + 1U] = request[offset + 1U];
        *response_len = (uint16_t)(*response_len + 2U);
        uint16_t value_length = 0U;
        UdsCallbackResult result =
            server->callbacks.read_did(server->context, did, &response[*response_len],
                                       &value_length, (uint16_t)(capacity - *response_len));
        if (result != UDS_RESULT_OK) {
            return callback_result(server, result, request, response, response_len, capacity);
        }
        if ((uint16_t)(capacity - *response_len) < value_length) {
            return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                     capacity);
        }
        *response_len = (uint16_t)(*response_len + value_length);
    }
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_security_access(UdsServer *server, const uint8_t *request,
                                                 uint16_t request_len, uint8_t *response,
                                                 uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_SECURITY_ACCESS
    if ((request_len < 2U) || (server->callbacks.security_seed == NULL) ||
        (server->callbacks.security_key == NULL)) {
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    uint8_t subfunction = request[1];
    if ((subfunction == 0U) || (subfunction > 0x7FU)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    uint8_t level = (uint8_t)((subfunction + 1U) / 2U);
    if ((subfunction & 1U) != 0U) {
        uint16_t seed_length = 0U;
        UdsCallbackResult result =
            server->callbacks.security_seed(server->context, level, &response[2], &seed_length,
                                            (capacity > 2U) ? (uint16_t)(capacity - 2U) : 0U);
        if (result != UDS_RESULT_OK) {
            return callback_result(server, result, request, response, response_len, capacity);
        }
        if (capacity < (uint16_t)(2U + seed_length)) {
            return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                     capacity);
        }
        response[0] = 0x67U;
        response[1] = subfunction;
        *response_len = (uint16_t)(2U + seed_length);
        return UDS_RESULT_OK;
    }
    if (request_len <= 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    UdsCallbackResult result = server->callbacks.security_key(server->context, level, &request[2],
                                                              (uint16_t)(request_len - 2U));
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    server->security_level = level;
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x67U;
    response[1] = subfunction;
    *response_len = 2U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_communication_control(UdsServer *server, const uint8_t *request,
                                                       uint16_t request_len, uint8_t *response,
                                                       uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_COMMUNICATION_CONTROL
    if ((request_len != 3U) || (server->callbacks.communication_control == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    UdsCallbackResult result =
        server->callbacks.communication_control(server->context, subfunction, request[2]);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 3U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x68U;
    response[1] = subfunction;
    response[2] = request[2];
    *response_len = 3U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_io_control(UdsServer *server, const uint8_t *request,
                                            uint16_t request_len, uint8_t *response,
                                            uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_IO_CONTROL_BY_IDENTIFIER
    if ((request_len < 4U) || (server->callbacks.io_control == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint16_t did = read_u16(&request[1]);
    response[0] = 0x6FU;
    response[1] = request[1];
    response[2] = request[2];
    *response_len = 3U;
    uint16_t extra_length = 0U;
    UdsCallbackResult result = server->callbacks.io_control(
        server->context, did, &request[3], (uint16_t)(request_len - 3U), &response[3],
        &extra_length, (capacity > 3U) ? (uint16_t)(capacity - 3U) : 0U);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((uint16_t)(capacity - *response_len) < extra_length) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    *response_len = (uint16_t)(*response_len + extra_length);
    return UDS_RESULT_OK;
#else
    (void)server;
    (void)request_len;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_routine_control(UdsServer *server, const uint8_t *request,
                                                 uint16_t request_len, uint8_t *response,
                                                 uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_ROUTINE_CONTROL
    if ((request_len < 4U) || (server->callbacks.routine_control == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    uint16_t routine_id = read_u16(&request[2]);
    response[0] = 0x71U;
    response[1] = subfunction;
    response[2] = request[2];
    response[3] = request[3];
    *response_len = 4U;
    uint16_t extra_length = 0U;
    UdsCallbackResult result = server->callbacks.routine_control(
        server->context, subfunction, routine_id, &request[4], (uint16_t)(request_len - 4U),
        &response[4], &extra_length, (capacity > 4U) ? (uint16_t)(capacity - 4U) : 0U);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((uint16_t)(capacity - *response_len) < extra_length) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    *response_len = (uint16_t)(*response_len + extra_length);
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_download(UdsServer *server, const uint8_t *request,
                                          uint16_t request_len, uint8_t *response,
                                          uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_REQUEST_DOWNLOAD
    if ((request_len < 3U) || (server->callbacks.request_download == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t address_length = (uint8_t)(request[1] >> 4U);
    uint8_t length_length = (uint8_t)(request[1] & 0x0FU);
    if ((address_length == 0U) || (address_length > 4U) || (length_length == 0U) ||
        (length_length > 4U) || (request_len != (uint16_t)(2U + address_length + length_length))) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint32_t address = read_be_u32(&request[2], address_length);
    uint32_t length = read_be_u32(&request[2U + address_length], length_length);
    uint16_t max_block_length = 0U;
    UdsCallbackResult result =
        server->callbacks.request_download(server->context, address, length, &max_block_length);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((max_block_length < 3U) || (capacity < 4U)) {
        return negative_response(request, UDS_NRC_UPLOAD_DOWNLOAD_NOT_ACCEPTED, response,
                                 response_len, capacity);
    }
    server->download_active = true;
    server->next_download_block = 1U;
    server->max_download_block_length = max_block_length;
    response[0] = 0x74U;
    response[1] = 0x20U;
    response[2] = (uint8_t)(max_block_length >> 8U);
    response[3] = (uint8_t)max_block_length;
    *response_len = 4U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_transfer_data(UdsServer *server, const uint8_t *request,
                                               uint16_t request_len, uint8_t *response,
                                               uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_TRANSFER_DATA
    if ((request_len < 3U) || !server->download_active ||
        (server->callbacks.transfer_data == NULL)) {
        return negative_response(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response, response_len,
                                 capacity);
    }
    if (request[1] != server->next_download_block) {
        return negative_response(request, UDS_NRC_WRONG_BLOCK_SEQUENCE_COUNTER, response,
                                 response_len, capacity);
    }
    uint16_t data_length = (uint16_t)(request_len - 2U);
    if (data_length > (uint16_t)(server->max_download_block_length - 2U)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    UdsCallbackResult result =
        server->callbacks.transfer_data(server->context, request[1], &request[2], data_length);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    server->next_download_block = (uint8_t)(server->next_download_block + 1U);
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x76U;
    response[1] = request[1];
    *response_len = 2U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_transfer_exit(UdsServer *server, const uint8_t *request,
                                               uint16_t request_len, uint8_t *response,
                                               uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_REQUEST_TRANSFER_EXIT
    if (!server->download_active || (server->callbacks.request_transfer_exit == NULL)) {
        return negative_response(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response, response_len,
                                 capacity);
    }
    *response_len = 0U;
    UdsCallbackResult result = server->callbacks.request_transfer_exit(
        server->context, &request[1], (uint16_t)(request_len - 1U), &response[1], response_len,
        (capacity > 1U) ? (uint16_t)(capacity - 1U) : 0U);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if (capacity < 1U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x77U;
    *response_len = (uint16_t)(*response_len + 1U);
    server->download_active = false;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_tester_present(const uint8_t *request, uint16_t request_len,
                                                uint8_t *response, uint16_t *response_len,
                                                uint16_t capacity) {
#if UDS_ENABLE_TESTER_PRESENT
    if (request_len != 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if (subfunction != 0U) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x7EU;
    response[1] = request[1];
    *response_len = 2U;
    return UDS_RESULT_OK;
#else
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_dtc_setting(UdsServer *server, const uint8_t *request,
                                             uint16_t request_len, uint8_t *response,
                                             uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_CONTROL_DTC_SETTING
    if ((request_len != 2U) || (server->callbacks.control_dtc_setting == NULL)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if ((subfunction != 0x01U) && (subfunction != 0x02U)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    UdsCallbackResult result = server->callbacks.control_dtc_setting(server->context, subfunction);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    server->dtc_setting_enabled = (subfunction == 0x01U);
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0xC5U;
    response[1] = subfunction;
    *response_len = 2U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

UdsCallbackResult uds_server_handle(UdsServer *server, const uint8_t *request, uint16_t request_len,
                                    uint8_t *response, uint16_t *response_len, uint16_t capacity,
                                    uint32_t now_ms) {
    if ((server == NULL) || (request == NULL) || (response == NULL) || (response_len == NULL) ||
        (capacity == 0U) || (request_len == 0U) || (request_len > UDS_MAX_REQUEST_LENGTH)) {
        return UDS_RESULT_ERROR;
    }
    *response_len = 0U;
    server->last_activity_ms = now_ms;
    uint8_t service = request[0];
    switch (service) {
    case 0x10U:
        return service_session_control(server, request, request_len, response, response_len,
                                       capacity);
    case 0x11U:
        return service_ecu_reset(server, request, request_len, response, response_len, capacity);
    case 0x19U:
        return service_read_dtc(server, request, request_len, response, response_len, capacity);
    case 0x22U:
        return service_read_data(server, request, request_len, response, response_len, capacity);
    case 0x27U:
        return service_security_access(server, request, request_len, response, response_len,
                                       capacity);
    case 0x28U:
        return service_communication_control(server, request, request_len, response, response_len,
                                             capacity);
    case 0x2FU:
        return service_io_control(server, request, request_len, response, response_len, capacity);
    case 0x31U:
        return service_routine_control(server, request, request_len, response, response_len,
                                       capacity);
    case 0x34U:
        return service_download(server, request, request_len, response, response_len, capacity);
    case 0x36U:
        return service_transfer_data(server, request, request_len, response, response_len,
                                     capacity);
    case 0x37U:
        return service_transfer_exit(server, request, request_len, response, response_len,
                                     capacity);
    case 0x3EU:
        return service_tester_present(request, request_len, response, response_len, capacity);
    case 0x85U:
        return service_dtc_setting(server, request, request_len, response, response_len, capacity);
    default:
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
}

UdsCallbackResult uds_server_tick(UdsServer *server, uint32_t now_ms) {
    if (server == NULL) {
        return UDS_RESULT_ERROR;
    }
    if (deadline_expired(now_ms, server->last_activity_ms + server->s3_server_timeout_ms)) {
        server->session = UDS_SESSION_DEFAULT;
        uds_server_reset_security(server);
        server->download_active = false;
        server->next_download_block = 1U;
        return UDS_RESULT_BUSY;
    }
    return UDS_RESULT_OK;
}

bool uds_server_reset_pending(const UdsServer *server) {
    return (server != NULL) && server->reset_pending;
}

void uds_server_clear_reset(UdsServer *server) {
    if (server != NULL) {
        server->reset_pending = false;
    }
}

uint8_t uds_server_session(const UdsServer *server) {
    return (server != NULL) ? server->session : UDS_SESSION_DEFAULT;
}

uint8_t uds_server_security_level(const UdsServer *server) {
    return (server != NULL) ? server->security_level : 0U;
}
