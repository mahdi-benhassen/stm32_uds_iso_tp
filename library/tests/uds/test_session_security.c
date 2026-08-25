/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_iso_tp/uds.h"
#include "uds_security_reference.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static bool key_callback_denied;
static UdsSecurityReferenceCallbacks reference_callbacks;
static uint8_t latest_seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];

static UdsCallbackResult security_seed(void *context, uint8_t level, uint8_t *seed,
                                       uint16_t *length, uint16_t capacity) {
    return uds_security_reference_seed_callback(context, level, seed, length, capacity);
}

static UdsCallbackResult security_key(void *context, uint8_t level, const uint8_t *key,
                                      uint16_t length) {
    if (key_callback_denied) {
        return UDS_RESULT_DENIED;
    }
    return uds_security_reference_key_callback(context, level, key, length);
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
    uds_security_reference_callbacks_init(&reference_callbacks, 0x12345678UL);
    memset(latest_seed, 0, sizeof(latest_seed));
    uds_server_init(server, &value, &reference_callbacks, now_ms);
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
    assert(response_length == 6U && response[0] == 0x67U && response[1] == subfunction);
    memcpy(latest_seed, &response[2], sizeof(latest_seed));
    assert(uds_server_security_seed_valid(server));
    assert(uds_server_security_state(server) == UDS_SECURITY_STATE_WAITING_FOR_KEY);
}

static void expect_key(UdsServer *server, uint32_t now_ms, uint8_t subfunction, bool valid,
                       uint8_t expected_nrc) {
    uint8_t response[64];
    uint16_t response_length = 0U;
    uint8_t level = (subfunction == UDS_SECURITY_SEND_KEY_LEVEL_1) ? UDS_SECURITY_LEVEL_1
                                                                   : UDS_SECURITY_LEVEL_5;
    uint8_t key[UDS_SECURITY_REFERENCE_MAX_KEY_LENGTH] = {0U};
    uint16_t key_length = 0U;
    assert(uds_security_reference_calculate_key(level, latest_seed, sizeof(latest_seed), key,
                                                sizeof(key), &key_length));
    assert(key_length == UDS_SECURITY_REFERENCE_SEED_LENGTH);
    uint8_t data[] = {0x27U, subfunction, key[0], key[1], key[2], key[3]};
    if (!valid) {
        data[2] ^= 0x01U;
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

static void test_reference_algorithm_vectors(void) {
    static const struct {
        uint8_t level;
        uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];
        uint8_t key[UDS_SECURITY_REFERENCE_SEED_LENGTH];
    } vectors[] = {
        {UDS_SECURITY_REFERENCE_LEVEL_1,
         {0x00U, 0x11U, 0x22U, 0x33U},
         {0xA5U, 0x4BU, 0xE1U, 0x0FU}},
        {UDS_SECURITY_REFERENCE_LEVEL_5,
         {0xDEU, 0xADU, 0xBEU, 0xEFU},
         {0x7BU, 0xF7U, 0x7DU, 0xD3U}},
    };
    for (size_t vector_index = 0U; vector_index < (sizeof(vectors) / sizeof(vectors[0]));
         ++vector_index) {
        uint8_t calculated[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
        uint16_t calculated_length = 0U;
        assert(uds_security_reference_calculate_key(vectors[vector_index].level,
                                                    vectors[vector_index].seed,
                                                    sizeof(vectors[vector_index].seed), calculated,
                                                    sizeof(calculated), &calculated_length));
        assert(calculated_length == sizeof(vectors[vector_index].key));
        assert(memcmp(calculated, vectors[vector_index].key, sizeof(calculated)) == 0);
    }

    UdsSecurityReference provider;
    uint8_t generated_seed[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
    uint16_t generated_length = 0U;
    uds_security_reference_init(&provider, 0x12345678UL, 3U, 10000U);
    assert(uds_security_reference_generate_seed(
               &provider, UDS_SECURITY_REFERENCE_LEVEL_1, generated_seed, &generated_length,
               sizeof(generated_seed), 10000U) == UDS_SECURITY_REFERENCE_OK);
    assert(memcmp(generated_seed, (const uint8_t[]){0x11U, 0xD5U, 0x93U, 0xFEU},
                  sizeof(generated_seed)) == 0);
    assert(uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_1, generated_seed,
                                                sizeof(generated_seed), generated_seed,
                                                sizeof(generated_seed), &generated_length));
    assert(memcmp(generated_seed, (const uint8_t[]){0xB4U, 0x8FU, 0x50U, 0xC2U},
                  sizeof(generated_seed)) == 0);

    uds_security_reference_init(&provider, 0x12345678UL, 3U, 10000U);
    assert(uds_security_reference_generate_seed(
               &provider, UDS_SECURITY_REFERENCE_LEVEL_5, generated_seed, &generated_length,
               sizeof(generated_seed), 10000U) == UDS_SECURITY_REFERENCE_OK);
    assert(memcmp(generated_seed, (const uint8_t[]){0x01U, 0x86U, 0x54U, 0x39U},
                  sizeof(generated_seed)) == 0);

    for (uint8_t seed_number = 0U; seed_number < 32U; ++seed_number) {
        uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];
        uint8_t key_a[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
        uint8_t key_b[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
        uint16_t key_a_length = 0U;
        uint16_t key_b_length = 0U;
        for (uint8_t index = 0U; index < UDS_SECURITY_REFERENCE_SEED_LENGTH; ++index) {
            seed[index] = (uint8_t)(seed_number * 7U + index * 29U);
        }
        assert(uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_1, seed,
                                                    sizeof(seed), key_a, sizeof(key_a),
                                                    &key_a_length));
        assert(uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_1, seed,
                                                    sizeof(seed), key_b, sizeof(key_b),
                                                    &key_b_length));
        assert(key_a_length == key_b_length);
        assert(memcmp(key_a, key_b, sizeof(key_a)) == 0);
    }

    UdsSecurityReferenceCallbacks callbacks;
    uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
    uint8_t key[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U};
    uint16_t seed_length = 0U;
    uds_security_reference_callbacks_init(&callbacks, 0x01020304UL);
    assert(uds_security_reference_seed_callback(&callbacks, UDS_SECURITY_REFERENCE_LEVEL_5, seed,
                                                &seed_length, sizeof(seed)) == UDS_RESULT_OK);
    assert(seed_length == sizeof(seed));
    assert(uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_5, seed, sizeof(seed),
                                                key, sizeof(key), &seed_length));
    key[0] ^= 0x01U;
    assert(uds_security_reference_key_callback(&callbacks, UDS_SECURITY_REFERENCE_LEVEL_5, key,
                                               sizeof(key)) == UDS_RESULT_INVALID_KEY);
    assert(uds_security_reference_key_callback(&callbacks, UDS_SECURITY_REFERENCE_LEVEL_5, key,
                                               sizeof(key)) == UDS_RESULT_SEQUENCE_ERROR);

    assert(!uds_security_reference_calculate_key(3U, seed, sizeof(seed), key, sizeof(key),
                                                 &seed_length));
    assert(!uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_1, seed, 3U, key,
                                                 sizeof(key), &seed_length));
}

