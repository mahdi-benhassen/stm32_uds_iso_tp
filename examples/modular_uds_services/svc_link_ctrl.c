/**
 * @file svc_link_ctrl.c
 * @brief UDS Service 0x87 LinkControl Implementation (ISO 14229-1)
 */

#include "svc_link_ctrl.h"
#include <stddef.h>

typedef enum {
    LINK_CTRL_IDLE = 0,
    LINK_CTRL_VERIFIED
} link_ctrl_state_t;

static link_ctrl_state_t s_link_state = LINK_CTRL_IDLE;
static uint8_t s_verified_mode = 0U;

void svc_link_control_reset(void) {
    s_link_state = LINK_CTRL_IDLE;
    s_verified_mode = 0U;
}

void svc_link_control(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x87U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    switch (sub) {
    case 0x01U: {
        /* verifyModeTransitionWithFixedParameter */
        if (req->data_len != 2U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        uint8_t mode = req->data[1];
        /* Standard CAN baudrate identifiers: 0x12 (125k), 0x13 (250k), 0x14 (500k), 0x15 (1M) */
        if ((mode < 0x12U) || (mode > 0x15U)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        s_verified_mode = mode;
        s_link_state = LINK_CTRL_VERIFIED;

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->len = 2U;
        break;
    }

    case 0x03U: {
        /* transitionMode */
        if (req->data_len != 1U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        /* ISO 14229-1: Sequence check - must be preceded by verifyMode */
        if (s_link_state != LINK_CTRL_VERIFIED) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
            resp->len = 3U;
            return;
        }

        s_link_state = LINK_CTRL_IDLE;

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->len = 2U;
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
