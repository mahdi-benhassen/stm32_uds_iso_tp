/**
 * @file svc_upload.c
 * @brief UDS Service 0x35 RequestUpload Implementation (ISO 14229-1)
 */

#include "svc_upload.h"
#include "uds_transfer_fsm.h"
#include <string.h>

void svc_request_upload(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 3U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x35U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint8_t alfid = req->data[1];
    uint8_t size_len = (uint8_t)(alfid >> 4U);
    uint8_t addr_len = (uint8_t)(alfid & 0x0FU);

    if ((size_len == 0U) || (size_len > 4U) || (addr_len == 0U) || (addr_len > 4U) ||
        (req->data_len != (uint16_t)(2U + addr_len + size_len))) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    uint32_t address = 0U;
    for (uint8_t i = 0U; i < addr_len; i++) {
        address = (address << 8U) | req->data[2U + i];
    }

    uint32_t size = 0U;
    for (uint8_t i = 0U; i < size_len; i++) {
        size = (size << 8U) | req->data[2U + addr_len + i];
    }

    if (size == 0U) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_OUT_OF_RANGE;
        resp->len = 3U;
        return;
    }

    uds_transfer_ctx_t *ctx = uds_transfer_get_context();
    ctx->state = TRANSFER_UPLOAD_ACTIVE;
    ctx->memory_address = address;
    ctx->memory_size = size;
    ctx->transferred_bytes = 0U;
    ctx->block_sequence_counter = 1U;

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = 0x20U; /* lengthFormatIdentifier: 2 bytes of maxNumberOfBlockLength */
    resp->data[2] = (uint8_t)(UDS_MAX_BLOCK_LENGTH >> 8U);
    resp->data[3] = (uint8_t)(UDS_MAX_BLOCK_LENGTH & 0xFFU);
    resp->len = 4U;
}
