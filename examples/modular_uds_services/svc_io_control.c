/**
 * @file svc_io_control.c
 * @brief UDS Service 0x2F InputOutputControlByIdentifier Implementation (ISO 14229-1)
 */

#include "svc_io_control.h"
#include <string.h>

#define MAX_IO_DIDS 4U

typedef struct {
    uint16_t id;
    uint8_t  current_state;
    uint8_t  default_state;
    bool     active_override;
} io_control_entry_t;

static io_control_entry_t s_io_table[MAX_IO_DIDS] = {
    { 0x0100U, 0x00U, 0x00U, false }, /* Status LED */
    { 0x0101U, 0x00U, 0x00U, false }, /* Power Relay */
    { 0x0102U, 0x50U, 0x50U, false }, /* PWM Cooling Fan (50%) */
    { 0x0103U, 0x00U, 0x00U, false }  /* Buzzer */
};

void svc_io_control(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 3U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x2FU;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint16_t did = (uint16_t)(((uint16_t)req->data[0] << 8U) | (uint16_t)req->data[1]);
    uint8_t ctrl_param = req->data[2];

    int8_t found_idx = -1;
    for (uint8_t i = 0U; i < MAX_IO_DIDS; i++) {
        if (s_io_table[i].id == did) {
            found_idx = (int8_t)i;
            break;
        }
    }

    if (found_idx < 0) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3U;
        return;
    }

    io_control_entry_t *entry = &s_io_table[found_idx];

    switch (ctrl_param) {
    case 0x00U: /* returnControlToECU */
        entry->active_override = false;
        entry->current_state = entry->default_state;
        break;

    case 0x01U: /* resetToDefault */
        entry->current_state = entry->default_state;
        entry->active_override = true;
        break;

    case 0x02U: /* freezeCurrentState */
        entry->active_override = true;
        break;

    case 0x03U: /* shortTermAdjustment */
        if (req->data_len < 4U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
        entry->current_state = req->data[3];
        entry->active_override = true;
        break;

    default:
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3U;
        return;
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = (uint8_t)(did >> 8U);
    resp->data[2] = (uint8_t)(did & 0xFFU);
    resp->data[3] = ctrl_param;
    resp->data[4] = entry->current_state;
    resp->len = 5U;
}
