/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "cia402_reference.h"

#include <stdbool.h>

#include "canopen_reference_co.h"
#include "canopen_reference_od.h"
#include "canopen_reference_config.h"
#include "canopen_reference_hw.h"

/* CiA 402 state encodings, interpreted with the standard state mask. */
#define CIA402_STATUS_STATE_MASK          UINT16_C(0x006F)
#define CIA402_STATUS_SWITCH_ON_DISABLED  UINT16_C(0x0040)
#define CIA402_STATUS_READY_TO_SWITCH_ON  UINT16_C(0x0021)
#define CIA402_STATUS_SWITCHED_ON         UINT16_C(0x0023)
#define CIA402_STATUS_OPERATION_ENABLED   UINT16_C(0x0027)
#define CIA402_STATUS_QUICK_STOP_ACTIVE   UINT16_C(0x0007)
#define CIA402_STATUS_FAULT_REACTION      UINT16_C(0x000F)
#define CIA402_STATUS_FAULT               UINT16_C(0x0008)

#define CIA402_CW_SWITCH_ON               UINT16_C(0x0001)
#define CIA402_CW_ENABLE_VOLTAGE          UINT16_C(0x0002)
#define CIA402_CW_QUICK_STOP              UINT16_C(0x0004)
#define CIA402_CW_ENABLE_OPERATION        UINT16_C(0x0008)
#define CIA402_CW_FAULT_RESET              UINT16_C(0x0080)
#define CIA402_CW_LOW_NIBBLE              UINT16_C(0x000F)

/* This reference supports common operation-mode values but does not implement
 * a trajectory generator, homing algorithm, torque loop, or power-stage safety
 * function. The hardware adapter owns those mechanisms. */
#if (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U)
static bool
Cia402Reference_ModeIsSupported(int8_t mode) {
    switch (mode) {
        case -3: /* Velocity mode. */
        case -1: /* Position mode. */
        case 1:  /* Profile position mode. */
        case 3:  /* Profile velocity mode. */
        case 4:  /* Profile torque mode. */
        case 6:  /* Homing mode. */
        case 8:  /* Cyclic synchronous position. */
        case 9:  /* Cyclic synchronous velocity. */
        case 10: /* Cyclic synchronous torque. */
            return true;
        default:
            return false;
    }
}
#endif

void
Cia402Reference_ForceDisable(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U)
    CANopenReferenceHw_DriveSetEnable(false);
    OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
    OD_APP.x6061_modesOfOperationDisplay = 0;
#else
    (void)0;
#endif
}

void
Cia402Reference_Init(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U)
    OD_APP.x603F_errorCode = 0U;
    OD_APP.x6040_controlword = 0U;
    OD_APP.x6060_modesOfOperation = 0;
    Cia402Reference_ForceDisable();
#else
    (void)0;
#endif
}

