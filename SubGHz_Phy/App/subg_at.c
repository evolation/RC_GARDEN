/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    subg_at.c
  * @author  MCD Application Team
  * @brief   AT command API
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
#include "platform.h"
#include "subg_at.h"
#include "sys_app.h"
#include "stm32_tiny_sscanf.h"
#include "app_version.h"
#include "subghz_phy_version.h"
#include "stm32_seq.h"
#include "utilities_def.h"
#include "radio.h"
#include "stm32_timer.h"
#include "stm32_systime.h"
#include "pwm_if.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/**
  * @brief  Print an unsigned int
  * @param  value to print
  */
static void print_u(uint32_t value);

/**
  * @brief  Check if character in parameter is alphanumeric
  * @param  Char for the alphanumeric check
  */
static int32_t isHex(char Char);

/**
  * @brief  Converts hex string to a nibble ( 0x0X )
  * @param  Char hex string
  * @retval the nibble. Returns 0xF0 in case input was not an hex string (a..f; A..F or 0..9)
  */
static uint8_t Char2Nibble(char Char);

/**
  * @brief  Convert a string into a buffer of data
  * @param  str string to convert
  * @param  data output buffer
  * @param  Size of input string
  */
static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/
ATEerror_t AT_return_ok(const char *param)
{
  return AT_OK;
}

ATEerror_t AT_return_error(const char *param)
{
  return AT_ERROR;
}

/* --------------- General commands --------------- */
ATEerror_t AT_version_get(const char *param)
{
  /* USER CODE BEGIN AT_version_get_1 */

  /* USER CODE END AT_version_get_1 */

  /* Get LoRa APP version*/
  AT_PRINTF("APPLICATION_VERSION: V%X.%X.%X\r\n",
            (uint8_t)(APP_VERSION_MAIN),
            (uint8_t)(APP_VERSION_SUB1),
            (uint8_t)(APP_VERSION_SUB2));

  /* Get MW SubGhz_Phy info */
  AT_PRINTF("MW_RADIO_VERSION:    V%X.%X.%X\r\n",
            (uint8_t)(SUBGHZ_PHY_VERSION_MAIN),
            (uint8_t)(SUBGHZ_PHY_VERSION_SUB1),
            (uint8_t)(SUBGHZ_PHY_VERSION_SUB2));

  return AT_OK;
  /* USER CODE BEGIN AT_version_get_2 */

  /* USER CODE END AT_version_get_2 */
}

ATEerror_t AT_verbose_get(const char *param)
{
  /* USER CODE BEGIN AT_verbose_get_1 */

  /* USER CODE END AT_verbose_get_1 */
  print_u(UTIL_ADV_TRACE_GetVerboseLevel());
  return AT_OK;
  /* USER CODE BEGIN AT_verbose_get_2 */

  /* USER CODE END AT_verbose_get_2 */
}

ATEerror_t AT_verbose_set(const char *param)
{
  /* USER CODE BEGIN AT_verbose_set_1 */

  /* USER CODE END AT_verbose_set_1 */
  const char *buf = param;
  int32_t lvl_nb;

  /* read and set the verbose level */
  if (1 != tiny_sscanf(buf, "%u", &lvl_nb))
  {
    AT_PRINTF("AT+VL: verbose level is not well set\r\n");
    return AT_PARAM_ERROR;
  }
  if ((lvl_nb > VLEVEL_H) || (lvl_nb < VLEVEL_OFF))
  {
    AT_PRINTF("AT+VL: verbose level out of range => 0(VLEVEL_OFF) to 3(VLEVEL_H)\r\n");
    return AT_PARAM_ERROR;
  }

  UTIL_ADV_TRACE_SetVerboseLevel(lvl_nb);

  return AT_OK;
  /* USER CODE BEGIN AT_verbose_set_2 */

  /* USER CODE END AT_verbose_set_2 */
}

ATEerror_t AT_LocalTime_get(const char *param)
{
  /* USER CODE BEGIN AT_LocalTime_get_1 */

  /* USER CODE END AT_LocalTime_get_1 */
  struct tm localtime;
  SysTime_t UnixEpoch = SysTimeGet();
  UnixEpoch.Seconds -= 18; /*removing leap seconds*/

  UnixEpoch.Seconds += 3600 * 2; /*adding 2 hours*/

  SysTimeLocalTime(UnixEpoch.Seconds,  & localtime);

  AT_PRINTF("LTIME:%02dh%02dm%02ds on %02d/%02d/%04d\r\n",
            localtime.tm_hour, localtime.tm_min, localtime.tm_sec,
            localtime.tm_mday, localtime.tm_mon + 1, localtime.tm_year + 1900);

  return AT_OK;
  /* USER CODE BEGIN AT_LocalTime_get_2 */

  /* USER CODE END AT_LocalTime_get_2 */
}

