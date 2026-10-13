/**
 * @file svc_security_access.c
 * @brief UDS Service 0x27: SecurityAccess Implementation (ISO 14229-1)
 * @details Implements Dual-Level Security:
 *          - Level 1 (0x01 / 0x02): 4-byte Seed/Key OEM basic authentication
 *          - Level 2 (0x03 / 0x04): 16-byte 128-bit cryptographic AES-CMAC-128
 */

#include "svc_security_access.h"
#include "crypto_aes_cmac.h"
#include <string.h>

static security_state_t s_sec_state = SECURITY_LOCKED;
static uint32_t         s_active_security_mask = UDS_SEC_LEVEL_LOCKED;
static uint8_t          s_sec_attempts = 0U;
static bool             s_lockout_active = false;
static uint32_t         s_lockout_ticks = 0U;

static uint8_t s_seed_l1[SECURITY_SEED_LEN_L1];
static uint8_t s_seed_l2[SECURITY_SEED_LEN_L2];

/* Default secret key for Level 1 XOR calculation */
static uint8_t s_secret_l1[4] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
static uint8_t s_secret_l1_len = 4U;

/* Default 128-bit AES master key for Level 2 (NIST SP 800-38B standard test key) */
static uint8_t s_aes_master_key[16] = {
    0x2BU, 0x7EU, 0x15U, 0x16U, 0x28U, 0xAEU, 0xD2U, 0xA6U,
    0xABU, 0xF7U, 0x15U, 0x88U, 0x09U, 0xCFU, 0x4FU, 0x3CU
};

/* PRNG state for seed generation */
static uint32_t s_prng_state = 0x87654321U;

static uint32_t prng_next(void) {
    /* 32-bit Xorshift PRNG */
    s_prng_state ^= s_prng_state << 13U;
    s_prng_state ^= s_prng_state >> 17U;
    s_prng_state ^= s_prng_state << 5U;
    if (s_prng_state == 0U) {
        s_prng_state = 0x13579BDFU;
    }
    return s_prng_state;
}

void uds_security_init(void) {
    s_sec_state = SECURITY_LOCKED;
    s_active_security_mask = UDS_SEC_LEVEL_LOCKED;
    s_sec_attempts = 0U;
    s_lockout_active = false;
    s_lockout_ticks = 0U;
    memset(s_seed_l1, 0, sizeof(s_seed_l1));
    memset(s_seed_l2, 0, sizeof(s_seed_l2));
    uds_server_dispatch_set_security_level(UDS_SEC_LEVEL_LOCKED);
}

void uds_security_reset(void) {
    s_sec_state = SECURITY_LOCKED;
    s_active_security_mask = UDS_SEC_LEVEL_LOCKED;
    memset(s_seed_l1, 0, sizeof(s_seed_l1));
    memset(s_seed_l2, 0, sizeof(s_seed_l2));
    uds_server_dispatch_set_security_level(UDS_SEC_LEVEL_LOCKED);
}

bool uds_security_is_unlocked(void) {
    return (s_active_security_mask != UDS_SEC_LEVEL_LOCKED);
}

bool uds_security_is_level_unlocked(uint8_t level) {
    if (level == 1U) {
        return ((s_active_security_mask & UDS_SEC_LEVEL_1) != 0U);
    }
    if (level == 2U) {
        return ((s_active_security_mask & UDS_SEC_LEVEL_2) != 0U);
    }
    return false;
}

uint32_t uds_security_get_active_mask(void) {
    return s_active_security_mask;
}

security_state_t uds_security_get_state(void) {
    return s_sec_state;
}

void uds_security_poll(void) {
    if (s_lockout_active) {
        if (s_lockout_ticks > 0U) {
            s_lockout_ticks--;
        } else {
            s_lockout_active = false;
            s_sec_attempts = 0U;
        }
    }
}

void uds_security_set_aes_key(const uint8_t key[16]) {
    if (key != NULL) {
        memcpy(s_aes_master_key, key, 16U);
    }
}

void uds_security_set_secret_l1(const uint8_t *secret, uint8_t len) {
    if ((secret != NULL) && (len > 0U) && (len <= 4U)) {
        memcpy(s_secret_l1, secret, len);
        s_secret_l1_len = len;
    }
}

