/*
 * SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0
 *
 * Project-owned runtime wrapper derived from the CANopenNode STM32 integration
 * model. Compile this file instead of CANopenNode_STM32/CO_app_STM32.c.
 */
/* CO_app_STM32.h transitively includes CANopen.h, so the entire third-party
 * include block must sit inside one suppression region; see
 * canopen_reference_co.h for the boundary rationale. */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include "CO_app_STM32.h"

#include <stdbool.h>

#include "CANopen.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include "can_acceptance_filter.h"
#include "canopen_reference_od.h"
#include "canopen_reference_config.h"
#include "canopen_reference_can_recovery.h"
#include "canopen_reference_cia302.h"
#include "canopen_reference_diagnostics.h"
#include "canopen_reference_gateway.h"
#include "canopen_reference_lss.h"
#include "canopen_reference_lifecycle.h"
#include "canopen_reference_storage.h"
#include "canopen_reference_timing.h"
#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
#include "canopen_reference_uds.h"
#endif
#include "cia401_reference.h"
#include "cia402_reference.h"
#include "cia418_reference.h"

#define CANOPEN_REFERENCE_NMT_CONTROL \
    (CO_NMT_STARTUP_TO_OPERATIONAL | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION)
#define CANOPEN_REFERENCE_FIRST_HB_MS        500U
#define CANOPEN_REFERENCE_SDO_SRV_TIMEOUT_MS 1000U
#define CANOPEN_REFERENCE_SDO_CLI_TIMEOUT_MS 1000U
CANopenNodeSTM32 *canopenNodeSTM32 = NULL;
CO_t *CO = NULL;

static uint32_t canopenReferenceLastTick;
static CANopenReferenceLssState canopenReferenceLssState;
static volatile bool canopenReferenceCanRecoveryPending;
static volatile bool canopenReferenceSafeFault;
static volatile bool canopenReferenceRecoveryAttemptActive;
static CANopenReferenceCanRecovery canopenReferenceCanRecovery;
static CANopenReferenceRuntimeState canopenReferenceRuntimeState = CANOPEN_REFERENCE_RUNTIME_INIT;
#if (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
static Cia418ReferenceState canopenReferenceCia418State;
#endif

static void
CANopenReference_SetRuntimeState(CANopenReferenceRuntimeState state) {
    canopenReferenceRuntimeState = state;
}

CANopenReferenceRuntimeState
CANopenReference_RuntimeState(void) {
    return canopenReferenceRuntimeState;
}

static bool
CANopenReference_FilterAdd(uint16_t *ids, uint32_t *count, uint16_t id) {
    return CANopenAcceptanceFilter_Add(ids, CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS, count, id);
}