void
Cia402Reference_Process1ms(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA402 != 0U)
    int32_t position;
    int32_t velocity;
    int16_t torque;
    uint16_t errorCode;
    bool hardwareFault;
    uint16_t controlword = OD_APP.x6040_controlword;
    uint16_t statusword;

    CANopenReferenceHw_DriveReadFeedback(&position, &velocity, &torque, &errorCode, &hardwareFault);
    OD_APP.x6064_positionActualValue = position;
    OD_APP.x606C_velocityActualValue = velocity;
    OD_APP.x6077_torqueActualValue = torque;
    OD_APP.x603F_errorCode = errorCode;

    /* A hardware fault or unmet interlock always removes the software request
     * for drive enable. A real design must also implement a hardware-safe path
     * that does not rely on the MCU, CAN network, or this function. */
    if (hardwareFault || !CANopenReferenceHw_DriveInterlocksHealthy()) {
        CANopenReferenceHw_DriveSetEnable(false);
        OD_APP.x6041_statusword = CIA402_STATUS_FAULT;
        OD_APP.x6061_modesOfOperationDisplay = 0;
        return;
    }

    statusword = OD_APP.x6041_statusword & CIA402_STATUS_STATE_MASK;
    if (statusword == CIA402_STATUS_FAULT || statusword == CIA402_STATUS_FAULT_REACTION) {
        CANopenReferenceHw_DriveSetEnable(false);
        if ((controlword & CIA402_CW_FAULT_RESET) != 0U) {
            OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
            OD_APP.x603F_errorCode = 0U;
        } else {
            OD_APP.x6041_statusword = CIA402_STATUS_FAULT;
        }
        OD_APP.x6061_modesOfOperationDisplay = 0;
        return;
    }

    switch (statusword) {
        case CIA402_STATUS_SWITCH_ON_DISABLED:
            if ((controlword & (CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP))
                == (CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP)) {
                OD_APP.x6041_statusword = CIA402_STATUS_READY_TO_SWITCH_ON;
            }
            break;

        case CIA402_STATUS_READY_TO_SWITCH_ON:
            if ((controlword & (CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP))
                != (CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP)) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
            } else if ((controlword & (CIA402_CW_SWITCH_ON | CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP))
                       == (CIA402_CW_SWITCH_ON | CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP)) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCHED_ON;
            }
            break;

        case CIA402_STATUS_SWITCHED_ON:
            if ((controlword & CIA402_CW_ENABLE_VOLTAGE) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
            } else if ((controlword & CIA402_CW_QUICK_STOP) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_QUICK_STOP_ACTIVE;
            } else if ((controlword & CIA402_CW_SWITCH_ON) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_READY_TO_SWITCH_ON;
            } else if ((controlword & CIA402_CW_LOW_NIBBLE)
                       == (CIA402_CW_SWITCH_ON | CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP
                           | CIA402_CW_ENABLE_OPERATION)) {
                OD_APP.x6041_statusword = CIA402_STATUS_OPERATION_ENABLED;
            }
            break;

        case CIA402_STATUS_OPERATION_ENABLED:
            if ((controlword & CIA402_CW_ENABLE_VOLTAGE) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
            } else if ((controlword & CIA402_CW_QUICK_STOP) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_QUICK_STOP_ACTIVE;
            } else if ((controlword & CIA402_CW_ENABLE_OPERATION) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCHED_ON;
            }
            break;

        case CIA402_STATUS_QUICK_STOP_ACTIVE:
            if ((controlword & CIA402_CW_ENABLE_VOLTAGE) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCH_ON_DISABLED;
            } else if ((controlword & CIA402_CW_QUICK_STOP) == 0U) {
                OD_APP.x6041_statusword = CIA402_STATUS_QUICK_STOP_ACTIVE;
            } else if ((controlword & CIA402_CW_LOW_NIBBLE)
                       == (CIA402_CW_SWITCH_ON | CIA402_CW_ENABLE_VOLTAGE | CIA402_CW_QUICK_STOP
                           | CIA402_CW_ENABLE_OPERATION)) {
                OD_APP.x6041_statusword = CIA402_STATUS_OPERATION_ENABLED;
            } else {
                OD_APP.x6041_statusword = CIA402_STATUS_SWITCHED_ON;
            }
            break;

        default:
            Cia402Reference_ForceDisable();
            return;
    }

    if ((OD_APP.x6041_statusword & CIA402_STATUS_STATE_MASK) == CIA402_STATUS_OPERATION_ENABLED) {
        if (Cia402Reference_ModeIsSupported(OD_APP.x6060_modesOfOperation)) {
            OD_APP.x6061_modesOfOperationDisplay = OD_APP.x6060_modesOfOperation;
            CANopenReferenceHw_DriveSetEnable(true);
            CANopenReferenceHw_DriveCommand(OD_APP.x6061_modesOfOperationDisplay, OD_APP.x607A_targetPosition,
                                            OD_APP.x60FF_targetVelocity, OD_APP.x6071_targetTorque);
        } else {
            /* Vendor-specific diagnostic code used only by this reference. */
            OD_APP.x603F_errorCode = UINT16_C(0xFF01);
            Cia402Reference_ForceDisable();
        }
    } else {
        OD_APP.x6061_modesOfOperationDisplay = 0;
        CANopenReferenceHw_DriveSetEnable(false);
    }
#else
    Cia402Reference_ForceDisable();
#endif
}
