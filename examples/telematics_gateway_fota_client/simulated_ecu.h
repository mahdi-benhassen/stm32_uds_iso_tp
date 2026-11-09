#ifndef SIMULATED_ECU_H
#define SIMULATED_ECU_H

#include <stdint.h>
#include <stdbool.h>
#include "uds_iso_tp/uds.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    UdsServer server;
    uint32_t active_seed;
    bool memory_erased;
    uint32_t download_address;
    uint32_t download_size;
    uint32_t bytes_received;
    uint8_t flash_buffer[1024];
    bool flash_verified;
    bool reset_triggered;
} simulated_ecu_t;

void simulated_ecu_init(simulated_ecu_t *ecu, uint32_t now_ms);
UdsCallbackResult simulated_ecu_process_request(simulated_ecu_t *ecu,
                                                const uint8_t *req, uint16_t req_len,
                                                uint8_t *resp, uint16_t *resp_len,
                                                uint16_t resp_cap, uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* SIMULATED_ECU_H */
