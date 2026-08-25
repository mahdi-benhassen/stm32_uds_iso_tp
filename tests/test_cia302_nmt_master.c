#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cia302_nmt_master.h"

typedef struct {
    cia302_frame_t frames[16];
    size_t frame_count;
    cia302_event_t events[32];
    size_t event_count;
    bool send_ok;
} test_context_t;

static bool send_frame(void *context, const cia302_frame_t *frame) {
    test_context_t *ctx = context;
    assert(ctx->frame_count < 16U);
    ctx->frames[ctx->frame_count++] = *frame;
    return ctx->send_ok;
}

static void receive_event(void *context, const cia302_event_t *event) {
    test_context_t *ctx = context;
    assert(ctx->event_count < 32U);
    ctx->events[ctx->event_count++] = *event;
}

static bool saw_event(const test_context_t *ctx, cia302_event_type_t type, uint8_t node_id) {
    for (size_t i = 0U; i < ctx->event_count; ++i) {
        if (ctx->events[i].type == type && ctx->events[i].node_id == node_id) {
            return true;
        }
    }
    return false;
}

int main(void) {
    test_context_t ctx = {.send_ok = true};
    cia302_nmt_master_t master;
    cia302_nmt_master_init(&master, 1U, send_frame, receive_event, &ctx);
    master.nmt_startup = 0x01U;
    master.boot_time_ms = 1000U;
    assert(cia302_nmt_master_configure(&master, 2U, true, true, 100U));
    assert(cia302_nmt_master_configure(&master, 3U, false, true, 100U));
    assert(!cia302_nmt_master_configure(&master, 1U, true, true, 100U));
    assert(cia302_nmt_master_start(&master, 0U));

    uint8_t bootup = 0U;
    cia302_nmt_master_receive(&master, 0x702U, &bootup, 1U, 100U);
    cia302_nmt_master_receive(&master, 0x703U, &bootup, 1U, 110U);
    cia302_nmt_master_process(&master, 120U);
    assert(master.network_ready);
    assert(ctx.frame_count == 2U);
    assert(ctx.frames[0].can_id == 0U && ctx.frames[0].dlc == 2U);
    assert(ctx.frames[0].data[0] == CIA302_NMT_START && ctx.frames[0].data[1] == 2U);
    assert(ctx.frames[1].data[0] == CIA302_NMT_START && ctx.frames[1].data[1] == 3U);
    assert(saw_event(&ctx, CIA302_EVENT_BOOTUP, 2U));
    assert(saw_event(&ctx, CIA302_EVENT_NETWORK_READY, 0U));

    cia302_nmt_master_process(&master, 250U);
    assert(saw_event(&ctx, CIA302_EVENT_HEARTBEAT_TIMEOUT, 2U));
    assert(saw_event(&ctx, CIA302_EVENT_HEARTBEAT_TIMEOUT, 3U));

    uint8_t malformed[2] = {CIA302_HEARTBEAT_OPERATIONAL, 0U};
    size_t before = ctx.event_count;
    cia302_nmt_master_receive(&master, 0x702U, malformed, 2U, 300U);
    assert(ctx.event_count == before + 1U);
    assert(ctx.events[ctx.event_count - 1U].type == CIA302_EVENT_INVALID_FRAME);

    assert(cia302_nmt_master_request(&master, CIA302_NMT_RESET_COMMUNICATION, 0U));
    assert(ctx.frames[ctx.frame_count - 1U].data[0] == CIA302_NMT_RESET_COMMUNICATION);
    assert(ctx.frames[ctx.frame_count - 1U].data[1] == 0U);
    assert(!cia302_nmt_master_request(&master, 0x7FU, 2U));

    test_context_t timeout_ctx = {.send_ok = true};
    cia302_nmt_master_t timeout_master;
    cia302_nmt_master_init(&timeout_master, 1U, send_frame, receive_event, &timeout_ctx);
    timeout_master.boot_time_ms = 50U;
    timeout_master.nmt_startup = CIA302_NMT_START;
    assert(cia302_nmt_master_configure(&timeout_master, 4U, true, true, 100U));
    assert(cia302_nmt_master_start(&timeout_master, 1000U));
    cia302_nmt_master_process(&timeout_master, 1051U);
    assert(!timeout_master.network_ready);
    assert(saw_event(&timeout_ctx, CIA302_EVENT_BOOT_TIMEOUT, 4U));

    /* Invalid configuration, callback, command, and startup-policy boundaries. */
    assert(!cia302_nmt_master_configure(NULL, 2U, true, true, 100U));
    assert(!cia302_nmt_master_configure(&master, 0U, true, true, 100U));
    assert(!cia302_nmt_master_configure(&master, 0xFFU, true, true, 100U));
    assert(!cia302_nmt_master_configure(&master, 1U, true, true, 100U));
    assert(!cia302_nmt_master_request(NULL, CIA302_NMT_START, 0U));
    assert(!cia302_nmt_master_request(&master, 0x7FU, 0U));
    assert(!cia302_nmt_master_request(&master, CIA302_NMT_START, 0xFFU));

    cia302_nmt_master_t no_send_master;
    cia302_nmt_master_init(&no_send_master, 1U, NULL, receive_event, &ctx);
    assert(!cia302_nmt_master_request(&no_send_master, CIA302_NMT_START, 0U));
    assert(!cia302_nmt_master_start(NULL, 100U));
    assert(!cia302_nmt_master_start(&no_send_master, 100U));

    test_context_t send_fail_ctx = {.send_ok = false};
    cia302_nmt_master_t send_fail_master;
    cia302_nmt_master_init(&send_fail_master, 1U, send_frame, receive_event, &send_fail_ctx);
    assert(cia302_nmt_master_configure(&send_fail_master, 2U, false, false, 0U));
    send_fail_master.nmt_startup = CIA302_NMT_START;
    assert(cia302_nmt_master_start(&send_fail_master, 0U));
    assert(!cia302_nmt_master_request(&send_fail_master, CIA302_NMT_START, 2U));

    cia302_nmt_master_receive(NULL, 0x702U, &bootup, 1U, 0U);
    cia302_nmt_master_receive(&send_fail_master, 0x702U, NULL, 1U, 0U);
    cia302_nmt_master_receive(&send_fail_master, 0x702U, &bootup, 0U, 0U);
    cia302_nmt_master_receive(&send_fail_master, CIA302_HEARTBEAT_BASE - 1U, &bootup, 1U, 0U);
    cia302_nmt_master_receive(&send_fail_master, CIA302_HEARTBEAT_BASE + CIA302_MAX_NODES + 1U,
                              &bootup, 1U, 0U);
    cia302_nmt_master_receive(&send_fail_master, 0x703U, &bootup, 1U, 0U);
    uint8_t preop = CIA302_HEARTBEAT_PREOP;
    uint8_t operational = CIA302_HEARTBEAT_OPERATIONAL;
    uint8_t stopped = CIA302_HEARTBEAT_STOPPED;
    uint8_t invalid_state = 0x7FU;
    cia302_nmt_master_receive(&send_fail_master, 0x702U, &preop, 1U, 1U);
    cia302_nmt_master_receive(&send_fail_master, 0x702U, &operational, 1U, 2U);
    cia302_nmt_master_receive(&send_fail_master, 0x702U, &stopped, 1U, 3U);
    cia302_nmt_master_receive(&send_fail_master, 0x702U, &invalid_state, 1U, 4U);
    cia302_nmt_master_process(NULL, 10U);
    cia302_nmt_master_process(&no_send_master, 10U);
    cia302_nmt_master_process(&send_fail_master, 10U);
    cia302_nmt_master_process(&send_fail_master, 11U);

    /* A node with no heartbeat timeout exercises the zero-timeout branch. */
    test_context_t policy_ctx = {.send_ok = true};
    cia302_nmt_master_t policy_master;
    cia302_nmt_master_init(&policy_master, 1U, send_frame, receive_event, &policy_ctx);
    policy_master.nmt_startup = CIA302_NMT_START;
    policy_master.boot_time_ms = 0U;
    assert(cia302_nmt_master_configure(&policy_master, 2U, false, true, 0U));
    assert(cia302_nmt_master_start(&policy_master, 20U));
    cia302_nmt_master_receive(&policy_master, 0x702U, &bootup, 1U, 21U);
    cia302_nmt_master_process(&policy_master, 22U);
    assert(policy_master.network_ready);
    assert(policy_ctx.frame_count == 1U);

    /* Startup bit 3 suppresses automatic NMT-start requests. */
    test_context_t no_auto_ctx = {.send_ok = true};
    cia302_nmt_master_t no_auto_master;
    cia302_nmt_master_init(&no_auto_master, 1U, send_frame, receive_event, &no_auto_ctx);
    no_auto_master.nmt_startup = CIA302_NMT_START | (1UL << 3U);
    assert(cia302_nmt_master_configure(&no_auto_master, 2U, false, true, 100U));
    assert(cia302_nmt_master_start(&no_auto_master, 30U));
    cia302_nmt_master_receive(&no_auto_master, 0x702U, &bootup, 1U, 31U);
    cia302_nmt_master_process(&no_auto_master, 32U);
    assert(no_auto_master.network_ready);
    assert(no_auto_ctx.frame_count == 0U);

    puts("cia302_nmt_master: PASS");
    return 0;
}