bool
CANopenReference_ConfigureCanFilter(uint8_t node_id) {
    CAN_HandleTypeDef *hcan;
    uint16_t ids[CANOPEN_REFERENCE_CAN_FILTER_MAX_IDS] = {0};
    uint32_t count = 0U;
    const uint32_t rpdo_cob_ids[] = {
        OD_PERSIST_COMM.x1400_RPDOCommunicationParameter.COB_IDUsedByRPDO,
        OD_PERSIST_COMM.x1401_RPDOCommunicationParameter.COB_IDUsedByRPDO,
        OD_PERSIST_COMM.x1402_RPDOCommunicationParameter.COB_IDUsedByRPDO,
        OD_PERSIST_COMM.x1403_RPDOCommunicationParameter.COB_IDUsedByRPDO,
    };

    if (canopenNodeSTM32 == NULL || canopenNodeSTM32->CANHandle == NULL
        || node_id == 0U || node_id > 127U) {
        return false;
    }
    hcan = canopenNodeSTM32->CANHandle;
    if (!CANopenReference_FilterAdd(ids, &count, 0x000U) /* NMT */
        || !CANopenReference_FilterAdd(ids, &count, 0x080U) /* SYNC */
        || !CANopenReference_FilterAdd(ids, &count, 0x100U) /* TIME */
        || !CANopenReference_FilterAdd(ids, &count, (uint16_t)(0x080U + node_id))) { /* EMCY */
        return false;
    }
    for (uint32_t i = 0U; i < 4U; ++i) {
        if ((rpdo_cob_ids[i] & 0x80000000UL) == 0U) {
            if (!CANopenReference_FilterAdd(ids, &count, (uint16_t)rpdo_cob_ids[i])) {
                return false;
            }
        }
    }
    if ((OD_RAM.x1200_SDOServerParameter.COB_IDClientToServerRx & 0x80000000UL) == 0U) {
        if (!CANopenReference_FilterAdd(ids, &count,
                                         (uint16_t)OD_RAM.x1200_SDOServerParameter.COB_IDClientToServerRx)) {
            return false;
        }
    }
    if ((OD_PERSIST_COMM.x1280_SDOClientParameter.COB_IDServerToClientRx & 0x80000000UL) == 0U) {
        if (!CANopenReference_FilterAdd(ids, &count,
                                         (uint16_t)OD_PERSIST_COMM.x1280_SDOClientParameter.COB_IDServerToClientRx)) {
            return false;
        }
    }
    if (!CANopenReference_FilterAdd(ids, &count, (uint16_t)(0x700U + node_id)) /* heartbeat */
#if CANOPEN_REFERENCE_ENABLE_CIA302_MASTER
        || !CANopenReference_FilterAdd(ids, &count,
                                       (uint16_t)(0x700U + CANOPEN_REFERENCE_CIA302_PEER_NODE_ID))
#endif
        || !CANopenReference_FilterAdd(ids, &count, 0x7E4U) /* LSS master -> slave */
        || !CANopenReference_FilterAdd(ids, &count, 0x7E5U)) { /* LSS slave -> master */
        return false;
    }
#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
    uint16_t uds_ids[2] = {(uint16_t)UDS_RX_CAN_ID, (uint16_t)UDS_TX_CAN_ID};
    uint32_t uds_count = 2U;
#endif

    for (uint32_t bank = 0U; bank < ((count + 3U) / 4U); ++bank) {
        CAN_FilterTypeDef filter = {0};
        uint32_t base = bank * 4U;
        uint16_t slot0 = base < count ? ids[base] : 0x000U;
        uint16_t slot1 = (base + 1U) < count ? ids[base + 1U] : 0x000U;
        uint16_t slot2 = (base + 2U) < count ? ids[base + 2U] : 0x000U;
        uint16_t slot3 = (base + 3U) < count ? ids[base + 3U] : 0x000U;
        filter.FilterBank = bank;
        filter.FilterMode = CAN_FILTERMODE_IDLIST;
        filter.FilterScale = CAN_FILTERSCALE_16BIT;
        filter.FilterIdHigh = (uint16_t)(slot0 << 5U);
        filter.FilterIdLow = (uint16_t)(slot1 << 5U);
        filter.FilterMaskIdHigh = (uint16_t)(slot2 << 5U);
        filter.FilterMaskIdLow = (uint16_t)(slot3 << 5U);
        filter.FilterFIFOAssignment = CAN_RX_FIFO0;
        filter.FilterActivation = ENABLE;
        filter.SlaveStartFilterBank = 14U;
        if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
            return false;
        }
    }
#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
    if (uds_count != 0U) {
        uint32_t bank = (count + 3U) / 4U;
        CAN_FilterTypeDef filter = {0};
        filter.FilterBank = bank;
        filter.FilterMode = CAN_FILTERMODE_IDLIST;
        filter.FilterScale = CAN_FILTERSCALE_16BIT;
        filter.FilterIdHigh = (uint16_t)(uds_ids[0] << 5U);
        filter.FilterIdLow = (uint16_t)(uds_ids[1] << 5U);
        filter.FilterMaskIdHigh = 0U;
        filter.FilterMaskIdLow = 0U;
        filter.FilterFIFOAssignment = CAN_RX_FIFO1;
        filter.FilterActivation = ENABLE;
        filter.SlaveStartFilterBank = 14U;
        if (HAL_CAN_ConfigFilter(hcan, &filter) != HAL_OK) {
            return false;
        }
    }