static void test_security_provider_contract(void) {
    UdsSecurityReference provider;
    uint8_t seed[UDS_SECURITY_REFERENCE_SEED_LENGTH];
    uint16_t seed_length = 0U;
    uds_security_reference_init(&provider, 0x12345678UL, 3U, 10000U);
    assert(uds_security_reference_state(&provider) == UDS_SECURITY_REFERENCE_STATE_DELAY);
    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                9999U) == UDS_SECURITY_REFERENCE_DELAY_ACTIVE);
    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                10000U) == UDS_SECURITY_REFERENCE_OK);
    assert(seed_length == UDS_SECURITY_REFERENCE_SEED_LENGTH);
    assert(uds_security_reference_state(&provider) == UDS_SECURITY_REFERENCE_STATE_WAITING_FOR_KEY);
    uint8_t wrong_key[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U, 0U, 0U, 0U};
    assert(uds_security_reference_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 10001U) ==
           UDS_SECURITY_REFERENCE_INVALID_KEY);
    assert(uds_security_reference_failed_attempts(&provider) == 1U);
    assert(uds_security_reference_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 10002U) ==
           UDS_SECURITY_REFERENCE_SEQUENCE_ERROR);

    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                10003U) == UDS_SECURITY_REFERENCE_OK);
    uint8_t key[UDS_SECURITY_REFERENCE_SEED_LENGTH] = {0U, 0U, 0U, 0U};
    uint16_t calculated_length = 0U;
    assert(uds_security_reference_calculate_key(UDS_SECURITY_REFERENCE_LEVEL_1, seed, sizeof(seed),
                                                key, sizeof(key), &calculated_length));
    assert(calculated_length == sizeof(key));
    assert(uds_security_reference_verify_key(&provider, 1U, key, sizeof(key), 10004U) ==
           UDS_SECURITY_REFERENCE_OK);
    assert(uds_security_reference_state(&provider) == UDS_SECURITY_REFERENCE_STATE_UNLOCKED);
    assert(uds_security_reference_security_level(&provider) == 1U);
    assert(uds_security_reference_failed_attempts(&provider) == 0U);

    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                20000U) == UDS_SECURITY_REFERENCE_OK);
    assert(uds_security_reference_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20001U) ==
           UDS_SECURITY_REFERENCE_INVALID_KEY);
    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                20002U) == UDS_SECURITY_REFERENCE_OK);
    assert(uds_security_reference_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20003U) ==
           UDS_SECURITY_REFERENCE_INVALID_KEY);
    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                20004U) == UDS_SECURITY_REFERENCE_OK);
    assert(uds_security_reference_verify_key(&provider, 1U, wrong_key, sizeof(wrong_key), 20005U) ==
           UDS_SECURITY_REFERENCE_ATTEMPTS_EXCEEDED);
    assert(uds_security_reference_state(&provider) == UDS_SECURITY_REFERENCE_STATE_DELAY);
    assert(uds_security_reference_generate_seed(&provider, 1U, seed, &seed_length, sizeof(seed),
                                                20006U) == UDS_SECURITY_REFERENCE_DELAY_ACTIVE);
    uds_security_reference_tick(&provider, 30005U);
    assert(uds_security_reference_state(&provider) == UDS_SECURITY_REFERENCE_STATE_LOCKED);
    assert(uds_security_reference_failed_attempts(&provider) == 2U);
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
    test_reference_algorithm_vectors();
    test_security_provider_contract();
    test_malformed_security_requests();
    return 0;
}
