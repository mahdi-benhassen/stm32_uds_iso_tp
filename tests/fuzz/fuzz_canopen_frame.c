/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
/*
 * Host-only structured fuzz target.
 *
 * This target exercises bounded CAN-frame injection through the transport-neutral
 * CiA 302/NMT protocol surface and arbitrary node-ID/bitrate inputs through the
 * project LSS policy. It is not embedded firmware, HIL, EMC, or conformance
 * evidence.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "canopen_reference_lss.h"
#include "cia302_nmt_master.h"

static bool
fuzz_send(void *context, const cia302_frame_t *frame) {
    (void)context;
    (void)frame;
    return true;
}

static void
fuzz_event(void *context, const cia302_event_t *event) {
    (void)context;
    (void)event;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (data == NULL || size == 0U) {
        return 0;
    }

    cia302_nmt_master_t master;
    cia302_nmt_master_init(&master, 1U, fuzz_send, fuzz_event, NULL);
    const uint8_t configured_node = (uint8_t)((data[0] % CIA302_MAX_NODES) + 1U);
    (void)cia302_nmt_master_configure(&master, configured_node, (data[0] & 1U) != 0U,
                                      (data[0] & 2U) != 0U, (uint16_t)data[0] + 1U);
    master.nmt_startup = CIA302_NMT_START;
    (void)cia302_nmt_master_start(&master, size > 1U ? data[1] : 0U);

    const uint16_t can_id = size >= 3U ? (uint16_t)(((uint16_t)data[1] << 8U | data[2]) & 0x07FFU) : 0U;
    const uint8_t dlc = size >= 4U ? (uint8_t)(data[3] & 0x0FU) : 0U;
    uint8_t payload[8] = {0U};
    const size_t available = size > 4U ? size - 4U : 0U;
    const size_t copied = available < sizeof(payload) ? available : sizeof(payload);
    if (copied > 0U) {
        (void)memcpy(payload, &data[4], copied);
    }
    (void)cia302_nmt_master_receive(&master, can_id, payload, dlc, (uint32_t)size);
    (void)cia302_nmt_master_process(&master, (uint32_t)size + (size > 5U ? data[5] : 0U));

    if (size >= 7U) {
        const uint8_t lss_node_id = data[6];
        const uint16_t lss_bitrate = (uint16_t)data[0] << 8U | data[1];
        (void)CANopenReferenceLss_BitrateSupported(lss_bitrate);
        (void)CANopenReferenceLss_StoreConfiguration(lss_node_id, lss_bitrate);
        CANopenReferenceLss_ActivateBitrate((uint16_t)data[2]);
    }

    return 0;
}
