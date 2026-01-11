/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    test_rf.c
  * @author  MCD Application Team
  * @brief   manages tx tests
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
#include "test_rf.h"
#include "radio.h"
#include "stm32_seq.h"
#include "utilities_def.h"

/* USER CODE BEGIN Includes */
#include "subg_command.h"

/* USER CODE END Includes */

/* External variables ---------------------------------------------------------*/
/* USER CODE BEGIN EV */

/* USER CODE END EV */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
#define F_868MHz                      868000000
#define P_14dBm                       14
#define P_22dBm                       22
#define SF12                          12
#define CR4o5                         1
#define CR4o8                         4      /* Enhanced error correction (4/8) */
#define EMISSION_POWER                P_22dBm /* Optimized: Maximum TX power for 2-way link */
#define LORA_PREAMBLE_LENGTH          8         /* Same for Tx and Rx */
#define LORA_SYMBOL_TIMEOUT           30        /* Symbols */
#define TX_TIMEOUT_VALUE              3000
#define RX_CONTINUOUS_ON              1
#define DEFAULT_PAYLOAD_LEN           16
#define DEFAULT_LDR_OPT               2
#define DEFAULT_FSK_DEVIATION         25000
#define DEFAULT_GAUSS_BT              3 /*Lora default in legacy*/
#define DEFAULT_LNA_ENABLED           1 /* Optimized: Enable Low Noise Amplifier (+10dB) */
#define DEFAULT_PABOOST_ENABLED       1 /* Optimized: Enable PA Boost for higher TX power */

/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* Optimized for maximum receive distance with RC_GUARD PWM signals */
static testParameter_t testParam = {
    TEST_LORA,                      /* Modulation: LoRa (best range) */
    F_868MHz,                       /* Frequency: 868 MHz EU ISM band */
    EMISSION_POWER,                 /* TX Power: 22 dBm (optimized for 2-way link) */
    BW_125kHz,                      /* Bandwidth: 125 kHz (balanced sensitivity/speed) */
    SF12,                           /* Spreading Factor: 12 (maximum range) */
    CR4o8,                          /* Coding Rate: 4/8 (enhanced error correction) */
    DEFAULT_LNA_ENABLED,            /* LNA: Enabled (+10dB frontend gain) */
    DEFAULT_PABOOST_ENABLED,        /* PA Boost: Enabled (supports 22 dBm TX) */
    DEFAULT_PAYLOAD_LEN,            /* Payload: 16 bytes */
    DEFAULT_FSK_DEVIATION,          /* FSK Dev: 25 kHz (N/A for LoRa) */
    DEFAULT_LDR_OPT,                /* Low Data Rate Opt: Auto for SF12 */
    DEFAULT_GAUSS_BT                /* BT Product: 3 (N/A for LoRa) */
};

static __IO uint32_t RadioTxDone_flag = 0;
static __IO uint32_t RadioTxTimeout_flag = 0;
static __IO uint32_t RadioRxDone_flag = 0;
static __IO uint32_t RadioRxTimeout_flag = 0;
static __IO uint32_t RadioError_flag = 0;
static __IO int16_t last_rx_rssi = 0;
static __IO int8_t last_rx_LoraSnr_FskCfo = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone(void);

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout(void);

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError(void);

/* USER CODE BEGIN PFP */
void TST_Radio_Init(void);
int32_t TST_Radio_ConfigTx(TxConfigGeneric_t *txConfig);
int32_t TST_Radio_ConfigRx(RxConfigGeneric_t *rxConfig);
int32_t TST_Radio_Send(const uint8_t *data, uint16_t size);

/* USER CODE END PFP */

/* Exported functions --------------------------------------------------------*/
void TST_Radio_Init(void)
{
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  Radio.Init(&RadioEvents);
}