ATEerror_t AT_reset(const char *param)
{
  /* USER CODE BEGIN AT_reset_1 */

  /* USER CODE END AT_reset_1 */
  NVIC_SystemReset();
  /* USER CODE BEGIN AT_reset_2 */

  /* USER CODE END AT_reset_2 */
}

/* --------------- Radio tests commands --------------- */
ATEerror_t AT_test_txTone(const char *param)
{
  /* USER CODE BEGIN AT_test_txTone_1 */

  /* USER CODE END AT_test_txTone_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_txTone_2 */

  /* USER CODE END AT_test_txTone_2 */
}

ATEerror_t AT_test_rxRssi(const char *param)
{
  /* USER CODE BEGIN AT_test_rxRssi_1 */

  /* USER CODE END AT_test_rxRssi_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_rxRssi_2 */

  /* USER CODE END AT_test_rxRssi_2 */
}

ATEerror_t AT_test_get_config(const char *param)
{
  /* USER CODE BEGIN AT_test_get_config_1 */

  /* USER CODE END AT_test_get_config_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_get_config_2 */

  /* USER CODE END AT_test_get_config_2 */
}

ATEerror_t AT_test_set_config(const char *param)
{
  /* USER CODE BEGIN AT_test_set_config_1 */

  /* USER CODE END AT_test_set_config_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_set_config_2 */

  /* USER CODE END AT_test_set_config_2 */
}

ATEerror_t AT_test_tx(const char *param)
{
  /* USER CODE BEGIN AT_test_tx_1 */

  /* USER CODE END AT_test_tx_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_tx_2 */

  /* USER CODE END AT_test_tx_2 */
}

ATEerror_t AT_test_rx(const char *param)
{
  /* USER CODE BEGIN AT_test_rx_1 */

  /* USER CODE END AT_test_rx_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_rx_2 */

  /* USER CODE END AT_test_rx_2 */
}

ATEerror_t AT_test_tx_hopping(const char *param)
{
  /* USER CODE BEGIN AT_test_tx_hopping_1 */

  /* USER CODE END AT_test_tx_hopping_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_tx_hopping_2 */

  /* USER CODE END AT_test_tx_hopping_2 */
}

ATEerror_t AT_test_stop(const char *param)
{
  /* USER CODE BEGIN AT_test_stop_1 */

  /* USER CODE END AT_test_stop_1 */
  (void)param;
  return AT_ERROR;
  /* USER CODE BEGIN AT_test_stop_2 */

  /* USER CODE END AT_test_stop_2 */
}

/* --------------- Radio access commands --------------- */
ATEerror_t AT_write_register(const char *param)
{
  /* USER CODE BEGIN AT_write_register_1 */

  /* USER CODE END AT_write_register_1 */
  uint8_t add[2];
  uint16_t add16;
  uint8_t data;

  if (strlen(param) != 7)
  {
    return AT_PARAM_ERROR;
  }

  if (stringToData(param, add, 2) != 0)
  {
    return AT_PARAM_ERROR;
  }
  param += 5;
  if (stringToData(param, &data, 1) != 0)
  {
    return AT_PARAM_ERROR;
  }
  add16 = (((uint16_t)add[0]) << 8) + (uint16_t)add[1];
  Radio.Write(add16, data);

  return AT_OK;
  /* USER CODE BEGIN AT_write_register_2 */

  /* USER CODE END AT_write_register_2 */
}

