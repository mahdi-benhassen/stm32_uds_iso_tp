#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "OD.h"
#include "canopen_reference_hw.h"
#include "cia401_reference.h"
#include "cia402_reference.h"

OD_APP_t OD_APP;

static uint8_t digitalInput;
static uint8_t digitalOutput;
static int16_t analogInput1;
static int16_t analogInput2;
static int16_t analogOutput1;
static bool interlocksHealthy;
static bool driveEnabled;
static bool hardwareFault;
static int32_t actualPosition;
static int32_t actualVelocity;
static int16_t actualTorque;
static uint16_t hardwareError;
static int8_t commandedMode;
static int32_t commandedPosition;
static int32_t commandedVelocity;
static int16_t commandedTorque;

uint8_t
CANopenReferenceHw_ReadDigitalInputs(void) {
    return digitalInput;
}

void
CANopenReferenceHw_WriteDigitalOutputs(uint8_t value) {
    digitalOutput = value;
}

int16_t
CANopenReferenceHw_ReadAnalogInput(uint8_t channel) {
    return (channel == 1U) ? analogInput1 : analogInput2;
}

void
CANopenReferenceHw_WriteAnalogOutput(uint8_t channel, int16_t value) {
    if (channel == 1U) {
        analogOutput1 = value;
    }
}

bool
CANopenReferenceHw_DriveInterlocksHealthy(void) {
    return interlocksHealthy;
}

void
CANopenReferenceHw_DriveSetEnable(bool enable) {
    driveEnabled = enable;
}

void
CANopenReferenceHw_DriveCommand(int8_t mode, int32_t position, int32_t velocity, int16_t torque) {
    commandedMode = mode;
    commandedPosition = position;
    commandedVelocity = velocity;
    commandedTorque = torque;
}

void
CANopenReferenceHw_DriveReadFeedback(int32_t *position, int32_t *velocity, int16_t *torque,
                                     uint16_t *errorCode, bool *faultActive) {
    *position = actualPosition;
    *velocity = actualVelocity;
    *torque = actualTorque;
    *errorCode = hardwareError;
    *faultActive = hardwareFault;
}

static void
resetFixture(void) {
    memset(&OD_APP, 0, sizeof(OD_APP));
    digitalInput = 0U;
    digitalOutput = 0U;
    analogInput1 = 0;
    analogInput2 = 0;
    analogOutput1 = 0;
    interlocksHealthy = false;
    driveEnabled = false;
    hardwareFault = false;
    actualPosition = 0;
    actualVelocity = 0;
    actualTorque = 0;
    hardwareError = 0U;
    commandedMode = 0;
    commandedPosition = 0;
    commandedVelocity = 0;
    commandedTorque = 0;
}

static void
testCia401Bridge(void) {
    resetFixture();
    /* Initialization must remove stale commanded outputs before the first PDO cycle. */
    digitalOutput = UINT8_C(0xFF);
    analogOutput1 = 32767;
    OD_APP.x6200_writeDigitalOutputs = UINT8_C(0xFF);
    OD_APP.x6422_writeAnalogOutput1 = 32767;
    Cia401Reference_Init();
    assert(OD_APP.x6200_writeDigitalOutputs == 0U);
    assert(OD_APP.x6422_writeAnalogOutput1 == 0);
    assert(digitalOutput == 0U);
    assert(analogOutput1 == 0);

    digitalInput = UINT8_C(0xA5);
    analogInput1 = 1234;
    analogInput2 = -2345;
    OD_APP.x6200_writeDigitalOutputs = UINT8_C(0x5A);
    OD_APP.x6422_writeAnalogOutput1 = 234;
    Cia401Reference_Process1ms();

    assert(OD_APP.x6000_readDigitalInputs == UINT8_C(0xA5));
    assert(OD_APP.x6401_readAnalogInput1 == 1234);
    assert(OD_APP.x6411_readAnalogInput2 == -2345);
    assert(digitalOutput == UINT8_C(0x5A));
    assert(analogOutput1 == 234);

    Cia401Reference_ForceSafeOutputs();
    assert(digitalOutput == 0U);
    assert(analogOutput1 == 0);
}

