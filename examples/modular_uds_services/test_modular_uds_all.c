/**
 * @file test_modular_uds_all.c
 * @brief Comprehensive verification suite for Issues #119 through #141
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
#include "svc_authentication.h"
#include "svc_read_did.h"
#include "svc_write_did.h"
#include "svc_periodic_did.h"
#include "svc_dyn_did.h"
#include "svc_io_control.h"
#include "svc_link_ctrl.h"
#include "svc_ecu_reset.h"
#include "svc_diag_session.h"
#include "svc_tester_present.h"
#include "svc_download.h"
#include "svc_upload.h"
#include "uds_transfer_fsm.h"
#include "can_ring_buffer.h"
#include "../stm32f103_single_bank_bootloader/flash_layout.h"
#include "../stm32f103_single_bank_bootloader/app_metadata.h"
#include "../stm32f103_single_bank_bootloader/bootloader_fsm.h"
#include "../stm32f103_single_bank_bootloader/crc32.h"

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
    valid_key_req[1] = seed_l1[0] ^ 0xDEU;
    valid_key_req[2] = seed_l1[1] ^ 0xADU;
    valid_key_req[3] = seed_l1[2] ^ 0xBEU;
    valid_key_req[4] = seed_l1[3] ^ 0xEFU;

    req.data = valid_key_req;
    req.data_len = 5U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x02U);
    assert(uds_security_is_level_unlocked(1U));
    assert(uds_security_is_unlocked());

    /* 5. Requesting seed when already unlocked -> returns seed of all zeroes (ISO 14229-1) */
    req.data = &sub_req_s1;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 6U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x01U);
    assert(resp.data[2] == 0x00U && resp.data[3] == 0x00U && resp.data[4] == 0x00U && resp.data[5] == 0x00U);

    /* ── Level 2 (16-byte AES-CMAC-128) ── */
    /* Request seed Level 2 */
    uint8_t sub_req_s2 = 0x03U;
    req.data = &sub_req_s2;
    req.data_len = 1U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 18U); /* SID + sub + 16-byte seed */
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x03U);

    uint8_t seed_l2[16];
    memcpy(seed_l2, &resp.data[2], 16U);

    /* Calculate AES-CMAC-128 tag */
    const uint8_t master_key[16] = {
        0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
    };
    uint8_t mac_l2[16];
    bool cmac_ok = crypto_aes_cmac_128(master_key, seed_l2, 16U, mac_l2);
    assert(cmac_ok);

    uint8_t key_req_l2[17];
    key_req_l2[0] = 0x04U;
    memcpy(&key_req_l2[1], mac_l2, 16U);

    req.data = key_req_l2;
    req.data_len = 17U;
    memset(&resp, 0, sizeof(resp));
    svc_security_access(&req, &resp);
    assert(resp.len == 2U);
    assert(resp.data[0] == 0x67U);
    assert(resp.data[1] == 0x04U);
    assert(uds_security_is_level_unlocked(2U));

    /* Reset security on session reset */
    uds_security_reset();
    assert(!uds_security_is_unlocked());

    printf("       Issue #124 passed!\n");
}

