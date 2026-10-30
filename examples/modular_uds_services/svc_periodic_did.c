/**
 * @file svc_periodic_did.c
 * @brief UDS Service 0x2A ReadDataByPeriodicIdentifier Implementation (ISO 14229-1)
 */

#include "svc_periodic_did.h"
#include "uds_did_table.h"
#include <string.h>

#define MAX_PERIODIC_DIDS 8U

typedef struct {
    uint8_t  periodic_id;
    uint8_t  mode;
    uint16_t period_ticks;
    uint16_t elapsed_ticks;
    bool     active;
} periodic_did_entry_t;

static periodic_did_entry_t s_periodic_schedule[MAX_PERIODIC_DIDS];
static uint8_t s_periodic_count = 0U;

void svc_periodic_did_reset(void) {
    memset(s_periodic_schedule, 0, sizeof(s_periodic_schedule));
    s_periodic_count = 0U;
}

void svc_read_periodic_id(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x2AU;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t mode = req->data[0];

    /* Subfunction mode check: 0x01 (slow), 0x02 (medium), 0x03 (fast), 0x04 (stop) */
    if ((mode < 0x01U) || (mode > 0x04U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    if (mode == 0x04U) {
        /* Stop sending */
        if (req->data_len == 1U) {
            /* Stop all */
            svc_periodic_did_reset();
        } else {
            /* Stop specific periodic IDs */
            for (uint16_t i = 1U; i < req->data_len; i++) {
                uint8_t pid = req->data[i];
                for (uint8_t j = 0U; j < MAX_PERIODIC_DIDS; j++) {
                    if (s_periodic_schedule[j].active && (s_periodic_schedule[j].periodic_id == pid)) {
                        s_periodic_schedule[j].active = false;
                    }
                }
            }
        }
        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->len = 1U;
        return;
    }

    /* Schedule periodic IDs */
    if (req->data_len < 2U) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint16_t period = (mode == 0x01U) ? 1000U : ((mode == 0x02U) ? 200U : 50U);

    for (uint16_t i = 1U; i < req->data_len; i++) {
        uint8_t pid = req->data[i];
        /* Find existing or slot */
        int8_t slot = -1;
        for (uint8_t j = 0U; j < MAX_PERIODIC_DIDS; j++) {
            if (s_periodic_schedule[j].active && (s_periodic_schedule[j].periodic_id == pid)) {
                slot = (int8_t)j;
                break;
            }
            if (!s_periodic_schedule[j].active && (slot < 0)) {
                slot = (int8_t)j;
            }
        }

        if (slot >= 0) {
            s_periodic_schedule[slot].periodic_id = pid;
            s_periodic_schedule[slot].mode = mode;
            s_periodic_schedule[slot].period_ticks = period;
            s_periodic_schedule[slot].elapsed_ticks = 0U;
            s_periodic_schedule[slot].active = true;
        }
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->len = 1U;
}

void svc_periodic_did_poll(void) {
    for (uint8_t i = 0U; i < MAX_PERIODIC_DIDS; i++) {
        if (s_periodic_schedule[i].active) {
            s_periodic_schedule[i].elapsed_ticks++;
            if (s_periodic_schedule[i].elapsed_ticks >= s_periodic_schedule[i].period_ticks) {
                s_periodic_schedule[i].elapsed_ticks = 0U;
                /* Period elapsed: ready for transmission */
            }
        }
    }
}
