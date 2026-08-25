#include "stm32f767_bxcan_adapter.h"
#include "stm32_fdcan_adapter.h"

#include <assert.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    IsoTpCanFrame last;
    bool called;
} FakeBus;

static bool fake_classic_send(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc) {
    FakeBus *bus = (FakeBus *)context;
    bus->last.can_id = can_id;
    bus->last.dlc = dlc;
    bus->last.is_fd = false;
    bus->last.bit_rate_switch = false;
    for (uint8_t index = 0U; index < dlc; ++index)
        bus->last.data[index] = data[index];
    bus->called = true;
    return true;
}

static bool fake_fd_send(void *context, uint32_t can_id, const uint8_t *data, uint8_t dlc,
                         bool bit_rate_switch) {
    FakeBus *bus = (FakeBus *)context;
    bus->last.can_id = can_id;
    bus->last.dlc = dlc;
    bus->last.is_fd = true;
    bus->last.bit_rate_switch = bit_rate_switch;
    for (uint8_t index = 0U; index < dlc; ++index)
        bus->last.data[index] = data[index];
    bus->called = true;
    return true;
}

static uint32_t fake_clock(void *context) {
    (void)context;
    return 0U;
}

static void test_classic(void) {
    FakeBus bus = {0};
    Stm32F767BxCanBinding binding = {
        .send_classic = fake_classic_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    assert(stm32f767_bxcan_endpoint_configure(&config, &binding, &callbacks, NULL));
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {.can_id = 0x7E0U, .dlc = 3U, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.called && bus.last.can_id == 0x7E8U && bus.last.dlc == 3U && !bus.last.is_fd);
}

static void test_fd(void) {
    FakeBus bus = {0};
    Stm32FdCanBinding binding = {.send_fd = fake_fd_send, .now_ms = fake_clock, .context = &bus};
    UdsCallbacks callbacks = {0};
    UdsIsoTpEndpointConfig config = {0};
    UdsIsoTpEndpoint endpoint;
    assert(stm32_fdcan_endpoint_configure(&config, &binding, &callbacks, NULL));
    assert(uds_isotp_endpoint_init(&endpoint, &config, 0U));
    IsoTpCanFrame request = {
        .can_id = 0x7E0U, .dlc = 8U, .is_fd = true, .data = {0x02U, 0x3EU, 0x00U}};
    assert(uds_isotp_endpoint_receive(&endpoint, &request, 0U) == ISOTP_TX_FRAME_READY);
    assert(uds_isotp_endpoint_process(&endpoint, 0U) == ISOTP_TX_FRAME_READY);
    assert(bus.called && bus.last.can_id == 0x7E8U && bus.last.dlc == 8U && bus.last.is_fd &&
           bus.last.bit_rate_switch);
}

int main(void) {
    test_classic();
    test_fd();
    return 0;
}
