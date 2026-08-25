/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_iso_tp/uds.h"
#include "uds_iso_tp/uds_security_provider.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool key_callback_denied;

static UdsCallbackResult security_seed(void *context, uint8_t level, uint8_t *seed,
                                       uint16_t *length, uint16_t capacity) {
    (void)context;
    if (((level != UDS_SECURITY_LEVEL_1) && (level != UDS_SECURITY_LEVEL_5)) || (capacity < 4U)) {
        return UDS_RESULT_OUT_OF_RANGE;
    }
    seed[0] = level;
    seed[1] = 0x12U;
    seed[2] = 0x34U;
    seed[3] = 0x56U;
    *length = 4U;
    return UDS_RESULT_OK;
}

static UdsCallbackResult security_key(void *context, uint8_t level, const uint8_t *key,
                                      uint16_t length) {
    (void)context;
    if (key_callback_denied) {
        return UDS_RESULT_DENIED;
    }
    if ((length != 4U) || (key[0] != 0xCAU) || (key[1] != 0xFEU) || (key[2] != level) ||
        (key[3] != 0x55U)) {
        return UDS_RESULT_INVALID_KEY;
    }
    return UDS_RESULT_OK;
}

static UdsCallbackResult ecu_reset(void *context, uint8_t subfunction) {
    (void)context;
    return (subfunction == 1U) ? UDS_RESULT_OK : UDS_RESULT_OUT_OF_RANGE;
}

static UdsCallbacks callbacks(void) {
    UdsCallbacks value;
    memset(&value, 0, sizeof(value));
    value.security_seed = security_seed;
    value.security_key = security_key;
    value.ecu_reset = ecu_reset;
    return value;
}

static UdsCallbackResult request(UdsServer *server, uint32_t now_ms, const uint8_t *data,
                                 uint16_t length, uint8_t *response, uint16_t *response_length) {
    return uds_server_handle(server, data, length, response, response_length, 64U, now_ms);
}

static void configure(UdsServer *server, uint32_t now_ms) {
    UdsCallbacks value = callbacks();
    uds_server_init(server, &value, NULL, now_ms);
    uds_server_set_timing(server, 100U, 10000U, 10000U, 1000U, 3U);
}

static void enter_extended(UdsServer *server, uint32_t now_ms) {
    uint8_t response[64];
    uint16_t response_length = 0U;
    const uint8_t data[] = {0x10U, UDS_SESSION_EXTENDED};
    assert(request(server, now_ms, data, sizeof(data), response, &response_length) ==
           UDS_RESULT_OK);
    assert(response_length == 6U && response[0] == 0x50U && response[1] == UDS_SESSION_EXTENDED);
}

static void expect_nrc(UdsServer *server, uint32_t now_ms, const uint8_t *data, uint16_t length,
                       uint8_t nrc) {
    uint8_t response[64];
    uint16_t response_length = 0U;
    assert(request(server, now_ms, data, length, response, &response_length) == UDS_RESULT_OK);
    assert(response_length == 3U && response[0] == 0x7FU && response[1] == data[0] &&
           response[2] == nrc);
}

static void expect_seed(UdsServer *server, uint32_t now_ms, uint8_t subfunction) {
    uint8_t response[64];
    uint16_t response_length = 0U;
    const uint8_t data[] = {0x27U, subfunction};
    assert(request(server, now_ms, data, sizeof(data), response, &response_length) ==
           UDS_RESULT_OK);
    assert(response_length == 6U && response[0] == 0x67U && response[1] == subfunction &&
           response[2] == (subfunction == UDS_SECURITY_REQUEST_SEED_LEVEL_1 ? 1U : 5U));
    assert(uds_server_security_seed_valid(server));
    assert(uds_server_security_state(server) == UDS_SECURITY_STATE_WAITING_FOR_KEY);
}

static void expect_key(UdsServer *server, uint32_t now_ms, uint8_t subfunction, bool valid,
                       uint8_t expected_nrc) {
    uint8_t response[64];
    uint16_t response_length = 0U;
    uint8_t level = (subfunction == UDS_SECURITY_SEND_KEY_LEVEL_1) ? UDS_SECURITY_LEVEL_1
                                                                   : UDS_SECURITY_LEVEL_5;
    uint8_t data[] = {0x27U, subfunction, 0xCAU, 0xFEU, level, 0x55U};
    if (!valid) {
        data[2] = 0x00U;
    }
    UdsCallbackResult result =
        request(server, now_ms, data, sizeof(data), response, &response_length);
    assert(result == UDS_RESULT_OK);
    if (valid) {
        assert(response_length == 2U && response[0] == 0x67U && response[1] == subfunction);
        assert(uds_server_security_state(server) == UDS_SECURITY_STATE_UNLOCKED);
        assert(!uds_server_security_seed_valid(server));
    } else {
        assert(response_length == 3U && response[0] == 0x7FU && response[1] == 0x27U &&
               response[2] == expected_nrc);
    }
}

