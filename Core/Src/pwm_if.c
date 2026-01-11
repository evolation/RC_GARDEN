/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    pwm_if.c
  * @brief   PWM interface implementation using STM32WLxx hardware timers
  *          50 Hz servo standard with 1.0–2.0 ms pulse width on TIM1.
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
typedef struct
{
  uint32_t duty_cycle;   /**< Logical “duty” 0–100 (mapped to 1–2 ms) */
  uint8_t  enabled;
  uint32_t channel;      /**< TIM_CHANNEL_1 / TIM_CHANNEL_2 */
  uint32_t compare_val;  /**< Cached CCR value */
} PWM_ChannelState_t;

/* -------------------------------------------------------------------------- */
/* Timing configuration                                                       */
/* -------------------------------------------------------------------------- */

/* Servo timing */
#define PWM_FREQUENCY_HZ        50U        /**< 50 Hz -> 20 ms period         */

/* Timer base frequency (after prescaler) */
#define PWM_TIMER_FREQ_HZ       1000000U   /**< 1 MHz -> 1 tick = 1 us        */

/* Period in timer ticks for 50 Hz: 1e6 / 50 = 20000 ticks -> 20 ms */
#define PWM_TIMER_PERIOD        (PWM_TIMER_FREQ_HZ / PWM_FREQUENCY_HZ)  /* 20000 */

/* Servo pulse widths in microseconds */
#define PWM_PULSE_MIN_US        1000U      /**< 1.0 ms                        */
#define PWM_PULSE_MAX_US        2000U      /**< 2.0 ms                        */

/* Convert to timer ticks (1 tick = 1 us at 1 MHz) */
#define PWM_PULSE_MIN_TICKS     (PWM_PULSE_MIN_US)
#define PWM_PULSE_MAX_TICKS     (PWM_PULSE_MAX_US)

/* Duty representation */
#define PWM_MAX_DUTY            100U       /**< 0–100 logical duty            */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;

static PWM_ChannelState_t pwm_states[PWM_CHANNEL_MAX] =
{
  { .duty_cycle = 50U, .enabled = 0, .channel = TIM_CHANNEL_1 }, /* PA8 */
  { .duty_cycle = 50U, .enabled = 0, .channel = TIM_CHANNEL_2 }  /* PA9 */
};

/* Private function prototypes -----------------------------------------------*/
static void MX_TIM1_PWM_Init(void);
static void PWM_UpdateCompare(PWM_Channel_t channel);

/* Exported functions --------------------------------------------------------*/

PWM_Status_t PWM_Init(void)
{
  /* Configure TIM1 in PWM mode @ 50 Hz */
  MX_TIM1_PWM_Init();

  /* Start PWM on both channels (output will be 1.5 ms by default after update) */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

  /* Explicitly enable Main Output for TIM1 (advanced timer requirement) */
  __HAL_TIM_MOE_ENABLE(&htim1);

  /* Force update event to ensure outputs are active */
  HAL_TIM_GenerateEvent(&htim1, TIM_EVENTSOURCE_UPDATE);

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
    return PWM_OK;
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
    return PWM_OK;
  }

  pwm_states[channel].enabled = 0;
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
  PWM_UpdateCompare(channel);

  return PWM_OK;
}

PWM_Status_t PWM_GetDutyCycle(PWM_Channel_t channel, uint32_t *duty_percent)
{
  if ((channel >= PWM_CHANNEL_MAX) || (duty_percent == NULL))
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

void PWM_Process(void)
{
  /* No-op: hardware PWM handles everything */
}

/* Private functions --------------------------------------------------------*/

/**
  * @brief Initialize TIM1 in PWM mode for 50 Hz servo signals.
  *
  * Timer clock assumed 32 MHz -> prescaler 31 -> 1 MHz timer clock.
  * Period = 20000 - 1 -> 20 ms.
  */
static void MX_TIM1_PWM_Init(void)
{
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance = TIM1;
  htim1.Init.Prescaler         = 47U;                            /* 48 MHz / (47+1) = 1 MHz */
  htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim1.Init.Period            = PWM_TIMER_PERIOD - 1U;          /* 20000 - 1 = 19999 ticks */
  htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }

  /* Common PWM channel configuration */
  sConfigOC.OCMode       = TIM_OCMODE_PWM1;
  sConfigOC.Pulse        = (PWM_PULSE_MIN_TICKS +
                           (PWM_PULSE_MAX_TICKS - PWM_PULSE_MIN_TICKS) / 2U); /* ~1.5 ms */
  sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

  /* Use PWM config, not OC */
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }

  /* Disable output compare preload for immediate CCR updates */
  LL_TIM_OC_DisablePreload(htim1.Instance, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_DisablePreload(htim1.Instance, LL_TIM_CHANNEL_CH2);

  sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime         = 0;
  sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;

  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Update PWM compare register for the specified channel.
  *
  * Maps 0–100 “duty” to 1.0–2.0 ms pulse:
  *
  *   duty = 0   -> 1.0 ms
  *   duty = 50  -> 1.5 ms
  *   duty = 100 -> 2.0 ms
  */
static void PWM_UpdateCompare(PWM_Channel_t channel)
{
  uint32_t duty = pwm_states[channel].duty_cycle;
  uint32_t compare_val;

  /* Linear mapping from [0..100] to [PWM_PULSE_MIN_TICKS..PWM_PULSE_MAX_TICKS] */
  compare_val = PWM_PULSE_MIN_TICKS
              + ((PWM_PULSE_MAX_TICKS - PWM_PULSE_MIN_TICKS) * duty) / PWM_MAX_DUTY;

  /* Clamp just in case */
  if (compare_val < PWM_PULSE_MIN_TICKS)
  {
    compare_val = PWM_PULSE_MIN_TICKS;
  }
  else if (compare_val > PWM_PULSE_MAX_TICKS)
  {
    compare_val = PWM_PULSE_MAX_TICKS;
  }

  pwm_states[channel].compare_val = compare_val;

  /* Update compare register (preload disabled, takes effect immediately) */
  __HAL_TIM_SET_COMPARE(&htim1, pwm_states[channel].channel, compare_val);
}

/* END OF FILE ---------------------------------------------------------------*/
