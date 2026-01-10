/*!
 * \file      pwm_monitor.c
 *
 * \brief     PWM Input Monitor Implementation
 *            Reads PWM signals from GPIO using input capture timers
 *            Sends unsolicited serial responses when duty > 1%
 *
 */

/* Includes ------------------------------------------------------------------*/
#include "pwm_monitor.h"
#include "pwm_if.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_hal_tim.h"
#include "stm32wlxx_ll_tim.h"
#include "usart_if.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/

#define PWM_MON_DUTY_THRESHOLD 1U     /**< Minimum duty cycle threshold (%) */
#define PWM_MON_TIMEOUT_MS 100U       /**< Signal timeout (ms) */
#define PWM_MON_TIMER_FREQ_HZ 1000000U /**< 1 MHz for 1 μs resolution */

/* Private typedef -----------------------------------------------------------*/

/**
 * @brief PWM Monitor internal state
 */
typedef struct
{
  uint32_t rising_edge_ticks;    /**< Tick count at rising edge */
  uint32_t falling_edge_ticks;   /**< Tick count at falling edge */
  uint32_t pulse_width_us;       /**< Calculated pulse width */
  uint32_t period_us;            /**< Calculated period */
  uint32_t duty_cycle;           /**< Calculated duty cycle % */
  bool signal_detected;          /**< Signal present flag */
  bool was_detected;             /**< Previous signal state */
  uint32_t previous_duty_cycle;  /**< Previous duty cycle for change detection */
  uint32_t last_update_ticks;    /**< Last measurement time */
  uint8_t edge_state;            /**< 0=waiting rising, 1=waiting falling */
} PWM_MON_State_t;

/* Private variables ---------------------------------------------------------*/

/**
 * @brief PWM Monitor states for each channel
 */
static PWM_MON_State_t pwm_mon_states[PWM_MON_MAX] = {
  {.rising_edge_ticks = 0, .falling_edge_ticks = 0, .edge_state = 0, .signal_detected = false, .previous_duty_cycle = 0},
  {.rising_edge_ticks = 0, .falling_edge_ticks = 0, .edge_state = 0, .signal_detected = false, .previous_duty_cycle = 0}
};

/**
 * @brief Timer overflow counter for extended range
 */
static volatile uint32_t timer_overflows = 0;

/* Private function prototypes -----------------------------------------------*/

static void PWM_MON_ProcessChannel(PWM_MON_Channel_t ch);
static inline uint32_t GetTimerValue(void);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize PWM Monitor with input capture on TIM1
 * 
 * Configuration:
 * - Timer: TIM1 (same as PWM output, but using CH3/CH4 for input)
 * - Clock: 32 MHz APB2 / 32 = 1 MHz (1 μs resolution)
 * - Channels: CH3 (PA10) and CH4 (PA11) as input capture
 * - Mode: Edge detection on both rising and falling edges
 */
PWM_MON_Status_t PWM_MON_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  timer_overflows = 0;

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure GPIO pins PA10 and PA11 as input capture alternate function */
  GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Configure Input Capture for Channel 3 (PA10) */
  sConfigIC.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;  /* Both rising and falling */
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;

  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure Input Capture for Channel 4 (PA11) */
  if (HAL_TIM_IC_ConfigChannel(&htim1, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  /* Enable TIM1 capture/compare interrupt in NVIC */
  HAL_NVIC_SetPriority(TIM1_CC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);

  /* Start input capture on both channels */
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_3);  /* CH3 = PA10 */
  HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_4);  /* CH4 = PA11 */

  /* Enable TIM1 update interrupt for overflow tracking */
  __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
  __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);

  return PWM_MON_OK;
}

/**
 * @brief Deinitialize PWM Monitor
 */
PWM_MON_Status_t PWM_MON_DeInit(void)
{
  HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_3);
  HAL_TIM_IC_Stop_IT(&htim1, TIM_CHANNEL_4);

  return PWM_MON_OK;
}

/**
 * @brief Get PWM measurement data
 */
