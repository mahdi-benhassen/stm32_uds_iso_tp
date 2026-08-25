#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "canopen_reference_protocol.h"

int main(void) {
    const uint8_t node_id = 0x23U;

    assert(CANOPEN_REFERENCE_COBID_NMT == 0x000U);
    assert(CanopenReference_HeartbeatCobId(node_id) == 0x723U);
    assert(CanopenReference_EmcyCobId(node_id) == 0x0A3U);
    assert(CanopenReference_SdoRequestCobId(node_id) == 0x623U);
    assert(CanopenReference_SdoResponseCobId(node_id) == 0x5A3U);
    assert(CanopenReference_TpdoCobId(1U, node_id) == 0x1A3U);
    assert(CanopenReference_TpdoCobId(2U, node_id) == 0x2A3U);
    assert(CanopenReference_TpdoCobId(3U, node_id) == 0x3A3U);
    assert(CanopenReference_TpdoCobId(4U, node_id) == 0x4A3U);
    assert(CanopenReference_TpdoCobId(5U, node_id) == 0x1B3U);
    assert(CanopenReference_TpdoCobId(6U, node_id) == 0x2B3U);
    assert(CanopenReference_TpdoCobId(0U, node_id) == 0U);
    assert(CanopenReference_TpdoCobId(7U, node_id) == 0U);

    assert(CANOPEN_REFERENCE_COBID_LSS_MASTER == 0x7E5U);
    assert(CANOPEN_REFERENCE_COBID_LSS_SLAVE == 0x7E4U);
    assert(CANOPEN_REFERENCE_SDO_UPLOAD_REQUEST == 0x40U);
    assert(CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_1 == 0x4FU);
    assert(CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_2 == 0x4BU);
    assert(CANOPEN_REFERENCE_SDO_UPLOAD_RESPONSE_4 == 0x43U);
    assert(CANOPEN_REFERENCE_SDO_DOWNLOAD_RESPONSE == 0x60U);

    assert(!CanopenReference_NodeIdValid(0x00U));
    assert(CanopenReference_NodeIdValid(0x01U));
    assert(CanopenReference_NodeIdValid(0x31U));
    assert(CanopenReference_NodeIdValid(0x3FU));
    assert(CanopenReference_NodeIdValid(0x7FU));
    assert(!CanopenReference_NodeIdValid(0x80U));

    /* Issue #13's 0x780 heartbeat image is intentionally not encoded: the
     * standard heartbeat formula is 0x700 + node-ID. */
    assert(CanopenReference_HeartbeatCobId(node_id) != 0x7A3U);

    puts("protocol_contract: PASS (standard CANopen COB-IDs, SDO bytes, node-ID policy)");
    return 0;
}
