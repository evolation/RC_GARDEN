/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pwm_if.h
  * @brief   Header for PWM interface module
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __PWM_IF_H__
#define __PWM_IF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief PWM Channel enumeration
 */
typedef enum
{
  PWM_CHANNEL_1 = 0,  /**< PWM Channel 1 */
  PWM_CHANNEL_2 = 1,  /**< PWM Channel 2 */
  PWM_CHANNEL_MAX = 2 /**< Maximum number of channels */
} PWM_Channel_t;

/**
 * @brief PWM Status enumeration
 */
typedef enum
{
  PWM_OK = 0,       /**< Operation successful */
  PWM_ERROR = 1,    /**< Operation failed */
  PWM_PARAM_ERROR = 2 /**< Invalid parameter */
} PWM_Status_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initialize PWM interface
 * @return PWM_Status_t PWM_OK if successful, PWM_ERROR otherwise
 */
PWM_Status_t PWM_Init(void);

/**
 * @brief Enable PWM on specified channel
 * @param channel PWM channel to enable
 * @return PWM_Status_t PWM_OK if successful, PWM_ERROR otherwise
 */
PWM_Status_t PWM_Enable(PWM_Channel_t channel);

/**
 * @brief Disable PWM on specified channel
 * @param channel PWM channel to disable
 * @return PWM_Status_t PWM_OK if successful, PWM_ERROR otherwise
 */
PWM_Status_t PWM_Disable(PWM_Channel_t channel);

/**
 * @brief Set duty cycle for PWM channel
 * @param channel PWM channel
 * @param duty_percent Duty cycle in percent (0-100)
 * @return PWM_Status_t PWM_OK if successful, PWM_PARAM_ERROR if duty_percent out of range
 */
PWM_Status_t PWM_SetDutyCycle(PWM_Channel_t channel, uint32_t duty_percent);

/**
 * @brief Get current duty cycle for PWM channel
 * @param channel PWM channel
 * @param duty_percent Pointer to store duty cycle value
 * @return PWM_Status_t PWM_OK if successful, PWM_ERROR otherwise
 */
PWM_Status_t PWM_GetDutyCycle(PWM_Channel_t channel, uint32_t *duty_percent);

/**
 * @brief Check if PWM channel is enabled
 * @param channel PWM channel
 * @return 1 if enabled, 0 if disabled
 */
uint8_t PWM_IsEnabled(PWM_Channel_t channel);

/**
 * @brief Process PWM signals (call this periodically, e.g., from SysTick every 0.1ms)
 *        For 50 Hz servo: call every 0.1ms for best resolution
 *        Can also call every 1ms for acceptable jitter
 */
void PWM_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_IF_H__ */
