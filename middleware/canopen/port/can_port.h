/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#ifndef CANOPEN_MIDDLEWARE_CAN_PORT_H
#define CANOPEN_MIDDLEWARE_CAN_PORT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_PORT_MAX_DLC 8U
/* One slot is reserved to distinguish empty from full. The STM32 façade can
 * therefore queue CAN_PORT_RX_QUEUE_CAPACITY - 1 frames. */
#define CAN_PORT_RX_QUEUE_CAPACITY 8U

typedef void (*can_port_rx_callback_t)(uint32_t id, uint8_t *data, uint8_t len);

/**
 * Initialize the selected CAN transport at the requested nominal bit rate.
 *
 * For SocketCAN builds, the interface name is supplied through CAN_PORT_IFACE
 * (default: vcan0) and the rate is informational because vcan has no bit-rate
 * controller. For STM32 builds, call can_port_stm32_bind() first.
 *
 * @return 0 on success or a negative errno-style status value.
 */
int can_port_init(uint32_t bitrate);

/**
 * Send one classic-CAN standard-data frame.
 *
 * @return 0 on acceptance or a negative errno-style status value.
 */
int can_port_send(uint32_t id, uint8_t *data, uint8_t len);

/**
 * Register one receive callback.
 *
 * Registration is a mainline operation. The STM32 façade publishes received
 * frames into a bounded queue from the board-owned RX ISR; it never invokes
 * this callback from interrupt context. The callback is invoked only by the
 * caller of can_port_poll() and may therefore perform mainline-safe work.
 * A nonzero timeout is bounded by the STM32 HAL tick and returns 0 when the
 * deadline expires. Unsupported STM32 targets return -ENOTSUP rather than
 * starting an unconfigured controller.
 */
void can_port_register_rx(can_port_rx_callback_t cb);

/**
 * Receive at most one frame and invoke the registered callback in caller
 * context. timeout_ms == 0 performs a non-blocking poll; returns 1 if a frame
 * was dispatched, 0 when no frame is available, and a negative errno-style
 * value on failure. For STM32F767, a nonzero timeout waits in bounded
 * mainline context using HAL_GetTick(); zero remains non-blocking.
 */
int can_port_poll(uint32_t timeout_ms);

/**
 * Close the host transport or reset the portable port state.
 *
 * This is a mainline lifecycle operation. It must not run concurrently with
 * the board RX ISR; disable the controller interrupt source before calling it.
 */
void can_port_deinit(void);

#ifdef CAN_PORT_STM32
#include "stm32f7xx_hal.h"

/**
 * Bind the CubeMX-generated CAN handle before calling can_port_init().
 *
 * The STM32 façade and CANopenNode_STM32 must never own HAL callbacks or the
 * lifecycle of the same bxCAN peripheral concurrently. Binding and
 * deinitialization are mainline-only lifecycle operations.
 *
 * @return 0 on success, -EINVAL for a null handle, or -EBUSY when already
 * bound.
 */
int can_port_stm32_bind(CAN_HandleTypeDef *hcan);

/**
 * Enqueue a validated frame read by a board-owned HAL RX ISR.
 *
 * This function is ISR-safe, bounded, and non-blocking. It copies the frame
 * into the fixed-size queue and never calls application code. When the queue
 * is full, the frame is dropped and can_port_stm32_rx_dropped() increments.
 */
void can_port_stm32_dispatch_rx_from_isr(uint32_t id, uint8_t *data, uint8_t len);

/** Return the number of ISR frames dropped because the receive queue was full. */
uint32_t can_port_stm32_rx_dropped(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CANOPEN_MIDDLEWARE_CAN_PORT_H */
