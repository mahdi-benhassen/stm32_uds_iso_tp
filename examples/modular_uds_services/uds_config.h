/**
 * @file uds_config.h
 * @brief UDS Stack Configuration Limits
 */

#ifndef UDS_CONFIG_H
#define UDS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifndef UDS_MAX_SERVICES
#define UDS_MAX_SERVICES 32U
#endif

typedef struct uds_app_config_s uds_app_config_t;

#endif /* UDS_CONFIG_H */