PWM_MON_Status_t PWM_MON_GetData(PWM_MON_Channel_t channel, PWM_MON_Data_t *data)
{
  if (channel >= PWM_MON_MAX || data == NULL)
  {
    return PWM_MON_ERROR;
  }

  if (!pwm_mon_states[channel].signal_detected)
  {
    return PWM_MON_NOT_READY;
  }

  /* Copy measurement data */
  data->pulse_width_us = pwm_mon_states[channel].pulse_width_us;
  data->period_us = pwm_mon_states[channel].period_us;
  data->duty_cycle = pwm_mon_states[channel].duty_cycle;
  data->signal_detected = pwm_mon_states[channel].signal_detected;
  data->last_update_ms = GetTimerValue() / 1000;

  return PWM_MON_OK;
}

/**
 * @brief Get duty cycle percentage
 */
uint32_t PWM_MON_GetDutyCycle(PWM_MON_Channel_t channel)
{
  if (channel >= PWM_MON_MAX)
  {
    return 0;
  }

  return pwm_mon_states[channel].duty_cycle;
}

/**
 * @brief Check if signal detected (duty > 1%)
 */
bool PWM_MON_SignalDetected(PWM_MON_Channel_t channel)
{
  if (channel >= PWM_MON_MAX)
  {
    return false;
  }

  return pwm_mon_states[channel].signal_detected && 
         (pwm_mon_states[channel].duty_cycle > PWM_MON_DUTY_THRESHOLD);
}

/**
 * @brief Process PWM Monitor - check for signal changes
 * 
 * Sends unsolicited response when:
 * - Signal appears (transitions from no signal to duty > 1%)
 * - Signal disappears (transitions from duty > 1% to no signal)
 */
void PWM_MON_Process(void)
{
  PWM_MON_Data_t data;

  for (uint8_t ch = 0; ch < PWM_MON_MAX; ch++)
  {
    PWM_MON_ProcessChannel((PWM_MON_Channel_t)ch);

    bool signal_changed = pwm_mon_states[ch].signal_detected != pwm_mon_states[ch].was_detected;
    bool duty_changed = false;

    /* Check for duty cycle change (threshold: 2% to avoid noise) */
    if (pwm_mon_states[ch].signal_detected)
    {
      uint32_t duty_diff = (pwm_mon_states[ch].duty_cycle > pwm_mon_states[ch].previous_duty_cycle) ?
                           (pwm_mon_states[ch].duty_cycle - pwm_mon_states[ch].previous_duty_cycle) :
                           (pwm_mon_states[ch].previous_duty_cycle - pwm_mon_states[ch].duty_cycle);

      if (duty_diff >= 2)  /* 2% threshold */
      {
        duty_changed = true;
      }
    }

    /* Send unsolicited response if signal state or duty cycle changed */
    if (signal_changed || duty_changed)
    {
      if (PWM_MON_GetData((PWM_MON_Channel_t)ch, &data) == PWM_MON_OK)
      {
        PWM_MON_SendResponse((PWM_MON_Channel_t)ch, &data);
      }

      /* Update previous state */
      pwm_mon_states[ch].was_detected = pwm_mon_states[ch].signal_detected;
      pwm_mon_states[ch].previous_duty_cycle = pwm_mon_states[ch].duty_cycle;
    }
  }
}

/**
 * @brief Send unsolicited PWM status response
 */
void PWM_MON_SendResponse(PWM_MON_Channel_t channel, const PWM_MON_Data_t *data)
{
  char response[128];
  uint8_t len;

  if (data->signal_detected)
  {
    /* Format: "+PWM,CH<n>,<duty>%,<pulse>ms" */
    len = snprintf(response, sizeof(response),
                   "+PWM,CH%u,%u%%,%u.%ums\r\n",
                   channel + 1,
                   data->duty_cycle,
                   data->pulse_width_us / 1000,
                   data->pulse_width_us % 1000);
  }
  else
  {
    /* Format: "+PWM,CH<n>,LOSS" */
    len = snprintf(response, sizeof(response),
                   "+PWM,CH%u,LOSS\r\n",
                   channel + 1);
  }

  /* Send via USART */
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)response, len, 100);
}

/**
 * @brief Input capture callback - processes captured values
 * 
 * Called on each edge (rising or falling)
 */
