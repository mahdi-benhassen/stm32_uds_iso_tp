/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 */
#include "uds_download.h"

#include <assert.h>
#include <stdint.h>

struct TestContext {
    uint32_t erase_calls;
    uint32_t program_calls;
    uint32_t verify_calls;
    uint32_t abort_calls;
    uint32_t watchdog_calls;
};

static UdsDownloadResult erase_start(void *context, uint32_t address, uint32_t length) {
    struct TestContext *test = context;
    assert(address == 0x08080000UL && length == 8U);
    ++test->erase_calls;
    return UDS_DOWNLOAD_OK;
}

static UdsDownloadResult erase_poll(void *context) {
    struct TestContext *test = context;
    ++test->erase_calls;
    return UDS_DOWNLOAD_OK;
}

static UdsDownloadResult program(void *context, uint32_t address, const uint8_t *data,
                                 uint16_t length) {
    struct TestContext *test = context;
    (void)data;
    assert((address == 0x08080000UL) || (address == 0x08080004UL));
    assert(length == 4U);
    ++test->program_calls;
    return UDS_DOWNLOAD_OK;
}

static UdsDownloadResult verify(void *context, const UdsDownloadMetadata *metadata,
                                uint32_t expected_crc32, bool has_expected_crc32) {
    struct TestContext *test = context;
    (void)expected_crc32;
    assert(metadata->received_length == 8U);
    assert(!has_expected_crc32 || metadata->crc_valid);
    ++test->verify_calls;
    return UDS_DOWNLOAD_OK;
}

static void abort_download(void *context) {
    struct TestContext *test = context;
    ++test->abort_calls;
}

static void watchdog(void *context) {
    struct TestContext *test = context;
    ++test->watchdog_calls;
}

static UdsDownloadMemoryMap memory_map(void) {
    UdsDownloadMemoryMap memory = {
        .staging_image = {0x08080000UL, 0x08081000UL},
        .bootloader = {0x08000000UL, 0x08040000UL},
        .active_application = {0x08040000UL, 0x08080000UL},
        .canopen_nvm = {0x08180000UL, 0x08200000UL},
        .diagnostic_storage = {0x08170000UL, 0x08180000UL},
        .erase_alignment = 4U,
        .program_alignment = 4U,
        .max_block_length = 64U,
        .activation_supported = false,
    };
    return memory;
}

static UdsDownloadCallbacks callbacks(void) {
    UdsDownloadCallbacks value = {
        .erase_start = erase_start,
        .erase_poll = erase_poll,
        .program = program,
        .verify_image = verify,
        .abort = abort_download,
        .watchdog_kick = watchdog,
    };
    return value;
}

int main(void) {
    struct TestContext context = {0};
    UdsDownload download;
    UdsDownloadMemoryMap memory = memory_map();
    UdsDownloadCallbacks download_callbacks = callbacks();
    uds_download_init(&download, &memory, &download_callbacks, &context);

    assert(uds_download_begin(&download, 0x08000000UL, 8U, 0U) == UDS_DOWNLOAD_OUT_OF_RANGE);
    assert(uds_download_begin(&download, 0x08080002UL, 4U, 0U) == UDS_DOWNLOAD_ALIGNMENT_ERROR);
    assert(uds_download_begin(&download, 0x08080000UL, 8U, 0U) == UDS_DOWNLOAD_OK);
    assert(uds_download_state(&download) == UDS_DOWNLOAD_ERASING);
    assert(uds_download_poll_erase(&download, 1U) == UDS_DOWNLOAD_OK);
    assert(uds_download_state(&download) == UDS_DOWNLOAD_RECEIVING);

    uint8_t payload[4] = {1U, 2U, 3U, 4U};
    assert(uds_download_write(&download, 2U, payload, sizeof(payload), 2U) ==
           UDS_DOWNLOAD_SEQUENCE_ERROR);
    assert(uds_download_write(&download, 1U, payload, sizeof(payload), 2U) == UDS_DOWNLOAD_OK);
    assert(uds_download_write(&download, 2U, payload, sizeof(payload), 3U) == UDS_DOWNLOAD_OK);
    assert(uds_download_finish(&download, 0U, false, 4U) == UDS_DOWNLOAD_OK);
    assert(uds_download_state(&download) == UDS_DOWNLOAD_COMPLETE);
    assert(uds_download_activation_pending(&download));
    assert(context.erase_calls == 2U && context.program_calls == 2U && context.verify_calls == 1U &&
           context.abort_calls == 0U);
    assert(context.watchdog_calls >= 3U);

    uds_download_init(&download, &memory, &download_callbacks, &context);
    assert(uds_download_begin(&download, 0x08080000UL, 8U, 100U) == UDS_DOWNLOAD_OK);
    assert(uds_download_tick(&download, 5101U) == UDS_DOWNLOAD_TIMEOUT);
    assert(uds_download_state(&download) == UDS_DOWNLOAD_ABORTED_STATE);
    assert(context.abort_calls == 1U);
    return 0;
}
