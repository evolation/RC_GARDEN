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
  uint32_t last_update_ticks;    /**< Last measurement time */
  uint8_t edge_state;            /**< 0=waiting rising, 1=waiting falling */
} PWM_MON_State_t;

/* Private variables ---------------------------------------------------------*/

/**
 * @brief TIM1 handle for input capture on CH3 and CH4
 */
static TIM_HandleTypeDef htim1_ic;

/**
 * @brief PWM Monitor states for each channel
 */
static PWM_MON_State_t pwm_mon_states[PWM_MON_MAX] = {
  {.rising_edge_ticks = 0, .falling_edge_ticks = 0, .edge_state = 0, .signal_detected = false},
  {.rising_edge_ticks = 0, .falling_edge_ticks = 0, .edge_state = 0, .signal_detected = false}
};

/**
 * @brief Timer overflow counter for extended range
 */
static volatile uint32_t timer_overflows = 0;

/* Private function prototypes -----------------------------------------------*/

static void MX_TIM1_InputCapture_Init(void);
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

  /* Enable GPIOA clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure PA10 and PA11 as input capture (floating input) */
  GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;  /* TIM1_CH3 and TIM1_CH4 */
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Initialize TIM1 for input capture */
  MX_TIM1_InputCapture_Init();

  /* Start input capture on both channels */
  HAL_TIM_IC_Start_IT(&htim1_ic, TIM_CHANNEL_3);  /* CH3 = PA10 */
  HAL_TIM_IC_Start_IT(&htim1_ic, TIM_CHANNEL_4);  /* CH4 = PA11 */

  return PWM_MON_OK;
}

/**
 * @brief Deinitialize PWM Monitor
 */
PWM_MON_Status_t PWM_MON_DeInit(void)
{
  HAL_TIM_IC_Stop_IT(&htim1_ic, TIM_CHANNEL_3);
  HAL_TIM_IC_Stop_IT(&htim1_ic, TIM_CHANNEL_4);
  HAL_TIM_Base_Stop_IT(&htim1_ic);

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

    /* Check for signal state change */
    if (pwm_mon_states[ch].signal_detected != pwm_mon_states[ch].was_detected)
    {
      /* Signal state changed - send unsolicited response */
      if (PWM_MON_GetData((PWM_MON_Channel_t)ch, &data) == PWM_MON_OK)
      {
        PWM_MON_SendResponse((PWM_MON_Channel_t)ch, &data);
      }

      /* Update previous state */
      pwm_mon_states[ch].was_detected = pwm_mon_states[ch].signal_detected;
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
    /* Format: "+PWM,CH<n>,<duty>%,<period>ms" */
    len = snprintf(response, sizeof(response),
                   "+PWM,CH%u,%u%%,%u.%uus\r\n",
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
  HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100);
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
      pulse_ticks = pwm_mon_states[ch].falling_edge_ticks - 
                    pwm_mon_states[ch].rising_edge_ticks;

      if (pulse_ticks > 0 && pulse_ticks < 500000)  /* 0.5 second max */
      {
        pwm_mon_states[ch].pulse_width_us = pulse_ticks;

        /* Mark signal as detected if above threshold */
        /* Assume 20ms period for servo: duty = pulse / 20000 * 100 */
        uint32_t estimated_duty = (pulse_ticks * 100) / 20000;
        pwm_mon_states[ch].duty_cycle = (estimated_duty > 100) ? 100 : estimated_duty;
        pwm_mon_states[ch].signal_detected = 
          (pwm_mon_states[ch].duty_cycle > PWM_MON_DUTY_THRESHOLD);

        pwm_mon_states[ch].last_update_ticks = GetTimerValue();
      }
    }
  }
}

/* Private functions --------------------------------------------------------*/

/**
 * @brief Initialize TIM1 for input capture on CH3 and CH4
 * 
 * Configuration:
 * - Prescaler: 31 (32MHz / 32 = 1MHz for 1μs resolution)
 * - Counter: 32-bit up counter
 * - Mode: Input capture on both CH3 (PA10) and CH4 (PA11)
 * - Edge: Both rising and falling edges trigger capture
 */
static void MX_TIM1_InputCapture_Init(void)
{
  TIM_IC_InitTypeDef sConfigIC = {0};

  htim1_ic.Instance = TIM1;
  htim1_ic.Init.Prescaler = 31;  /* 1 MHz operation */
  htim1_ic.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1_ic.Init.Period = 0xFFFF;  /* 16-bit counter for input capture */
  htim1_ic.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1_ic.Init.RepetitionCounter = 0;
  htim1_ic.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_IC_Init(&htim1_ic) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure Input Capture for Channel 3 (PA10) */
  sConfigIC.ICPolarity = TIM_ICPOLARITY_BOTHEDGE;  /* Both rising and falling */
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;

  if (HAL_TIM_IC_ConfigChannel(&htim1_ic, &sConfigIC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }

  /* Configure Input Capture for Channel 4 (PA11) */
  if (HAL_TIM_IC_ConfigChannel(&htim1_ic, &sConfigIC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }

  /* Start timer base */
  HAL_TIM_Base_Start_IT(&htim1_ic);
}

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
  return LL_TIM_GetCounter(TIM1) + (timer_overflows << 16);
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
    /* Get which channel triggered the capture */
    if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC3))
    {
      PWM_MON_InputCapture_Callback(TIM_CHANNEL_3, 
                                    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3));
    }

    if (__HAL_TIM_GET_FLAG(htim, TIM_FLAG_CC4))
    {
      PWM_MON_InputCapture_Callback(TIM_CHANNEL_4, 
                                    HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4));
    }
  }
}

/**
 * @brief HAL TIM1 MSP Initialization for Input Capture
 */
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    __HAL_RCC_TIM1_CLK_ENABLE();
    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
  }
}

/**
 * @brief HAL TIM1 MSP Deinitialization
 */
void HAL_TIM_IC_MspDeInit(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    __HAL_RCC_TIM1_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(TIM1_CC_IRQn);
  }
}

/* END OF FILE ---------------------------------------------------------------*/
