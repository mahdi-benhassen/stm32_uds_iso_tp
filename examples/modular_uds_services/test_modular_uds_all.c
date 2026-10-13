/**
 * @file test_modular_uds_all.c
 * @brief Comprehensive verification suite for Issues #119, #120, #121, #122, #123, and #124
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "uds_server.h"
#include "uds_session.h"
#include "svc_read_dtc.h"
#include "svc_control_dtc.h"
#include "svc_security_access.h"
#include "crypto_aes_cmac.h"
#include "dtc_store.h"
#include "dtc_table_85.h"
#include "dtc_nvm_adapter.h"
#include "isotp_fd.h"

static bool s_mock_tx_called = false;
static can_frame_t s_mock_tx_frame;

static bool mock_can_tx_cb(const can_frame_t *frame, void *user) {
    (void)user;
    s_mock_tx_called = true;
    s_mock_tx_frame = *frame;
    return true;
}

/* ── Test 1: Issue #119 - Service 19 03 (reportDTCSnapshotIdentification) ── */
static void test_issue_119_read_dtc_19_03(void) {
    printf("[TEST] Running Issue #119: Service 19 03 ...\n");

    dtc_store_init();
    uds_request_t req;
    uds_response_t resp;

    /* 1. Empty store -> 59 03 response of length 2 */
    uint8_t req_data_03 = 0x03U;
    req.sid = 0x19U;
    req.data = &req_data_03;
    req.data_len = 1U;
    req.functional = false;

    memset(&resp, 0, sizeof(resp));
    svc_read_dtc_info(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0x59U);
    assert(resp.data[1] == 0x03U);

    /* 2. Add DTCs with and without snapshot */
    uint8_t snap[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    /* DTC 1: Has snapshot */
    dtc_store_add(0x123456U, 0x2FU, snap, sizeof(snap), NULL, 0);
    /* DTC 2: No snapshot */
    dtc_store_add(0x654321U, 0x28U, NULL, 0, NULL, 0);
    /* DTC 3: Has snapshot */
    dtc_store_add(0x789ABCU, 0x2FU, snap, sizeof(snap), NULL, 0);

    memset(&resp, 0, sizeof(resp));
    svc_read_dtc_info(&req, &resp);
    /* Expect 2 header bytes + 2 DTC records * 4 bytes = 10 bytes */
    assert(resp.len == 10U);
    assert(resp.data[0] == 0x59U);
    assert(resp.data[1] == 0x03U);

    /* First entry: 12 34 56 01 */
    assert(resp.data[2] == 0x12U);
    assert(resp.data[3] == 0x34U);
    assert(resp.data[4] == 0x56U);
    assert(resp.data[5] == 0x01U);

    /* Second entry: 78 9A BC 01 */
    assert(resp.data[6] == 0x78U);
    assert(resp.data[7] == 0x9AU);
    assert(resp.data[8] == 0xBCU);
    assert(resp.data[9] == 0x01U);

    /* 3. Length check: req->data_len != 1 should return NRC 0x13 */
    uint8_t req_data_invalid[2] = { 0x03U, 0x00U };
    req.data = req_data_invalid;
    req.data_len = 2U;
    memset(&resp, 0, sizeof(resp));
    svc_read_dtc_info(&req, &resp);
    assert(resp.len == 3U);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x19U);
    assert(resp.data[2] == NRC_INCORRECT_MSG_LEN_OR_FORMAT);

    printf("       Issue #119 passed!\n");
}

/* ── Test 2: Issue #122 - Service 0x85 ControlDTCSetting debouncing link ── */
static void test_issue_122_control_dtc_switch(void) {
    printf("[TEST] Running Issue #122: Service 0x85 debouncing switch ...\n");

    dtc_store_init();
    assert(dtc_is_setting_enabled() == true);

    dtc_runtime_item_t item;
    item.dtc = 0x123456U;
    item.status = 0x50U;
    item.fault_detection_counter = 0;
    item.aging_counter = 0U;
    item.occurrence_counter = 0U;

    /* 1. Debounce with setting ON: counter increments */
    dtc_process_sample(&item, true, 10, 5);
    assert(item.fault_detection_counter == 10);

    /* 2. Turn DTC setting OFF via Service 0x85 */
    uds_request_t req;
    uds_response_t resp;
    uint8_t sub_off = 0x02U;
    req.sid = 0x85U;
    req.data = &sub_off;
    req.data_len = 1U;
    req.functional = false;

    memset(&resp, 0, sizeof(resp));
    svc_control_dtc(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0xC5U);
    assert(resp.data[1] == 0x02U);
    assert(dtc_is_setting_enabled() == false);

    /* 3. Debounce with setting OFF: counter and status MUST NOT change! */
    dtc_process_sample(&item, true, 10, 5);
    assert(item.fault_detection_counter == 10); /* Frozen */

    /* 4. Turn DTC setting back ON */
    uint8_t sub_on = 0x01U;
    req.data = &sub_on;
    memset(&resp, 0, sizeof(resp));
    svc_control_dtc(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0xC5U);
    assert(resp.data[1] == 0x01U);
    assert(dtc_is_setting_enabled() == true);

    /* 5. Debounce continues */
    dtc_process_sample(&item, true, 10, 5);
    assert(item.fault_detection_counter == 20);

    printf("       Issue #122 passed!\n");
}

