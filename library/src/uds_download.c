/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#include "uds_download.h"

#include <stddef.h>
#include <string.h>

static bool time_expired(uint32_t now_ms, uint32_t deadline_ms) {
    return (int32_t)(now_ms - deadline_ms) >= 0;
}

static bool range_inside(UdsDownloadRegion region, uint32_t address, uint32_t length) {
    if (length == 0U || address < region.start || region.end_exclusive < region.start) {
        return false;
    }
    return length <= (region.end_exclusive - address);
}

static bool aligned(uint32_t value, uint32_t alignment) {
    return (alignment != 0U) && ((value % alignment) == 0U);
}

static bool overlaps(UdsDownloadRegion left, UdsDownloadRegion right) {
    return (left.start < right.end_exclusive) && (right.start < left.end_exclusive);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint16_t length) {
    uint32_t value = crc;
    for (uint16_t index = 0U; index < length; ++index) {
        value ^= data[index];
        for (uint8_t bit = 0U; bit < 8U; ++bit) {
            value = ((value & 1U) != 0U) ? ((value >> 1U) ^ 0xEDB88320UL) : (value >> 1U);
        }
    }
    return value;
}

void uds_download_init(UdsDownload *download, const UdsDownloadMemoryMap *memory,
                       const UdsDownloadCallbacks *callbacks, void *context) {
    if (download == NULL) {
        return;
    }
    if (memory != NULL) {
        download->memory = *memory;
    } else {
        download->memory.staging_image.start = 0U;
        download->memory.staging_image.end_exclusive = 0U;
        download->memory.bootloader.start = 0U;
        download->memory.bootloader.end_exclusive = 0U;
        download->memory.active_application.start = 0U;
        download->memory.active_application.end_exclusive = 0U;
        download->memory.persistent_storage.start = 0U;
        download->memory.persistent_storage.end_exclusive = 0U;
        download->memory.diagnostic_storage.start = 0U;
        download->memory.diagnostic_storage.end_exclusive = 0U;
        download->memory.erase_alignment = 0U;
        download->memory.program_alignment = 0U;
        download->memory.max_block_length = 0U;
        download->memory.activation_supported = false;
    }
    if (callbacks != NULL) {
        download->callbacks = *callbacks;
    } else {
        download->callbacks.erase_start = NULL;
        download->callbacks.erase_poll = NULL;
        download->callbacks.program = NULL;
        download->callbacks.verify_image = NULL;
        download->callbacks.abort = NULL;
        download->callbacks.watchdog_kick = NULL;
    }
    download->context = context;
    download->state = UDS_DOWNLOAD_IDLE;
    download->metadata.magic = 0U;
    download->metadata.image_address = 0U;
    download->metadata.image_length = 0U;
    download->metadata.received_length = 0U;
    download->metadata.crc32 = 0U;
    for (uint8_t index = 0U; index < 32U; ++index)
        download->metadata.hash[index] = 0U;
    download->metadata.crc_valid = false;
    download->metadata.hash_valid = false;
    download->metadata.activation_pending = false;
    download->expected_block = 1U;
    download->deadline_ms = 0U;
    download->crc32 = 0xFFFFFFFFUL;
}

UdsDownloadResult uds_download_begin(UdsDownload *download, uint32_t address, uint32_t length,
                                     uint32_t now_ms) {
    if ((download == NULL) || (download->callbacks.erase_start == NULL) ||
        (download->callbacks.erase_poll == NULL) || (download->callbacks.program == NULL) ||
        (download->callbacks.verify_image == NULL)) {
        return UDS_DOWNLOAD_INVALID_ARGUMENT;
    }
    UdsDownloadRegion image = {address, address + length};
    if ((length == 0U) || (image.end_exclusive < address) ||
        !range_inside(download->memory.staging_image, address, length) ||
        overlaps(image, download->memory.bootloader) ||
        overlaps(image, download->memory.active_application) ||
        overlaps(image, download->memory.persistent_storage) ||
        overlaps(image, download->memory.diagnostic_storage)) {
        return UDS_DOWNLOAD_OUT_OF_RANGE;
    }
    if (!aligned(address, download->memory.erase_alignment) ||
        !aligned(length, download->memory.erase_alignment) ||
        !aligned(address, download->memory.program_alignment)) {
        return UDS_DOWNLOAD_ALIGNMENT_ERROR;
    }
    if ((download->memory.max_block_length < 3U) ||
        (download->memory.max_block_length > UDS_DOWNLOAD_MAX_CHUNK_LENGTH)) {
        return UDS_DOWNLOAD_INVALID_ARGUMENT;
    }
    UdsDownloadResult result = download->callbacks.erase_start(download->context, address, length);
    if (result != UDS_DOWNLOAD_OK) {
        return result;
    }
    download->metadata.magic = UDS_DOWNLOAD_METADATA_MAGIC;
    download->metadata.image_address = address;
    download->metadata.image_length = length;
    download->metadata.received_length = 0U;
    download->metadata.crc32 = 0xFFFFFFFFUL;
    download->metadata.crc_valid = false;
    download->metadata.hash_valid = false;
    download->metadata.activation_pending = false;
    download->crc32 = 0xFFFFFFFFUL;
    download->expected_block = 1U;
    download->deadline_ms = now_ms + UDS_DOWNLOAD_TIMEOUT_MS;
    download->state = UDS_DOWNLOAD_ERASING;
    return UDS_DOWNLOAD_OK;
}