int32_t TST_Radio_ConfigTx(TxConfigGeneric_t *txConfig)
{
  static const uint8_t syncword[] = { 0xC1, 0x94, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00 };

  if (txConfig == NULL)
  {
    return -1;
  }

  Radio.SetChannel(testParam.freq);
  UTIL_MEM_set_8(txConfig, 0, sizeof(*txConfig));

  if (testParam.modulation == TEST_FSK)
  {
    /*fsk modulation*/
    txConfig->fsk.ModulationShaping =
      (RADIO_FSK_ModShapings_t)((testParam.BTproduct == 0) ? 0 : testParam.BTproduct + 7);
    txConfig->fsk.FrequencyDeviation = testParam.fskDev;
    txConfig->fsk.BitRate = testParam.loraSf_datarate; /*BitRate*/
    txConfig->fsk.PreambleLen = 3;   /*in Byte        */
    txConfig->fsk.SyncWordLength = 3; /*in Byte        */
    txConfig->fsk.SyncWord = syncword; /*SyncWord Buffer*/
    txConfig->fsk.whiteSeed = 0x01FF; /*WhiteningSeed  */
    txConfig->fsk.HeaderType = RADIO_FSK_PACKET_VARIABLE_LENGTH; /* If the header is explicit, it will be transmitted in the GFSK packet. If the header is implicit, it will not be transmitted*/
    txConfig->fsk.CrcLength = RADIO_FSK_CRC_2_BYTES_CCIT;       /* Size of the CRC block in the GFSK packet*/
    txConfig->fsk.CrcPolynomial = 0x1021;
    txConfig->fsk.Whitening = RADIO_FSK_DC_FREE_OFF;
    Radio.RadioSetTxGenericConfig(GENERIC_FSK, txConfig, testParam.power, TX_TIMEOUT_VALUE);
  }
  else if (testParam.modulation == TEST_MSK)
  {
    /*fsk modulation*/
    txConfig->msk.ModulationShaping =
      (RADIO_FSK_ModShapings_t)((testParam.BTproduct == 0) ? 0 : testParam.BTproduct + 7);
    txConfig->msk.BitRate = testParam.loraSf_datarate; /*BitRate*/
    txConfig->msk.PreambleLen = 3;   /*in Byte        */
    txConfig->msk.SyncWordLength = 3; /*in Byte        */
    txConfig->msk.SyncWord = syncword; /*SyncWord Buffer*/
    txConfig->msk.whiteSeed = 0x01FF; /*WhiteningSeed  */
    txConfig->msk.HeaderType = RADIO_FSK_PACKET_VARIABLE_LENGTH; /* If the header is explicit, it will be transmitted in the GFSK packet. If the header is implicit, it will not be transmitted*/
    txConfig->msk.CrcLength = RADIO_FSK_CRC_2_BYTES_CCIT;       /* Size of the CRC block in the GFSK packet*/
    txConfig->msk.CrcPolynomial = 0x1021;
    txConfig->msk.Whitening = RADIO_FSK_DC_FREE_OFF;
    Radio.RadioSetTxGenericConfig(GENERIC_MSK, txConfig, testParam.power, TX_TIMEOUT_VALUE);
  }
  else if (testParam.modulation == TEST_LORA)
  {
    /*lora modulation*/
    txConfig->lora.Bandwidth = (RADIO_LoRaBandwidths_t) testParam.bandwidth;
    txConfig->lora.SpreadingFactor = (RADIO_LoRaSpreadingFactors_t) testParam.loraSf_datarate; /*BitRate*/
    txConfig->lora.Coderate = (RADIO_LoRaCodingRates_t)testParam.codingRate;
    txConfig->lora.LowDatarateOptimize = (RADIO_Ld_Opt_t)testParam.lowDrOpt; /*0 inactive, 1 active, 2: auto*/
    txConfig->lora.PreambleLen = LORA_PREAMBLE_LENGTH;
    txConfig->lora.LengthMode = RADIO_LORA_PACKET_VARIABLE_LENGTH;
    txConfig->lora.CrcMode = RADIO_LORA_CRC_ON;
    txConfig->lora.IqInverted = RADIO_LORA_IQ_NORMAL;
    Radio.RadioSetTxGenericConfig(GENERIC_LORA, txConfig, testParam.power, TX_TIMEOUT_VALUE);
    Radio.SetPublicNetwork(false); /*set private syncword*/
  }
  else if (testParam.modulation == TEST_BPSK)
  {
    txConfig->bpsk.BitRate = testParam.loraSf_datarate; /*BitRate*/
    Radio.RadioSetTxGenericConfig(GENERIC_BPSK, txConfig, testParam.power, TX_TIMEOUT_VALUE);
  }
  else
  {
    return -1;
  }

  return 0;
}