ATEerror_t AT_read_register(const char *param)
{
  /* USER CODE BEGIN AT_read_register_1 */

  /* USER CODE END AT_read_register_1 */
  uint8_t add[2];
  uint16_t add16;
  uint8_t data;

  if (strlen(param) != 4)
  {
    return AT_PARAM_ERROR;
  }

  if (stringToData(param, add, 2) != 0)
  {
    return AT_PARAM_ERROR;
  }

  add16 = (((uint16_t)add[0]) << 8) + (uint16_t)add[1];
  data = Radio.Read(add16);
  AT_PRINTF("REG 0x%04X=0x%02X", add16, data);

  return AT_OK;
  /* USER CODE BEGIN AT_read_register_2 */

  /* USER CODE END AT_read_register_2 */
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/
static void print_u(uint32_t value)
{
  /* USER CODE BEGIN print_u_1 */

  /* USER CODE END print_u_1 */
  AT_PRINTF("%u\r\n", value);
  /* USER CODE BEGIN print_u_2 */

  /* USER CODE END print_u_2 */
}

static uint8_t Char2Nibble(char Char)
{
  if (((Char >= '0') && (Char <= '9')))
  {
    return Char - '0';
  }
  else if (((Char >= 'a') && (Char <= 'f')))
  {
    return Char - 'a' + 10;
  }
  else if ((Char >= 'A') && (Char <= 'F'))
  {
    return Char - 'A' + 10;
  }
  else
  {
    return 0xF0;
  }
  /* USER CODE BEGIN CertifSend_2 */

  /* USER CODE END CertifSend_2 */
}

static int32_t stringToData(const char *str, uint8_t *data, uint32_t Size)
{
  /* USER CODE BEGIN stringToData_1 */

  /* USER CODE END stringToData_1 */
  char hex[3];
  hex[2] = 0;
  int32_t ii = 0;
  while (Size-- > 0)
  {
    hex[0] = *str++;
    hex[1] = *str++;

    /*check if input is hex */
    if ((isHex(hex[0]) == -1) || (isHex(hex[1]) == -1))
    {
      return -1;
    }
    /*check if input is even nb of character*/
    if ((hex[1] == '\0') || (hex[1] == ','))
    {
      return -1;
    }
    data[ii] = (Char2Nibble(hex[0]) << 4) + Char2Nibble(hex[1]);

    ii++;
  }

  return 0;
  /* USER CODE BEGIN stringToData_2 */

  /* USER CODE END stringToData_2 */
}

static int32_t isHex(char Char)
{
  /* USER CODE BEGIN isHex_1 */

  /* USER CODE END isHex_1 */
  if (((Char >= '0') && (Char <= '9')) ||
      ((Char >= 'a') && (Char <= 'f')) ||
      ((Char >= 'A') && (Char <= 'F')))
  {
    return 0;
  }
  else
  {
    return -1;
  }
  /* USER CODE BEGIN isHex_2 */

  /* USER CODE END isHex_2 */
}

/* --------------- PWM commands --------------- */
ATEerror_t AT_pwm1_get(const char *param)
{
  /* USER CODE BEGIN AT_pwm1_get_1 */

  /* USER CODE END AT_pwm1_get_1 */
  uint32_t duty_cycle = 0;

  if (PWM_GetDutyCycle(PWM_CHANNEL_1, &duty_cycle) == PWM_OK)
  {
    AT_PRINTF("+PWM1:%lu\r\n", duty_cycle);
    return AT_OK;
  }
  else
  {
    return AT_ERROR;
  }

  /* USER CODE BEGIN AT_pwm1_get_2 */

  /* USER CODE END AT_pwm1_get_2 */
}

ATEerror_t AT_pwm1_set(const char *param)
{
  /* USER CODE BEGIN AT_pwm1_set_1 */

  /* USER CODE END AT_pwm1_set_1 */
  uint32_t duty_cycle = 0;
  const char *buf = param;

  if (tiny_sscanf(buf, "%lu", &duty_cycle) != 1)
  {
    return AT_PARAM_ERROR;
  }

  if (PWM_SetDutyCycle(PWM_CHANNEL_1, duty_cycle) != PWM_OK)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;

  /* USER CODE BEGIN AT_pwm1_set_2 */

  /* USER CODE END AT_pwm1_set_2 */
}

ATEerror_t AT_pwm2_get(const char *param)
{
  /* USER CODE BEGIN AT_pwm2_get_1 */

  /* USER CODE END AT_pwm2_get_1 */
  uint32_t duty_cycle = 0;

  if (PWM_GetDutyCycle(PWM_CHANNEL_2, &duty_cycle) == PWM_OK)
  {
    AT_PRINTF("+PWM2:%lu\r\n", duty_cycle);
    return AT_OK;
  }
  else
  {
    return AT_ERROR;
  }

  /* USER CODE BEGIN AT_pwm2_get_2 */

  /* USER CODE END AT_pwm2_get_2 */
}

ATEerror_t AT_pwm2_set(const char *param)
{
  /* USER CODE BEGIN AT_pwm2_set_1 */

  /* USER CODE END AT_pwm2_set_1 */
  uint32_t duty_cycle = 0;
  const char *buf = param;

  if (tiny_sscanf(buf, "%lu", &duty_cycle) != 1)
  {
    return AT_PARAM_ERROR;
  }

  if (PWM_SetDutyCycle(PWM_CHANNEL_2, duty_cycle) != PWM_OK)
  {
    return AT_PARAM_ERROR;
  }

  return AT_OK;

  /* USER CODE BEGIN AT_pwm2_set_2 */

  /* USER CODE END AT_pwm2_set_2 */
}

/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
