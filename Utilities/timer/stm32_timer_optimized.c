/*!
 * \file      stm32_timer_optimized.c
 *
 * \brief     Optimized Hardware Timer Implementation using STM32WLxx HAL
 *
 * \copyright Revised BSD License
 *
 * \author    Optimized for STM32WL55xx with multi-timer support
 */

/* Includes ------------------------------------------------------------------*/
#include "stm32_timer_optimized.h"
#include "stm32_timer.h"
#include "stm32wlxx_hal.h"
#include "stm32wlxx_ll_tim.h"
#include "stm32wlxx_ll_lptim.h"

/** @addtogroup TIMER_OPTIMIZED
  * @{
  */

/* Private macro definitions -------------------------------------------------*/

/* Timer frequency constants */
#define LPTIM_FREQUENCY          32000U   /* 32 kHz LSE clock */
#define TIM_FREQUENCY            32000000U /* 32 MHz system clock */
#define RTC_FREQUENCY            32768U    /* 32.768 kHz LSE */

/* Minimum timeout in ticks */
#define MIN_ALARM_DELAY          3U

/* Private variables ---------------------------------------------------------*/

/* Timer handles */
static TIM_HandleTypeDef htim1 = {0};
static TIM_HandleTypeDef htim2 = {0};
static TIM_HandleTypeDef htim16 = {0};
static LPTIM_HandleTypeDef hlptim1 = {0};
static LPTIM_HandleTypeDef hlptim2 = {0};
extern RTC_HandleTypeDef hrtc;

/* Timer list head */
static HW_TIMER_Object_t *TimerListHead = NULL;

/* Context variable */
static uint32_t TimerContext = 0;
static HW_TIMER_Type_t ActiveTimerType = HW_TIMER_RTC;

/* Private function prototypes -----------------------------------------------*/

static HW_TIMER_Type_t SelectOptimalTimer(uint32_t Duration);
static uint32_t Convert_ms2Ticks(uint32_t Milliseconds, HW_TIMER_Type_t TimerType);
static uint32_t Convert_Ticks2ms(uint32_t Ticks, HW_TIMER_Type_t TimerType);
static void TimerInsertNewHeadTimer(HW_TIMER_Object_t *TimerObject);
static void TimerInsertTimer(HW_TIMER_Object_t *TimerObject);
static bool TimerExists(HW_TIMER_Object_t *TimerObject);
static void TimerSetTimeout(HW_TIMER_Object_t *TimerObject);
static inline uint32_t GetTimerValue(HW_TIMER_Type_t TimerType);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize optimized timer system with all available timers
 */
UTIL_TIMER_Status_t HW_TIMER_Init(void)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    TimerListHead = NULL;
    TimerContext = 0;
    ActiveTimerType = HW_TIMER_RTC;

    /* Initialize LPTIM1 - Primary low-power timer */
    if (HW_TIMER_ConfigureTimer(HW_TIMER_LPTIM1, NULL) != UTIL_TIMER_OK)
    {
        return UTIL_TIMER_HW_ERROR;
    }

    /* Initialize RTC - Backup timer for calendar */
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    hrtc.IsEnabled.RtcFeatures = UINT32_MAX;

    return ret;
}

/**
 * @brief Deinitialize timer system
 */
UTIL_TIMER_Status_t HW_TIMER_DeInit(void)
{
    HW_TIMER_Stop(TimerListHead);
    HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);

    return UTIL_TIMER_OK;
}

/**
 * @brief Create and configure a hardware timer
 *
 * Automatically selects the best timer based on duration and power requirements
 */
UTIL_TIMER_Status_t HW_TIMER_Create(HW_TIMER_Object_t *TimerObject,
                                    uint32_t PeriodValue,
                                    UTIL_TIMER_Mode_t Mode,
                                    HW_TIMER_Type_t TimerType,
                                    void (*Callback)(void *),
                                    void *Argument)
{
    if ((TimerObject != NULL) && (Callback != NULL))
    {
        /* Auto-select timer if set to RTC (default) */
        if (TimerType == HW_TIMER_RTC)
        {
            TimerType = SelectOptimalTimer(PeriodValue);
        }

        TimerObject->Timestamp = 0U;
        TimerObject->ReloadValue = Convert_ms2Ticks(PeriodValue, TimerType);
        TimerObject->IsPending = 0U;
        TimerObject->IsRunning = 0U;
        TimerObject->IsReloadStopped = 0U;
        TimerObject->Callback = Callback;
        TimerObject->argument = Argument;
        TimerObject->Mode = Mode;
        TimerObject->TimerType = TimerType;
        TimerObject->Next = NULL;

        return UTIL_TIMER_OK;
    }

    return UTIL_TIMER_INVALID_PARAM;
}

