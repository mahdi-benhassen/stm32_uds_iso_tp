#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bcm_app.h"
#include "bcm_uds_adapter.h"

int main(void)
{
    printf("===================================================================\n");
    printf(" Automotive Application Example 1: Body Control Module (BCM)\n");
    printf(" Middleware: STM32 UDS ISO-TP Standalone Protocol Stack\n");
    printf("===================================================================\n\n");

    /* 1. Initialize BCM Hardware & Application State */
    bcm_app_init();
    printf("[BCM] Application initialized. Battery = 12.6V, Ambient Temp = 21.5 C\n");

    /* 2. Initialize UDS Server Middleware */
    UdsServer uds_server;
    uint32_t current_time_ms = 1000U;
    bcm_uds_adapter_init(&uds_server, current_time_ms);
    printf("[UDS Middleware] Server initialized in Default Session (0x01).\n\n");

    uint8_t response_buf[256];
    uint16_t response_len = 0U;

    /* 3. Diagnostic Request: Read Data By Identifier (0x22 0x01 0x00 - State Snapshot) */
    printf("--- Test 1: Diagnostic Service 0x22 (Read DID 0x0100 - State Snapshot) ---\n");
    const uint8_t req_read_state[3] = {0x22U, 0x01U, 0x00U};
    UdsCallbackResult res = uds_server_handle(&uds_server, req_read_state, sizeof(req_read_state),
                                              response_buf, &response_len, sizeof(response_buf),
                                              current_time_ms);
    assert(res == UDS_RESULT_OK);
    printf("-> Client Request:  [ 22 01 00 ]\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("] (Positive Response 0x62 DID 0x0100)\n");
    assert(response_buf[0] == 0x62U);
    assert(response_buf[1] == 0x01U && response_buf[2] == 0x00U);
    printf("   Decoded: Headlights=%d, DoorsLocked=%s\n\n",
           response_buf[3], (response_buf[8] & 0x03) == 0x03 ? "YES" : "NO");

    /* 4. Switch to Extended Diagnostic Session (0x10 0x03) required for I/O Control */
    printf("--- Test 2: Switch to Extended Diagnostic Session (0x10 0x03) ---\n");
    const uint8_t req_session_ext[2] = {0x10U, 0x03U};
    res = uds_server_handle(&uds_server, req_session_ext, sizeof(req_session_ext),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x50U && response_buf[1] == 0x03U);
    printf("<- Server Response: [ 50 03 ... ] (In Extended Session)\n\n");

    /* 5. Diagnostic Request: InputOutputControlByIdentifier (0x2F 0x01 0x02 0x03 0x01)
     * Force Headlights to LOW_BEAM via ShortTermAdjustment */
    printf("--- Test 3: Diagnostic Service 0x2F (I/O Override Headlights to LOW_BEAM) ---\n");
    const uint8_t req_io_override[5] = {0x2FU, 0x01U, 0x02U, 0x03U, 0x01U};
    res = uds_server_handle(&uds_server, req_io_override, sizeof(req_io_override),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    printf("-> Client Request:  [ 2F 01 02 03 01 ] (DID=0x0102, shortTermAdjustment=0x03, val=0x01)\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("]\n");

    /* Step BCM app forward and verify headlights are now overridden */
    bcm_app_process(50U);
    assert(bcm_app_get_state()->headlights == BCM_LIGHT_LOW_BEAM);
    printf("   [BCM Physical State] Headlights successfully overridden to LOW_BEAM by diagnostic middleware!\n\n");

    /* 6. Diagnostic Request: Release I/O Control (0x2F 0x01 0x02 0x00 - returnControlToECU) */
    printf("--- Test 4: Diagnostic Service 0x2F (Return Control of Headlights to ECU) ---\n");
    const uint8_t req_io_release[4] = {0x2FU, 0x01U, 0x02U, 0x00U};
    res = uds_server_handle(&uds_server, req_io_release, sizeof(req_io_release),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    bcm_app_process(50U);
    assert(bcm_app_get_state()->headlights == BCM_LIGHT_OFF);
    printf("   [BCM Physical State] Headlight control returned to ECU (State: OFF).\n\n");

    /* 7. Fault Simulation & Diagnostic Service 0x19 (Read DTC Information) */
    printf("--- Test 5: DTC Fault Injection & Diagnostic Service 0x19 ---\n");
    bcm_app_inject_fault_light_short(true);
    bcm_app_inject_fault_door_stall(true);
    bcm_app_process(10U);
    printf("   [BCM Sensors] Injected faults: Exterior Light Short Circuit + Door Lock Actuator Stall\n");

    /* Service 0x19 0x01: reportNumberOfDTCByStatusMask */
    const uint8_t req_dtc_count[3] = {0x19U, 0x01U, 0xFFU};
    res = uds_server_handle(&uds_server, req_dtc_count, sizeof(req_dtc_count),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    printf("-> Client Request:  [ 19 01 FF ]\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("] -> DTC Count: %d\n", response_buf[4]);
    assert(response_buf[4] == 2U);

    /* 7. Clear DTCs via Service 0x14 */
    printf("\n--- Test 5: Clear DTCs (Service 0x14 FF FF FF) ---\n");
    const uint8_t req_clear_dtc[4] = {0x14U, 0xFFU, 0xFFU, 0xFFU};
    res = uds_server_handle(&uds_server, req_clear_dtc, sizeof(req_clear_dtc),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x54U);
    printf("<- Server Response: [ 54 ] (Positive Response: All DTCs Cleared)\n");

    printf("\n===================================================================\n");
    printf(" SUCCESS: BCM Application cleanly executed using UDS ISO-TP middleware!\n");
    printf("===================================================================\n");
    return 0;
}
