/**
 * @file svc_read_dtc.c
 * @brief UDS Service 0x19: ReadDTCInformation (ISO 14229-1)
 * @details Integrates seamlessly with dtc_store.h without duplicate type definitions.
 */

#include "svc_read_dtc.h"
#include "dtc_store.h"
#include <string.h>

void svc_read_dtc_info(const uds_request_t *req, uds_response_t *resp) {
    if (req->data_len < 1U) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);
    uint8_t avail_mask = dtc_store_get_status_mask();

    switch (sub) {
    case 0x01: {
        /* reportNumberOfDTCByStatusMask */
        if (req->data_len < 2U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
        uint8_t req_mask = req->data[1];
        uint16_t count = dtc_store_count_by_mask(req_mask);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = avail_mask;
        resp->data[3] = 0x01U; /* DTC format: ISO 14229-1 */
        resp->data[4] = (uint8_t)(count >> 8U);
        resp->data[5] = (uint8_t)(count & 0xFFU);
        resp->len = 6U;
        break;
    }

    case 0x02: {
        /* reportDTCByStatusMask */
        if (req->data_len < 2U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
        uint8_t req_mask = req->data[1];

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = avail_mask;
        uint16_t pos = 3U;

        uint8_t total = dtc_store_count();
        for (uint8_t i = 0U; i < total; i++) {
            const dtc_entry_t *e = dtc_store_get_by_index(i);
            if ((e == NULL) || ((e->status & req_mask) == 0U)) {
                continue;
            }

            if ((pos + 4U) > ISOTP_TX_BUF_SIZE) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_RESPONSE_TOO_LONG;
                resp->len = 3U;
                return;
            }

            resp->data[pos++] = (uint8_t)(e->dtc >> 16U);
            resp->data[pos++] = (uint8_t)(e->dtc >> 8U);
            resp->data[pos++] = (uint8_t)(e->dtc & 0xFFU);
            resp->data[pos++] = e->status;
        }

        resp->len = pos;
        break;
    }

    case 0x03: {
        /* reportDTCSnapshotIdentification (ISO 14229-1 Section 11.3.1.4, Issue #119) */
        if (req->data_len != 1U) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        uint16_t pos = 2U;

        uint8_t total = dtc_store_count();
        for (uint8_t i = 0U; i < total; i++) {
            const dtc_entry_t *e = dtc_store_get_by_index(i);
            if ((e != NULL) && e->has_snapshot && (e->snapshot_len > 0U)) {
                if ((pos + 4U) > ISOTP_TX_BUF_SIZE) {
                    resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                    resp->data[1] = req->sid;
                    resp->data[2] = NRC_RESPONSE_TOO_LONG;
                    resp->len = 3U;
                    return;
                }

                resp->data[pos++] = (uint8_t)(e->dtc >> 16U);
                resp->data[pos++] = (uint8_t)(e->dtc >> 8U);
                resp->data[pos++] = (uint8_t)(e->dtc & 0xFFU);
                resp->data[pos++] = 0x01U; /* DTCSnapshotRecordNumber = 0x01 */
            }
        }

        resp->len = pos;
        break;
    }

    case 0x0A: {
        /* reportSupportedDTC (ISO 14229-1 Section 11.3.1.10) */
        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = avail_mask;
        uint16_t pos = 3U;

        uint8_t total = dtc_store_count();
        for (uint8_t i = 0U; i < total; i++) {
            const dtc_entry_t *e = dtc_store_get_by_index(i);
            if (e == NULL) {
                continue;
            }

            if ((pos + 4U) > ISOTP_TX_BUF_SIZE) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_RESPONSE_TOO_LONG;
                resp->len = 3U;
                return;
            }

            resp->data[pos++] = (uint8_t)(e->dtc >> 16U);
            resp->data[pos++] = (uint8_t)(e->dtc >> 8U);
            resp->data[pos++] = (uint8_t)(e->dtc & 0xFFU);
            resp->data[pos++] = e->status;
        }

        resp->len = pos;
        break;
    }

    case 0x04: {
        /* reportDTCSnapshotRecordByDTCNumber */
        if (req->data_len < 5U) { /* SubFunction + DTC (3B) + RecordNum (1B) */
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
        uint32_t dtc = ((uint32_t)req->data[1] << 16U) |
                       ((uint32_t)req->data[2] << 8U)  |
                       (uint32_t)req->data[3];
        uint8_t rec_num = req->data[4];

        const dtc_entry_t *e = dtc_store_find(dtc);
        if (e == NULL) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(e->dtc >> 16U);
        resp->data[3] = (uint8_t)(e->dtc >> 8U);
        resp->data[4] = (uint8_t)(e->dtc & 0xFFU);
        resp->data[5] = e->status;
        uint16_t pos = 6U;

        if (e->has_snapshot && ((rec_num == 0x01U) || (rec_num == 0xFFU))) {
            if ((pos + e->snapshot_len) <= ISOTP_TX_BUF_SIZE) {
                memcpy(&resp->data[pos], e->snapshot, e->snapshot_len);
                pos = (uint16_t)(pos + e->snapshot_len);
            }
        }

        resp->len = pos;
        break;
    }

    case 0x06: {
        /* reportDTCExtDataRecordByDTCNumber (Issue #108) */
        if (req->data_len < 5U) { /* SubFunction + DTC (3B) + RecordNum (1B) */
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
        uint32_t dtc = ((uint32_t)req->data[1] << 16U) |
                       ((uint32_t)req->data[2] << 8U)  |
                       (uint32_t)req->data[3];
        uint8_t rec_num = req->data[4];

        const dtc_entry_t *e = dtc_store_find(dtc);
        if (e == NULL) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(e->dtc >> 16U);
        resp->data[3] = (uint8_t)(e->dtc >> 8U);
        resp->data[4] = (uint8_t)(e->dtc & 0xFFU);
        resp->data[5] = e->status;
        uint16_t pos = 6U;

        /* Extended Data Record 0x01: Fault Occurrence Counter */
        if ((rec_num == 0x01U) || (rec_num == 0xFFU)) {
            if ((pos + 2U) <= ISOTP_TX_BUF_SIZE) {
                resp->data[pos++] = 0x01U; /* Record number */
                resp->data[pos++] = e->occurrence_counter;
            }
        }

        /* Extended Data Record 0x02: Aging Counter */
        if ((rec_num == 0x02U) || (rec_num == 0xFFU)) {
            if ((pos + 2U) <= ISOTP_TX_BUF_SIZE) {
                resp->data[pos++] = 0x02U; /* Record number */
                resp->data[pos++] = e->aging_counter;
            }
        }

        resp->len = pos;
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
