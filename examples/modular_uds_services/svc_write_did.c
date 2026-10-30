/**
 * @file svc_write_did.c
 * @brief UDS Service 0x2E WriteDataByIdentifier Implementation (ISO 14229-1)
 */

#include "svc_write_did.h"
#include "uds_did_table.h"
#include "svc_security_access.h"
#include <string.h>

void svc_write_data_by_id(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 3U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x2EU;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint16_t did = (uint16_t)(((uint16_t)req->data[0] << 8U) | (uint16_t)req->data[1]);
    const uint8_t *payload = &req->data[2];
    uint16_t payload_len = (uint16_t)(req->data_len - 2U);

    const uds_did_record_t *rec = uds_did_table_find(did);
    if ((rec == NULL) || !rec->writable) {
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

    if (!uds_did_table_write(did, payload, payload_len)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
        resp->len = 3U;
        return;
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = (uint8_t)(did >> 8U);
    resp->data[2] = (uint8_t)(did & 0xFFU);
    resp->len = 3U;
}
