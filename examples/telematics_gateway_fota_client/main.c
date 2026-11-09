#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "fota_client.h"
#include "simulated_ecu.h"

int main(void)
{
    printf("===================================================================\n");
    printf(" Automotive Application Example 3: Telematics Gateway FOTA Client\n");
    printf(" Role: Diagnostic Flashing Master (Tester) using UDS / ISO-TP\n");
    printf("===================================================================\n\n");

    /* 1. Dummy firmware payload to be flashed onto target ECU (256 bytes) */
    uint8_t dummy_firmware[256];
    for (int i = 0; i < 256; ++i) {
        dummy_firmware[i] = (uint8_t)(i ^ 0x5A);
    }

    /* 2. Initialize Target ECU running UDS Server Middleware */
    simulated_ecu_t target_ecu;
    uint32_t now_ms = 1000U;
    simulated_ecu_init(&target_ecu, now_ms);
    printf("[Target ECU] Booted in Default Session (0x01). Flash ready.\n");

    /* 3. Initialize Telematics Gateway FOTA Client */
    fota_client_t client;
    fota_client_init(&client, 0x08040000U /* STM32 Flash Bank 2 */, dummy_firmware, (uint32_t)sizeof(dummy_firmware));
    printf("[FOTA Client] Initialized. Target Address: 0x08040000, Size: %u bytes\n\n",
           (unsigned int)sizeof(dummy_firmware));

    uint8_t req_buf[512];
    uint16_t req_len = 0U;
    uint8_t resp_buf[512];
    uint16_t resp_len = 0U;
    int step_counter = 1;

    /* Execute the 10-step Automotive Flashing State Machine */
    while (fota_client_get_step(&client) != FOTA_STEP_COMPLETED &&
           fota_client_get_step(&client) != FOTA_STEP_FAILED) {

        fota_step_t step_before = fota_client_get_step(&client);
        bool built = fota_client_build_next_request(&client, req_buf, &req_len, sizeof(req_buf));
        assert(built);

        printf("[Step %02d] Client -> ECU: [ ", step_counter++);
        for (uint16_t i = 0; i < (req_len > 12 ? 12 : req_len); ++i) {
            printf("%02X ", req_buf[i]);
        }
        if (req_len > 12) printf("... (%u bytes total) ", req_len);
        printf("]\n");

        /* Target ECU processes request via UDS server */
        UdsCallbackResult res = simulated_ecu_process_request(&target_ecu, req_buf, req_len,
                                                              resp_buf, &resp_len, sizeof(resp_buf),
                                                              now_ms);
        assert(res == UDS_RESULT_OK);

        printf("          ECU -> Client: [ ");
        for (uint16_t i = 0; i < resp_len; ++i) {
            printf("%02X ", resp_buf[i]);
        }
        printf("] (Positive Response)\n");

        /* Client ingests response and advances state machine */
        bool accepted = fota_client_handle_response(&client, resp_buf, resp_len);
        assert(accepted);

        if (step_before == FOTA_STEP_SECURITY_KEY) {
            printf("          >>> Target ECU Unlocked (Security Level 1)\n");
        } else if (step_before == FOTA_STEP_ERASE_MEMORY) {
            printf("          >>> Target Flash Erase Completed\n");
        } else if (step_before == FOTA_STEP_VERIFY_DEPENDENCY) {
            printf("          >>> Target Cryptographic Image Integrity Verified\n");
        } else if (step_before == FOTA_STEP_ECU_RESET) {
            printf("          >>> Target HardReset Triggered! Activating new firmware.\n");
        }
        now_ms += 20U;
    }

    assert(fota_client_get_step(&client) == FOTA_STEP_COMPLETED);
    assert(target_ecu.reset_triggered);
    assert(target_ecu.bytes_received == sizeof(dummy_firmware));
    assert(memcmp(target_ecu.flash_buffer, dummy_firmware, sizeof(dummy_firmware)) == 0);

    printf("\n===================================================================\n");
    printf(" SUCCESS: Full 10-Step FOTA Flashing Cycle Completed over UDS!\n");
    printf(" Transferred: %u bytes to 0x08040000, Flash CRC Verified, ECU Rebooted.\n",
           target_ecu.bytes_received);
    printf("===================================================================\n");
    return 0;
}
