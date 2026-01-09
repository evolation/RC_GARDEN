/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pwm_if.c
  * @brief   PWM interface implementation using software PWM on separate GPIO pins
  *          Note: STM32WL55 has limited timer support, so software PWM is used
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
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/**
 * @brief PWM Channel state structure
 */
typedef struct
{
  uint32_t duty_cycle;  /**< Current duty cycle (0-100) */
  uint8_t enabled;      /**< Channel enabled flag */
  uint32_t pin;         /**< GPIO pin for this channel */
  GPIO_TypeDef *port;   /**< GPIO port for this channel */
} PWM_ChannelState_t;

/* Private define ------------------------------------------------------------*/
#define PWM_MAX_DUTY 100U          /**< Maximum duty cycle in percent */
#define PWM_PERIOD_MS 20U          /**< PWM period in milliseconds (50 Hz - servo standard) */
#define PWM_PULSE_MIN_MS 1U        /**< Minimum pulse width in ms (1.0 ms) */
#define PWM_PULSE_MAX_MS 2U        /**< Maximum pulse width in ms (2.0 ms) */

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
/**
 * @brief PWM Channel states - one per channel
 */
static PWM_ChannelState_t pwm_states[PWM_CHANNEL_MAX] = {
  {.duty_cycle = 50U, .enabled = 0, .pin = PWM1_Pin, .port = PWM1_GPIO_Port},  /* Channel 1 - PA8 */
  {.duty_cycle = 50U, .enabled = 0, .pin = PWM2_Pin, .port = PWM2_GPIO_Port}   /* Channel 2 - PA9 */
};

/* Counter for software PWM timing */
static uint32_t pwm_counter = 0;

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief Update PWM outputs based on duty cycle and counter
 */
static void PWM_Update(void);

/* Exported functions --------------------------------------------------------*/

PWM_Status_t PWM_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Configure GPIO pins for output */
  GPIO_InitStruct.Pin = PWM1_Pin | PWM2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Set both to low initially */
  HAL_GPIO_WritePin(GPIOA, PWM1_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, PWM2_Pin, GPIO_PIN_RESET);

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
  /* Set pin low when disabled */
  HAL_GPIO_WritePin(pwm_states[channel].port, pwm_states[channel].pin, GPIO_PIN_RESET);
  
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
 * @brief Called periodically from main loop to update PWM signals
 *        This function should be called approximately every 1 millisecond
 *        
 *        50 Hz servo standard: 20 ms period
 *        Pulse width: 1.0-2.0 ms (maps to 0-100% throttle)
 */
void PWM_Process(void)
{
  /* Increment counter modulo PWM_PERIOD_MS (20 ms for 50 Hz) */
  pwm_counter++;
  if (pwm_counter >= PWM_PERIOD_MS)
  {
    pwm_counter = 0;
  }

  /* Update PWM outputs based on current counter */
  PWM_Update();
}

/* Private functions --------------------------------------------------------*/

/**
 * @brief Update PWM output states based on counter and duty cycle
 *        Maps duty cycle (0-100) to servo pulse width (1.0-2.0 ms)
 *        Standard servo/ESC control: 50 Hz, 1-2 ms pulse
 */
static void PWM_Update(void)
{
  uint8_t ch;
  uint32_t pulse_width_ms;  /* Pulse width in milliseconds (1-2) */

  for (ch = 0; ch < PWM_CHANNEL_MAX; ch++)
  {
    if (!pwm_states[ch].enabled)
    {
      /* Keep disabled channels low */
      HAL_GPIO_WritePin(pwm_states[ch].port, pwm_states[ch].pin, GPIO_PIN_RESET);
      continue;
    }

    /* Map duty cycle (0-100) to pulse width (1.0-2.0 ms) */
    /* pulse_width_ms = 1 + (duty_cycle * 1) / 100 */
    /* Example: 0% → 1ms, 50% → 1.5ms, 100% → 2ms */
    pulse_width_ms = PWM_PULSE_MIN_MS + (pwm_states[ch].duty_cycle / 100U);

    /* Output high during pulse width, low for rest of period */
    if (pwm_counter < pulse_width_ms)
    {
      HAL_GPIO_WritePin(pwm_states[ch].port, pwm_states[ch].pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(pwm_states[ch].port, pwm_states[ch].pin, GPIO_PIN_RESET);
    }
  }
}
