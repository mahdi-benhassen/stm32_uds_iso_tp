/*
 * SPDX-License-Identifier: LicenseRef-STM32-UDS-Research-Education-Commercial-1.0
 */
#ifndef STM32_UDS_ISO_TP_DOWNLOAD_H
#define STM32_UDS_ISO_TP_DOWNLOAD_H

#include <stdbool.h>
#include <stdint.h>

#ifndef UDS_DOWNLOAD_MAX_CHUNK_LENGTH
#define UDS_DOWNLOAD_MAX_CHUNK_LENGTH 256U
#endif

#ifndef UDS_DOWNLOAD_MAX_PROTECTED_REGIONS
#define UDS_DOWNLOAD_MAX_PROTECTED_REGIONS 4U
#endif

#ifndef UDS_DOWNLOAD_TIMEOUT_MS
#define UDS_DOWNLOAD_TIMEOUT_MS 5000U
#endif

#define UDS_DOWNLOAD_METADATA_MAGIC 0x55445349UL /* "UDSI" */

typedef enum {
    UDS_DOWNLOAD_OK = 0,
    UDS_DOWNLOAD_BUSY,
    UDS_DOWNLOAD_INVALID_ARGUMENT,
    UDS_DOWNLOAD_NOT_READY,
    UDS_DOWNLOAD_OUT_OF_RANGE,
    UDS_DOWNLOAD_ALIGNMENT_ERROR,
    UDS_DOWNLOAD_OVERFLOW,
    UDS_DOWNLOAD_SEQUENCE_ERROR,
    UDS_DOWNLOAD_PROGRAM_ERROR,
    UDS_DOWNLOAD_ERASE_ERROR,
    UDS_DOWNLOAD_VERIFY_ERROR,
    UDS_DOWNLOAD_TIMEOUT,
    UDS_DOWNLOAD_ABORTED
} UdsDownloadResult;

typedef enum {
    UDS_DOWNLOAD_IDLE = 0,
    UDS_DOWNLOAD_ERASING,
    UDS_DOWNLOAD_RECEIVING,
    UDS_DOWNLOAD_VERIFYING,
    UDS_DOWNLOAD_COMPLETE,
    UDS_DOWNLOAD_ABORTED_STATE
} UdsDownloadState;

typedef struct {
    uint32_t start;
    uint32_t end_exclusive;
} UdsDownloadRegion;

typedef struct {
    UdsDownloadRegion staging_image;
    UdsDownloadRegion bootloader;
    UdsDownloadRegion active_application;
    UdsDownloadRegion persistent_storage;
    UdsDownloadRegion diagnostic_storage;
    uint32_t erase_alignment;
    uint32_t program_alignment;
    uint16_t max_block_length;
    bool activation_supported;
} UdsDownloadMemoryMap;

typedef struct {
    uint32_t magic;
    uint32_t image_address;
    uint32_t image_length;
    uint32_t received_length;
    uint32_t crc32;
    uint8_t hash[32];
    bool crc_valid;
    bool hash_valid;
    bool activation_pending;
} UdsDownloadMetadata;

typedef UdsDownloadResult (*UdsDownloadEraseStartFn)(void *context, uint32_t address,
                                                     uint32_t length);
typedef UdsDownloadResult (*UdsDownloadErasePollFn)(void *context);
typedef UdsDownloadResult (*UdsDownloadProgramFn)(void *context, uint32_t address,
                                                  const uint8_t *data, uint16_t length);
typedef UdsDownloadResult (*UdsDownloadVerifyFn)(void *context, const UdsDownloadMetadata *metadata,
                                                 uint32_t expected_crc32, bool has_expected_crc32);
typedef void (*UdsDownloadAbortFn)(void *context);
typedef void (*UdsDownloadWatchdogFn)(void *context);

typedef struct {
    UdsDownloadEraseStartFn erase_start;
    UdsDownloadErasePollFn erase_poll;
    UdsDownloadProgramFn program;
    UdsDownloadVerifyFn verify_image;
    UdsDownloadAbortFn abort;
    UdsDownloadWatchdogFn watchdog_kick;
} UdsDownloadCallbacks;

typedef struct {
    UdsDownloadMemoryMap memory;
    UdsDownloadCallbacks callbacks;
    void *context;
    UdsDownloadState state;
    UdsDownloadMetadata metadata;
    uint8_t expected_block;
    uint32_t deadline_ms;
    uint32_t crc32;
} UdsDownload;

void uds_download_init(UdsDownload *download, const UdsDownloadMemoryMap *memory,
                       const UdsDownloadCallbacks *callbacks, void *context);
UdsDownloadResult uds_download_begin(UdsDownload *download, uint32_t address, uint32_t length,
                                     uint32_t now_ms);
UdsDownloadResult uds_download_poll_erase(UdsDownload *download, uint32_t now_ms);
UdsDownloadResult uds_download_write(UdsDownload *download, uint8_t block, const uint8_t *data,
                                     uint16_t length, uint32_t now_ms);
UdsDownloadResult uds_download_finish(UdsDownload *download, uint32_t expected_crc32,
                                      bool has_expected_crc32, uint32_t now_ms);
UdsDownloadResult uds_download_tick(UdsDownload *download, uint32_t now_ms);
void uds_download_abort(UdsDownload *download);
UdsDownloadState uds_download_state(const UdsDownload *download);
const UdsDownloadMetadata *uds_download_metadata(const UdsDownload *download);
bool uds_download_activation_pending(const UdsDownload *download);

#endif
