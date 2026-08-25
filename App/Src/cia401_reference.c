/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "cia401_reference.h"

#include "canopen_reference_co.h"
#include "canopen_reference_od.h"
#include "canopen_reference_config.h"
#include "canopen_reference_hw.h"

void
Cia401Reference_Init(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA401 != 0U)
    if (CANOPEN_REFERENCE_OUTPUTS_DEFAULT_SAFE != 0U) {
        OD_APP.x6200_writeDigitalOutputs = 0U;
        OD_APP.x6422_writeAnalogOutput1 = 0;
        CANopenReferenceHw_WriteDigitalOutputs(0U);
        CANopenReferenceHw_WriteAnalogOutput(1U, 0);
    }
#else
    /* The selected personality owns a different OD; there are no CiA 401
     * application objects to initialize in this build. */
    (void)0;
#endif
}

void
Cia401Reference_ForceSafeOutputs(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA401 != 0U)
    OD_APP.x6200_writeDigitalOutputs = 0U;
    OD_APP.x6422_writeAnalogOutput1 = 0;
    CANopenReferenceHw_WriteDigitalOutputs(0U);
    CANopenReferenceHw_WriteAnalogOutput(1U, 0);
#else
    (void)0;
#endif
}

void
Cia401Reference_Process1ms(void) {
#if (CANOPEN_REFERENCE_ENABLE_CIA401 != 0U)
    /* Copy physical inputs into PDO/SDO visible objects before TPDO processing. */
    OD_APP.x6000_readDigitalInputs = CANopenReferenceHw_ReadDigitalInputs();
    OD_APP.x6401_readAnalogInput1 = CANopenReferenceHw_ReadAnalogInput(1U);
    OD_APP.x6411_readAnalogInput2 = CANopenReferenceHw_ReadAnalogInput(2U);

    /* RPDO and SDO writes are already reflected in OD_APP at this point. */
    CANopenReferenceHw_WriteDigitalOutputs(OD_APP.x6200_writeDigitalOutputs);
    CANopenReferenceHw_WriteAnalogOutput(1U, OD_APP.x6422_writeAnalogOutput1);
#else
    Cia401Reference_ForceSafeOutputs();
#endif
}