/**
 * @brief Start hardware timer with optimized configuration
 */
UTIL_TIMER_Status_t HW_TIMER_Start(HW_TIMER_Object_t *TimerObject)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;
    uint32_t elapsedTime;
    uint32_t ticks;
    uint32_t minValue = MIN_ALARM_DELAY;

    if ((TimerObject != NULL) && (!TimerExists(TimerObject)) && (TimerObject->IsRunning == 0U))
    {
        ticks = TimerObject->ReloadValue;

        if (ticks < minValue)
        {
            ticks = minValue;
        }

        TimerObject->Timestamp = ticks;
        TimerObject->IsPending = 0U;
        TimerObject->IsRunning = 1U;
        TimerObject->IsReloadStopped = 0U;

        if (TimerListHead == NULL)
        {
            TimerContext = GetTimerValue(TimerObject->TimerType);
            TimerInsertNewHeadTimer(TimerObject);
        }
        else
        {
            elapsedTime = GetTimerValue(TimerObject->TimerType) - TimerContext;
            TimerObject->Timestamp += elapsedTime;

            if (TimerObject->Timestamp < TimerListHead->Timestamp)
            {
                TimerInsertNewHeadTimer(TimerObject);
            }
            else
            {
                TimerInsertTimer(TimerObject);
            }
        }
    }
    else
    {
        ret = UTIL_TIMER_INVALID_PARAM;
    }

    return ret;
}

/**
 * @brief Stop hardware timer
 */
UTIL_TIMER_Status_t HW_TIMER_Stop(HW_TIMER_Object_t *TimerObject)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    if (TimerObject != NULL)
    {
        HW_TIMER_Object_t *prev = TimerListHead;
        HW_TIMER_Object_t *cur = TimerListHead;
        TimerObject->IsReloadStopped = 1U;

        if (TimerListHead != NULL)
        {
            TimerObject->IsRunning = 0U;

            if (TimerListHead == TimerObject)
            {
                TimerListHead->IsPending = 0;
                if (TimerListHead->Next != NULL)
                {
                    TimerListHead = TimerListHead->Next;
                    TimerSetTimeout(TimerListHead);
                }
                else
                {
                    /* Stop all active timers */
                    HAL_LPTIM_TimeOut_Stop_IT(&hlptim1);
                    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
                    TimerListHead = NULL;
                }
            }
            else
            {
                while (cur != NULL)
                {
                    if (cur == TimerObject)
                    {
                        if (cur->Next != NULL)
                        {
                            cur = cur->Next;
                            prev->Next = cur;
                        }
                        else
                        {
                            cur = NULL;
                            prev->Next = cur;
                        }
                        break;
                    }
                    else
                    {
                        prev = cur;
                        cur = cur->Next;
                    }
                }
            }

            ret = UTIL_TIMER_OK;
        }
    }
    else
    {
        ret = UTIL_TIMER_INVALID_PARAM;
    }

    return ret;
}

/**
 * @brief Set new timer period
 */
UTIL_TIMER_Status_t HW_TIMER_SetPeriod(HW_TIMER_Object_t *TimerObject,
                                       uint32_t NewPeriodValue)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    if (TimerObject == NULL)
    {
        ret = UTIL_TIMER_INVALID_PARAM;
    }
    else
    {
        TimerObject->ReloadValue = Convert_ms2Ticks(NewPeriodValue, TimerObject->TimerType);
        if (TimerExists(TimerObject))
        {
            (void)HW_TIMER_Stop(TimerObject);
            ret = HW_TIMER_Start(TimerObject);
        }
    }

    return ret;
}

/**
 * @brief Get remaining time on timer
 */
UTIL_TIMER_Status_t HW_TIMER_GetRemainingTime(HW_TIMER_Object_t *TimerObject,
                                              uint32_t *ElapsedTime)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    if (TimerExists(TimerObject))
    {
        uint32_t time = GetTimerValue(TimerObject->TimerType) - TimerContext;

        if (TimerObject->Timestamp < time)
        {
            *ElapsedTime = 0;
        }
        else
        {
            *ElapsedTime = TimerObject->Timestamp - time;
        }
    }
    else
    {
        ret = UTIL_TIMER_INVALID_PARAM;
    }

    return ret;
}

/**
 * @brief Check if timer is running
 */
uint32_t HW_TIMER_IsRunning(HW_TIMER_Object_t *TimerObject)
{
    if (TimerObject != NULL)
    {
        return TimerObject->IsRunning;
    }
    else
    {
        return 0;
    }
}