void svc_security_access(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x27U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    /* Validate supported sub-functions (0x01, 0x02, 0x03, 0x04) */
    if ((sub < UDS_SEC_SUB_REQUEST_SEED_L1) || (sub > UDS_SEC_SUB_SEND_KEY_L2)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    /* Lockout time delay enforcement (NRC 0x37) */
    if (s_lockout_active) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED;
        resp->len = 3U;
        return;
    }

    switch (sub) {
    case UDS_SEC_SUB_REQUEST_SEED_L1: {
        if (req->data_len != 1U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;

        /* ISO 14229-1: If already unlocked at Level 1, return seed of all zeroes */
        if ((s_active_security_mask & UDS_SEC_LEVEL_1) != 0U) {
            memset(&resp->data[2], 0, SECURITY_SEED_LEN_L1);
            resp->len = 2U + SECURITY_SEED_LEN_L1;
            break;
        }

        /* Generate new 4-byte seed */
        uint32_t rnd = prng_next();
        s_seed_l1[0] = (uint8_t)(rnd >> 24U);
        s_seed_l1[1] = (uint8_t)(rnd >> 16U);
        s_seed_l1[2] = (uint8_t)(rnd >> 8U);
        s_seed_l1[3] = (uint8_t)(rnd & 0xFFU);
        if (rnd == 0U) {
            s_seed_l1[0] = 0xA5U;
        }

        memcpy(&resp->data[2], s_seed_l1, SECURITY_SEED_LEN_L1);
        s_sec_state = SECURITY_SEED_SENT_L1;
        resp->len = 2U + SECURITY_SEED_LEN_L1;
        break;
    }

    case UDS_SEC_SUB_SEND_KEY_L1: {
        if (req->data_len != (1U + SECURITY_KEY_LEN_L1)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        /* Enforce request sequence (NRC 0x24) */
        if (s_sec_state != SECURITY_SEED_SENT_L1) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3U;
            return;
        }

        /* Anti-replay: invalidate seed immediately */
        s_sec_state = SECURITY_LOCKED;

        /* Calculate expected key: seed XOR secret */
        uint8_t expected_key[SECURITY_KEY_LEN_L1];
        for (uint8_t i = 0U; i < SECURITY_KEY_LEN_L1; i++) {
            expected_key[i] = (uint8_t)(s_seed_l1[i] ^ s_secret_l1[i % s_secret_l1_len]);
        }

        if (crypto_constant_time_equal(&req->data[1], expected_key, SECURITY_KEY_LEN_L1)) {
            /* Key verified */
            s_sec_attempts = 0U;
            s_sec_state = SECURITY_UNLOCKED_L1;
            s_active_security_mask |= UDS_SEC_LEVEL_1;
            uds_server_dispatch_set_security_level(s_active_security_mask);

            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = sub;
            resp->len = 2U;
        } else {
            /* Invalid key */
            s_sec_attempts++;
            if (s_sec_attempts >= SECURITY_MAX_ATTEMPTS) {
                s_lockout_active = true;
                s_lockout_ticks = SECURITY_LOCKOUT_MS;
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_EXCEED_NUMBER_OF_ATTEMPTS;
                resp->len = 3U;
            } else {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_INVALID_KEY;
                resp->len = 3U;
            }
            return;
        }
        break;
    }

    case UDS_SEC_SUB_REQUEST_SEED_L2: {
        if (req->data_len != 1U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;

        /* ISO 14229-1: If already unlocked at Level 2, return 16 zeroes */
        if ((s_active_security_mask & UDS_SEC_LEVEL_2) != 0U) {
            memset(&resp->data[2], 0, SECURITY_SEED_LEN_L2);
            resp->len = 2U + SECURITY_SEED_LEN_L2;
            break;
        }

        /* Generate 16-byte random seed */
        for (uint8_t i = 0U; i < 4U; i++) {
            uint32_t rnd = prng_next();
            s_seed_l2[i * 4U + 0U] = (uint8_t)(rnd >> 24U);
            s_seed_l2[i * 4U + 1U] = (uint8_t)(rnd >> 16U);
            s_seed_l2[i * 4U + 2U] = (uint8_t)(rnd >> 8U);
            s_seed_l2[i * 4U + 3U] = (uint8_t)(rnd & 0xFFU);
        }

        memcpy(&resp->data[2], s_seed_l2, SECURITY_SEED_LEN_L2);
        s_sec_state = SECURITY_SEED_SENT_L2;
        resp->len = 2U + SECURITY_SEED_LEN_L2;
        break;
    }

    case UDS_SEC_SUB_SEND_KEY_L2: {
        if (req->data_len != (1U + SECURITY_KEY_LEN_L2)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        /* Enforce sequence error */
        if (s_sec_state != SECURITY_SEED_SENT_L2) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3U;
            return;
        }

        /* Invalidate seed */
        s_sec_state = SECURITY_LOCKED;

        /* Generate expected AES-CMAC-128 tag */
        uint8_t expected_mac[SECURITY_KEY_LEN_L2];
        if (!crypto_aes_cmac_128(s_aes_master_key, s_seed_l2, SECURITY_SEED_LEN_L2, expected_mac)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
            resp->len = 3U;
            return;
        }

        /* Constant-time comparison */
        if (crypto_constant_time_equal(&req->data[1], expected_mac, SECURITY_KEY_LEN_L2)) {
            s_sec_attempts = 0U;
            s_sec_state = SECURITY_UNLOCKED_L2;
            s_active_security_mask |= UDS_SEC_LEVEL_2;
            uds_server_dispatch_set_security_level(s_active_security_mask);

            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = sub;
            resp->len = 2U;
        } else {
            s_sec_attempts++;
            if (s_sec_attempts >= SECURITY_MAX_ATTEMPTS) {
                s_lockout_active = true;
                s_lockout_ticks = SECURITY_LOCKOUT_MS;
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_EXCEED_NUMBER_OF_ATTEMPTS;
                resp->len = 3U;
            } else {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_INVALID_KEY;
                resp->len = 3U;
            }
            return;
        }
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
