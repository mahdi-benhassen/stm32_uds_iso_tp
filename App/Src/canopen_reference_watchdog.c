/* SPDX-License-Identifier: LicenseRef-STM32-CANopen-Research-Education-Commercial-1.0 */
#include "canopen_reference_watchdog.h"
#include "canopen_reference_config.h"
#include "main.h"

#if CANOPEN_REFERENCE_ENABLE_IWDG
#include "stm32f7xx_hal_iwdg.h"
static IWDG_HandleTypeDef s_iwdg;
#endif

static volatile uint32_t s_timer_ticks;
static uint32_t s_last_mainline_tick;
static uint32_t s_mainline_ticks;
static uint32_t s_start_tick;
static uint32_t s_reset_flags;
#if defined(STM32F767xx)
#define WATCHDOG_FAULT_MAGIC 0xCA11FA11UL
static uint32_t s_previous_fault;
#endif

#if defined(STM32F767xx)
static void
watchdog_backup_access_enable(void) {
    __HAL_RCC_PWR_CLK_ENABLE();
    SET_BIT(RCC->APB1ENR, RCC_APB1ENR_RTCEN);
    SET_BIT(PWR->CR1, PWR_CR1_DBP);
}
#endif

void
CANopenReferenceWatchdog_Init(void) {
#if defined(STM32F767xx)
    s_reset_flags = RCC->CSR;
    watchdog_backup_access_enable();
    if (RTC->BKP0R == WATCHDOG_FAULT_MAGIC) {
        s_previous_fault = RTC->BKP1R;
        RTC->BKP0R = 0U;
        RTC->BKP1R = 0U;
    }
    __HAL_RCC_CLEAR_RESET_FLAGS();
#else
    s_reset_flags = 0U;
#endif
#if CANOPEN_REFERENCE_ENABLE_IWDG
    const uint32_t nominal_lsi_hz = 32000U;
    uint32_t reload = (nominal_lsi_hz * CANOPEN_REFERENCE_IWDG_TIMEOUT_MS) / (64U * 1000U);
    uint32_t lsi_start_tick;

    if (reload < 1U) {
        reload = 1U;
    }
    if (reload > 0x0FFFU) {
        reload = 0x0FFFU;
    }
    __HAL_RCC_LSI_ENABLE();
    lsi_start_tick = HAL_GetTick();
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET) {
        if ((uint32_t)(HAL_GetTick() - lsi_start_tick) >= CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS) {
            Error_Handler();
            return;
        }
    }
    s_iwdg.Instance = IWDG;
    s_iwdg.Init.Prescaler = IWDG_PRESCALER_64;
    s_iwdg.Init.Reload = reload;
    s_iwdg.Init.Window = IWDG_WINDOW_DISABLE;
    if (HAL_IWDG_Init(&s_iwdg) != HAL_OK) {
        Error_Handler();
        return;
    }
#endif
    s_timer_ticks = 0U;
    s_last_mainline_tick = 0U;
    s_mainline_ticks = 0U;
    s_start_tick = HAL_GetTick();
}

void
CANopenReferenceWatchdog_TickISR(void) {
    (void)__atomic_fetch_add(&s_timer_ticks, 1U, __ATOMIC_RELAXED);
}

void
CANopenReferenceWatchdog_Process(void) {
    uint32_t timer_ticks = __atomic_load_n(&s_timer_ticks, __ATOMIC_ACQUIRE);

    if ((uint32_t)(HAL_GetTick() - s_start_tick) < CANOPEN_REFERENCE_IWDG_STARTUP_GRACE_MS) {
#if CANOPEN_REFERENCE_ENABLE_IWDG
        (void)HAL_IWDG_Refresh(&s_iwdg);
#endif
        return;
    }
    if (timer_ticks == s_last_mainline_tick) {
        return;
    }
    s_last_mainline_tick = timer_ticks;
    s_mainline_ticks++;
#if CANOPEN_REFERENCE_ENABLE_IWDG
    if (HAL_IWDG_Refresh(&s_iwdg) != HAL_OK) {
        Error_Handler();
    }
#endif
}

uint32_t
CANopenReferenceWatchdog_MainlineTicks(void) {
    return s_mainline_ticks;
}

uint32_t
CANopenReferenceWatchdog_ResetFlags(void) {
    return s_reset_flags;
}

void
CANopenReferenceWatchdog_RecordFatalFault(uint32_t code) {
#if defined(STM32F767xx)
    watchdog_backup_access_enable();
    RTC->BKP0R = WATCHDOG_FAULT_MAGIC;
    RTC->BKP1R = code;
#else
    (void)code;
#endif
}

uint32_t
CANopenReferenceWatchdog_PreviousFault(void) {
#if defined(STM32F767xx)
    return s_previous_fault;
#else
    return 0U;
#endif
}
