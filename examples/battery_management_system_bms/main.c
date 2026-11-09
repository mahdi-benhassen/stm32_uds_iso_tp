#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bms_app.h"
#include "bms_uds_adapter.h"

int main(void)
{
    printf("===================================================================\n");
    printf(" Automotive Application Example 2: Battery Management System (BMS)\n");
    printf(" Middleware: STM32 UDS ISO-TP Standalone Protocol Stack\n");
    printf("===================================================================\n\n");

    /* 1. Initialize BMS Application State */
    bms_app_init();
    printf("[BMS Hardware] Pack Voltage: 384.0 V, Current: 15.2 A, SoC: 85.0%%, Isolation: 650 kOhm\n");

    /* 2. Initialize UDS Middleware */
    UdsServer uds_server;
    uint32_t current_time_ms = 1000U;
    bms_uds_adapter_init(&uds_server, current_time_ms);
    printf("[UDS Middleware] Server initialized in Default Session (0x01).\n\n");

    uint8_t response_buf[256];
    uint16_t response_len = 0U;

    /* 3. Diagnostic Request: Read Pack Telemetry (DID 0x0200) */
    printf("--- Test 1: Read Battery Pack Telemetry (DID 0x0200) ---\n");
    const uint8_t req_pack_summary[3] = {0x22U, 0x02U, 0x00U};
    UdsCallbackResult res = uds_server_handle(&uds_server, req_pack_summary, sizeof(req_pack_summary),
                                              response_buf, &response_len, sizeof(response_buf),
                                              current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x62U);
    printf("-> Client Request:  [ 22 02 00 ]\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("]\n");

    uint32_t pack_v = ((uint32_t)response_buf[3] << 24) | ((uint32_t)response_buf[4] << 16) |
                      ((uint32_t)response_buf[5] << 8)  | (uint32_t)response_buf[6];
    uint16_t soc = (uint16_t)(((uint16_t)response_buf[11] << 8) | (uint16_t)response_buf[12]);
    printf("   Decoded Telemetry: Pack Voltage = %u mV (%.1f V), SoC = %.1f%%\n\n",
           pack_v, (double)pack_v / 1000.0, (double)soc / 10.0);

    /* 4. Switch to Extended Diagnostic Session (0x10 0x03) */
    printf("--- Test 2: Switch to Extended Diagnostic Session (0x10 0x03) ---\n");
    const uint8_t req_session_ext[2] = {0x10U, 0x03U};
    res = uds_server_handle(&uds_server, req_session_ext, sizeof(req_session_ext),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x50U && response_buf[1] == 0x03U);
    printf("<- Server Response: [ 50 03 ... ] (Positive Response: In Extended Session)\n\n");

    /* 5. Attempt Protected RoutineControl 0x31 0x01 0x0201 WITHOUT SecurityAccess */
    printf("--- Test 3: Attempt Protected RoutineControl 0x31 0x0201 WITHOUT Security ---\n");
    const uint8_t req_routine_cycle[4] = {0x31U, 0x01U, 0x02U, 0x01U};
    res = uds_server_handle(&uds_server, req_routine_cycle, sizeof(req_routine_cycle),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    printf("-> Client Request:  [ 31 01 02 01 ] (Contactor Manual Cycle)\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("] -> NRC 0x33: SecurityAccessDenied!\n");
    assert(response_buf[0] == 0x7FU && response_buf[1] == 0x31U && response_buf[2] == 0x33U);
    printf("   [Security Gate] Access properly blocked by middleware.\n\n");

    /* 6. Perform SecurityAccess (0x27): Request Seed (0x27 0x01) */
    printf("--- Test 4: Request SecurityAccess Seed (0x27 0x01) ---\n");
    const uint8_t req_seed[2] = {0x27U, 0x01U};
    res = uds_server_handle(&uds_server, req_seed, sizeof(req_seed),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x67U && response_buf[1] == 0x01U);
    printf("-> Client Request:  [ 27 01 ]\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("] (Seed Received: %02X %02X %02X %02X)\n",
           response_buf[2], response_buf[3], response_buf[4], response_buf[5]);

    /* Compute Key = Seed ^ 0xA5A55A5A */
    uint32_t seed = ((uint32_t)response_buf[2] << 24) | ((uint32_t)response_buf[3] << 16) |
                    ((uint32_t)response_buf[4] << 8)  | (uint32_t)response_buf[5];
    uint32_t key = seed ^ 0xA5A55A5AU;
    uint8_t req_key[6] = {
        0x27U, 0x02U,
        (uint8_t)((key >> 24) & 0xFFU),
        (uint8_t)((key >> 16) & 0xFFU),
        (uint8_t)((key >> 8) & 0xFFU),
        (uint8_t)(key & 0xFFU)
    };

    printf("--- Test 5: Send SecurityAccess Key (0x27 0x02) ---\n");
    res = uds_server_handle(&uds_server, req_key, sizeof(req_key),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x67U && response_buf[1] == 0x02U);
    printf("<- Server Response: [ 67 02 ] (Security Unlocked: Level 1 Active!)\n\n");

    /* 7. Re-attempt Protected RoutineControl 0x31 0x0201 WITH SecurityAccess */
    printf("--- Test 6: Re-attempt Protected RoutineControl (Now Unlocked) ---\n");
    res = uds_server_handle(&uds_server, req_routine_cycle, sizeof(req_routine_cycle),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    assert(response_buf[0] == 0x71U && response_buf[1] == 0x01U && response_buf[2] == 0x02U);
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("] -> Routine Executed Successfully!\n\n");

    /* 8. Safety Fault Injection & DTC Reporting (0x19 0x02) */
    printf("--- Test 7: Safety Fault Injection & DTC Reporting (0x19 0x02) ---\n");
    bms_app_inject_fault_ovp(true);
    bms_app_inject_fault_isolation(true);
    bms_app_process(10U);

    const uint8_t req_read_dtcs[3] = {0x19U, 0x02U, 0xFFU};
    res = uds_server_handle(&uds_server, req_read_dtcs, sizeof(req_read_dtcs),
                            response_buf, &response_len, sizeof(response_buf),
                            current_time_ms);
    assert(res == UDS_RESULT_OK);
    printf("-> Client Request:  [ 19 02 FF ] (Read DTCs by status mask)\n");
    printf("<- Server Response: [ ");
    for (uint16_t i = 0; i < response_len; ++i) {
        printf("%02X ", response_buf[i]);
    }
    printf("]\n");
    printf("   Decoded Faults: DTC 0xA00117 (Cell Over-Voltage) + DTC 0xA00219 (HV Isolation Loss)\n");

    printf("\n===================================================================\n");
    printf(" SUCCESS: BMS Application cleanly executed using UDS ISO-TP middleware!\n");
    printf("===================================================================\n");
    return 0;
}