/* ── Test 3: Issue #123 - 29-bit Extended CAN IDs in isotp_fd ── */
static void test_issue_123_29bit_extended_can_id(void) {
    printf("[TEST] Running Issue #123: 29-bit Extended CAN IDs ...\n");

    /* Verify address macros */
    uint32_t req_id = ISOTP_29BIT_PHYS_REQ(0x00U, 0xF1U);
    uint32_t resp_id = ISOTP_29BIT_PHYS_RESP(0xF1U, 0x00U);
    assert(req_id == 0x18DA00F1U);
    assert(resp_id == 0x18DAF100U);
    assert(ISOTP_IS_EXT_ID(req_id) == true);
    assert(ISOTP_IS_EXT_ID(0x7E0U) == false);

    /* Initialize ISO-TP channel with 29-bit IDs */
    isotp_fd_channel_t ch;
    isotp_fd_init(&ch, req_id, resp_id, false, mock_can_tx_cb, NULL);
    assert(ch.rx_is_ext == true);
    assert(ch.tx_is_ext == true);

    /* Transmit single frame */
    s_mock_tx_called = false;
    uint8_t payload[4] = { 0x22, 0xF1, 0x90, 0x00 };
    isotp_result_t res = isotp_fd_send(&ch, payload, 4U, 100U);
    assert(res == ISOTP_OK);
    assert(s_mock_tx_called == true);
    assert(s_mock_tx_frame.is_ext == true);
    assert(s_mock_tx_frame.id == resp_id);
    assert(s_mock_tx_frame.data[0] == 0x04U); /* SF DL = 4 */
    assert(s_mock_tx_frame.data[1] == 0x22U);

    /* Receive single frame with standard 11-bit ID: should be rejected */
    can_frame_t rx_std;
    memset(&rx_std, 0, sizeof(rx_std));
    rx_std.id = 0x7E0U;
    rx_std.is_ext = false;
    rx_std.dlc = 8U;
    rx_std.data[0] = 0x02U; rx_std.data[1] = 0x10U; rx_std.data[2] = 0x01U;
    isotp_fd_on_rx(&ch, &rx_std, 105U);
    assert(!isotp_fd_rx_ready(&ch));

    /* Receive single frame with correct 29-bit extended ID: accepted! */
    can_frame_t rx_ext;
    memset(&rx_ext, 0, sizeof(rx_ext));
    rx_ext.id = req_id;
    rx_ext.is_ext = true;
    rx_ext.dlc = 8U;
    rx_ext.data[0] = 0x02U; rx_ext.data[1] = 0x10U; rx_ext.data[2] = 0x01U;
    isotp_fd_on_rx(&ch, &rx_ext, 110U);
    assert(isotp_fd_rx_ready(&ch));

    uint32_t rx_len = 0U;
    const uint8_t *rx_buf = isotp_fd_rx_data(&ch, &rx_len);
    assert(rx_len == 2U);
    assert(rx_buf[0] == 0x10U);
    assert(rx_buf[1] == 0x01U);
    isotp_fd_rx_done(&ch);

    printf("       Issue #123 passed!\n");
}

