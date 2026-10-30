/**
 * @file flash_layout.h
 * @brief STM32F103 128KB Flash Memory Layout for Single Bootloader & Single App (Issue #140)
 */

#ifndef STM32F103_FLASH_LAYOUT_H
#define STM32F103_FLASH_LAYOUT_H

#include <stdint.h>

/* STM32F103 Medium/High Density Flash Parameters */
#define FLASH_BASE_ADDR            0x08000000U
#define FLASH_TOTAL_SIZE           (128U * 1024U)   /* 128 KB */
#define FLASH_PAGE_SIZE            2048U            /* 2 KB per page */
#define FLASH_TOTAL_PAGES          64U

/* 1. Bootloader Partition: 48 KB (Pages 0 - 23) */
#define BOOTLOADER_START_ADDR      0x08000000U
#define BOOTLOADER_SIZE            (48U * 1024U)
#define BOOTLOADER_PAGE_COUNT      24U

/* 2. Slot A Application Partition: 60 KB (Pages 24 - 53) */
#define APP_SLOT_A_START_ADDR      0x0800C000U
#define APP_SLOT_A_SIZE            (60U * 1024U)
#define APP_SLOT_A_PAGE_COUNT      30U

/* 3. Metadata A Partition: 2 KB (Page 54) */
#define METADATA_A_START_ADDR      0x0801B000U
#define METADATA_A_SIZE            (2U * 1024U)
#define METADATA_A_PAGE_COUNT      1U

/* 4. DTC Data Partition: 18 KB (Pages 55 - 63) */
#define DTC_DATA_START_ADDR        0x0801B800U
#define DTC_DATA_SIZE              (18U * 1024U)
#define DTC_DATA_PAGE_COUNT        9U

/* Verification Magic Constant */
#define APP_METADATA_MAGIC         0x55445331U      /* 'UDS1' */
#define BOOTLOADER_WINDOW_MS       2000U            /* 2.0-second boot window */

#endif /* STM32F103_FLASH_LAYOUT_H */