UdsDownloadResult uds_download_poll_erase(UdsDownload *download, uint32_t now_ms) {
    if ((download == NULL) || (download->state != UDS_DOWNLOAD_ERASING) ||
        (download->callbacks.erase_poll == NULL)) {
        return UDS_DOWNLOAD_NOT_READY;
    }
    if (time_expired(now_ms, download->deadline_ms)) {
        uds_download_abort(download);
        return UDS_DOWNLOAD_TIMEOUT;
    }
    if (download->callbacks.watchdog_kick != NULL) {
        download->callbacks.watchdog_kick(download->context);
    }
    UdsDownloadResult result = download->callbacks.erase_poll(download->context);
    if (result == UDS_DOWNLOAD_OK) {
        download->state = UDS_DOWNLOAD_RECEIVING;
        download->deadline_ms = now_ms + UDS_DOWNLOAD_TIMEOUT_MS;
    }
    return result;
}

UdsDownloadResult uds_download_write(UdsDownload *download, uint8_t block, const uint8_t *data,
                                     uint16_t length, uint32_t now_ms) {
    if ((download == NULL) || (data == NULL) || (length == 0U) ||
        (length > UDS_DOWNLOAD_MAX_CHUNK_LENGTH) || (download->state != UDS_DOWNLOAD_RECEIVING) ||
        (download->callbacks.program == NULL)) {
        return UDS_DOWNLOAD_NOT_READY;
    }
    if (time_expired(now_ms, download->deadline_ms)) {
        uds_download_abort(download);
        return UDS_DOWNLOAD_TIMEOUT;
    }
    if (block != download->expected_block) {
        return UDS_DOWNLOAD_SEQUENCE_ERROR;
    }
    if (length > (download->metadata.image_length - download->metadata.received_length)) {
        return UDS_DOWNLOAD_OVERFLOW;
    }
    if ((download->metadata.received_length % download->memory.program_alignment) != 0U ||
        (length % download->memory.program_alignment) != 0U) {
        return UDS_DOWNLOAD_ALIGNMENT_ERROR;
    }
    uint32_t address = download->metadata.image_address + download->metadata.received_length;
    UdsDownloadResult result =
        download->callbacks.program(download->context, address, data, length);
    if (result != UDS_DOWNLOAD_OK) {
        return UDS_DOWNLOAD_PROGRAM_ERROR;
    }
    download->metadata.received_length += length;
    download->crc32 = crc32_update(download->crc32, data, length);
    download->metadata.crc32 = download->crc32;
    download->expected_block = (uint8_t)(download->expected_block + 1U);
    download->deadline_ms = now_ms + UDS_DOWNLOAD_TIMEOUT_MS;
    if (download->callbacks.watchdog_kick != NULL) {
        download->callbacks.watchdog_kick(download->context);
    }
    return UDS_DOWNLOAD_OK;
}

UdsDownloadResult uds_download_finish(UdsDownload *download, uint32_t expected_crc32,
                                      bool has_expected_crc32, uint32_t now_ms) {
    if ((download == NULL) || (download->state != UDS_DOWNLOAD_RECEIVING) ||
        (download->metadata.received_length != download->metadata.image_length) ||
        (download->callbacks.verify_image == NULL)) {
        return UDS_DOWNLOAD_NOT_READY;
    }
    if (time_expired(now_ms, download->deadline_ms)) {
        uds_download_abort(download);
        return UDS_DOWNLOAD_TIMEOUT;
    }
    download->state = UDS_DOWNLOAD_VERIFYING;
    download->metadata.crc_valid =
        !has_expected_crc32 || ((download->crc32 ^ 0xFFFFFFFFUL) == expected_crc32);
    if (!download->metadata.crc_valid) {
        uds_download_abort(download);
        return UDS_DOWNLOAD_VERIFY_ERROR;
    }
    UdsDownloadResult result = download->callbacks.verify_image(
        download->context, &download->metadata, expected_crc32, has_expected_crc32);
    if (result != UDS_DOWNLOAD_OK) {
        uds_download_abort(download);
        return result;
    }
    download->metadata.activation_pending = !download->memory.activation_supported;
    download->state = UDS_DOWNLOAD_COMPLETE;
    return UDS_DOWNLOAD_OK;
}

UdsDownloadResult uds_download_tick(UdsDownload *download, uint32_t now_ms) {
    if ((download == NULL) || (download->state == UDS_DOWNLOAD_IDLE) ||
        (download->state == UDS_DOWNLOAD_COMPLETE) ||
        (download->state == UDS_DOWNLOAD_ABORTED_STATE)) {
        return UDS_DOWNLOAD_OK;
    }
    if (time_expired(now_ms, download->deadline_ms)) {
        uds_download_abort(download);
        return UDS_DOWNLOAD_TIMEOUT;
    }
    if (download->callbacks.watchdog_kick != NULL) {
        download->callbacks.watchdog_kick(download->context);
    }
    return UDS_DOWNLOAD_OK;
}

void uds_download_abort(UdsDownload *download) {
    if (download == NULL) {
        return;
    }
    if (download->callbacks.abort != NULL) {
        download->callbacks.abort(download->context);
    }
    download->state = UDS_DOWNLOAD_ABORTED_STATE;
    download->metadata.activation_pending = false;
}

UdsDownloadState uds_download_state(const UdsDownload *download) {
    return (download != NULL) ? download->state : UDS_DOWNLOAD_ABORTED_STATE;
}

const UdsDownloadMetadata *uds_download_metadata(const UdsDownload *download) {
    return (download != NULL) ? &download->metadata : NULL;
}

bool uds_download_activation_pending(const UdsDownload *download) {
    return (download != NULL) && download->metadata.activation_pending;
}