static void test_transition_policy(void) {
    assert(uds_session_transition_allowed(UDS_SESSION_DEFAULT, UDS_SESSION_DEFAULT) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_DEFAULT, UDS_SESSION_EXTENDED) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_DEFAULT, UDS_SESSION_PROGRAMMING) ==
           UDS_SESSION_TRANSITION_DENIED);
    assert(uds_session_transition_allowed(UDS_SESSION_DEFAULT, UDS_SESSION_SAFETY) ==
           UDS_SESSION_TRANSITION_DENIED);
    assert(uds_session_transition_allowed(UDS_SESSION_EXTENDED, UDS_SESSION_DEFAULT) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_EXTENDED, UDS_SESSION_PROGRAMMING) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_EXTENDED, UDS_SESSION_SAFETY) ==
           UDS_SESSION_TRANSITION_DENIED);
    assert(uds_session_transition_allowed(UDS_SESSION_PROGRAMMING, UDS_SESSION_DEFAULT) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_PROGRAMMING, UDS_SESSION_EXTENDED) ==
           UDS_SESSION_TRANSITION_DENIED);
    assert(uds_session_transition_allowed(UDS_SESSION_SAFETY, UDS_SESSION_DEFAULT) ==
           UDS_SESSION_TRANSITION_ALLOWED);
    assert(uds_session_transition_allowed(UDS_SESSION_SAFETY, UDS_SESSION_EXTENDED) ==
           UDS_SESSION_TRANSITION_DENIED);
}

static void test_session_control(void) {
    UdsServer server;
    uint8_t response[64];
    uint16_t response_length = 0U;
    configure(&server, 0U);
    assert(uds_server_session(&server) == UDS_SESSION_DEFAULT);
    enter_extended(&server, 0U);
    assert(uds_server_session(&server) == UDS_SESSION_EXTENDED);

    const uint8_t programming[] = {0x10U, UDS_SESSION_PROGRAMMING};
    assert(request(&server, 1U, programming, sizeof(programming), response, &response_length) ==
           UDS_RESULT_OK);
    assert(response[0] == 0x50U && response[1] == UDS_SESSION_PROGRAMMING);
    assert(uds_server_reset_pending(&server));
    assert(uds_server_session(&server) == UDS_SESSION_PROGRAMMING);

    uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, 2U);
    assert(uds_server_session(&server) == UDS_SESSION_DEFAULT);
    assert(uds_server_security_state(&server) == UDS_SECURITY_STATE_LOCKED);
    assert(!uds_server_security_seed_valid(&server));
    assert(!uds_server_reset_pending(&server));

    enter_extended(&server, 3U);
    const uint8_t suppressed[] = {0x10U, (uint8_t)(UDS_SESSION_EXTENDED | 0x80U)};
    response_length = 99U;
    assert(request(&server, 4U, suppressed, sizeof(suppressed), response, &response_length) ==
           UDS_RESULT_NO_RESPONSE);
    assert(response_length == 0U);

    const uint8_t invalid_length[] = {0x10U};
    expect_nrc(&server, 5U, invalid_length, sizeof(invalid_length),
               UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
    const uint8_t invalid_session[] = {0x10U, 0x05U};
    expect_nrc(&server, 6U, invalid_session, sizeof(invalid_session),
               UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);

    const uint8_t to_default[] = {0x10U, UDS_SESSION_DEFAULT};
    assert(request(&server, 7U, to_default, sizeof(to_default), response, &response_length) ==
           UDS_RESULT_OK);
    assert(uds_server_reset_pending(&server));
    assert(server.pending_reset_reason == UDS_RESET_NORMAL);
}

static void test_s3_and_reset(void) {
    UdsServer server;
    uint8_t response[64];
    uint16_t response_length = 0U;
    configure(&server, 0U);
    uds_server_set_timing(&server, 100U, 0U, 10000U, 1000U, 3U);
    uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, 0U);
    enter_extended(&server, 0U);
    assert(uds_server_tick(&server, 99U) == UDS_RESULT_OK);
    assert(uds_server_session(&server) == UDS_SESSION_EXTENDED);
    const uint8_t tester_present[] = {0x3EU, 0x00U};
    assert(request(&server, 99U, tester_present, sizeof(tester_present), response,
                   &response_length) == UDS_RESULT_OK);
    assert(uds_server_tick(&server, 198U) == UDS_RESULT_OK);
    assert(uds_server_tick(&server, 199U) == UDS_RESULT_BUSY);
    assert(uds_server_session(&server) == UDS_SESSION_DEFAULT);
    assert(uds_server_security_level(&server) == 0U);

    enter_extended(&server, 200U);
    const uint8_t reset[] = {0x11U, 0x01U};
    assert(request(&server, 201U, reset, sizeof(reset), response, &response_length) ==
           UDS_RESULT_OK);
    assert(uds_server_reset_pending(&server));
    uds_server_apply_reset(&server, UDS_RESET_NORMAL, 202U);
    assert(uds_server_session(&server) == UDS_SESSION_DEFAULT);
    assert(uds_server_security_state(&server) == UDS_SECURITY_STATE_LOCKED);
    const uint8_t seed[] = {0x27U, UDS_SECURITY_REQUEST_SEED_LEVEL_1};
    expect_nrc(&server, 203U, seed, sizeof(seed), UDS_NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION);
}