/**
 * @brief Get current hardware timer value
 */
uint32_t HW_TIMER_GetValue(HW_TIMER_Type_t TimerType)
{
    uint32_t value = 0;

    switch (TimerType)
    {
        case HW_TIMER_LPTIM1:
            value = LL_LPTIM_GetCounter(LPTIM1);
            break;

        case HW_TIMER_LPTIM2:
            value = LL_LPTIM_GetCounter(LPTIM2);
            break;

        case HW_TIMER_TIM2:
            value = LL_TIM_GetCounter(TIM2);
            break;

        case HW_TIMER_RTC:
            value = LL_RTC_TIME_GetSubSecond(RTC);
            break;

        default:
            break;
    }

    return value;
}

/**
 * @brief Configure specific hardware timer
 */
UTIL_TIMER_Status_t HW_TIMER_ConfigureTimer(HW_TIMER_Type_t TimerType,
                                            const HW_TIMER_Config_t *Config)
{
    UTIL_TIMER_Status_t ret = UTIL_TIMER_OK;

    switch (TimerType)
    {
        case HW_TIMER_LPTIM1:
            /* LPTIM1 is pre-configured for low-power operation */
            /* Clock: LSE 32 kHz */
            hlptim1.Instance = LPTIM1;
            hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LSECLK;
            hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV1;
            hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
            hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
            hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
            hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
            hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
            hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;

            if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
            {
                ret = UTIL_TIMER_HW_ERROR;
            }
            break;

        case HW_TIMER_TIM2:
            /* TIM2 is 32-bit general purpose timer */
            htim2.Instance = TIM2;
            htim2.Init.Prescaler = 31;  /* 32 MHz / 32 = 1 MHz */
            htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
            htim2.Init.Period = 0xFFFFFFFF;
            htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
            htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

            if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
            {
                ret = UTIL_TIMER_HW_ERROR;
            }
            break;

        case HW_TIMER_RTC:
            /* RTC is pre-initialized by HAL */
            ActiveTimerType = HW_TIMER_RTC;
            break;

        default:
            ret = UTIL_TIMER_INVALID_PARAM;
            break;
    }

    return ret;
}

/**
 * @brief Get low-power timer recommendation
 */
HW_TIMER_Type_t HW_TIMER_GetLowPowerTimer(void)
{
    /* LPTIM1 with LSE is the lowest power option */
    return HW_TIMER_LPTIM1;
}

/**
 * @brief Switch timer context to optimize power
 */
UTIL_TIMER_Status_t HW_TIMER_SwitchContext(HW_TIMER_Type_t PreferredTimer)
{
    if (PreferredTimer >= HW_TIMER_RTC)
    {
        return UTIL_TIMER_INVALID_PARAM;
    }

    ActiveTimerType = PreferredTimer;
    return UTIL_TIMER_OK;
}

/**
 * @brief Timer interrupt handler
 */
void HW_TIMER_IRQ_Handler(void)
{
    HW_TIMER_Object_t *cur;
    uint32_t old, now, DeltaContext;

    old = TimerContext;
    now = GetTimerValue(ActiveTimerType);
    TimerContext = now;

    DeltaContext = now - old; /* Intentional wrap around */

    /* Update timeStamp based upon new Time Reference */
    if (TimerListHead != NULL)
    {
        cur = TimerListHead;
        do
        {
            if (cur->Timestamp > DeltaContext)
            {
                cur->Timestamp -= DeltaContext;
            }
            else
            {
                cur->Timestamp = 0;
            }
            cur = cur->Next;
        } while (cur != NULL);
    }

    /* Execute expired timer and update the list */
    while ((TimerListHead != NULL) && (TimerListHead->Timestamp == 0U))
    {
        cur = TimerListHead;
        TimerListHead = TimerListHead->Next;
        cur->IsPending = 0;
        cur->IsRunning = 0;
        cur->Callback(cur->argument);

        if ((cur->Mode == UTIL_TIMER_PERIODIC) && (cur->IsReloadStopped == 0U))
        {
            (void)HW_TIMER_Start(cur);
        }
    }

    /* Start the next TimerListHead if it exists and it is not pending */
    if ((TimerListHead != NULL) && (TimerListHead->IsPending == 0U))
    {
        TimerSetTimeout(TimerListHead);
    }
}

/* Private functions --------------------------------------------------------*/

/**
 * @brief Select optimal timer based on duration and power
 */