#endif
    return count != 0U;
}

static void
CANopenReference_ApplyIdentity(void) {
    OD_PERSIST_COMM.x1018_identity.vendor_ID = CANOPEN_REFERENCE_VENDOR_ID;
    OD_PERSIST_COMM.x1018_identity.productCode = CANOPEN_REFERENCE_PRODUCT_CODE;
    OD_PERSIST_COMM.x1018_identity.revisionNumber = CANOPEN_REFERENCE_REVISION;
    OD_PERSIST_COMM.x1018_identity.serialNumber = CANOPEN_REFERENCE_SERIAL;
    OD_PERSIST_COMM.x1017_producerHeartbeatTime = CANOPEN_REFERENCE_HEARTBEAT_MS;
}

static void
CANopenReference_ForceSafeApplication(void) {
    Cia401Reference_ForceSafeOutputs();
    Cia402Reference_ForceDisable();
#if (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
    Cia418Reference_ForceSafe(&canopenReferenceCia418State);
#endif
}

static int
CANopenReference_FailRuntime(uint32_t fault_code) {
    canopenReferenceSafeFault = !canopenReferenceRecoveryAttemptActive;
    if (canopenReferenceSafeFault) {
        CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_SAFE_FAULT);
    } else {
        CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_REINITIALIZING);
    }
    CANopenReferenceDiagnostics_ReportRuntimeFault(fault_code);
    CANopenReference_ForceSafeApplication();
    if (canopenNodeSTM32 != NULL) {
        (void)HAL_TIM_Base_Stop_IT(canopenNodeSTM32->timerHandle);
    }
    if (CO != NULL && CO->CANmodule != NULL) {
        CO->CANmodule->CANnormal = false;
        CO_CANmodule_disable(CO->CANmodule);
    }
    return -(int)fault_code;
}

static void
CANopenReference_ProcessCanRecovery(uint32_t now) {
    int result;

    if (canopenReferenceCanRecoveryPending) {
        canopenReferenceCanRecoveryPending = false;
        CANopenReferenceCanRecovery_Request(&canopenReferenceCanRecovery, now);
    }
    if (!CANopenReferenceCanRecovery_Ready(&canopenReferenceCanRecovery, now)) {
        return;
    }
    canopenReferenceRecoveryAttemptActive = true;
    result = canopen_app_resetCommunication();
    canopenReferenceRecoveryAttemptActive = false;
    CANopenReferenceDiagnostics_ReportCanRecovery(result == 0);
    CANopenReferenceCanRecovery_Complete(&canopenReferenceCanRecovery, result == 0, now);
    if (result == 0) {
        return;
    }
    if (CANopenReferenceCanRecovery_State(&canopenReferenceCanRecovery)
        == CANOPEN_REFERENCE_CAN_FAULT) {
        (void)CANopenReference_FailRuntime(0xCA000001UL);
    }
}

