/**
 * @file svc_authentication.c
 * @brief UDS Service 0x29 Authentication Implementation (ISO 14229-1:2020)
 */

#include "svc_authentication.h"
#include "crypto_aes_cmac.h"
#include "uds_server.h"
#include <string.h>

typedef enum {
    AUTH_STATE_DEAUTHENTICATED = 0,
    AUTH_STATE_CHALLENGE_SENT,
    AUTH_STATE_AUTHENTICATED
} auth_fsm_state_t;

static auth_fsm_state_t s_auth_state = AUTH_STATE_DEAUTHENTICATED;
static uint8_t s_challenge[UDS_AUTH_CHALLENGE_LEN];
static uint8_t s_auth_key[16] = {
    0x29U, 0x01U, 0x29U, 0x02U, 0x29U, 0x03U, 0x29U, 0x04U,
    0x29U, 0x05U, 0x29U, 0x06U, 0x29U, 0x07U, 0x29U, 0x08U
};

/* PRNG state */
static uint32_t s_auth_prng = 0x12345678U;

static uint32_t auth_rand(void) {
    s_auth_prng ^= s_auth_prng << 13U;
    s_auth_prng ^= s_auth_prng >> 17U;
    s_auth_prng ^= s_auth_prng << 5U;
    if (s_auth_prng == 0U) {
        s_auth_prng = 0x9ABCDEF1U;
    }
    return s_auth_prng;
}

void uds_auth_init(void) {
    s_auth_state = AUTH_STATE_DEAUTHENTICATED;
    memset(s_challenge, 0, sizeof(s_challenge));
}

void uds_auth_reset(void) {
    s_auth_state = AUTH_STATE_DEAUTHENTICATED;
    memset(s_challenge, 0, sizeof(s_challenge));
}

bool uds_auth_is_authenticated(void) {
    return (s_auth_state == AUTH_STATE_AUTHENTICATED);
}

void uds_auth_set_master_key(const uint8_t key[16]) {
    if (key != NULL) {
        memcpy(s_auth_key, key, 16U);
    }
}

void svc_authentication(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x29U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    switch (sub) {
    case UDS_AUTH_SUB_DEAUTHENTICATE: {
        /* De-authenticate: resets session authentication */
        uds_auth_reset();
        uds_server_set_security_level(UDS_SEC_LEVEL_LOCKED);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = 0x00U; /* returnParameter: success */
        resp->len = 3U;
        break;
    }

    case UDS_AUTH_SUB_REQUEST_CHALLENGE: {
        /* Request challenge: generates 16-byte random challenge */
        for (uint8_t i = 0U; i < 4U; i++) {
            uint32_t r = auth_rand();
            s_challenge[i * 4U + 0U] = (uint8_t)(r >> 24U);
            s_challenge[i * 4U + 1U] = (uint8_t)(r >> 16U);
            s_challenge[i * 4U + 2U] = (uint8_t)(r >> 8U);
            s_challenge[i * 4U + 3U] = (uint8_t)(r & 0xFFU);
        }

        s_auth_state = AUTH_STATE_CHALLENGE_SENT;

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = 0x00U; /* status: ok */
        memcpy(&resp->data[3], s_challenge, UDS_AUTH_CHALLENGE_LEN);
        resp->len = 3U + UDS_AUTH_CHALLENGE_LEN;
        break;
    }

    case UDS_AUTH_SUB_VERIFY_PROOF_UNIDIR:
    case UDS_AUTH_SUB_PROOF_OF_OWNERSHIP: {
        /* Verify Proof of Ownership: expects 16-byte MAC */
        if (req->data_len != (1U + UDS_AUTH_PROOF_LEN)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        if (s_auth_state != AUTH_STATE_CHALLENGE_SENT) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3U;
            return;
        }

        /* Invalidate challenge */
        s_auth_state = AUTH_STATE_DEAUTHENTICATED;

        uint8_t expected_mac[16];
        if (!crypto_aes_cmac_128(s_auth_key, s_challenge, UDS_AUTH_CHALLENGE_LEN, expected_mac)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
            resp->len = 3U;
            return;
        }

        if (!crypto_constant_time_equal(&req->data[1], expected_mac, 16U)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INVALID_KEY;
            resp->len = 3U;
            return;
        }

        /* Authentication successful! Promote security level */
        s_auth_state = AUTH_STATE_AUTHENTICATED;
        uds_server_set_security_level(UDS_SEC_LEVEL_1 | UDS_SEC_LEVEL_2);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = 0x00U; /* returnParameter: verification successful */
        resp->len = 3U;
        break;
    }

    default:
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    if (suppress) {
        resp->suppress = true;
        resp->len = 0U;
    }
}
