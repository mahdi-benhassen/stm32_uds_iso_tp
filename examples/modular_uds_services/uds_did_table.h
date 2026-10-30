/**
 * @file uds_did_table.h
 * @brief Unified Data Identifier (DID) Registry for Services 0x22, 0x2E, 0x2A, 0x2C
 */

#ifndef UDS_DID_TABLE_H
#define UDS_DID_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define UDS_DID_MAX_ENTRIES 32U
#define UDS_DID_MAX_DATA_LEN 64U

typedef struct {
    uint16_t id;
    uint8_t  data[UDS_DID_MAX_DATA_LEN];
    uint16_t len;
    bool     writable;
    bool     secured;
} uds_did_record_t;

#ifdef __cplusplus
extern "C" {
#endif

void uds_did_table_init(void);
const uds_did_record_t *uds_did_table_find(uint16_t id);
bool uds_did_table_write(uint16_t id, const uint8_t *data, uint16_t len);
bool uds_did_table_register(uint16_t id, const uint8_t *initial_data, uint16_t len,
                            bool writable, bool secured);

#ifdef __cplusplus
}
#endif

#endif /* UDS_DID_TABLE_H */