int
canopen_app_init(CANopenNodeSTM32 *instance) {
    uint32_t heapMemoryUsed = 0U;

    if (instance == NULL || instance->CANHandle == NULL || instance->timerHandle == NULL
        || instance->HWInitFunction == NULL) {
        return -1;
    }

    canopenNodeSTM32 = instance;
    CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_INIT);
    canopenReferenceSafeFault = false;
    canopenReferenceCanRecoveryPending = false;
    canopenReferenceRecoveryAttemptActive = false;
    CANopenReferenceCanRecovery_Init(&canopenReferenceCanRecovery,
                                     CANOPEN_REFERENCE_CAN_RECOVERY_WAIT_MS,
                                     CANOPEN_REFERENCE_CAN_RECOVERY_MAX_ATTEMPTS);
    CANopenReference_ApplyIdentity();
    CANopenReference_ForceSafeApplication();

    CO = CO_new(NULL, &heapMemoryUsed);
    (void)heapMemoryUsed;
    if (CO == NULL) {
        CANopenReference_ForceSafeApplication();
        return -2;
    }
    canopenNodeSTM32->canOpenStack = CO;

    if (canopen_app_resetCommunication() != 0) {
        CANopenReference_FailRuntime(0xCA000002UL);
        CANopenReference_ForceSafeApplication();
        CO_delete(CO);
        CO = NULL;
        canopenNodeSTM32->canOpenStack = NULL;
        return -3;
    }
    return 0;
}

int
canopen_app_resetCommunication(void) {
    CO_ReturnError_t error;
    uint32_t errorInfo = 0U;
    CO_LSS_address_t lssAddress;

    if (CO == NULL || canopenNodeSTM32 == NULL) {
        CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_SAFE_FAULT);
        return -1;
    }

    CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_REINITIALIZING);
    CANopenReference_ForceSafeApplication();
    CANopenReferenceCia302_Deinit();
    CO->CANmodule->CANnormal = false;
    CO_CANsetConfigurationMode((void *)canopenNodeSTM32);
    CO_CANmodule_disable(CO->CANmodule);

    error = CO_CANinit(CO, canopenNodeSTM32, 0U);
    if (error != CO_ERROR_NO) {
        return CANopenReference_FailRuntime(0xCA000010UL);
    }
#if defined(CAN_IT_ERROR)
    if (HAL_CAN_ActivateNotification(canopenNodeSTM32->CANHandle,
                                     CAN_IT_ERROR | CAN_IT_ERROR_WARNING | CAN_IT_ERROR_PASSIVE | CAN_IT_BUSOFF
                                         | CAN_IT_LAST_ERROR_CODE)
        != HAL_OK) {
        return CANopenReference_FailRuntime(0xCA000011UL);
    }
#endif
    if (CANopenReferenceStorage_Init(CO) != CO_ERROR_NO) {
        return CANopenReference_FailRuntime(0xCA000012UL);
    }

    lssAddress.identity.vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID;
    lssAddress.identity.productCode = OD_PERSIST_COMM.x1018_identity.productCode;
    lssAddress.identity.revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber;
    lssAddress.identity.serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber;
    error = CO_LSSinit(CO, &lssAddress, &canopenNodeSTM32->desiredNodeID, &canopenNodeSTM32->baudrate);
    if (error != CO_ERROR_NO) {
        return CANopenReference_FailRuntime(0xCA000013UL);
    }
    CANopenReferenceLss_Init(CO, &canopenReferenceLssState);
    CANopenReferenceCia302_PrepareOd();

    canopenNodeSTM32->activeNodeID = canopenNodeSTM32->desiredNodeID;
    if (!CANopenReference_ConfigureCanFilter(canopenNodeSTM32->activeNodeID)) {
        return CANopenReference_FailRuntime(0xCA000014UL);
    }
    error = CO_CANopenInit(CO, NULL, NULL, OD, NULL, CANOPEN_REFERENCE_NMT_CONTROL,
                           CANOPEN_REFERENCE_FIRST_HB_MS, CANOPEN_REFERENCE_SDO_SRV_TIMEOUT_MS,
                           CANOPEN_REFERENCE_SDO_CLI_TIMEOUT_MS, true, canopenNodeSTM32->activeNodeID, &errorInfo);
    if (error != CO_ERROR_NO && error != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        (void)errorInfo;
        return CANopenReference_FailRuntime(0xCA000015UL);
    }

    CANopenReferenceGateway_Init(CO);
    CANopenReferenceCia302_Init(CO, canopenNodeSTM32->activeNodeID, HAL_GetTick());

    error = CO_CANopenInitPDO(CO, CO->em, OD, canopenNodeSTM32->activeNodeID, &errorInfo);
    if (error != CO_ERROR_NO && error != CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        (void)errorInfo;
        return CANopenReference_FailRuntime(0xCA000016UL);
    }
