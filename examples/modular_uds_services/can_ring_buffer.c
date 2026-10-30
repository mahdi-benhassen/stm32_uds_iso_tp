/**
 * @file can_ring_buffer.c
 * @brief Lock-Free Interrupt-Safe CAN Ring Buffer Queue Implementation
 */

#include "can_ring_buffer.h"
#include <string.h>

void can_ring_buffer_init(can_ring_buffer_t *rb) {
    if (rb != NULL) {
        rb->head = 0U;
        rb->tail = 0U;
        rb->drop_count = 0U;
        memset(rb->frames, 0, sizeof(rb->frames));
    }
}

bool can_ring_buffer_push(can_ring_buffer_t *rb, const can_frame_t *frame) {
    if ((rb == NULL) || (frame == NULL)) {
        return false;
    }

    uint32_t current_head = rb->head;
    uint32_t current_tail = rb->tail;
    uint32_t next_head = (current_head + 1U) & CAN_RING_BUFFER_MASK;

    /* Check for queue full condition */
    if (next_head == current_tail) {
        rb->drop_count++;
        return false;
    }

    /* Enqueue frame */
    rb->frames[current_head] = *frame;

    /* Advance head atomically for consumer */
    rb->head = next_head;
    return true;
}

bool can_ring_buffer_pop(can_ring_buffer_t *rb, can_frame_t *out_frame) {
    if ((rb == NULL) || (out_frame == NULL)) {
        return false;
    }

    uint32_t current_head = rb->head;
    uint32_t current_tail = rb->tail;

    /* Check for queue empty condition */
    if (current_head == current_tail) {
        return false;
    }

    /* Dequeue frame */
    *out_frame = rb->frames[current_tail];

    /* Advance tail */
    rb->tail = (current_tail + 1U) & CAN_RING_BUFFER_MASK;
    return true;
}

bool can_ring_buffer_peek(const can_ring_buffer_t *rb, can_frame_t *out_frame) {
    if ((rb == NULL) || (out_frame == NULL)) {
        return false;
    }

    uint32_t current_head = rb->head;
    uint32_t current_tail = rb->tail;

    if (current_head == current_tail) {
        return false;
    }

    *out_frame = rb->frames[current_tail];
    return true;
}

bool can_ring_buffer_is_empty(const can_ring_buffer_t *rb) {
    if (rb == NULL) {
        return true;
    }
    return (rb->head == rb->tail);
}

bool can_ring_buffer_is_full(const can_ring_buffer_t *rb) {
    if (rb == NULL) {
        return false;
    }
    return (((rb->head + 1U) & CAN_RING_BUFFER_MASK) == rb->tail);
}

uint32_t can_ring_buffer_count(const can_ring_buffer_t *rb) {
    if (rb == NULL) {
        return 0U;
    }
    uint32_t current_head = rb->head;
    uint32_t current_tail = rb->tail;
    return ((current_head - current_tail) & CAN_RING_BUFFER_MASK);
}

uint32_t can_ring_buffer_get_drop_count(const can_ring_buffer_t *rb) {
    if (rb == NULL) {
        return 0U;
    }
    return rb->drop_count;
}
