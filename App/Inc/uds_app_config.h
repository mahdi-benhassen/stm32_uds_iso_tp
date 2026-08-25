#ifndef STM32_UDS_ISO_TP_UDS_APP_CONFIG_H
#define STM32_UDS_ISO_TP_UDS_APP_CONFIG_H

/* Application policy: the STM32F767 bxCAN UDS profile uses padded Classic CAN frames. */
#define UDS_APP_CLASSIC_PADDING_ENABLED 1U
#define UDS_APP_CLASSIC_PADDING_VALUE 0xCCU

#endif
