/**
 * @file can_ring_buffer.h
 * @brief Lock-Free Interrupt-Safe CAN/CAN-FD Circular Ring Buffer Queue (Issue #137)
 * @details Designed for high-frequency CAN receive ISRs and background ISO-TP processing.
 */

#ifndef CAN_RING_BUFFER_H
#define CAN_RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "can_frame_fd.h"

#ifndef CAN_RING_BUFFER_CAPACITY
#define CAN_RING_BUFFER_CAPACITY 32U  /* Must be a power of 2 */
#endif

#define CAN_RING_BUFFER_MASK (CAN_RING_BUFFER_CAPACITY - 1U)

typedef struct {
    can_frame_t       frames[CAN_RING_BUFFER_CAPACITY];
    volatile uint32_t head;          /**< Write index (advanced by ISR) */
    volatile uint32_t tail;          /**< Read index (advanced by main thread) */
    volatile uint32_t drop_count;    /**< Overrun counter if queue is full */
} can_ring_buffer_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ring buffer queue
 */
void can_ring_buffer_init(can_ring_buffer_t *rb);

/**
 * @brief Push a CAN frame into the ring buffer from ISR (Producer)
 * @return true if enqueued successfully, false if queue is full (overrun)
 */
bool can_ring_buffer_push(can_ring_buffer_t *rb, const can_frame_t *frame);

/**
 * @brief Pop a CAN frame from the ring buffer in background loop (Consumer)
 * @return true if a frame was dequeued, false if queue is empty
 */
bool can_ring_buffer_pop(can_ring_buffer_t *rb, can_frame_t *out_frame);

/**
 * @brief Peek at next frame without removing it
 */
bool can_ring_buffer_peek(const can_ring_buffer_t *rb, can_frame_t *out_frame);

/**
 * @brief Check if queue is empty
 */
bool can_ring_buffer_is_empty(const can_ring_buffer_t *rb);

/**
 * @brief Check if queue is full
 */
bool can_ring_buffer_is_full(const can_ring_buffer_t *rb);

/**
 * @brief Number of frames currently in the queue
 */
uint32_t can_ring_buffer_count(const can_ring_buffer_t *rb);

/**
 * @brief Retrieve and clear overrun count
 */
uint32_t can_ring_buffer_get_drop_count(const can_ring_buffer_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* CAN_RING_BUFFER_H */