void PWM_MON_InputCapture_Callback(uint32_t channel, uint32_t capture_value)
{
  PWM_MON_Channel_t ch;
  uint32_t pulse_ticks;
  uint32_t period_ticks;

  /* Map HAL channel to monitor channel */
  if (channel == TIM_CHANNEL_3)
  {
    ch = PWM_MON_CH1;  /* PA10 */
  }
  else if (channel == TIM_CHANNEL_4)
  {
    ch = PWM_MON_CH2;  /* PA11 */
  }
  else
  {
    return;  /* Invalid channel */
  }

  if (ch >= PWM_MON_MAX)
  {
    return;
  }

  /* Check current GPIO level to determine edge type */
  GPIO_TypeDef *port = (ch == PWM_MON_CH1) ? GPIOA : GPIOA;
  uint16_t pin = (ch == PWM_MON_CH1) ? GPIO_PIN_10 : GPIO_PIN_11;

  if (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET)
  {
    /* Rising edge detected */
    pwm_mon_states[ch].rising_edge_ticks = capture_value;
    pwm_mon_states[ch].edge_state = 1;  /* Waiting for falling edge */
  }
  else
  {
    /* Falling edge detected */
    if (pwm_mon_states[ch].edge_state == 1)
    {
      pwm_mon_states[ch].falling_edge_ticks = capture_value;
      pwm_mon_states[ch].edge_state = 0;  /* Waiting for next rising edge */

      /* Calculate pulse width and duty cycle */
      period_ticks = __HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;
      if (pwm_mon_states[ch].falling_edge_ticks >= pwm_mon_states[ch].rising_edge_ticks)
      {
        pulse_ticks = pwm_mon_states[ch].falling_edge_ticks -
                      pwm_mon_states[ch].rising_edge_ticks;
      }
      else
      {
        pulse_ticks = (period_ticks - pwm_mon_states[ch].rising_edge_ticks) +
                      pwm_mon_states[ch].falling_edge_ticks;
      }

      if (pulse_ticks > 0 && pulse_ticks < 500000)  /* 0.5 second max */
      {
        pwm_mon_states[ch].pulse_width_us = pulse_ticks;

        /* Mark signal as detected if above threshold */
        /* Servo PWM mapping: 1.0ms (1000 ticks) = 0%, 2.0ms (2000 ticks) = 100%
           Formula: duty = (pulse_ticks - 1000) / 10 */
        if (pulse_ticks >= 1000)
        {
          uint32_t estimated_duty = (pulse_ticks - 1000) / 10;
          pwm_mon_states[ch].duty_cycle = (estimated_duty > 100) ? 100 : estimated_duty;
        }
        else
        {
          pwm_mon_states[ch].duty_cycle = 0;
        }
        
        pwm_mon_states[ch].signal_detected = 
          (pwm_mon_states[ch].duty_cycle > PWM_MON_DUTY_THRESHOLD);

        pwm_mon_states[ch].last_update_ticks = GetTimerValue();
      }
    }
  }
}

/* Private functions --------------------------------------------------------*/

/* Note: TIM1 is now initialized by PWM_Init() in pwm_if.c
 * Input capture channels are configured in PWM_MON_Init()
 * This function is no longer needed since TIM1 is shared between PWM output and input capture
 */

/**
 * @brief Process single PWM channel - check for signal timeout
 */
static void PWM_MON_ProcessChannel(PWM_MON_Channel_t ch)
{
  uint32_t current_ticks = GetTimerValue();
  uint32_t time_since_last_update = current_ticks - pwm_mon_states[ch].last_update_ticks;

  /* If no capture for PWM_MON_TIMEOUT_MS, signal is lost */
  if (time_since_last_update > (PWM_MON_TIMEOUT_MS * 1000))
  {
    pwm_mon_states[ch].signal_detected = false;
    pwm_mon_states[ch].duty_cycle = 0;
  }
}

/**
 * @brief Get current timer value with overflow handling
 */
static inline uint32_t GetTimerValue(void)
{
  uint32_t period_ticks = __HAL_TIM_GET_AUTORELOAD(&htim1) + 1U;
  return (timer_overflows * period_ticks) + LL_TIM_GetCounter(TIM1);
}

/**
 * @brief Timer overflow callback (called from HAL)
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    timer_overflows++;
  }
}

/**
 * @brief Timer input capture callback
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    /* HAL_TIM_IRQHandler sets htim->Channel to the active channel */
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3)
    {
      PWM_MON_InputCapture_Callback(TIM_CHANNEL_3,
                                    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3));
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4)
    {
      PWM_MON_InputCapture_Callback(TIM_CHANNEL_4,
                                    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4));
    }
  }
}




/* END OF FILE ---------------------------------------------------------------*/