static void test_security_delay_and_lockout(void) {
    UdsServer server;
    configure(&server, 0U);
    uds_server_set_timing(&server, 60000U, 10000U, 10000U, 1000U, 3U);
    enter_extended(&server, 0U);
    const uint8_t seed[] = {0x27U, UDS_SECURITY_REQUEST_SEED_LEVEL_1};
    expect_nrc(&server, 9999U, seed, sizeof(seed), UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED);
    expect_seed(&server, 10000U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    expect_key(&server, 10001U, UDS_SECURITY_SEND_KEY_LEVEL_1, false, UDS_NRC_INVALID_KEY);
    assert(uds_server_security_failed_attempts(&server) == 1U);
    expect_nrc(&server, 10002U, (const uint8_t[]){0x27U, UDS_SECURITY_SEND_KEY_LEVEL_1}, 2U,
               UDS_NRC_REQUEST_SEQUENCE_ERROR);

    expect_seed(&server, 10003U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    expect_key(&server, 10004U, UDS_SECURITY_SEND_KEY_LEVEL_1, false, UDS_NRC_INVALID_KEY);
    assert(uds_server_security_failed_attempts(&server) == 2U);
    expect_seed(&server, 10005U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    expect_key(&server, 10006U, UDS_SECURITY_SEND_KEY_LEVEL_1, false,
               UDS_NRC_EXCEEDED_NUMBER_OF_ATTEMPTS);
    assert(uds_server_security_failed_attempts(&server) == 3U);
    assert(uds_server_security_state(&server) == UDS_SECURITY_STATE_DELAY);
    expect_nrc(&server, 10007U, seed, sizeof(seed), UDS_NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED);
    assert(uds_server_tick(&server, 20006U) == UDS_RESULT_OK);
    assert(uds_server_security_failed_attempts(&server) == 2U);
    expect_seed(&server, 20007U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
}

static void test_security_success_seed_lifecycle_and_levels(void) {
    UdsServer server;
    configure(&server, 0U);
    uds_server_set_timing(&server, 60000U, 10000U, 10000U, 1000U, 3U);
    uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, 30000U);
    enter_extended(&server, 30000U);
    expect_seed(&server, 30001U, UDS_SECURITY_REQUEST_SEED_LEVEL_5);
    expect_key(&server, 30002U, UDS_SECURITY_SEND_KEY_LEVEL_5, true, 0U);
    assert(uds_server_security_level(&server) == UDS_SECURITY_LEVEL_5);
    assert(uds_server_security_failed_attempts(&server) == 0U);
    expect_nrc(&server, 30003U, (const uint8_t[]){0x27U, UDS_SECURITY_SEND_KEY_LEVEL_5, 0xCAU}, 3U,
               UDS_NRC_REQUEST_SEQUENCE_ERROR);

    expect_seed(&server, 30004U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    assert(uds_server_tick(&server, 31004U) == UDS_RESULT_OK);
    assert(!uds_server_security_seed_valid(&server));
    expect_nrc(&server, 31005U, (const uint8_t[]){0x27U, UDS_SECURITY_SEND_KEY_LEVEL_1, 0xCAU}, 3U,
               UDS_NRC_REQUEST_SEQUENCE_ERROR);

    expect_seed(&server, 31006U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    key_callback_denied = true;
    expect_nrc(&server, 31007U,
               (const uint8_t[]){0x27U, UDS_SECURITY_SEND_KEY_LEVEL_1, 0xCAU, 0xFEU, 0x01U, 0x55U},
               6U, UDS_NRC_CONDITIONS_NOT_CORRECT);
    key_callback_denied = false;
    assert(uds_server_security_failed_attempts(&server) == 0U);
    assert(uds_server_security_seed_valid(&server));

    const uint8_t to_default[] = {0x10U, UDS_SESSION_DEFAULT};
    uint8_t response[64];
    uint16_t response_length = 0U;
    assert(request(&server, 31008U, to_default, sizeof(to_default), response, &response_length) ==
           UDS_RESULT_OK);
    assert(uds_server_security_level(&server) == 0U);
    assert(!uds_server_security_seed_valid(&server));
}

static void test_security_provider_contract(void) {
    UdsSecurityProvider provider;
    uint8_t seed[UDS_SECURITY_PROVIDER_SEED_LENGTH];
    uint16_t seed_length = 0U;
    uds_security_provider_init(&provider, 0x12345678UL, 3U, 10000U);
    assert(uds_security_provider_state(&provider) == UDS_SECURITY_PROVIDER_STATE_DELAY);
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               9999U) == UDS_SECURITY_DELAY_ACTIVE);
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               10000U) == UDS_SECURITY_OK);
    assert(seed_length == UDS_SECURITY_PROVIDER_SEED_LENGTH);
    assert(uds_security_provider_state(&provider) == UDS_SECURITY_PROVIDER_STATE_WAITING_FOR_KEY);
    uint8_t wrong_key[UDS_SECURITY_PROVIDER_SEED_LENGTH] = {0U, 0U, 0U, 0U};
    assert(uds_security_provider_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 10001U) ==
           UDS_SECURITY_INVALID_KEY);
    assert(uds_security_provider_failed_attempts(&provider) == 1U);
    assert(uds_security_provider_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 10002U) ==
           UDS_SECURITY_SEQUENCE_ERROR);

    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               10003U) == UDS_SECURITY_OK);
    uint8_t key[UDS_SECURITY_PROVIDER_SEED_LENGTH] = {0U, 0U, 0U, 0U};
    static const uint8_t test_mask[UDS_SECURITY_PROVIDER_SEED_LENGTH] = {0xA5U, 0x5AU, 0xC3U,
                                                                         0x3CU};
    for (size_t index = 0U; index < sizeof(key); ++index) {
        key[index] = (uint8_t)(seed[index] ^ test_mask[index]);
    }
    assert(uds_security_provider_verify_key(&provider, 1U, key, sizeof(key), 10004U) ==
           UDS_SECURITY_OK);
    assert(uds_security_provider_state(&provider) == UDS_SECURITY_PROVIDER_STATE_UNLOCKED);
    assert(uds_security_provider_security_level(&provider) == 1U);
    assert(uds_security_provider_failed_attempts(&provider) == 0U);

    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               20000U) == UDS_SECURITY_OK);
    assert(uds_security_provider_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20001U) ==
           UDS_SECURITY_INVALID_KEY);
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               20002U) == UDS_SECURITY_OK);
    assert(uds_security_provider_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20003U) ==
           UDS_SECURITY_INVALID_KEY);
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               20004U) == UDS_SECURITY_OK);
    assert(uds_security_provider_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20005U) ==
           UDS_SECURITY_ATTEMPTS_EXCEEDED);
    assert(uds_security_provider_state(&provider) == UDS_SECURITY_PROVIDER_STATE_DELAY);
    assert(uds_security_provider_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                               20006U) == UDS_SECURITY_DELAY_ACTIVE);
    uds_security_provider_tick(&provider, 30005U);
    assert(uds_security_provider_state(&provider) == UDS_SECURITY_PROVIDER_STATE_LOCKED);
    assert(uds_security_provider_failed_attempts(&provider) == 2U);
}

