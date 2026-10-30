/**
 * @file svc_read_did.c
 * @brief UDS Service 0x22 ReadDataByIdentifier Implementation (ISO 14229-1)
 */

#include "svc_read_did.h"
#include "uds_did_table.h"
#include "svc_security_access.h"
#include <string.h>

void svc_read_data_by_id(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 2U) || ((req->data_len % 2U) != 0U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x22U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    uint16_t resp_pos = 1U;
    uint16_t num_dids = (uint16_t)(req->data_len / 2U);

    for (uint16_t i = 0U; i < num_dids; i++) {
        uint16_t did = (uint16_t)(((uint16_t)req->data[i * 2U] << 8U) | (uint16_t)req->data[i * 2U + 1U]);
        const uds_did_record_t *rec = uds_did_table_find(did);

        if (rec == NULL) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        if (rec->secured && !uds_security_is_unlocked()) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_SECURITY_ACCESS_DENIED;
            resp->len = 3U;
            return;
        }

        /* Check response buffer bounds */
        if ((resp_pos + 2U + rec->len) > sizeof(resp->data)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_RESPONSE_TOO_LONG;
            resp->len = 3U;
            return;
        }

        resp->data[resp_pos++] = (uint8_t)(did >> 8U);
        resp->data[resp_pos++] = (uint8_t)(did & 0xFFU);
        memcpy(&resp->data[resp_pos], rec->data, rec->len);
        resp_pos = (uint16_t)(resp_pos + rec->len);
    }

    resp->len = resp_pos;
}