static void
testCia402StateAndInterlocks(void) {
    resetFixture();
    Cia402Reference_Init();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0040));
    assert(!driveEnabled);

    /* Unhealthy interlocks always produce a fault state and remove enable. */
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0008));
    assert(!driveEnabled);

    interlocksHealthy = true;
    OD_APP.x6040_controlword = UINT16_C(0x0080); /* fault reset */
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0040));

    OD_APP.x6040_controlword = UINT16_C(0x0006);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0021));

    OD_APP.x6040_controlword = UINT16_C(0x0007);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0023));

    OD_APP.x6060_modesOfOperation = 3;
    OD_APP.x607A_targetPosition = 100000;
    OD_APP.x60FF_targetVelocity = 5000;
    OD_APP.x6071_targetTorque = 123;
    actualPosition = 987;
    actualVelocity = 654;
    actualTorque = 32;
    OD_APP.x6040_controlword = UINT16_C(0x000F);
    Cia402Reference_Process1ms();

    assert(OD_APP.x6041_statusword == UINT16_C(0x0027));
    assert(OD_APP.x6061_modesOfOperationDisplay == 3);
    assert(driveEnabled);
    assert(commandedMode == 3);
    assert(commandedPosition == 100000);
    assert(commandedVelocity == 5000);
    assert(commandedTorque == 123);
    assert(OD_APP.x6064_positionActualValue == 987);
    assert(OD_APP.x606C_velocityActualValue == 654);
    assert(OD_APP.x6077_torqueActualValue == 32);

    interlocksHealthy = false;
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0008));
    assert(!driveEnabled);
}
static void
testCia402QuickStopAndUnsupportedMode(void) {
    resetFixture();
    Cia402Reference_Init();
    interlocksHealthy = true;
    OD_APP.x6040_controlword = UINT16_C(0x0006);
    Cia402Reference_Process1ms();
    OD_APP.x6040_controlword = UINT16_C(0x0007);
    Cia402Reference_Process1ms();
    OD_APP.x6060_modesOfOperation = 3;
    OD_APP.x6040_controlword = UINT16_C(0x000F);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0027));
    assert(driveEnabled);
    /* Clearing quick-stop must immediately remove the drive-enable request. */
    OD_APP.x6040_controlword = UINT16_C(0x000B);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0007));
    assert(!driveEnabled);
    OD_APP.x6040_controlword = UINT16_C(0x000F);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0027));
    /* Unsupported operation modes fail closed with the reference diagnostic. */
    OD_APP.x6060_modesOfOperation = 2;
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0040));
    assert(OD_APP.x6061_modesOfOperationDisplay == 0);
    assert(OD_APP.x603F_errorCode == UINT16_C(0xFF01));
    assert(!driveEnabled);
}
static void
testCia402DisableVoltageTakesPriority(void) {
    resetFixture();
    Cia402Reference_Init();
    interlocksHealthy = true;
    OD_APP.x6040_controlword = UINT16_C(0x0006);
    Cia402Reference_Process1ms();
    OD_APP.x6040_controlword = UINT16_C(0x0007);
    Cia402Reference_Process1ms();
    OD_APP.x6060_modesOfOperation = 3;
    OD_APP.x6040_controlword = UINT16_C(0x000F);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0027));
    assert(driveEnabled);
    /* A simultaneous quick-stop request cannot mask removal of enable voltage. */
    OD_APP.x6040_controlword = UINT16_C(0x0009);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0040));
    assert(!driveEnabled);
}
static void
testCia402HardwareFaultRecovery(void) {
    resetFixture();
    Cia402Reference_Init();
    interlocksHealthy = true;
    hardwareFault = true;
    hardwareError = UINT16_C(0x2310);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0008));
    assert(OD_APP.x603F_errorCode == UINT16_C(0x2310));
    assert(!driveEnabled);
    /* Reset may only be accepted after the hardware fault is no longer active. */
    OD_APP.x6040_controlword = UINT16_C(0x0080);
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0008));
    hardwareFault = false;
    hardwareError = 0U;
    Cia402Reference_Process1ms();
    assert(OD_APP.x6041_statusword == UINT16_C(0x0040));
    assert(OD_APP.x603F_errorCode == 0U);
    assert(!driveEnabled);
}
int
main(void) {
    testCia401Bridge();
    testCia402StateAndInterlocks();
    testCia402QuickStopAndUnsupportedMode();
    testCia402DisableVoltageTakesPriority();
    testCia402HardwareFaultRecovery();

    puts("CiA 401 and CiA 402 reference profile tests passed.");
    return 0;
}