#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
    if (CANopenReference_UDS_Init(canopenNodeSTM32->CANHandle, HAL_GetTick()) != 0) {
        return CANopenReference_FailRuntime(0xCA000019UL);
    }
#endif

    Cia401Reference_Init();
    Cia402Reference_Init();
#if (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
    /* Initialize the selected live CANopenNode CiA 418 OD personality. */
    Cia418Reference_Init(&canopenReferenceCia418State);
#endif

    if (HAL_TIM_Base_Start_IT(canopenNodeSTM32->timerHandle) != HAL_OK) {
        CANopenReference_ForceSafeApplication();
        return CANopenReference_FailRuntime(0xCA000017UL);
    }

    CO_CANsetNormalMode(CO->CANmodule);
    canopenReferenceLastTick = HAL_GetTick();
    canopenReferenceSafeFault = false;
    CANopenReferenceCanRecovery_Complete(&canopenReferenceCanRecovery, true, canopenReferenceLastTick);
    CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_RUNNING);
    return 0;
}

void
canopen_app_process(void) {
    uint32_t timing_start = CANopenReferenceTiming_MainlineEnter();
    uint32_t now;
    uint32_t elapsedUs;
    CO_NMT_reset_cmd_t resetCommand;

    if (CO == NULL || canopenNodeSTM32 == NULL || canopenReferenceSafeFault) {
        CANopenReferenceTiming_MainlineExit(timing_start);
        return;
    }

    now = HAL_GetTick();
    CANopenReference_ProcessCanRecovery(now);
    if (now == canopenReferenceLastTick) {
        CANopenReferenceTiming_MainlineExit(timing_start);
        return;
    }
    elapsedUs = (now - canopenReferenceLastTick) * 1000U;
    canopenReferenceLastTick = now;

    CANopenReferenceCia302_PreProcess(now);
    resetCommand = CO_process(CO, CANopenReferenceGateway_Authorized(), elapsedUs, NULL);
    CANopenReferenceCia302_Process(now);
#if (CANOPEN_REFERENCE_ENABLE_UDS != 0U)
    CANopenReference_UDS_Process(now);
    if (CANopenReference_UDS_ResetPending()) {
        CANopenReference_UDS_ClearReset();
        CANopenReference_ForceSafeApplication();
        HAL_NVIC_SystemReset();
    }
#endif
    canopenNodeSTM32->outStatusLEDRed = CO_LED_RED(CO->LEDs, CO_LED_CANopen);
    canopenNodeSTM32->outStatusLEDGreen = CO_LED_GREEN(CO->LEDs, CO_LED_CANopen);
    CANopenReferenceDiagnostics_Process(canopenNodeSTM32->activeNodeID, CO->CANmodule->CANerrorStatus,
                                        canopenNodeSTM32->outStatusLEDGreen, canopenNodeSTM32->outStatusLEDRed, now);

    if (resetCommand == CO_RESET_COMM) {
        CANopenReference_SetRuntimeState(CANOPEN_REFERENCE_RUNTIME_RESET_REQUESTED);
        (void)HAL_TIM_Base_Stop_IT(canopenNodeSTM32->timerHandle);
        CO_CANsetConfigurationMode((void *)canopenNodeSTM32);
        CO_delete(CO);
        CO = NULL;
        canopenNodeSTM32->canOpenStack = NULL;
        if (canopen_app_init(canopenNodeSTM32) != 0) {
            (void)CANopenReference_FailRuntime(0xCA000018UL);
        }
    } else if (resetCommand == CO_RESET_APP) {
        CANopenReference_ForceSafeApplication();
        HAL_NVIC_SystemReset();
    }
    CANopenReferenceTiming_MainlineExit(timing_start);
}

