/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds.h"
#include "uds_iso_tp/uds_dtc.h"
#include "uds_iso_tp/uds_services.h"

#include <stddef.h>

static bool deadline_expired(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static bool session_supported(uint8_t session) {
    return (session == UDS_SESSION_DEFAULT) || (session == UDS_SESSION_PROGRAMMING) ||
           (session == UDS_SESSION_EXTENDED) || (session == UDS_SESSION_SAFETY);
}

static bool security_session_allowed(uint8_t session) {
    return (session == UDS_SESSION_PROGRAMMING) || (session == UDS_SESSION_EXTENDED);
}

static uint8_t session_mask(uint8_t session) {
    switch (session) {
    case UDS_SESSION_DEFAULT:
        return UDS_SESSION_MASK_DEFAULT;
    case UDS_SESSION_PROGRAMMING:
        return UDS_SESSION_MASK_PROGRAMMING;
    case UDS_SESSION_EXTENDED:
        return UDS_SESSION_MASK_EXTENDED;
    case UDS_SESSION_SAFETY:
        return UDS_SESSION_MASK_SAFETY;
    default:
        return 0U;
    }
}

static const UdsServiceAttribute default_service_attribute = {
    0U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
    UDS_ADDRESS_MODE_BOTH};

static const UdsServiceAttribute service_attributes[] = {
    {0x10U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_MODE_BOTH},
    {0x11U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x19U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_MODE_BOTH},
    {0x22U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_MODE_BOTH},
    {0x27U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING | UDS_SESSION_MASK_EXTENDED,
     UDS_SECURITY_MASK_NONE, UDS_ADDRESS_PHYSICAL},
    {0x28U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_MODE_BOTH},
    {0x2FU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_EXTENDED, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x31U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING | UDS_SESSION_MASK_EXTENDED,
     UDS_SECURITY_MASK_NONE, UDS_ADDRESS_PHYSICAL},
    {0x34U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x36U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x37U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x3EU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_MODE_BOTH},
    {0x85U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x83U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x84U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x86U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x87U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x23U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING | UDS_SESSION_MASK_EXTENDED,
     UDS_SECURITY_MASK_NONE, UDS_ADDRESS_PHYSICAL},
    {0x24U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x2AU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x2CU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_EXTENDED, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x2EU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING | UDS_SESSION_MASK_EXTENDED,
     UDS_SECURITY_MASK_NONE, UDS_ADDRESS_PHYSICAL},
    {0x3DU, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x14U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x35U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x29U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_ALL, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
    {0x38U, UDS_SERVICE_ANY_SUBFUNCTION, UDS_SESSION_MASK_PROGRAMMING, UDS_SECURITY_MASK_NONE,
     UDS_ADDRESS_PHYSICAL},
};

const UdsServiceAttribute *uds_service_attribute(uint8_t sid, uint8_t subservice) {
    const UdsServiceAttribute *fallback = NULL;
    for (size_t index = 0U; index < (sizeof(service_attributes) / sizeof(service_attributes[0]));
         ++index) {
        const UdsServiceAttribute *attribute = &service_attributes[index];
        if (attribute->sid != sid)
            continue;
        if (attribute->subservice == subservice)
            return attribute;
        if (attribute->subservice == UDS_SERVICE_ANY_SUBFUNCTION)
            fallback = attribute;
    }
    return (fallback != NULL) ? fallback : &default_service_attribute;
}

bool uds_service_attribute_allows(const UdsServiceAttribute *attribute, uint8_t session,
                                  uint8_t security_level, UdsAddressMode address_mode) {
    if (attribute == NULL)
        return false;
    if ((attribute->session_mask & session_mask(session)) == 0U)
        return false;
    if ((attribute->address_mode != UDS_ADDRESS_MODE_BOTH) &&
        ((attribute->address_mode & address_mode) == 0U))
        return false;
    if ((attribute->security_mask != UDS_SECURITY_MASK_NONE) &&
        ((attribute->security_mask & (uint16_t)(1U << security_level)) == 0U))
        return false;
    return true;
}

bool uds_security_subfunction_level(uint8_t subfunction, uint8_t *level, bool *is_seed) {
    if ((level == NULL) || (is_seed == NULL)) {
        return false;
    }
    switch (subfunction) {
    case UDS_SECURITY_REQUEST_SEED_LEVEL_1:
        *level = UDS_SECURITY_LEVEL_1;
        *is_seed = true;
        return true;
    case UDS_SECURITY_SEND_KEY_LEVEL_1:
        *level = UDS_SECURITY_LEVEL_1;
        *is_seed = false;
        return true;
    case UDS_SECURITY_REQUEST_SEED_LEVEL_2:
        *level = UDS_SECURITY_LEVEL_2;
        *is_seed = true;
        return true;
    case UDS_SECURITY_SEND_KEY_LEVEL_2:
        *level = UDS_SECURITY_LEVEL_2;
        *is_seed = false;
        return true;
    case UDS_SECURITY_REQUEST_SEED_LEVEL_3:
        *level = UDS_SECURITY_LEVEL_3;
        *is_seed = true;
        return true;
    case UDS_SECURITY_SEND_KEY_LEVEL_3:
        *level = UDS_SECURITY_LEVEL_3;
        *is_seed = false;
        return true;
    case UDS_SECURITY_REQUEST_SEED_LEVEL_4:
        *level = UDS_SECURITY_LEVEL_4;
        *is_seed = true;
        return true;
    case UDS_SECURITY_SEND_KEY_LEVEL_4:
        *level = UDS_SECURITY_LEVEL_4;
        *is_seed = false;
        return true;
    case UDS_SECURITY_REQUEST_SEED_LEVEL_5:
        *level = UDS_SECURITY_LEVEL_5;
        *is_seed = true;
        return true;
    case UDS_SECURITY_SEND_KEY_LEVEL_5:
        *level = UDS_SECURITY_LEVEL_5;
        *is_seed = false;
        return true;
    default:
        return false;
    }
}

static void security_invalidate_seed(UdsServer *server) {
    server->security_seed_valid = false;
    server->security_seed_level = 0U;
    server->security_seed_timer_active = false;
    if (server->security_state == UDS_SECURITY_STATE_WAITING_FOR_KEY) {
        server->security_state = UDS_SECURITY_STATE_LOCKED_READY;
    }
}

static void security_tick(UdsServer *server, uint32_t now_ms) {
    if (server->security_lockout_active &&
        deadline_expired(now_ms, server->security_lockout_until_ms)) {
        server->security_lockout_active = false;
        server->security_failed_attempts = 0U;
        server->security_state = UDS_SECURITY_STATE_LOCKED_READY;
    }
    if (server->security_seed_timer_active &&
        deadline_expired(now_ms, server->security_seed_expiry_ms)) {
        security_invalidate_seed(server);
    }
}

static bool security_delay_active(const UdsServer *server, uint32_t now_ms) {
    return server->security_lockout_active &&
           !deadline_expired(now_ms, server->security_lockout_until_ms);
}

static void security_reset_for_ecu_reset(UdsServer *server, uint32_t now_ms,
                                         UdsResetReason reason) {
    (void)reason;
    server->security_level = 0U;
    server->security_state = UDS_SECURITY_STATE_LOCKED_READY;
    server->security_failed_attempts = 0U;
    server->security_seed_level = 0U;
    server->security_seed_valid = false;
    server->security_seed_timer_active = false;
    server->security_lockout_active = false;
    server->security_lockout_until_ms = now_ms;
}

static void security_reset_for_session_change(UdsServer *server) {
    server->security_level = 0U;
    security_invalidate_seed(server);
    server->security_state = server->security_lockout_active ? UDS_SECURITY_STATE_LOCKOUT
                                                             : UDS_SECURITY_STATE_LOCKED_READY;
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
    case UDS_RESULT_SUBFUNCTION_NOT_SUPPORTED:
        return UDS_NRC_SUBFUNCTION_NOT_SUPPORTED;
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
        server->callbacks.ecu_reset_execute = NULL;
        server->callbacks.control_dtc_setting = NULL;
        server->callbacks.clear_dtc = NULL;
        server->callbacks.service_backends = NULL;
    }
    server->context = context;
    server->session = UDS_SESSION_DEFAULT;
    server->security_level = 0U;
    server->security_state = UDS_SECURITY_STATE_LOCKED_READY;
    server->security_failed_attempts = 0U;
    server->security_max_attempts = UDS_DEFAULT_SECURITY_MAX_ATTEMPTS;
    server->security_seed_level = 0U;
    server->security_initial_delay_until_ms = now_ms;
    server->security_initial_delay_ms = UDS_DEFAULT_SECURITY_INITIAL_DELAY_MS;
    server->security_lockout_ms = UDS_DEFAULT_SECURITY_LOCKOUT_MS;
    server->security_seed_timeout_ms = UDS_DEFAULT_SECURITY_SEED_TIMEOUT_MS;
    server->security_initial_delay_active = false;
    server->security_lockout_active = false;
    server->security_seed_timer_active = false;
    server->security_seed_valid = false;
    server->security_lockout_until_ms = now_ms;
    server->security_seed_expiry_ms = 0U;
    server->pending_reset_reason = UDS_RESET_NORMAL;
    server->pending_reset_subfunction = 0U;
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
    security_reset_for_session_change(server);
}

void uds_server_apply_reset(UdsServer *server, UdsResetReason reason, uint32_t now_ms) {
    if (server == NULL) {
        return;
    }
    server->session = UDS_SESSION_DEFAULT;
    server->download_active = false;
    server->next_download_block = 1U;
    server->max_download_block_length = 0U;
    server->reset_pending = false;
    server->pending_reset_reason = UDS_RESET_NORMAL;
    server->pending_reset_subfunction = 0U;
    server->last_activity_ms = now_ms;
    security_reset_for_ecu_reset(server, now_ms, reason);
}

UdsSessionTransitionResult uds_session_transition_allowed(uint8_t current_session,
                                                          uint8_t requested_session) {
    if (!session_supported(current_session) || !session_supported(requested_session)) {
        return UDS_SESSION_TRANSITION_DENIED;
    }
    if (current_session == requested_session) {
        return UDS_SESSION_TRANSITION_ALLOWED;
    }
    switch (current_session) {
    case UDS_SESSION_DEFAULT:
        return (requested_session == UDS_SESSION_EXTENDED) ? UDS_SESSION_TRANSITION_ALLOWED
                                                           : UDS_SESSION_TRANSITION_DENIED;
    case UDS_SESSION_EXTENDED:
        return ((requested_session == UDS_SESSION_DEFAULT) ||
                (requested_session == UDS_SESSION_PROGRAMMING))
                   ? UDS_SESSION_TRANSITION_ALLOWED
                   : UDS_SESSION_TRANSITION_DENIED;
    case UDS_SESSION_PROGRAMMING:
    case UDS_SESSION_SAFETY:
        return (requested_session == UDS_SESSION_DEFAULT) ? UDS_SESSION_TRANSITION_ALLOWED
                                                          : UDS_SESSION_TRANSITION_DENIED;
    default:
        return UDS_SESSION_TRANSITION_DENIED;
    }
}

UdsCallbackResult uds_server_request_session(UdsServer *server, uint8_t requested_session,
                                             uint32_t now_ms) {
    if ((server == NULL) || !session_supported(requested_session)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    uint8_t current_session = server->session;
    if (uds_session_transition_allowed(current_session, requested_session) !=
        UDS_SESSION_TRANSITION_ALLOWED) {
        return UDS_RESULT_DENIED;
    }
    security_tick(server, now_ms);
    security_reset_for_session_change(server);
    server->session = requested_session;
    server->download_active = false;
    server->next_download_block = 1U;
    server->last_activity_ms = now_ms;
    if (requested_session == UDS_SESSION_PROGRAMMING) {
        server->reset_pending = true;
        server->pending_reset_reason = UDS_RESET_PROGRAMMING;
        server->pending_reset_subfunction = 0U;
    } else if ((requested_session == UDS_SESSION_DEFAULT) &&
               (current_session != UDS_SESSION_DEFAULT)) {
        server->reset_pending = true;
        server->pending_reset_reason = UDS_RESET_NORMAL;
        server->pending_reset_subfunction = 0U;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult service_session_control(UdsServer *server, const uint8_t *request,
                                                 uint16_t request_len, uint8_t *response,
                                                 uint16_t *response_len, uint16_t capacity,
                                                 uint32_t now_ms) {
#if UDS_ENABLE_SESSION_CONTROL
    if (request_len != 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if (!session_supported(subfunction)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    UdsCallbackResult result = uds_server_request_session(server, subfunction, now_ms);
    if (result != UDS_RESULT_OK) {
        return negative_response(request, result_to_nrc(result), response, response_len, capacity);
    }
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
    uint16_t p2_star_10ms = (uint16_t)(server->p2_star_server_ms / 10U);
    response[4] = (uint8_t)(p2_star_10ms >> 8U);
    response[5] = (uint8_t)p2_star_10ms;
    *response_len = 6U;
    return UDS_RESULT_OK;
#else
    (void)server;
    (void)now_ms;
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
    if ((subfunction < UDS_RESET_TYPE_HARD) ||
        (subfunction > UDS_RESET_TYPE_DISABLE_RAPID_POWER_SHUTDOWN) ||
        (server->callbacks.ecu_reset == NULL)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    UdsCallbackResult result = server->callbacks.ecu_reset(server->context, subfunction);
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    if ((request[1] & UDS_SUPPRESS_POSITIVE_RESPONSE) != 0U) {
        server->reset_pending = true;
        server->pending_reset_reason = UDS_RESET_NORMAL;
        server->pending_reset_subfunction = subfunction;
        return UDS_RESULT_NO_RESPONSE;
    }
    if (capacity < 2U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x51U;
    response[1] = subfunction;
    *response_len = 2U;
    server->reset_pending = true;
    server->pending_reset_reason = UDS_RESET_NORMAL;
    server->pending_reset_subfunction = subfunction;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_clear_dtc(UdsServer *server, const uint8_t *request,
                                           uint16_t request_len, uint8_t *response,
                                           uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_READ_DTC_INFORMATION
    if (request_len != 4U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    if (server->callbacks.clear_dtc == NULL) {
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    uint32_t group = read_be_u32(&request[1], 3U);
    UdsCallbackResult result = server->callbacks.clear_dtc(server->context, group);
    if (result != UDS_RESULT_OK)
        return callback_result(server, result, request, response, response_len, capacity);
    if (capacity < 1U)
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    response[0] = 0x54U;
    *response_len = 1U;
    return UDS_RESULT_OK;
#else
    (void)server;
    return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                             capacity);
#endif
}

static UdsCallbackResult service_modular_backend(UdsServer *server, const uint8_t *request,
                                                 uint16_t request_len, uint8_t *response,
                                                 uint16_t *response_len, uint16_t capacity) {
    UdsServiceHandlerFn handler =
        uds_service_backends_handler(server->callbacks.service_backends, request[0]);
    if (handler == NULL)
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    UdsCallbackResult result =
        handler(server->context, request, request_len, response, response_len, capacity);
    if (result == UDS_RESULT_NO_RESPONSE)
        return result;
    if (result != UDS_RESULT_OK)
        return callback_result(server, result, request, response, response_len, capacity);
    if (*response_len > capacity)
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    return UDS_RESULT_OK;
}

static UdsCallbackResult service_read_dtc(UdsServer *server, const uint8_t *request,
                                          uint16_t request_len, uint8_t *response,
                                          uint16_t *response_len, uint16_t capacity) {
#if UDS_ENABLE_READ_DTC_INFORMATION
    if (request_len < 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    uint8_t subfunction = (uint8_t)(request[1] & 0x7FU);
    if (!uds_dtc_subfunction_supported(subfunction)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    if (!uds_dtc_request_length_valid(subfunction, request_len)) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    if (capacity < 1U) {
        return negative_response(request, UDS_NRC_RESPONSE_TOO_LONG, response, response_len,
                                 capacity);
    }
    response[0] = 0x59U;
    *response_len = 1U;
    uint16_t extra_length = 0U;
    UdsCallbackResult result;
    if (server->callbacks.dtc_backend != NULL) {
        const UdsDtcBackend *backend = server->callbacks.dtc_backend;
        uint32_t required = uds_dtc_capability_for_subfunction(subfunction);
        if ((backend->capabilities & required) == 0U) {
            return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response,
                                     response_len, capacity);
        }
        if (backend->report == NULL) {
            return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                     capacity);
        }
        result = backend->report(server->context, subfunction, request, request_len, &response[1],
                                 &extra_length, (uint16_t)(capacity - 1U));
    } else {
        if (server->callbacks.read_dtc == NULL) {
            return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                     capacity);
        }
        result = server->callbacks.read_dtc(server->context, subfunction, request, request_len,
                                            &response[1], &extra_length, (uint16_t)(capacity - 1U));
    }
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
                                                 uint16_t *response_len, uint16_t capacity,
                                                 uint32_t now_ms) {
#if UDS_ENABLE_SECURITY_ACCESS
    if (request_len < 2U) {
        return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                 response, response_len, capacity);
    }
    if ((server->callbacks.security_seed == NULL) || (server->callbacks.security_key == NULL)) {
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    security_tick(server, now_ms);
    if (!security_session_allowed(server->session)) {
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION, response,
                                 response_len, capacity);
    }
    uint8_t subfunction = request[1];
    uint8_t level = 0U;
    bool is_seed = false;
    if (!uds_security_subfunction_level(subfunction, &level, &is_seed)) {
        return negative_response(request, UDS_NRC_SUBFUNCTION_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
    if (security_delay_active(server, now_ms)) {
        return negative_response(request, UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED, response,
                                 response_len, capacity);
    }
    if (is_seed) {
        if (request_len != 2U) {
            return negative_response(request, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT,
                                     response, response_len, capacity);
        }
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
        server->security_seed_level = level;
        server->security_seed_valid = true;
        server->security_seed_timer_active = server->security_seed_timeout_ms != 0U;
        server->security_seed_expiry_ms = now_ms + server->security_seed_timeout_ms;
        server->security_state = UDS_SECURITY_STATE_WAITING_FOR_KEY;
        response[0] = 0x67U;
        response[1] = subfunction;
        *response_len = (uint16_t)(2U + seed_length);
        return UDS_RESULT_OK;
    }
    if ((request_len <= 2U) || !server->security_seed_valid ||
        (server->security_seed_level != level)) {
        return negative_response(request, UDS_NRC_REQUEST_SEQUENCE_ERROR, response, response_len,
                                 capacity);
    }
    UdsCallbackResult result = server->callbacks.security_key(server->context, level, &request[2],
                                                              (uint16_t)(request_len - 2U));
    if (result == UDS_RESULT_INVALID_KEY) {
        security_invalidate_seed(server);
        server->security_level = 0U;
        server->security_failed_attempts = (uint8_t)(server->security_failed_attempts + 1U);
        if (server->security_failed_attempts >= server->security_max_attempts) {
            server->security_lockout_active = server->security_lockout_ms != 0U;
            server->security_lockout_until_ms = now_ms + server->security_lockout_ms;
            server->security_state = server->security_lockout_active
                                         ? UDS_SECURITY_STATE_LOCKOUT
                                         : UDS_SECURITY_STATE_LOCKED_READY;
            return negative_response(request, UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS, response,
                                     response_len, capacity);
        }
        return negative_response(request, UDS_NRC_INVALID_KEY, response, response_len, capacity);
    }
    if (result != UDS_RESULT_OK) {
        return callback_result(server, result, request, response, response_len, capacity);
    }
    server->security_level = level;
    server->security_failed_attempts = 0U;
    security_invalidate_seed(server);
    server->security_state = UDS_SECURITY_STATE_UNLOCKED;
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
    (void)now_ms;
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

UdsCallbackResult uds_server_handle_addressed(UdsServer *server, const uint8_t *request,
                                              uint16_t request_len, uint8_t *response,
                                              uint16_t *response_len, uint16_t capacity,
                                              UdsAddressMode address_mode, uint32_t now_ms) {
    if ((server == NULL) || (request == NULL) || (response == NULL) || (response_len == NULL) ||
        (capacity == 0U) || (request_len == 0U) || (request_len > UDS_MAX_REQUEST_LENGTH)) {
        return UDS_RESULT_ERROR;
    }
    if ((address_mode != UDS_ADDRESS_PHYSICAL) && (address_mode != UDS_ADDRESS_FUNCTIONAL)) {
        return UDS_RESULT_ERROR;
    }
    uint8_t service = request[0];
    const UdsServiceAttribute *attribute = uds_service_attribute(
        service, (request_len > 1U) ? (request[1] & 0x7FU) : UDS_SERVICE_ANY_SUBFUNCTION);
    if (attribute->sid != 0U) {
        if ((attribute->address_mode != UDS_ADDRESS_MODE_BOTH) &&
            ((attribute->address_mode & address_mode) == 0U)) {
            return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION,
                                     response, response_len, capacity);
        }
        if ((attribute->session_mask & session_mask(server->session)) == 0U) {
            return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION,
                                     response, response_len, capacity);
        }
        if ((attribute->security_mask != UDS_SECURITY_MASK_NONE) &&
            ((attribute->security_mask & (uint16_t)(1U << server->security_level)) == 0U)) {
            return negative_response(request, UDS_NRC_SECURITY_ACCESS_DENIED, response,
                                     response_len, capacity);
        }
    }
    *response_len = 0U;
    server->last_activity_ms = now_ms;
    switch (service) {
    case 0x10U:
        return service_session_control(server, request, request_len, response, response_len,
                                       capacity, now_ms);
    case 0x11U:
        return service_ecu_reset(server, request, request_len, response, response_len, capacity);
    case 0x14U:
        return service_clear_dtc(server, request, request_len, response, response_len, capacity);
    case 0x19U:
        return service_read_dtc(server, request, request_len, response, response_len, capacity);
    case 0x22U:
        return service_read_data(server, request, request_len, response, response_len, capacity);
    case 0x27U:
        return service_security_access(server, request, request_len, response, response_len,
                                       capacity, now_ms);
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
    case 0x23U:
    case 0x24U:
    case 0x29U:
    case 0x2AU:
    case 0x2CU:
    case 0x2EU:
    case 0x35U:
    case 0x38U:
    case 0x3DU:
    case 0x83U:
    case 0x84U:
    case 0x86U:
    case 0x87U:
        return service_modular_backend(server, request, request_len, response, response_len,
                                       capacity);
    default:
        return negative_response(request, UDS_NRC_SERVICE_NOT_SUPPORTED, response, response_len,
                                 capacity);
    }
}

UdsCallbackResult uds_server_tick(UdsServer *server, uint32_t now_ms) {
    if (server == NULL) {
        return UDS_RESULT_ERROR;
    }
    security_tick(server, now_ms);
    if ((uint32_t)(now_ms - server->last_activity_ms) >= server->s3_server_timeout_ms) {
        server->session = UDS_SESSION_DEFAULT;
        security_reset_for_session_change(server);
        server->download_active = false;
        server->next_download_block = 1U;
        server->last_activity_ms = now_ms;
        return UDS_RESULT_BUSY;
    }
    return UDS_RESULT_OK;
}

UdsCallbackResult uds_server_handle(UdsServer *server, const uint8_t *request, uint16_t request_len,
                                    uint8_t *response, uint16_t *response_len, uint16_t capacity,
                                    uint32_t now_ms) {
    return uds_server_handle_addressed(server, request, request_len, response, response_len,
                                       capacity, UDS_ADDRESS_PHYSICAL, now_ms);
}

void uds_server_set_timing(UdsServer *server, uint32_t s3_timeout_ms,
                           uint32_t security_initial_delay_ms, uint32_t security_lockout_ms,
                           uint32_t security_seed_timeout_ms, uint8_t security_max_attempts) {

    if (server == NULL) {
        return;
    }
    server->s3_server_timeout_ms = s3_timeout_ms;
    /* Kept for source compatibility; the startup delay is intentionally inert. */
    server->security_initial_delay_ms = security_initial_delay_ms;
    server->security_initial_delay_active = false;
    server->security_lockout_ms = security_lockout_ms;
    server->security_seed_timeout_ms = security_seed_timeout_ms;
    server->security_max_attempts =
        (security_max_attempts == 0U) ? UDS_DEFAULT_SECURITY_MAX_ATTEMPTS : security_max_attempts;
}

bool uds_server_reset_pending(const UdsServer *server) {
    return (server != NULL) && server->reset_pending;
}

UdsCallbackResult uds_server_complete_reset(UdsServer *server) {
    if (server == NULL)
        return UDS_RESULT_ERROR;
    if (!server->reset_pending)
        return UDS_RESULT_SEQUENCE_ERROR;
    if (server->callbacks.ecu_reset_execute == NULL)
        return UDS_RESULT_NOT_SUPPORTED;
    uint8_t subfunction = server->pending_reset_subfunction;
    server->callbacks.ecu_reset_execute(server->context, subfunction);
    server->reset_pending = false;
    server->pending_reset_reason = UDS_RESET_NORMAL;
    server->pending_reset_subfunction = 0U;
    return UDS_RESULT_OK;
}

void uds_server_clear_reset(UdsServer *server) {
    if (server != NULL) {
        server->reset_pending = false;
        server->pending_reset_reason = UDS_RESET_NORMAL;
        server->pending_reset_subfunction = 0U;
    }
}

uint8_t uds_server_session(const UdsServer *server) {
    return (server != NULL) ? server->session : UDS_SESSION_DEFAULT;
}

UdsSecurityState uds_server_security_state(const UdsServer *server) {
    return (server != NULL) ? server->security_state : UDS_SECURITY_STATE_LOCKED_READY;
}

uint8_t uds_server_security_level(const UdsServer *server) {
    return (server != NULL) ? server->security_level : 0U;
}

uint8_t uds_server_security_failed_attempts(const UdsServer *server) {
    return (server != NULL) ? server->security_failed_attempts : 0U;
}

bool uds_server_security_seed_valid(const UdsServer *server) {
    return (server != NULL) && server->security_seed_valid;
}
