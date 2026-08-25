/* Test-only substitute for Generated/OD.h. */
#ifndef TEST_FAKE_OD_H
#define TEST_FAKE_OD_H

#include <stdint.h>

typedef struct {
    uint8_t x6000_readDigitalInputs;
    uint8_t x6200_writeDigitalOutputs;
    int16_t x6401_readAnalogInput1;
    int16_t x6411_readAnalogInput2;
    int16_t x6422_writeAnalogOutput1;
    uint16_t x603F_errorCode;
    uint16_t x6040_controlword;
    uint16_t x6041_statusword;
    int8_t x6060_modesOfOperation;
    int8_t x6061_modesOfOperationDisplay;
    int32_t x6064_positionActualValue;
    int32_t x607A_targetPosition;
    int16_t x6071_targetTorque;
    int16_t x6077_torqueActualValue;
    int32_t x606C_velocityActualValue;
    int32_t x60FF_targetVelocity;
} OD_APP_t;

extern OD_APP_t OD_APP;

#endif /* TEST_FAKE_OD_H */