/* ── Test 4: Issue #124 - Dual-Level 0x27 Security Access (Level 1 & Level 2 AES-CMAC128) ── */
static void test_issue_124_dual_level_security(void) {
    printf("[TEST] Running Issue #124: Dual-Level 0x27 Security Access ...\n");

    uds_security_init();
    assert(!uds_security_is_unlocked());

    uds_request_t req;
    uds_response_t resp;

    /* ── Level 1 (4-byte XOR) ── */
    /* 1. Request Seed Level 1 */
    uint8_t sub_req_s1 = 0x01U;
    req.sid = 0x27U;
    req.data = &sub_req_s1;
    req.data_len = 1U;
    req.functional = false;

    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 6U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x01U);

    uint8_t seed_l1[4];
    memcpy(seed_l1, &resp.data[2], 4U);

    /* 2. Send invalid key */
    uint8_t invalid_key_req[5] = { 0x02U, 0x00, 0x00, 0x00, 0x00 };
    req.data = invalid_key_req;
    req.data_len = 5U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 3U);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[2] == NRC_INVALID_KEY);

    /* 3. Send valid key without requesting seed -> Sequence Error NRC 0x24 */
    req.data = invalid_key_req;
    req.data_len = 5U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 3U);
    assert(resp.data[2] == NRC_REQUEST_SEQUENCE_ERROR);

    /* 4. Request seed again, compute valid key and send */
    req.data = &sub_req_s1;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 6U);
    memcpy(seed_l1, &resp.data[2], 4U);

    uint8_t valid_key_req[5];
    valid_key_req[0] = 0x02U;
    uint8_t secret[4] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
    for (int i = 0; i < 4; i++) {
        valid_key_req[1 + i] = (uint8_t)(seed_l1[i] ^ secret[i]);
    }
    req.data = valid_key_req;
    req.data_len = 5U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x02U);
    assert(uds_security_is_level_unlocked(1) == true);
    assert((uds_security_get_active_mask() & UDS_SEC_LEVEL_1) != 0U);

    /* 5. Requesting seed again when already unlocked returns all zeroes */
    req.data = &sub_req_s1;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 6U);
    assert(resp.data[2] == 0 && resp.data[3] == 0 && resp.data[4] == 0 && resp.data[5] == 0);

    /* ── Level 2 (16-byte Cryptographic AES-CMAC-128) ── */
    /* 1. Request Seed Level 2 */
    uint8_t sub_req_s2 = 0x03U;
    req.data = &sub_req_s2;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 18U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x03U);

    uint8_t seed_l2[16];
    memcpy(seed_l2, &resp.data[2], 16U);

    /* 2. Compute 128-bit AES-CMAC tag over the seed */
    static const uint8_t aes_key[16] = {
        0x2BU, 0x7EU, 0x15U, 0x16U, 0x28U, 0xAEU, 0xD2U, 0xA6U,
        0xABU, 0xF7U, 0x15U, 0x88U, 0x09U, 0xCFU, 0x4FU, 0x3CU
    };
    uint8_t expected_mac[16];
    assert(crypto_aes_cmac_128(aes_key, seed_l2, 16U, expected_mac));

    uint8_t valid_l2_key_req[17];
    valid_l2_key_req[0] = 0x04U;
    memcpy(&valid_l2_key_req[1], expected_mac, 16U);

    req.data = valid_l2_key_req;
    req.data_len = 17U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x04U);
    assert(uds_security_is_level_unlocked(2) == true);
    assert((uds_security_get_active_mask() & UDS_SEC_LEVEL_2) != 0U);

    /* 3. Requesting seed Level 2 when already unlocked returns 16 zeroes */
    req.data = &sub_req_s2;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 18U);
    for (int i = 0; i < 16; i++) {
        assert(resp.data[2 + i] == 0);
    }

    /* 4. Reset locks both levels */
    uds_security_reset();
    assert(uds_security_is_unlocked() == false);
    assert(uds_security_is_level_unlocked(1) == false);
    assert(uds_security_is_level_unlocked(2) == false);

    printf("       Issue #124 passed!\n");
}

/* ── Test 5: Issue #121 - Complete Drop-in Replacement uds_server.c / uds_server.h ── */
static void test_issue_121_drop_in_uds_server(void) {
    printf("[TEST] Running Issue #121: Drop-in uds_server ...\n");

    uds_server_init(NULL);
    uds_response_t resp;

    /* Physical request for Service 0x19 01 (ReadDTC numberOfDTCByStatusMask) -> handled */
    uint8_t req_19_01[3] = { 0x19U, 0x01U, 0xFFU };
    bool ok = uds_server_process(req_19_01, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x59U);

    /* Functional request for Physical-only service (e.g. 0x27 SecurityAccess) -> dropped silently */
    uint8_t req_27_01[2] = { 0x27U, 0x01U };
    ok = uds_server_process(req_27_01, 2U, true, &resp);
    assert(ok == false); /* Dropped silently without NRC per ISO 14229-1 */

    /* Functional request for Service 0x19 (allowed functional) -> handled */
    ok = uds_server_process(req_19_01, 3U, true, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x59U);

    printf("       Issue #121 passed!\n");
}

/* ── Test 6: Issue #120 - Connect 85 DTCs store with Flash NVM adapter ── */
static void test_issue_120_dtc_nvm_adapter(void) {
    printf("[TEST] Running Issue #120: DTC NVM Adapter ...\n");

    /* Initialize 85 DTCs and NVM adapter */
    dtc_table_85_init();
    assert(dtc_store_count() == DTC_CONFIGURED_COUNT);

    dtc_nvm_init();

    /* Modify status of DTC */
    dtc_nvm_save(0xD00616U, 0x2FU);
    assert(dtc_nvm_is_dirty() == true);

    /* Flush to flash */
    bool flushed = dtc_nvm_flush();
    assert(flushed == true);
    assert(dtc_nvm_is_dirty() == false);

    printf("       Issue #120 passed!\n");
}

int main(void) {
    printf("===================================================\n");
    printf("  Executing Modular UDS Verification Suite (119-124)\n");
    printf("===================================================\n");

    test_issue_119_read_dtc_19_03();
    test_issue_122_control_dtc_switch();
    test_issue_123_29bit_extended_can_id();
    test_issue_124_dual_level_security();
    test_issue_121_drop_in_uds_server();
    test_issue_120_dtc_nvm_adapter();

    printf("===================================================\n");
    printf("  ALL 6 TESTS PASSED SUCCESSFULLY! (100%% OK)\n");
    printf("===================================================\n");
    return 0;
}