void
canopen_app_interrupt(void) {
    bool_t syncWas = false;
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t app_interrupt_start = CANopenReferenceTiming_PhaseEnter();
#endif

    if (CO == NULL || canopenNodeSTM32 == NULL || CO->nodeIdUnconfigured || !CO->CANmodule->CANnormal) {
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
        CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.app_interrupt_cycles_max,
                                         app_interrupt_start);
#endif
        return;
    }

    CO_LOCK_OD(CO->CANmodule);
#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t sync_start = CANopenReferenceTiming_PhaseEnter();
#endif
    syncWas = CO_process_SYNC(CO, 1000U, NULL);
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.sync_cycles_max, sync_start);
#endif
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t rpdo_start = CANopenReferenceTiming_PhaseEnter();
#endif
    CO_process_RPDO(CO, syncWas, 1000U, NULL);
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.rpdo_cycles_max, rpdo_start);
#endif
#endif

    /* Bounded application work occurs after commands enter the OD and before
     * status/inputs are packed into TPDOs. No blocking driver, flash, or
     * printf operation is permitted in this interrupt context. */
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t cia401_start = CANopenReferenceTiming_PhaseEnter();
#endif
    Cia401Reference_Process1ms();
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.cia401_cycles_max, cia401_start);
    uint32_t cia402_start = CANopenReferenceTiming_PhaseEnter();
#endif
    Cia402Reference_Process1ms();
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.cia402_cycles_max, cia402_start);
#endif
#if (CANOPEN_REFERENCE_ENABLE_CIA418 != 0U)
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t cia418_start = CANopenReferenceTiming_PhaseEnter();
#endif
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.cia418_cycles_max, cia418_start);
#endif
#endif

#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    uint32_t tpdo_start = CANopenReferenceTiming_PhaseEnter();
#endif
    CO_process_TPDO(CO, syncWas, 1000U, NULL);
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.tpdo_cycles_max, tpdo_start);
#endif
#endif
    CO_UNLOCK_OD(CO->CANmodule);
#if (CANOPEN_REFERENCE_ENABLE_TIMING_INSTRUMENTATION != 0U)
    CANopenReferenceTiming_PhaseExit(&canopenReferenceTimingStats.app_interrupt_cycles_max,
                                     app_interrupt_start);
#endif
}


void
HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    uint32_t hal_error;

    if (hcan == NULL || canopenNodeSTM32 == NULL || hcan != canopenNodeSTM32->CANHandle) {
        return;
    }

    hal_error = HAL_CAN_GetError(hcan);
    CANopenReferenceDiagnostics_ReportCanHardwareError(hal_error);
    if (CO != NULL && CO->CANmodule != NULL) {
        if ((hal_error & HAL_CAN_ERROR_BOF) != 0U) {
            CO->CANmodule->CANerrorStatus |= CO_CAN_ERRTX_BUS_OFF;
        }
        if ((hal_error & (HAL_CAN_ERROR_ACK | HAL_CAN_ERROR_TIMEOUT)) != 0U) {
            CO->CANmodule->CANerrorStatus |= CO_CAN_ERRTX_WARNING;
        }
        if ((hal_error & (HAL_CAN_ERROR_RX_FOV0 | HAL_CAN_ERROR_RX_FOV1)) != 0U) {
            CO->CANmodule->CANerrorStatus |= CO_CAN_ERRRX_OVERFLOW;
        }
        if ((hal_error & (HAL_CAN_ERROR_STF | HAL_CAN_ERROR_FOR | HAL_CAN_ERROR_BR)) != 0U) {
            CO->CANmodule->CANerrorStatus |= CO_CAN_ERRRX_WARNING;
        }
        if ((hal_error & HAL_CAN_ERROR_BOF) != 0U) {
            canopenReferenceCanRecoveryPending = true;
        }
    }
}
