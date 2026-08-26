#ifndef TEST_C092_FDCAN_SHIM_H
#define TEST_C092_FDCAN_SHIM_H

#include <stdint.h>

typedef enum { HAL_ERROR = 0U, HAL_OK = 1U } HAL_StatusTypeDef;

typedef struct {
    void *Instance;
} FDCAN_HandleTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t MessageMarker;
    uint32_t EventType;
} FDCAN_TxEventFifoTypeDef;

#define FDCAN_DLC_BYTES_0 0U
#define FDCAN_DLC_BYTES_1 1U
#define FDCAN_DLC_BYTES_2 2U
#define FDCAN_DLC_BYTES_3 3U
#define FDCAN_DLC_BYTES_4 4U
#define FDCAN_DLC_BYTES_5 5U
#define FDCAN_DLC_BYTES_6 6U
#define FDCAN_DLC_BYTES_7 7U
#define FDCAN_DLC_BYTES_8 8U
#define FDCAN_STANDARD_ID 1U
#define FDCAN_DATA_FRAME 0U
#define FDCAN_ESI_ACTIVE 0U
#define FDCAN_BRS_OFF 0U
#define FDCAN_CLASSIC_CAN 0U
#define FDCAN_STORE_TX_EVENTS 1U
#define FDCAN_TX_EVENT 0U
#define FDCAN_IT_TX_EVT_FIFO_ELT_LOST (1UL << 0U)
#define FDCAN_IT_TX_EVT_FIFO_FULL (1UL << 1U)
#define FDCAN_IT_TX_EVT_FIFO_NEW_DATA (1UL << 2U)

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan,
                                                const FDCAN_TxHeaderTypeDef *header,
                                                const uint8_t *data);
HAL_StatusTypeDef HAL_FDCAN_GetTxEvent(FDCAN_HandleTypeDef *hfdcan,
                                       FDCAN_TxEventFifoTypeDef *event);

#endif