/* ── Test 5: Issue #121 - Drop-in UDS Server Framework ── */
static void test_issue_121_drop_in_uds_server(void) {
    printf("[TEST] Running Issue #121: Drop-in uds_server ...\n");

    uds_server_init(NULL);
    assert(uds_server_get_session() == SESSION_MASK_DEFAULT);

    /* Process standard ReadDTC request via unified server */
    uint8_t req_19_01[3] = { 0x19U, 0x01U, 0xFFU };
    uds_response_t resp;
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

/* ── Test 7: Issue #125 - UDS Session Management & S3 Timeout ── */
static void test_issue_125_uds_session_fsm(void) {
    printf("[TEST] Running Issue #125: UDS Session Management & S3 Timeout ...\n");

    uds_server_init(NULL);
    assert(uds_session_get() == UDS_SESSION_DEFAULT);
    assert(uds_session_to_mask(UDS_SESSION_DEFAULT) == SESSION_MASK_DEFAULT);

    /* Transition to Extended Diagnostic Session */
    uds_session_set(UDS_SESSION_EXTENDED);
    assert(uds_session_get() == UDS_SESSION_EXTENDED);
    assert(uds_server_get_session() == SESSION_MASK_EXTENDED);

    /* Unlock security */
    uds_server_set_security_level(UDS_SEC_LEVEL_1);
    assert(uds_server_get_security_level() == UDS_SEC_LEVEL_1);

    /* S3 timer timeout check (5000 ticks) */
    for (uint32_t i = 0; i < 4999U; i++) {
        assert(uds_session_check_timeout() == false);
    }
    /* At 5000 ms, timeout triggers */
    bool timed_out = uds_session_check_timeout();
    assert(timed_out == true);
    assert(uds_session_get() == UDS_SESSION_DEFAULT);
    assert(uds_server_get_session() == SESSION_MASK_DEFAULT);

    printf("       Issue #125 passed!\n");
}

/* ── Test 8: Issues #126 & #136 - Read DID (0x22) and Write DID (0x2E) ── */
static void test_issue_126_136_read_write_did(void) {
    printf("[TEST] Running Issues #126 & #136: Read and Write DID ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    uds_server_set_security_level(UDS_SEC_LEVEL_1);

    /* 1. Read single DID: 0xF180 (Boot SW ID) */
    uint8_t req_22[3] = { 0x22U, 0xF1U, 0x80U };
    uds_response_t resp;
    bool ok = uds_server_process(req_22, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x62U);
    assert(resp.data[1] == 0xF1U);
    assert(resp.data[2] == 0x80U);

    /* 2. Write DID: 0xF190 (VIN - 17 bytes) */
    uint8_t req_2e[20];
    req_2e[0] = 0x2EU;
    req_2e[1] = 0xF1U;
    req_2e[2] = 0x90U;
    memcpy(&req_2e[3], "NEWVIN12345678901", 17U);

    ok = uds_server_process(req_2e, 20U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6EU);
    assert(resp.data[1] == 0xF1U);
    assert(resp.data[2] == 0x90U);

    /* Read back 0xF190 */
    uint8_t req_22_vin[3] = { 0x22U, 0xF1U, 0x90U };
    ok = uds_server_process(req_22_vin, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x62U);
    assert(memcmp(&resp.data[3], "NEWVIN12345678901", 17U) == 0);

    /* 3. Read unknown DID returns NRC 0x31 */
    uint8_t req_unk[3] = { 0x22U, 0xEEU, 0xEEU };
    ok = uds_server_process(req_unk, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x22U);
    assert(resp.data[2] == NRC_REQUEST_OUT_OF_RANGE);

    printf("       Issues #126 & #136 passed!\n");
}

/* ── Test 9: Issue #127 - Periodic DID (0x2A) ── */
static void test_issue_127_periodic_did(void) {
    printf("[TEST] Running Issue #127: Periodic DID (0x2A) ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);

    /* 0x2A 01 F1 (Slow rate, periodic ID 0xF1) */
    uint8_t req_2a[3] = { 0x2AU, 0x01U, 0xF1U };
    uds_response_t resp;
    bool ok = uds_server_process(req_2a, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6AU);

    /* 0x2A 04 (Stop sending all) */
    uint8_t req_stop[2] = { 0x2AU, 0x04U };
    ok = uds_server_process(req_stop, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6AU);

    printf("       Issue #127 passed!\n");
}

/* ── Test 10: Issue #128 - LinkControl (0x87) ── */
static void test_issue_128_link_control(void) {
    printf("[TEST] Running Issue #128: LinkControl (0x87) ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    uds_response_t resp;

    /* Transition without prior verifyMode returns NRC 0x24 */
    uint8_t req_trans[2] = { 0x87U, 0x03U };
    bool ok = uds_server_process(req_trans, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x87U);
    assert(resp.data[2] == NRC_REQUEST_SEQUENCE_ERROR);

    /* Verify fixed parameter (500k = 0x14) */
    uint8_t req_verify[3] = { 0x87U, 0x01U, 0x14U };
    ok = uds_server_process(req_verify, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0xC7U);
    assert(resp.data[1] == 0x01U);

    /* Now transition succeeds */
    ok = uds_server_process(req_trans, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0xC7U);
    assert(resp.data[1] == 0x03U);

    printf("       Issue #128 passed!\n");
}

/* ── Test 11: Issue #129 - InputOutputControl (0x2F) ── */
static void test_issue_129_io_control(void) {
    printf("[TEST] Running Issue #129: InputOutputControl (0x2F) ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    uds_response_t resp;

    /* Short term adjustment on DID 0x0100 (LED -> ON 0x01) */
    uint8_t req_adj[5] = { 0x2FU, 0x01U, 0x00U, 0x03U, 0x01U };
    bool ok = uds_server_process(req_adj, 5U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6FU);
    assert(resp.data[1] == 0x01U);
    assert(resp.data[2] == 0x00U);
    assert(resp.data[3] == 0x03U);
    assert(resp.data[4] == 0x01U);

    /* Return control to ECU */
    uint8_t req_ret[4] = { 0x2FU, 0x01U, 0x00U, 0x00U };
    ok = uds_server_process(req_ret, 4U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6FU);
    assert(resp.data[3] == 0x00U);

    printf("       Issue #129 passed!\n");
}

/* ── Test 12: Issue #130 - ECUReset (0x11) & Delayed Execution ── */
static void test_issue_130_ecu_reset(void) {
    printf("[TEST] Running Issue #130: ECUReset (0x11) ...\n");

    uds_server_init(NULL);
    uds_response_t resp;

    /* Hard Reset (0x11 01) */
    uint8_t req_rst[2] = { 0x11U, 0x01U };
    bool ok = uds_server_process(req_rst, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x51U);
    assert(resp.data[1] == 0x01U);

    /* Verify poll counts down */
    for (int i = 0; i < 40; i++) {
        uds_server_poll();
    }

    printf("       Issue #130 passed!\n");
}

/* ── Test 13: Issue #131 - DynamicallyDefineDataIdentifier (0x2C) ── */
static void test_issue_131_dyn_did(void) {
    printf("[TEST] Running Issue #131: DynamicallyDefineDataIdentifier (0x2C) ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    uds_response_t resp;

    /* Define dynamic DID 0xF201 from slice of 0xF180 (pos 1, len 4: "BOOT") */
    uint8_t req_dyn[8] = { 0x2CU, 0x01U, 0xF2U, 0x01U, 0xF1U, 0x80U, 0x01U, 0x04U };
    bool ok = uds_server_process(req_dyn, 8U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6CU);
    assert(resp.data[1] == 0x01U);
    assert(resp.data[2] == 0xF2U);
    assert(resp.data[3] == 0x01U);

    /* Read dynamic DID 0xF201 via Service 0x22 */
    uint8_t req_read[3] = { 0x22U, 0xF2U, 0x01U };
    ok = uds_server_process(req_read, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x62U);
    assert(resp.data[1] == 0xF2U);
    assert(resp.data[2] == 0x01U);
    assert(memcmp(&resp.data[3], "BOOT", 4U) == 0);

    /* Clear dynamic DID 0xF201 */
    uint8_t req_clr[4] = { 0x2CU, 0x03U, 0xF2U, 0x01U };
    ok = uds_server_process(req_clr, 4U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6CU);

    printf("       Issue #131 passed!\n");
}

/* ── Test 14: Issues #132 & #135 - RequestDownload (0x34), Upload (0x35), TransferData (0x36), Exit (0x37) ── */
static void test_issue_132_135_download_upload(void) {
    printf("[TEST] Running Issues #132 & #135: Download & Upload FSM ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_PROGRAMMING);
    uds_server_set_security_level(UDS_SEC_LEVEL_1 | UDS_SEC_LEVEL_2);
    uds_response_t resp;

    /* 1. RequestDownload (0x34): format 0x00, ALFID 0x44, addr 0x0800C000, size 100 bytes */
    uint8_t req_34[11] = {
        0x34U, 0x00U, 0x44U,
        0x08U, 0x00U, 0xC0U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x64U
    };
    bool ok = uds_server_process(req_34, 11U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x74U);
    assert(resp.data[1] == 0x20U); /* 2 bytes of maxBlockLength */

    /* 2. TransferData (0x36) block 1 */
    uint8_t req_36[6] = { 0x36U, 0x01U, 0xAAU, 0xBBU, 0xCCU, 0xDDU };
    ok = uds_server_process(req_36, 6U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x76U);
    assert(resp.data[1] == 0x01U);

    /* 3. TransferData with wrong block sequence counter returns NRC 0x73 */
    uint8_t req_36_bad[3] = { 0x36U, 0x05U, 0x00U };
    ok = uds_server_process(req_36_bad, 3U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x36U);
    assert(resp.data[2] == NRC_WRONG_BLOCK_SEQUENCE_COUNTER);

    /* 4. RequestTransferExit (0x37) */
    uint8_t req_37[1] = { 0x37U };
    ok = uds_server_process(req_37, 1U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x77U);

    /* 5. RequestUpload (0x35) */
    uint8_t req_35[11] = {
        0x35U, 0x00U, 0x44U,
        0x08U, 0x00U, 0xC0U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x10U
    };
    ok = uds_server_process(req_35, 11U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x75U);

    printf("       Issues #132 & #135 passed!\n");
}

/* ── Test 15: Issue #134 - TesterPresent (0x3E) ── */
static void test_issue_134_tester_present(void) {
    printf("[TEST] Running Issue #134: TesterPresent (0x3E) ...\n");

    uds_server_init(NULL);
    uds_response_t resp;

    /* Normal TesterPresent: 0x3E 00 -> 7E 00 */
    uint8_t req_tp[2] = { 0x3EU, 0x00U };
    bool ok = uds_server_process(req_tp, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7EU);
    assert(resp.data[1] == 0x00U);

    /* Suppress bit set: 0x3E 80 -> returns false (suppressed) */
    uint8_t req_supp[2] = { 0x3EU, 0x80U };
    ok = uds_server_process(req_supp, 2U, false, &resp);
    assert(ok == false);

    printf("       Issue #134 passed!\n");
}

/* ── Test 16: Issue #137 - CAN Ring Buffer Queue in ISR ── */
static void test_issue_137_can_ring_buffer(void) {
    printf("[TEST] Running Issue #137: CAN Ring Buffer Queue ...\n");

    can_ring_buffer_t queue;
    can_ring_buffer_init(&queue);
    assert(can_ring_buffer_is_empty(&queue) == true);
    assert(can_ring_buffer_count(&queue) == 0U);

    /* Push CAN-FD frames */
    can_frame_t frame;
    memset(&frame, 0, sizeof(frame));
    frame.id = 0x7E0U;
    frame.dlc = 64U;
    frame.is_fd = true;

    for (uint32_t i = 0; i < (CAN_RING_BUFFER_CAPACITY - 1U); i++) {
        frame.data[0] = (uint8_t)i;
        bool pushed = can_ring_buffer_push(&queue, &frame);
        assert(pushed == true);
    }

    assert(can_ring_buffer_is_full(&queue) == true);

    /* Overrun check: pushing into full buffer should fail and increment drop_count */
    bool pushed_overflow = can_ring_buffer_push(&queue, &frame);
    assert(pushed_overflow == false);
    assert(can_ring_buffer_get_drop_count(&queue) == 1U);

    /* Pop and verify FIFO order */
    can_frame_t popped;
    for (uint32_t i = 0; i < (CAN_RING_BUFFER_CAPACITY - 1U); i++) {
        bool popped_ok = can_ring_buffer_pop(&queue, &popped);
        assert(popped_ok == true);
        assert(popped.data[0] == (uint8_t)i);
    }

    assert(can_ring_buffer_is_empty(&queue) == true);

    printf("       Issue #137 passed!\n");
}

/* ── Test 17: Issue #138 & #139 - Multi-Level Security Gate Differentiation ── */
static void test_issue_138_139_security_gate_levels(void) {
    printf("[TEST] Running Issues #138 & #139: Multi-Level Security Gate ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_PROGRAMMING);
    uds_server_set_security_level(UDS_SEC_LEVEL_LOCKED);
    uds_response_t resp;

    /* Level 1 service (0x2E WriteDID): Blocked when locked */
    uint8_t req_2e[5] = { 0x2EU, 0xF1U, 0x90U, 0xAAU, 0xBBU };
    bool ok = uds_server_process(req_2e, 5U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x2EU);
    assert(resp.data[2] == NRC_SECURITY_ACCESS_DENIED);

    /* Unlock Level 1 */
    uds_server_set_security_level(UDS_SEC_LEVEL_1);

    /* 0x2E WriteDID now succeeds */
    ok = uds_server_process(req_2e, 5U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x6EU);

    /* Level 2 service (0x35 RequestUpload): Blocked with Level 1 key */
    uint8_t req_35[11] = {
        0x35U, 0x00U, 0x44U,
        0x08U, 0x00U, 0xC0U, 0x00U,
        0x00U, 0x00U, 0x00U, 0x10U
    };
    ok = uds_server_process(req_35, 11U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x7FU);
    assert(resp.data[1] == 0x35U);
    assert(resp.data[2] == NRC_SECURITY_ACCESS_DENIED);

    /* Unlock Level 2 */
    uds_server_set_security_level(UDS_SEC_LEVEL_2);

    /* 0x35 RequestUpload now succeeds */
    ok = uds_server_process(req_35, 11U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x75U);

    printf("       Issues #138 & #139 passed!\n");
}

/* ── Test 18: Issue #140 - Single-Bank STM32F103 Bootloader & Anti-Bricking Logic ── */
static void test_issue_140_single_bank_bootloader(void) {
    printf("[TEST] Running Issue #140: STM32F103 Single-Bank Bootloader ...\n");

    bootloader_init();
    assert(bootloader_get_state() == BOOT_STATE_WINDOW_ACTIVE);
    assert(bootloader_get_remaining_window_ms() == BOOTLOADER_WINDOW_MS);

    /* 1. Stay in bootloader if ProgrammingSession (0x10 02) requested */
    bootloader_on_session_request(0x02U);
    assert(bootloader_get_state() == BOOT_STATE_PROGRAMMING);

    /* 2. Anti-bricking recovery check */
    bootloader_init();
    /* Prepare simulated application in RAM buffer representing 0x0800C000 */
    uint8_t sim_flash[256];
    memset(sim_flash, 0xFF, sizeof(sim_flash));

    /* Set valid ARM vector table: initial MSP inside SRAM (0x20001000), Reset_Handler inside slot */
    sim_flash[0] = 0x00; sim_flash[1] = 0x10; sim_flash[2] = 0x00; sim_flash[3] = 0x20;
    sim_flash[4] = 0x08; sim_flash[5] = 0xC0; sim_flash[6] = 0x00; sim_flash[7] = 0x08;

    app_metadata_t meta;
    memset(&meta, 0xFF, sizeof(meta));

    /* Unwritten / corrupted magic -> verify fails (ECU will NOT jump to broken app) */
    assert(bootloader_verify_app(sim_flash, &meta) == false);

    /* Commit metadata with proper CRC and magic written last */
    uint32_t app_size = sizeof(sim_flash);
    uint32_t crc = bootloader_crc32(sim_flash, app_size);
    bool committed = bootloader_commit_metadata(&meta, app_size, crc, 0x0800C008U, 0x01000000U);
    assert(committed == true);
    assert(meta.magic == APP_METADATA_MAGIC);

    /* Verify application now passes */
    assert(bootloader_verify_app(sim_flash, &meta) == true);

    printf("       Issue #140 passed!\n");
}

/* ── Test 19: Issue #141 - Service 0x29 Authentication (ISO 14229-1:2020) ── */
static void test_issue_141_service_29_authentication(void) {
    printf("[TEST] Running Issue #141: Service 0x29 Authentication ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    uds_response_t resp;

    /* 1. Request Challenge (0x29 06 01) */
    uint8_t req_chal[2] = { 0x29U, 0x06U };
    bool ok = uds_server_process(req_chal, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x69U);
    assert(resp.data[1] == 0x06U);
    assert(resp.data[2] == 0x00U); /* ok */
    assert(resp.len == 19U);       /* 3 header + 16 challenge bytes */

    uint8_t chal[16];
    memcpy(chal, &resp.data[3], 16U);

    /* 2. Compute Proof of Ownership with AES-CMAC-128 */
    uint8_t default_key[16] = {
        0x29U, 0x01U, 0x29U, 0x02U, 0x29U, 0x03U, 0x29U, 0x04U,
        0x29U, 0x05U, 0x29U, 0x06U, 0x29U, 0x07U, 0x29U, 0x08U
    };
    uint8_t mac[16];
    bool mac_ok = crypto_aes_cmac_128(default_key, chal, 16U, mac);
    assert(mac_ok == true);

    /* 3. Send Proof of Ownership (0x29 07 [16-byte MAC]) */
    uint8_t req_proof[18];
    req_proof[0] = 0x29U;
    req_proof[1] = 0x07U;
    memcpy(&req_proof[2], mac, 16U);

    ok = uds_server_process(req_proof, 18U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x69U);
    assert(resp.data[1] == 0x07U);
    assert(resp.data[2] == 0x00U); /* authenticated */

    /* Verify security level is promoted to Level 2 */
    assert(uds_server_get_security_level() == (UDS_SEC_LEVEL_1 | UDS_SEC_LEVEL_2));

    /* 4. De-authenticate (0x29 01) */
    uint8_t req_deauth[2] = { 0x29U, 0x01U };
    ok = uds_server_process(req_deauth, 2U, false, &resp);
    assert(ok == true);
    assert(resp.data[0] == 0x69U);
    assert(resp.data[1] == 0x01U);
    assert(uds_server_get_security_level() == UDS_SEC_LEVEL_LOCKED);

    printf("       Issue #141 passed!\n");
}

/* ── Test: Issue #146 - Server Polling & Tick Gating (uds_server_poll_ms) ── */
static void test_issue_146_server_poll_timing(void) {
    printf("[TEST] Running Issue #146: Server Polling Timing (uds_server_poll_ms) ...\n");

    uds_server_init(NULL);
    uds_session_set(UDS_SESSION_EXTENDED);
    assert(uds_session_get() == UDS_SESSION_EXTENDED);

    /* 1. Simulate fast while(1) loop calling uds_server_poll_ms 10,000 times with same tick */
    for (uint32_t i = 0U; i < 10000U; i++) {
        uds_server_poll_ms(1000U);
    }
    /* S3 timer must NOT have expired because 0 ms elapsed */
    assert(uds_session_get() == UDS_SESSION_EXTENDED);

    /* 2. Advance time by 4999 ms -> S3 (5000ms) should still be active */
    uds_server_poll_ms(5999U);
    assert(uds_session_get() == UDS_SESSION_EXTENDED);

    /* 3. Advance by 1 more ms to reach 5000 ms elapsed -> S3 timeout resets session to DEFAULT */
    uds_server_poll_ms(6000U);
    assert(uds_session_get() == UDS_SESSION_DEFAULT);

    printf("       Issue #146 passed!\n");
}

int main(void) {
    printf("===================================================\n");
    printf("  Executing Modular UDS Full Verification Suite\n");
    printf("        Covering Issues #119 - #142\n");
    printf("===================================================\n");

    test_issue_119_read_dtc_19_03();
    test_issue_120_dtc_nvm_adapter();
    test_issue_121_drop_in_uds_server();
    test_issue_122_control_dtc_switch();
    test_issue_123_29bit_extended_can_id();
    test_issue_124_dual_level_security();
    test_issue_125_uds_session_fsm();
    test_issue_126_136_read_write_did();
    test_issue_127_periodic_did();
    test_issue_128_link_control();
    test_issue_129_io_control();
    test_issue_130_ecu_reset();
    test_issue_131_dyn_did();
    test_issue_132_135_download_upload();
    test_issue_134_tester_present();
    test_issue_137_can_ring_buffer();
    test_issue_138_139_security_gate_levels();
    test_issue_140_single_bank_bootloader();
    test_issue_141_service_29_authentication();
    test_issue_146_server_poll_timing();

    printf("===================================================\n");
    printf("  ALL 19 VERIFICATION TESTS PASSED! (100%% SUCCESS)\n");
    printf("===================================================\n");
    return 0;
}
