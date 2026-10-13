#include "svc_control_dtc.h"
#include "dtc_store.h"

void svc_control_dtc(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1U) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = (raw_sub & 0x80U) != 0U;

    if ((sub != 0x01U) && (sub != 0x02U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_SUBFUNCTION_NOT_SUPPORTED;
        resp->len = 3U;
        return;
    }

    /* Subfunction 0x01: DTC Setting ON, 0x02: DTC Setting OFF */
    dtc_set_setting_enabled(sub == 0x01U);

    if (suppress) {
        resp->suppress = true;
        resp->len = 0U;
        return;
    }

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = sub;
    resp->len = 2U;
}
