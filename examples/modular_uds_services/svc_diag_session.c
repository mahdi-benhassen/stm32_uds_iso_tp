/**
 * @file svc_diag_session.c
 * @brief UDS Service 0x10 DiagnosticSessionControl Implementation (ISO 14229-1)
 */

#include "svc_diag_session.h"
#include "uds_session.h"
#include <stddef.h>

#define UDS_P2_SERVER_MAX_MS       50U     /* 50 ms standard P2 server */
#define UDS_P2_STAR_SERVER_MAX_RES 500U    /* 5000 ms (resolution 10ms -> 500) */

void svc_diag_session_control(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len != 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x10U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    if ((sub < (uint8_t)UDS_SESSION_DEFAULT) || (sub > (uint8_t)UDS_SESSION_ENGINEERING)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    /* Perform session transition */
    uds_session_set((uds_session_t)sub);

    /* Construct positive response: 50 [sub] [P2_ms_high P2_ms_low] [P2*_res_high P2*_res_low] */
    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->data[2] = (uint8_t)(UDS_P2_SERVER_MAX_MS >> 8U);
    resp->data[3] = (uint8_t)(UDS_P2_SERVER_MAX_MS & 0xFFU);
    resp->data[4] = (uint8_t)(UDS_P2_STAR_SERVER_MAX_RES >> 8U);
    resp->data[5] = (uint8_t)(UDS_P2_STAR_SERVER_MAX_RES & 0xFFU);
    resp->len = 6U;

    if (suppress) {
        resp->suppress = true;
        resp->len = 0U;
    }
}
