/*!
 * \file      pwm_monitor.h
 *
 * \brief     PWM Input Monitor - Reads PWM signals from GPIO pins
 *            Monitors two PWM inputs and sends unsolicited serial responses
 *            when signal duty cycle exceeds threshold (>1%)
 *
 */

#ifndef __PWM_MONITOR_H__
#define __PWM_MONITOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
 * @brief PWM Monitor Channel enumeration
 */
typedef enum
{
  PWM_MON_CH1 = 0,  /**< Monitor Channel 1 (PA10) */
  PWM_MON_CH2 = 1,  /**< Monitor Channel 2 (PA11) */
  PWM_MON_MAX = 2   /**< Maximum channels */
} PWM_MON_Channel_t;

/**
 * @brief PWM Monitor Status enumeration
 */
typedef enum
{
  PWM_MON_OK = 0,
  PWM_MON_ERROR = 1,
  PWM_MON_NOT_READY = 2
} PWM_MON_Status_t;

/**
 * @brief PWM Measurement data
 */
typedef struct
{
  uint32_t pulse_width_us;  /**< Pulse width in microseconds */
  uint32_t period_us;       /**< Period in microseconds */
  uint32_t duty_cycle;      /**< Duty cycle in percent (0-100) */
  bool signal_detected;     /**< Signal presence flag */
  uint32_t last_update_ms;  /**< Timestamp of last update */
} PWM_MON_Data_t;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initialize PWM Monitor
 * 
 * Configures GPIO pins as inputs with input capture timers
 * Monitors PA10 (TIM1_CH3) and PA11 (TIM1_CH4) for PWM signals
 * 
 * @return PWM_MON_Status_t initialization status
 */
PWM_MON_Status_t PWM_MON_Init(void);

/**
 * @brief Deinitialize PWM Monitor
 * @return PWM_MON_Status_t deinitialization status
 */
PWM_MON_Status_t PWM_MON_DeInit(void);

/**
 * @brief Get PWM measurement for specified channel
 * 
 * @param channel PWM monitor channel
 * @param data Pointer to PWM measurement data structure
 * @return PWM_MON_Status_t operation status
 */
PWM_MON_Status_t PWM_MON_GetData(PWM_MON_Channel_t channel, PWM_MON_Data_t *data);

/**
 * @brief Get duty cycle percentage (0-100)
 * 
 * @param channel PWM monitor channel
 * @return Duty cycle in percent, or 0 if not available
 */
uint32_t PWM_MON_GetDutyCycle(PWM_MON_Channel_t channel);

/**
 * @brief Check if PWM signal is detected (duty > 1%)
 * 
 * @param channel PWM monitor channel
 * @return true if signal detected and duty > 1%, false otherwise
 */
bool PWM_MON_SignalDetected(PWM_MON_Channel_t channel);

/**
 * @brief Process PWM monitor (call periodically)
 * 
 * Checks for signal changes and generates unsolicited responses
 * Send over serial when duty cycle exceeds 1%
 */
void PWM_MON_Process(void);

/**
 * @brief Send unsolicited PWM status over serial
 * 
 * @param channel Channel that triggered the response
 * @param data PWM measurement data to send
 */
void PWM_MON_SendResponse(PWM_MON_Channel_t channel, const PWM_MON_Data_t *data, uint32_t pkt_counter);

/**
 * @brief Input capture callback (called from timer ISR)
 * 
 * Internal function - measures PWM pulse width using input capture
 */
void PWM_MON_InputCapture_Callback(uint32_t channel, uint32_t capture_value);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_MONITOR_H__ */
