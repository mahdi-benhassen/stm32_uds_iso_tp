/**
 * @file svc_download.c
 * @brief UDS Services 0x34 RequestDownload, 0x36 TransferData, 0x37 RequestTransferExit (ISO 14229-1)
 */

#include "svc_download.h"
#include "uds_transfer_fsm.h"
#include <string.h>

static uds_transfer_ctx_t s_transfer_ctx;

uds_transfer_ctx_t *uds_transfer_get_context(void) {
    return &s_transfer_ctx;
}

void uds_transfer_reset(void) {
    memset(&s_transfer_ctx, 0, sizeof(s_transfer_ctx));
    s_transfer_ctx.state = TRANSFER_IDLE;
}

void svc_request_download(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 3U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x34U;
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

    s_transfer_ctx.state = TRANSFER_DOWNLOAD_ACTIVE;
    s_transfer_ctx.memory_address = address;
    s_transfer_ctx.memory_size = size;
    s_transfer_ctx.transferred_bytes = 0U;
    s_transfer_ctx.block_sequence_counter = 1U;

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->data[1] = 0x20U; /* lengthFormatIdentifier: 2 bytes of maxNumberOfBlockLength */
    resp->data[2] = (uint8_t)(UDS_MAX_BLOCK_LENGTH >> 8U);
    resp->data[3] = (uint8_t)(UDS_MAX_BLOCK_LENGTH & 0xFFU);
    resp->len = 4U;
}

void svc_transfer_data(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if ((req == NULL) || (req->data_len < 1U)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = (req != NULL) ? req->sid : 0x36U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    if ((s_transfer_ctx.state != TRANSFER_DOWNLOAD_ACTIVE) &&
        (s_transfer_ctx.state != TRANSFER_UPLOAD_ACTIVE)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
        resp->len = 3U;
        return;
    }

    uint8_t bsc = req->data[0];
    if (bsc != s_transfer_ctx.block_sequence_counter) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_WRONG_BLOCK_SEQUENCE_COUNTER;
        resp->len = 3U;
        return;
    }

    if (s_transfer_ctx.state == TRANSFER_DOWNLOAD_ACTIVE) {
        uint16_t chunk_len = (uint16_t)(req->data_len - 1U);
        s_transfer_ctx.transferred_bytes += chunk_len;
        s_transfer_ctx.block_sequence_counter = (uint8_t)(s_transfer_ctx.block_sequence_counter + 1U);

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = bsc;
        resp->len = 2U;
    } else {
        /* Upload transfer */
        uint32_t remaining = (s_transfer_ctx.memory_size > s_transfer_ctx.transferred_bytes) ?
                             (s_transfer_ctx.memory_size - s_transfer_ctx.transferred_bytes) : 0U;
        uint16_t send_len = (remaining > (UDS_MAX_BLOCK_LENGTH - 2U)) ?
                            (uint16_t)(UDS_MAX_BLOCK_LENGTH - 2U) : (uint16_t)remaining;

        resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
        resp->data[1] = bsc;
        /* Simulate upload data pattern */
        for (uint16_t i = 0U; i < send_len; i++) {
            resp->data[2U + i] = (uint8_t)((s_transfer_ctx.transferred_bytes + i) & 0xFFU);
        }
        resp->len = 2U + send_len;

        s_transfer_ctx.transferred_bytes += send_len;
        s_transfer_ctx.block_sequence_counter = (uint8_t)(s_transfer_ctx.block_sequence_counter + 1U);
    }
}

void svc_transfer_exit(const uds_request_t *req, uds_response_t *resp) {
    if (resp == NULL) {
        return;
    }
    if (req == NULL) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = 0x37U;
        resp->data[2] = NRC_INCORRECT_MSG_LEN_OR_FORMAT;
        resp->len = 3U;
        return;
    }

    if ((s_transfer_ctx.state != TRANSFER_DOWNLOAD_ACTIVE) &&
        (s_transfer_ctx.state != TRANSFER_UPLOAD_ACTIVE)) {
        resp->data[0] = UDS_SID_NEGATIVE_RESPONSE;
        resp->data[1] = req->sid;
        resp->data[2] = NRC_REQUEST_SEQUENCE_ERROR;
        resp->len = 3U;
        return;
    }

    uds_transfer_reset();

    resp->data[0] = UDS_POSITIVE_RESPONSE(req->sid);
    resp->len = 1U;
}
