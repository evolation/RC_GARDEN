/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pwm_if.c
  * @brief   PWM interface implementation using STM32WLxx hardware timers
  *          Optimized for low-power operation with TIM1
  *          50 Hz servo standard with 1.0-2.0 ms pulse width
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "pwm_if.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_tim.h"
#include "stm32wlxx_ll_tim.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief PWM Channel state structure - Hardware PWM using TIM1
 */
typedef struct
{
  uint32_t duty_cycle;   /**< Current duty cycle (0-100) */
  uint8_t enabled;       /**< Channel enabled flag */
  uint32_t channel;      /**< TIM channel (TIM_CHANNEL_1/2) */
  uint32_t compare_val;  /**< Compare register value */
} PWM_ChannelState_t;

/* Private define ------------------------------------------------------------*/
#define PWM_MAX_DUTY 100U           /**< Maximum duty cycle in percent */
#define PWM_PERIOD_MS 20U           /**< PWM period in milliseconds (50 Hz - servo standard) */
#define PWM_PULSE_MIN_MS 1U         /**< Minimum pulse width in ms (1.0 ms) */
#define PWM_PULSE_MAX_MS 2U         /**< Maximum pulse width in ms (2.0 ms) */

/* Hardware PWM Configuration for TIM1 */
#define PWM_TIMER_FREQ_HZ 1000000U /**< 1 MHz timer frequency for 1 microsecond resolution */
#define PWM_TIMER_PERIOD (PWM_PERIOD_MS * 1000U)  /**< 20000 microseconds = 20 ms for 50 Hz */
#define PWM_PULSE_MIN_TICKS (PWM_PULSE_MIN_MS * 1000U)  /**< 1000 ticks for 1.0 ms */
#define PWM_PULSE_MAX_TICKS (PWM_PULSE_MAX_MS * 1000U)  /**< 2000 ticks for 2.0 ms */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief TIM1 handle for hardware PWM (shared with PWM monitor)
 * Note: Declared here but exposed via pwm_if.h for use by pwm_monitor.c
 */
TIM_HandleTypeDef htim1;

/**
 * @brief PWM Channel states - using TIM1 channels
 */
static PWM_ChannelState_t pwm_states[PWM_CHANNEL_MAX] = {
  {.duty_cycle = 50U, .enabled = 0, .channel = TIM_CHANNEL_1},  /* Channel 1 - PA8/TIM1_CH1 */
  {.duty_cycle = 50U, .enabled = 0, .channel = TIM_CHANNEL_2}   /* Channel 2 - PA9/TIM1_CH2 */
};

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief Initialize TIM1 in PWM mode
 */
static void MX_TIM1_PWM_Init(void);

/**
 * @brief Update PWM compare registers based on duty cycle
 */
static void PWM_UpdateCompare(PWM_Channel_t channel);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize PWM using hardware timers (TIM1)
 */
PWM_Status_t PWM_Init(void)
{
  /* Initialize TIM1 for PWM mode */
  MX_TIM1_PWM_Init();

  /* Start PWM output on both channels (disabled by default in PWM states) */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

  return PWM_OK;
}

PWM_Status_t PWM_Enable(PWM_Channel_t channel)
{
  if (channel >= PWM_CHANNEL_MAX)
  {
    return PWM_PARAM_ERROR;
  }

  if (pwm_states[channel].enabled)
  {
    return PWM_OK;  /* Already enabled */
  }

  pwm_states[channel].enabled = 1;
  HAL_TIM_PWM_Start(&htim1, pwm_states[channel].channel);

  return PWM_OK;
}

PWM_Status_t PWM_Disable(PWM_Channel_t channel)
{
  if (channel >= PWM_CHANNEL_MAX)
  {
    return PWM_PARAM_ERROR;
  }

  if (!pwm_states[channel].enabled)
  {
    return PWM_OK;  /* Already disabled */
  }

  pwm_states[channel].enabled = 0;
  /* Stop PWM output on this channel */
  HAL_TIM_PWM_Stop(&htim1, pwm_states[channel].channel);

  return PWM_OK;
}

PWM_Status_t PWM_SetDutyCycle(PWM_Channel_t channel, uint32_t duty_percent)
{
  if (channel >= PWM_CHANNEL_MAX)
  {
    return PWM_PARAM_ERROR;
  }

  if (duty_percent > PWM_MAX_DUTY)
  {
    return PWM_PARAM_ERROR;
  }

  pwm_states[channel].duty_cycle = duty_percent;

  /* Update hardware timer compare register */
  PWM_UpdateCompare(channel);

  return PWM_OK;
}

PWM_Status_t PWM_GetDutyCycle(PWM_Channel_t channel, uint32_t *duty_percent)
{
  if (channel >= PWM_CHANNEL_MAX || duty_percent == NULL)
  {
    return PWM_PARAM_ERROR;
  }

  *duty_percent = pwm_states[channel].duty_cycle;
  return PWM_OK;
}

uint8_t PWM_IsEnabled(PWM_Channel_t channel)
{
  if (channel >= PWM_CHANNEL_MAX)
  {
    return 0;
  }

  return pwm_states[channel].enabled;
}

/**
 * @brief Called periodically to update PWM duty cycles
 *        No longer needed with hardware PWM, but kept for API compatibility
 *        Hardware timers handle all updates automatically
 */
void PWM_Process(void)
{
  /* Hardware PWM handles everything - no software updates needed */
  /* This function is now a no-op but kept for backward compatibility */
}

/* Private functions --------------------------------------------------------*/

/**
 * @brief Initialize TIM1 in PWM mode
 * 
 *        Configuration:
 *        - Timer: TIM1 (Advanced timer with break function)
 *        - Clock: 32 MHz APB2
 *        - Prescaler: 31 (to get 1 MHz operation)
 *        - Period: 20000 ticks = 20 ms (50 Hz frequency)
 *        - PWM Mode: Mode 1 (PWM mode)
 *        - Channels: CH1 (PA8), CH2 (PA9) as PWM outputs
 */
static void MX_TIM1_PWM_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 31;  /* 32 MHz / 32 = 1 MHz */
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = PWM_TIMER_PERIOD - 1;  /* 20000 - 1 = 19999 for 20ms */
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure PWM for Channel 1 (PA8) */
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = PWM_PULSE_MIN_TICKS;  /* Start at 1.0 ms pulse */
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure PWM for Channel 2 (PA9) */
  if (HAL_TIM_OC_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;

  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief Update PWM compare register for specified channel
 *
 *        Maps duty cycle (0-100%) to pulse width (1.0-2.0 ms)
 *        Formula: pulse_ticks = 1000 + (duty_cycle * 10)
 *        - 0% duty = 1000 ticks = 1.0 ms
 *        - 50% duty = 1500 ticks = 1.5 ms
 *        - 100% duty = 2000 ticks = 2.0 ms
 */
static void PWM_UpdateCompare(PWM_Channel_t channel)
{
  uint32_t compare_val;

  /* Map 0-100% duty cycle to 1000-2000 ticks (1.0-2.0 ms) */
  compare_val = PWM_PULSE_MIN_TICKS + (pwm_states[channel].duty_cycle * 10U);

  /* Clamp to maximum */
  if (compare_val > PWM_PULSE_MAX_TICKS)
  {
    compare_val = PWM_PULSE_MAX_TICKS;
  }

  pwm_states[channel].compare_val = compare_val;

  /* Update hardware compare register */
  __HAL_TIM_SET_COMPARE(&htim1, pwm_states[channel].channel, compare_val);
}

/* END OF FILE ---------------------------------------------------------------*/
