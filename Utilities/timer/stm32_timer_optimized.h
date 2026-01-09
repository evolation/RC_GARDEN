/*!
 * \file      stm32_timer_optimized.h
 *
 * \brief     Optimized Hardware Timer Implementation using STM32WLxx HAL
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \author    Optimized for STM32WL55xx with multi-timer support
 */

#ifndef UTIL_TIMER_OPTIMIZED_H__
#define UTIL_TIMER_OPTIMIZED_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_tim.h"
#include "stm32wlxx_ll_lptim.h"
#include "stm32wlxx_hal_tim.h"
#include "utilities_conf.h"

/** @defgroup TIMER_OPTIMIZED Optimized Hardware Timer Server
  * @{
  */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Hardware timer types available on STM32WL55xx
 */
typedef enum {
  HW_TIMER_LPTIM1 = 0,      /*!< Low-Power Timer 1 - Best for low power sleep */
  HW_TIMER_LPTIM2 = 1,      /*!< Low-Power Timer 2 - Alternative LP timer */
  HW_TIMER_TIM1   = 2,      /*!< General Purpose Timer 1 - High resolution */
  HW_TIMER_TIM2   = 3,      /*!< General Purpose Timer 2 - 32-bit counter */
  HW_TIMER_TIM16  = 4,      /*!< General Purpose Timer 16 - Alternative */
  HW_TIMER_RTC    = 5       /*!< Real-Time Clock - Calendar + Alarm */
} HW_TIMER_Type_t;

/**
 * @brief Optimized Timer object description
 */
typedef struct HW_TimerEvent_s
{
    uint32_t Timestamp;           /*!< Expiring timer value in ticks */
    uint32_t ReloadValue;         /*!< Reload Value when Timer is restarted */
    uint8_t IsPending;            /*!< Is the timer waiting for an event */
    uint8_t IsRunning;            /*!< Is the timer running */
    uint8_t IsReloadStopped;      /*!< Is the reload stopped */
    UTIL_TIMER_Mode_t Mode;       /*!< Timer type: one-shot/continuous */
    HW_TIMER_Type_t TimerType;    /*!< Hardware timer type */
    void (*Callback)(void *);     /*!< Callback function */
    void *argument;               /*!< Callback argument */
    struct HW_TimerEvent_s *Next; /*!< Pointer to next Timer object */
} HW_TIMER_Object_t;

/**
 * @brief Hardware timer configuration
 */
typedef struct {
    HW_TIMER_Type_t TimerType;
    uint32_t Frequency;           /*!< Timer frequency in Hz */
    uint32_t ClockDivision;       /*!< Clock division ratio */
    uint32_t CounterMode;         /*!< Counter mode (up/down/center) */
    uint32_t EnableIRQ;           /*!< Enable interrupt */
    uint32_t Prescaler;           /*!< Prescaler value */
} HW_TIMER_Config_t;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize optimized timer system
 * @return UTIL_TIMER_Status_t initialization status
 */
UTIL_TIMER_Status_t HW_TIMER_Init(void);

/**
 * @brief Deinitialize optimized timer system
 * @return UTIL_TIMER_Status_t deinitialization status
 */
UTIL_TIMER_Status_t HW_TIMER_DeInit(void);

/**
 * @brief Create and configure a hardware timer
 * @param TimerObject Timer object to configure
 * @param PeriodValue Period in milliseconds
 * @param Mode Timer mode (one-shot or periodic)
 * @param TimerType Hardware timer type to use
 * @param Callback Callback function on timer expiry
 * @param Argument Callback argument
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_Create(HW_TIMER_Object_t *TimerObject,
                                    uint32_t PeriodValue,
                                    UTIL_TIMER_Mode_t Mode,
                                    HW_TIMER_Type_t TimerType,
                                    void (*Callback)(void *),
                                    void *Argument);

/**
 * @brief Start hardware timer
 * @param TimerObject Timer object to start
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_Start(HW_TIMER_Object_t *TimerObject);

/**
 * @brief Stop hardware timer
 * @param TimerObject Timer object to stop
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_Stop(HW_TIMER_Object_t *TimerObject);

/**
 * @brief Set timer period
 * @param TimerObject Timer object
 * @param NewPeriodValue New period in milliseconds
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_SetPeriod(HW_TIMER_Object_t *TimerObject,
                                       uint32_t NewPeriodValue);

/**
 * @brief Get remaining time
 * @param TimerObject Timer object
 * @param ElapsedTime Pointer to elapsed time
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_GetRemainingTime(HW_TIMER_Object_t *TimerObject,
                                              uint32_t *ElapsedTime);

/**
 * @brief Check if timer is running
 * @param TimerObject Timer object
 * @return 1 if running, 0 otherwise
 */
uint32_t HW_TIMER_IsRunning(HW_TIMER_Object_t *TimerObject);

/**
 * @brief Get current timer value
 * @param TimerType Timer type
 * @return Current timer value in ticks
 */
uint32_t HW_TIMER_GetValue(HW_TIMER_Type_t TimerType);

/**
 * @brief IRQ Handler for timer events
 */
void HW_TIMER_IRQ_Handler(void);

/**
 * @brief Configure specific hardware timer
 * @param TimerType Timer type
 * @param Config Configuration structure
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_ConfigureTimer(HW_TIMER_Type_t TimerType,
                                            const HW_TIMER_Config_t *Config);

/**
 * @brief Get available timer with lowest power consumption
 * @return Best timer type for low power operation
 */
HW_TIMER_Type_t HW_TIMER_GetLowPowerTimer(void);

/**
 * @brief Switch timer context to optimize power consumption
 * @param PreferredTimer Preferred timer type
 * @return UTIL_TIMER_Status_t operation status
 */
UTIL_TIMER_Status_t HW_TIMER_SwitchContext(HW_TIMER_Type_t PreferredTimer);

#ifdef __cplusplus
}
#endif

/**
  * @}
  */

#endif /* UTIL_TIMER_OPTIMIZED_H__ */
