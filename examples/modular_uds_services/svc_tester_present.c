/**
 * @file svc_tester_present.c
 * @brief UDS Service 0x3E TesterPresent Implementation (ISO 14229-1)
 */

#include "svc_tester_present.h"
#include "uds_session.h"
#include <stddef.h>

void svc_tester_present(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len != 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x3EU;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    if (sub != 0x00U) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    /* Refresh S3 timer on valid TesterPresent */
    uds_session_refresh();

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->len = 2U;

    if (suppress) {
        resp->suppress = true;
        resp->len = 0U;
    }
}