static HW_TIMER_Type_t SelectOptimalTimer(uint32_t Duration)
{
    /* For short durations (<100ms), use general-purpose timer */
    if (Duration < 100)
    {
        return HW_TIMER_TIM2;
    }
    /* For medium durations, use LPTIM1 (best power efficiency) */
    else if (Duration < 10000)
    {
        return HW_TIMER_LPTIM1;
    }
    /* For long durations, use RTC */
    else
    {
        return HW_TIMER_RTC;
    }
}

/**
 * @brief Convert milliseconds to ticks based on timer type
 */
static uint32_t Convert_ms2Ticks(uint32_t Milliseconds, HW_TIMER_Type_t TimerType)
{
    uint32_t ticks = 0;

    switch (TimerType)
    {
        case HW_TIMER_LPTIM1:
        case HW_TIMER_LPTIM2:
            /* 32 kHz LSE = 32 ticks per millisecond */
            ticks = (Milliseconds * 32);
            break;

        case HW_TIMER_TIM2:
            /* Prescaled to 1 MHz = 1000 ticks per millisecond */
            ticks = (Milliseconds * 1000);
            break;

        case HW_TIMER_RTC:
            /* 1 Hz subsecond = 1/32768 second */
            ticks = (Milliseconds * 32);
            break;

        default:
            ticks = Milliseconds;
            break;
    }

    return ticks;
}

/**
 * @brief Convert ticks to milliseconds based on timer type
 */
static uint32_t Convert_Ticks2ms(uint32_t Ticks, HW_TIMER_Type_t TimerType)
{
    uint32_t ms = 0;

    switch (TimerType)
    {
        case HW_TIMER_LPTIM1:
        case HW_TIMER_LPTIM2:
            ms = (Ticks / 32);
            break;

        case HW_TIMER_TIM2:
            ms = (Ticks / 1000);
            break;

        case HW_TIMER_RTC:
            ms = (Ticks / 32);
            break;

        default:
            ms = Ticks;
            break;
    }

    return ms;
}

/**
 * @brief Check if timer exists in list
 */
static bool TimerExists(HW_TIMER_Object_t *TimerObject)
{
    HW_TIMER_Object_t *cur = TimerListHead;

    while (cur != NULL)
    {
        if (cur == TimerObject)
        {
            return true;
        }
        cur = cur->Next;
    }

    return false;
}

/**
 * @brief Insert timer at head of list
 */
static void TimerInsertNewHeadTimer(HW_TIMER_Object_t *TimerObject)
{
    HW_TIMER_Object_t *cur = TimerListHead;

    if (cur != NULL)
    {
        cur->IsPending = 0;
    }

    TimerObject->Next = cur;
    TimerListHead = TimerObject;
    TimerSetTimeout(TimerListHead);
}

/**
 * @brief Insert timer in sorted position in list
 */
static void TimerInsertTimer(HW_TIMER_Object_t *TimerObject)
{
    HW_TIMER_Object_t *cur = TimerListHead;
    HW_TIMER_Object_t *next = TimerListHead->Next;

    while (cur->Next != NULL)
    {
        if (TimerObject->Timestamp > next->Timestamp)
        {
            cur = next;
            next = next->Next;
        }
        else
        {
            cur->Next = TimerObject;
            TimerObject->Next = next;
            return;
        }
    }

    cur->Next = TimerObject;
    TimerObject->Next = NULL;
}

/**
 * @brief Set timeout on the head timer
 */
static void TimerSetTimeout(HW_TIMER_Object_t *TimerObject)
{
    uint32_t minTicks = MIN_ALARM_DELAY;
    TimerObject->IsPending = 1;

    /* Adjust timestamp if deadline is too soon */
    if (TimerObject->Timestamp < (GetTimerValue(TimerObject->TimerType) - TimerContext + minTicks))
    {
        TimerObject->Timestamp = GetTimerValue(TimerObject->TimerType) - TimerContext + minTicks;
    }

    /* Start hardware timer based on type */
    switch (TimerObject->TimerType)
    {
        case HW_TIMER_LPTIM1:
            HAL_LPTIM_TimeOut_Start_IT(&hlptim1, 65535, TimerObject->Timestamp);
            break;

        case HW_TIMER_TIM2:
            HAL_TIM_Base_Start_IT(&htim2);
            break;

        case HW_TIMER_RTC:
            {
                RTC_AlarmTypeDef sAlarm = {0};
                sAlarm.Alarm = RTC_ALARM_A;
                sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
                sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDBINMASK_NONE;
                HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, 0);
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Get inline timer value (optimized)
 */
static inline uint32_t GetTimerValue(HW_TIMER_Type_t TimerType)
{
    return HW_TIMER_GetValue(TimerType);
}

/**
  * @}
  */
