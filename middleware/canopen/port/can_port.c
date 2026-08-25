/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
/*
 * Optional STM32 classic-CAN transport facade.
 *
 * This source is intentionally not linked into the default CANopenNode target.
 * CANopenNode_STM32 already owns HAL CAN callbacks and the controller lifecycle
 * for the production node. Use this facade for standalone diagnostics, a test
 * client, or a future adapter after assigning exclusive callback ownership.
 */
#include "can_port.h"

#ifdef CAN_PORT_STM32

#include <errno.h>
#include <stddef.h>
#include <string.h>

typedef struct {
    uint32_t id;
    uint8_t data[CAN_PORT_MAX_DLC];
    uint8_t len;
} can_port_rx_frame_t;

/*
 * The board-owned RX interrupt is the sole producer and can_port_poll() is the
 * sole consumer. A reserved slot distinguishes a full queue from an empty one.
 * The queue isolates application callbacks from interrupt context and keeps ISR
 * work bounded to validation, a small copy, and atomic index publication.
 */
static CAN_HandleTypeDef *s_hcan;
static can_port_rx_callback_t s_rx_callback;
static can_port_rx_frame_t s_rx_queue[CAN_PORT_RX_QUEUE_CAPACITY];
static uint32_t s_rx_head;
static uint32_t s_rx_tail;
static uint32_t s_rx_dropped;

typedef struct {
    uint32_t bitrate;
    uint32_t prescaler;
    uint32_t time_seg1_tq;
    uint32_t time_seg2_tq;
} can_port_timing_t;

/* APB1 is 54 MHz on the reference STM32F767 clock tree. The 800 kbit/s
 * entry is the closest legal bxCAN divisor (794.1 kbit/s, -0.74%), while all
 * other entries are exact. Applications requiring a different clock tree must
 * provide a board-specific facade. */
static const can_port_timing_t s_timing_table[] = {
    {10000U, 216U, 16U, 8U},
    {20000U, 108U, 16U, 8U},
    {50000U,  60U, 14U, 3U},
    {125000U, 27U, 12U, 3U},
    {250000U, 12U, 14U, 3U},
    {500000U,  6U, 14U, 3U},
    {800000U,  4U, 13U, 3U},
    {1000000U, 3U, 14U, 3U},
};

static const can_port_timing_t *
can_port_find_timing(uint32_t bitrate) {
    for (size_t i = 0U; i < (sizeof(s_timing_table) / sizeof(s_timing_table[0])); ++i) {
        if (s_timing_table[i].bitrate == bitrate) {
            return &s_timing_table[i];
        }
    }
    return NULL;
}

static uint32_t
can_port_next_index(uint32_t index) {
    return (index + 1U) % CAN_PORT_RX_QUEUE_CAPACITY;
}

static void
can_port_reset_rx_queue(void) {
    __atomic_store_n(&s_rx_head, 0U, __ATOMIC_RELEASE);
    __atomic_store_n(&s_rx_tail, 0U, __ATOMIC_RELEASE);
    __atomic_store_n(&s_rx_dropped, 0U, __ATOMIC_RELEASE);
}

int
can_port_stm32_bind(CAN_HandleTypeDef *hcan) {
    if (hcan == NULL) {
        return -EINVAL;
    }
    if (s_hcan != NULL) {
        return -EBUSY;
    }
    s_hcan = hcan;
    return 0;
}

int
can_port_init(uint32_t bitrate) {
    const can_port_timing_t *timing = can_port_find_timing(bitrate);

    if (s_hcan == NULL) {
        return -ENODEV;
    }
    if (timing == NULL) {
        return -EINVAL;
    }

#if defined(STM32F767xx) || defined(CAN_PORT_FAKE_F767)
    s_hcan->Init.Prescaler = timing->prescaler;
    s_hcan->Init.TimeSeg1 = (timing->time_seg1_tq == 12U) ? CAN_BS1_12TQ
                                                            : (timing->time_seg1_tq == 13U) ? CAN_BS1_13TQ
                                                                                             : (timing->time_seg1_tq == 14U) ? CAN_BS1_14TQ
                                                                                                                              : CAN_BS1_16TQ;
    s_hcan->Init.TimeSeg2 = (timing->time_seg2_tq == 3U) ? CAN_BS2_3TQ : CAN_BS2_8TQ;
    if (HAL_CAN_Init(s_hcan) != HAL_OK) {
        return -EIO;
    }
#else
    (void)timing;
    return -ENOTSUP;
#endif
    can_port_reset_rx_queue();
    if (HAL_CAN_Start(s_hcan) != HAL_OK) {
        return -EIO;
    }
    if (HAL_CAN_ActivateNotification(s_hcan,
                                     CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING
                                         | CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_ERROR)
        != HAL_OK) {
        (void)HAL_CAN_Stop(s_hcan);
        return -EIO;
    }
    return 0;
}