static void test_malformed_security_requests(void) {
    UdsServer server;
    configure(&server, 0U);
    enter_extended(&server, 0U);
    uds_server_apply_reset(&server, UDS_RESET_PROGRAMMING, 10000U);
    enter_extended(&server, 10000U);
    expect_nrc(&server, 10001U, (const uint8_t[]){0x27U}, 1U,
               UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
    expect_nrc(&server, 10002U, (const uint8_t[]){0x27U, 0x00U}, 2U,
               UDS_NRC_SUBFUNCTION_NOT_SUPPORTED);
    expect_nrc(&server, 10003U, (const uint8_t[]){0x27U, UDS_SECURITY_SEND_KEY_LEVEL_1, 0xCAU}, 3U,
               UDS_NRC_REQUEST_SEQUENCE_ERROR);
    expect_seed(&server, 10004U, UDS_SECURITY_REQUEST_SEED_LEVEL_1);
    expect_nrc(&server, 10005U, (const uint8_t[]){0x27U, UDS_SECURITY_REQUEST_SEED_LEVEL_1, 0x00U},
               3U, UDS_NRC_INCORRECT_MESSAGE_LENGTH_OR_INVALID_FORMAT);
}

int main(void) {
    test_transition_policy();
    test_session_control();
    test_s3_and_reset();
    test_security_delay_and_lockout();
    test_security_success_seed_lifecycle_and_levels();
    test_security_provider_contract();
    test_malformed_security_requests();
    return 0;
}