int32_t TST_Radio_ConfigRx(RxConfigGeneric_t *rxConfig)
{
  static const uint8_t syncword[] = { 0xC1, 0x94, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x00 };

  if (rxConfig == NULL)
  {
    return -1;
  }

  Radio.SetChannel(testParam.freq);
  UTIL_MEM_set_8(rxConfig, 0, sizeof(*rxConfig));

  if (testParam.modulation == TEST_FSK)
  {
    /*fsk modulation*/
    rxConfig->fsk.ModulationShaping =
      (RADIO_FSK_ModShapings_t)((testParam.BTproduct == 0) ? 0 : testParam.BTproduct + 8);
    rxConfig->fsk.Bandwidth = testParam.bandwidth;
    rxConfig->fsk.BitRate = testParam.loraSf_datarate; /*BitRate*/
    rxConfig->fsk.PreambleLen = 3; /*in Byte*/
    rxConfig->fsk.SyncWordLength = 3; /*in Byte*/
    rxConfig->fsk.SyncWord = syncword; /*SyncWord Buffer*/
    rxConfig->fsk.PreambleMinDetect = RADIO_FSK_PREAMBLE_DETECTOR_08_BITS;
    rxConfig->fsk.whiteSeed = 0x01FF; /*WhiteningSeed*/
    rxConfig->fsk.LengthMode = RADIO_FSK_PACKET_VARIABLE_LENGTH; /* If the header is explicit, it will be transmitted in the GFSK packet. If the header is implicit, it will not be transmitted*/
    rxConfig->fsk.CrcLength = RADIO_FSK_CRC_2_BYTES_CCIT;       /* Size of the CRC block in the GFSK packet*/
    rxConfig->fsk.CrcPolynomial = 0x1021;
    rxConfig->fsk.Whitening = RADIO_FSK_DC_FREE_OFF;
    rxConfig->fsk.MaxPayloadLength = 255;
    rxConfig->fsk.AddrComp = RADIO_FSK_ADDRESSCOMP_FILT_OFF;
    Radio.RadioSetRxGenericConfig(GENERIC_FSK, rxConfig, RX_CONTINUOUS_ON, 0);
  }
  else if (testParam.modulation == TEST_LORA)
  {
    /*Lora*/
    rxConfig->lora.Bandwidth = (RADIO_LoRaBandwidths_t) testParam.bandwidth;
    rxConfig->lora.SpreadingFactor = (RADIO_LoRaSpreadingFactors_t) testParam.loraSf_datarate; /*BitRate*/
    rxConfig->lora.Coderate = (RADIO_LoRaCodingRates_t)testParam.codingRate;
    rxConfig->lora.LowDatarateOptimize = (RADIO_Ld_Opt_t)testParam.lowDrOpt; /*0 inactive, 1 active, 2: auto*/
    rxConfig->lora.PreambleLen = LORA_PREAMBLE_LENGTH;
    rxConfig->lora.LengthMode = RADIO_LORA_PACKET_VARIABLE_LENGTH;
    rxConfig->lora.CrcMode = RADIO_LORA_CRC_ON;
    rxConfig->lora.IqInverted = RADIO_LORA_IQ_NORMAL;
    Radio.RadioSetRxGenericConfig(GENERIC_LORA, rxConfig, RX_CONTINUOUS_ON, LORA_SYMBOL_TIMEOUT);
    Radio.SetPublicNetwork(false); /*set private syncword*/
  }
  else
  {
    return -1;
  }

  return 0;
}

int32_t TST_Radio_Send(const uint8_t *data, uint16_t size)
{
  if (size == 0U)
  {
    return 0;
  }

  if (data == NULL)
  {
    return -1;
  }

  Radio.Send((uint8_t *)data, size);
  return 0;
}

/* USER CODE BEGIN EF */

/* USER CODE END EF */

/* Private Functions Definition -----------------------------------------------*/

void OnTxDone(void)
{
  /* USER CODE BEGIN OnTxDone_1 */

  /* USER CODE END OnTxDone_1 */
  /* Set TxDone flag */
  RadioTxDone_flag = 1;
  UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  /* USER CODE BEGIN OnTxDone_2 */

  /* USER CODE END OnTxDone_2 */
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t LoraSnr_FskCfo)
{
  /* USER CODE BEGIN OnRxDone_1 */

  /* USER CODE END OnRxDone_1 */
  last_rx_rssi = rssi;
  last_rx_LoraSnr_FskCfo = LoraSnr_FskCfo;

  if ((payload != NULL) && (size > 0U))
  {
    CMD_ReceiveBuffer(payload, size);
  }

  /* Set Rxdone flag */
  RadioRxDone_flag = 1;
  UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  /* USER CODE BEGIN OnRxDone_2 */

  /* USER CODE END OnRxDone_2 */
}

void OnTxTimeout(void)
{
  /* USER CODE BEGIN OnTxTimeout_1 */

  /* USER CODE END OnTxTimeout_1 */
  /* Set timeout flag */
  RadioTxTimeout_flag = 1;
  UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  /* USER CODE BEGIN OnTxTimeout_2 */

  /* USER CODE END OnTxTimeout_2 */
}

void OnRxTimeout(void)
{
  /* USER CODE BEGIN OnRxTimeout_1 */

  /* USER CODE END OnRxTimeout_1 */
  /* Set timeout flag */
  RadioRxTimeout_flag = 1;
  UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  /* USER CODE BEGIN OnRxTimeout_2 */

  /* USER CODE END OnRxTimeout_2 */
}

void OnRxError(void)
{
  /* USER CODE BEGIN OnRxError_1 */

  /* USER CODE END OnRxError_1 */
  /* Set error flag */
  RadioError_flag = 1;
  UTIL_SEQ_SetEvt(1 << CFG_SEQ_Evt_RadioOnTstRF);
  /* USER CODE BEGIN OnRxError_2 */

  /* USER CODE END OnRxError_2 */
}

/* USER CODE BEGIN PrFD */

/* USER CODE END PrFD */