int
can_port_send(uint32_t id, uint8_t *data, uint8_t len) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox = 0U;

    if (s_hcan == NULL || data == NULL || len > CAN_PORT_MAX_DLC || id > 0x7FFU) {
        return -EINVAL;
    }
    if (HAL_CAN_GetTxMailboxesFreeLevel(s_hcan) == 0U) {
        return -EAGAIN;
    }

    header.StdId = id;
    header.ExtId = 0U;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = len;
    header.TransmitGlobalTime = DISABLE;
    return HAL_CAN_AddTxMessage(s_hcan, &header, data, &mailbox) == HAL_OK ? 0 : -EIO;
}

void
can_port_register_rx(can_port_rx_callback_t cb) {
    /* The callback is consumed only by mainline can_port_poll(), but atomic
     * publication also keeps registration/de-registration well-defined. */
    __atomic_store_n(&s_rx_callback, cb, __ATOMIC_RELEASE);
}

static int
can_port_poll_one(void) {
    uint32_t tail = __atomic_load_n(&s_rx_tail, __ATOMIC_RELAXED);
    uint32_t head = __atomic_load_n(&s_rx_head, __ATOMIC_ACQUIRE);
    can_port_rx_frame_t frame;
    can_port_rx_callback_t callback;

    if (tail == head) {
        return 0;
    }
    frame = s_rx_queue[tail];
    __atomic_store_n(&s_rx_tail, can_port_next_index(tail), __ATOMIC_RELEASE);
    callback = __atomic_load_n(&s_rx_callback, __ATOMIC_ACQUIRE);
    if (callback != NULL) {
        callback(frame.id, frame.data, frame.len);
    }
    return 1;
}

int
can_port_poll(uint32_t timeout_ms) {
    uint32_t start_tick;
    int dispatched = can_port_poll_one();

    if (dispatched != 0 || timeout_ms == 0U) {
        return dispatched;
    }
#if !defined(STM32F767xx) && !defined(CAN_PORT_FAKE_F767)
    (void)timeout_ms;
    return -ENOTSUP;
#else
    start_tick = HAL_GetTick();
    do {
        if (can_port_poll_one() != 0) {
            return 1;
        }
        if ((uint32_t)(HAL_GetTick() - start_tick) >= timeout_ms) {
            return 0;
        }
        HAL_Delay(1U);
    } while (1);
#endif
}

void
can_port_deinit(void) {
    if (s_hcan != NULL) {
        (void)HAL_CAN_DeactivateNotification(s_hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO1_MSG_PENDING
                                                          | CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_ERROR);
        (void)HAL_CAN_Stop(s_hcan);
    }
    __atomic_store_n(&s_rx_callback, (can_port_rx_callback_t)NULL, __ATOMIC_RELEASE);
    can_port_reset_rx_queue();
    s_hcan = NULL;
}

void
can_port_stm32_dispatch_rx_from_isr(uint32_t id, uint8_t *data, uint8_t len) {
    uint32_t head;
    uint32_t next_head;
    uint32_t tail;
    can_port_rx_frame_t *slot;

    if (data == NULL || len > CAN_PORT_MAX_DLC || id > 0x7FFU) {
        return;
    }

    head = __atomic_load_n(&s_rx_head, __ATOMIC_RELAXED);
    next_head = can_port_next_index(head);
    tail = __atomic_load_n(&s_rx_tail, __ATOMIC_ACQUIRE);
    if (next_head == tail) {
        (void)__atomic_fetch_add(&s_rx_dropped, 1U, __ATOMIC_RELAXED);
        return;
    }

    slot = &s_rx_queue[head];
    slot->id = id;
    slot->len = len;
    (void)memcpy(slot->data, data, len);
    __atomic_store_n(&s_rx_head, next_head, __ATOMIC_RELEASE);
}

uint32_t
can_port_stm32_rx_dropped(void) {
    return __atomic_load_n(&s_rx_dropped, __ATOMIC_ACQUIRE);
}

#else

/* A deliberate build failure prevents accidental selection of this source for
 * a host transport. Compile vcan_port.c for SocketCAN builds instead. */
#error "can_port.c requires CAN_PORT_STM32; use vcan_port.c for host builds"

#endif
