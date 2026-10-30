/**
 * @file svc_dyn_did.c
 * @brief UDS Service 0x2C DynamicallyDefineDataIdentifier Implementation (ISO 14229-1)
 */

#include "svc_dyn_did.h"
#include "uds_did_table.h"
#include <string.h>

#define MAX_DYN_DIDS 4U
#define MAX_SOURCES_PER_DYN_DID 4U

typedef struct {
    uint16_t src_did;
    uint8_t  position; /* 1-based byte offset */
    uint8_t  length;   /* slice length in bytes */
} dyn_source_slice_t;

typedef struct {
    uint16_t           dyn_did;
    dyn_source_slice_t sources[MAX_SOURCES_PER_DYN_DID];
    uint8_t            source_count;
    bool               defined;
} dyn_did_def_t;

static dyn_did_def_t s_dyn_defs[MAX_DYN_DIDS];

void svc_dyn_did_init(void) {
    memset(s_dyn_defs, 0, sizeof(s_dyn_defs));
}

void svc_dyn_define_did(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x2CU;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t raw_sub = req->data[0];
    uint8_t sub = raw_sub & 0x7FU;
    bool suppress = ((raw_sub & 0x80U) != 0U);

    if (sub == 0x01U) {
        /* defineByIdentifier: [0x01, dynDID_Hi, dynDID_Lo, (srcDID_Hi, srcDID_Lo, pos, len)...] */
        if ((req->data_len < 7U) || (((req->data_len - 3U) % 4U) != 0U)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }

        uint16_t dyn_did = (uint16_t)(((uint16_t)req->data[1] << 8U) | (uint16_t)req->data[2]);
        if ((dyn_did < 0xF200U) || (dyn_did > 0xF3FFU)) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        uint8_t slice_count = (uint8_t)((req->data_len - 3U) / 4U);
        if (slice_count > MAX_SOURCES_PER_DYN_DID) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
            resp->len = 3U;
            return;
        }

        /* Find slot or overwrite */
        int8_t slot = -1;
        for (uint8_t i = 0U; i < MAX_DYN_DIDS; i++) {
            if (s_dyn_defs[i].defined && (s_dyn_defs[i].dyn_did == dyn_did)) {
                slot = (int8_t)i;
                break;
            }
            if (!s_dyn_defs[i].defined && (slot < 0)) {
                slot = (int8_t)i;
            }
        }

        if (slot < 0) {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_CONDITIONS_NOT_CORRECT;
            resp->len = 3U;
            return;
        }

        uint8_t composite[UDS_DID_MAX_DATA_LEN] = { 0 };
        uint16_t comp_pos = 0U;

        for (uint8_t i = 0U; i < slice_count; i++) {
            uint16_t offset = (uint16_t)(3U + (i * 4U));
            uint16_t src = (uint16_t)(((uint16_t)req->data[offset] << 8U) | (uint16_t)req->data[offset + 1U]);
            uint8_t pos = req->data[offset + 2U];
            uint8_t len = req->data[offset + 3U];

            const uds_did_record_t *src_rec = uds_did_table_find(src);
            if ((src_rec == NULL) || (pos == 0U) || ((pos - 1U + len) > src_rec->len)) {
                resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
                resp->data[1] = req->sid;
                resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
                resp->len = 3U;
                return;
            }

            if ((comp_pos + len) <= sizeof(composite)) {
                memcpy(&composite[comp_pos], &src_rec->data[pos - 1U], len);
                comp_pos = (uint16_t)(comp_pos + len);
            }
        }

        s_dyn_defs[slot].dyn_did = dyn_did;
        s_dyn_defs[slot].source_count = slice_count;
        s_dyn_defs[slot].defined = true;

        /* Register or update in main DID table */
        uds_did_table_register(dyn_did, composite, comp_pos, false, false);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = sub;
        resp->data[2] = (uint8_t)(dyn_did >> 8U);
        resp->data[3] = (uint8_t)(dyn_did & 0xFFU);
        resp->len = 4U;
    } else if (sub == 0x03U) {
        /* clearDynamicallyDefinedDataIdentifier */
        if (req->data_len == 1U) {
            /* Clear all */
            svc_dyn_did_init();
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = sub;
            resp->len = 2U;
        } else if (req->data_len == 3U) {
            uint16_t dyn_did = (uint16_t)(((uint16_t)req->data[1] << 8U) | (uint16_t)req->data[2]);
            for (uint8_t i = 0U; i < MAX_DYN_DIDS; i++) {
                if (s_dyn_defs[i].defined && (s_dyn_defs[i].dyn_did == dyn_did)) {
                    s_dyn_defs[i].defined = false;
                }
            }
            resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
            resp->data[1] = sub;
            resp->data[2] = (uint8_t)(dyn_did >> 8U);
            resp->data[3] = (uint8_t)(dyn_did & 0xFFU);
            resp->len = 4U;
        } else {
            resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
            resp->data[1] = req->sid;
            resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
            resp->len = 3U;
            return;
        }
    } else {
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
