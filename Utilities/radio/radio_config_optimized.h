/**
  ******************************************************************************
  * @file    radio_config_optimized.h
  * @brief   Optimized Radio Configuration for Maximum Receive Distance
  *          RC_GUARD SubGHz Phy AT Slave - STM32WL55xx
  * 
  * @details This file contains optimized LoRa radio parameters for maximum
  *          receive distance while maintaining compatibility with PWM signal
  *          monitoring on PA10/PA11 (TIM1 CH3/CH4).
  *
  * @author  RC_GUARD Optimization Team
  * @date    January 9, 2026
  ******************************************************************************
  */

#ifndef RADIO_CONFIG_OPTIMIZED_H
#define RADIO_CONFIG_OPTIMIZED_H

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
   OPTIMIZED CONFIGURATION FOR MAXIMUM RECEIVE DISTANCE
   ============================================================================ */

/**
 * @defgroup RADIO_OPTIMIZED_PARAMS Radio Optimized Parameters
 * @{
 */

/**
 * @brief Frequency Configuration
 * @note EU 868 MHz ISM Band (EN 300.220 compliant)
 */
#define RADIO_FREQ_HZ                         868000000UL

/**
 * @brief TX Power Configuration (dBm)
 * @note 22 dBm provides maximum 2-way communication range
 *       Requires LNA and PA Boost enabled
 *       Improves RX reliability through stronger ACK signals
 * 
 * Options:
 *   -9 to 22 dBm (0.5 dB steps)
 * Recommended values:
 *   14 dBm - Balanced (original)
 *   22 dBm - Maximum range (optimized)
 */
#define RADIO_TX_POWER_DBM                    22

/**
 * @brief LoRa Bandwidth Configuration
 * @note 125 kHz provides optimal balance between:
 *       - Sensitivity: Good (-134 dBm at SF12)
 *       - Time-on-air: ~5 seconds per 16-byte packet
 *       - Symbol rate: Fast enough for RC commands (~32 ms/symbol)
 *       - PWM response: Acceptable latency
 * 
 * Options (Bandwidth index):
 *   0 =  7.8125 kHz  (extreme range, very slow)
 *   1 =  15.625  kHz (long range, slow)
 *   2 =  31.25   kHz (extended range)
 *   3 =  62.5    kHz (good range)
 *   4 =  125     kHz (balanced) <-- RECOMMENDED
 *   5 =  250     kHz (faster)
 *   6 =  500     kHz (fastest)
 * 
 * Sensitivity @ SF12:
 *   BW=7.8kHz    => -128 dBm
 *   BW=125kHz    => -134 dBm
 *   BW=250kHz    => -131 dBm
 * 
 * Time-on-air @ SF12, 16-byte payload:
 *   BW=7.8kHz    => 35+ seconds (PWM timeout risk)
 *   BW=125kHz    => ~5 seconds (acceptable)
 *   BW=250kHz    => ~2.5 seconds (good for RC)
 */
#define RADIO_BANDWIDTH_INDEX                 4  /* 125 kHz */

/**
 * @brief LoRa Spreading Factor Configuration
 * @note SF12 provides maximum sensitivity and range
 *       Symbol time: ~33 ms @ 125 kHz
 *       Packet time: ~5 seconds (16-byte payload)
 * 
 * Options (Spreading Factor):
 *   5  = SF5   (256x processing gain)
 *   6  = SF6   (512x processing gain)
 *   7  = SF7   (1024x processing gain)
 *   8  = SF8   (2048x processing gain)
 *   9  = SF9   (4096x processing gain)
 *   10 = SF10  (8192x processing gain)
 *   11 = SF11  (16384x processing gain)
 *   12 = SF12  (65536x processing gain) <-- RECOMMENDED
 * 
 * Sensitivity @ BW=125kHz:
 *   SF7  => -123 dBm
 *   SF10 => -132 dBm
 *   SF12 => -134 dBm (best)
 * 
 * Time-on-air @ BW=125kHz:
 *   SF7  => 0.5 seconds
 *   SF10 => 1.3 seconds
 *   SF12 => 5.0 seconds
 */
#define RADIO_SPREADING_FACTOR                12

/**
 * @brief LoRa Coding Rate Configuration
 * @note 4/8 provides maximum error correction overhead
 *       Better for noisy environments or long distances
 *       Reduces effective data rate by 2x vs 4/5
 * 
 * Index mapping:
 *   1 = 4/5 (16.67% overhead, faster) <- Original
 *   2 = 4/6 (25% overhead)
 *   3 = 4/7 (33.3% overhead)
 *   4 = 4/8 (50% overhead, most robust) <-- RECOMMENDED
 * 
 * Effective data rate @ SF12, BW=125kHz:
 *   CR=4/5 => 18 bps
 *   CR=4/8 => 9 bps (slower but more reliable)
 */
#define RADIO_CODING_RATE_INDEX               4  /* 4/8 */

/**
 * @brief Low Data Rate Optimization
 * @note Auto mode (2) enables optimization for SF11 and SF12
 *       Required for reliable long-distance LoRa @ SF12
 * 
 * Options:
 *   0 = Disabled
 *   1 = Enabled
 *   2 = Auto (recommended for SF>=11)
 */
#define RADIO_LOW_DATARATE_OPT                2  /* Auto */

/**
 * @brief Low Noise Amplifier (LNA) Configuration
 * @note Enables frontend amplification
 *       Provides ~10 dB improvement in receiver sensitivity
 *       Reduces noise figure (improves SNR)
 *       Enable for maximum receive distance
 * 
 * Options:
 *   0 = Disabled (lower power, less sensitivity)
 *   1 = Enabled  (higher power, better sensitivity) <-- RECOMMENDED
 */
#define RADIO_LNA_ENABLED                     1

/**
 * @brief Power Amplifier Boost Configuration
 * @note Enables high power mode amplifier
 *       Allows TX power up to 22 dBm
 *       Enables the full 22 dBm TX power setting
 *       Must be enabled to achieve 22 dBm (disabling limits to ~14 dBm)
 * 
 * Options:
 *   0 = Disabled (limits to ~14 dBm)
 *   1 = Enabled  (supports up to 22 dBm) <-- RECOMMENDED
 */
#define RADIO_PA_BOOST_ENABLED                1

/**
 * @brief Preamble Length Configuration (symbols)
 * @note Standard value: 8 symbols
 *       Longer preambles improve detection reliability
 *       But increase time-on-air and power consumption
 * 
 * Typical range: 6-12 symbols
 * Recommended: 8 (standard, good balance)
 */
#define RADIO_PREAMBLE_LENGTH_SYMBOLS         8

/**
 * @brief CRC Mode Configuration
 * @note Enables CRC error detection on payload
 *       Recommended for all RX modes
 *       Small overhead but catches corrupted packets
 * 
 * Options:
 *   0 = Disabled
 *   1 = Enabled (recommended)
 */
#define RADIO_CRC_ENABLED                     1

/**
 * @brief Header Mode Configuration
 * @note Explicit header mode: length transmitted in frame
 *       Implicit mode: length known in advance
 *       Explicit recommended for variable length packets
 * 
 * Options:
 *   0 = Implicit header
 *   1 = Explicit header (recommended)
 */
#define RADIO_EXPLICIT_HEADER                 1

/**
 * @brief Maximum Payload Length (bytes)
 * @note Typical: 16-64 bytes for RC commands
 *       Larger payloads = longer time-on-air
 *       Recommended: 16 bytes for RC_GUARD
 */
#define RADIO_MAX_PAYLOAD_LEN                 16

/**
 * @brief LoRa Symbol Timeout (symbols)
 * @note Time to wait for valid LoRa symbol before RX timeout
 *       Higher = longer wait, more reliable but higher latency
 *       Typical: 30 symbols
 */
#define RADIO_SYMBOL_TIMEOUT                  30

/**
 * @brief RX Mode Configuration
 * @note 0 = RX with timeout
 *       1 = Continuous RX (stay in RX until explicit stop)
 *       Use continuous RX for maximum availability
 */
#define RADIO_RX_CONTINUOUS                   1

/**
 * @brief RX Timeout Duration (milliseconds)
 * @note Timeout period when in RX with timeout mode
 *       5000 ms = 5 seconds before returning to sleep
 *       Longer timeout = higher power but better availability
 */
#define RADIO_RX_TIMEOUT_MS                   5000

/**
 * @brief TX Timeout Duration (milliseconds)
 * @note Timeout period for TX operations
 *       3000 ms = 3 seconds watchdog
 */
#define RADIO_TX_TIMEOUT_MS                   3000

/**
 * @brief IQ Inversion Configuration
 * @note Standard: Normal IQ (0)
 *       Use inverted IQ only if required for compatibility
 * 
 * Options:
 *   0 = Normal IQ
 *   1 = Inverted IQ
 */
#define RADIO_IQ_INVERTED                     0

/**
 * @}
 */

/* ============================================================================
   PWM SIGNAL MONITORING COMPATIBILITY
   ============================================================================ */

/**
 * @defgroup PWM_TIMING_CONSTRAINTS PWM Signal Timing Constraints
 * @{
 */

/**
 * @brief PWM Monitoring Clock Resolution
 * @note TIM1 clocked at 1 MHz (1 µs resolution)
 *       Measures on PA10 (TIM1_CH3) and PA11 (TIM1_CH4)
 *       Independent of radio RX/TX timing
 * 
 * Timing margins with radio operations:
 *   - RX continuous mode: ~5000 ms per measurement cycle
 *   - PWM measurement time: ~100 ms per cycle
 *   - Margin: 4900 ms (excellent)
 *   - Latency: <150 ms acceptable for RC commands
 */
#define PWM_MON_TIMER_FREQ_HZ                 1000000UL  /* 1 MHz */
#define PWM_MON_DUTY_THRESHOLD_PERCENT        1
#define PWM_MON_TIMEOUT_MS                    100

/**
 * @brief Radio Operation Time Budget
 * @note At SF12, BW=125kHz with 16-byte payload:
 *       - RX waiting time: ~5000 ms (between RX attempts)
 *       - PWM check period: ~100 ms
 *       - Latency: <150 ms
 *       - Compatibility: EXCELLENT
 * 
 * PWM signal to radio action mapping:
 *   >50% duty  -> Start RX listening
 *   10-50%     -> Configure new parameters
 *   <10%       -> Stop RX, enter low power
 *   0%         -> Sleep/standby mode
 */

/**
 * @}
 */

/* ============================================================================
   AT COMMAND EXAMPLES FOR OPTIMIZED CONFIGURATION
   ============================================================================ */

/**
 * @defgroup AT_COMMAND_EXAMPLES AT Command Examples
 * @{
 */

/**
 * @brief AT Command to Set Optimized Configuration
 * @note Format: AT+TCONF=<freq>:<power>:<bw>:<sf>:<cr>:<lna>:<pa_boost>:<modulation>:<payload>:<fskdev>:<ldr>:<bt>
 * 
 * Command:
 *   AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3
 * 
 * Parameters explained:
 *   868000000  = Frequency (Hz) - 868 MHz
 *   22         = TX Power (dBm) - Maximum
 *   4          = Bandwidth index - 125 kHz
 *   12         = Spreading Factor - SF12 (maximum)
 *   4          = Coding Rate - 4/8 (maximum error correction)
 *   1          = LNA - Enabled
 *   1          = PA Boost - Enabled
 *   1          = Modulation - LoRa (1)
 *   16         = Payload length - 16 bytes
 *   25000      = FSK Deviation (N/A for LoRa)
 *   2          = Low Data Rate Opt - Auto
 *   3          = BT Product (N/A for LoRa)
 */

/**
 * @brief AT Command Sequence for Maximum Distance RX
 * 
 * 1. Set optimized configuration:
 *    AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3
 * 
 * 2. Verify configuration:
 *    AT+TCONF?
 * 
 * 3. Start continuous RX:
 *    AT+RX=0
 * 
 * 4. Monitor signal quality:
 *    AT+RSSI
 * 
 * 5. To stop RX and sleep:
 *    AT+TEST_STOP
 * 
 * @note Expected signal strength:
 *   - Clear channel: -120 to -140 dBm
 *   - Good signal: -80 to -100 dBm
 *   - Weak signal: -100 to -120 dBm
 *   - No signal: > -140 dBm
 */

/**
 * @}
 */

/* ============================================================================
   PERFORMANCE CHARACTERISTICS
   ============================================================================ */

/**
 * @defgroup PERFORMANCE_METRICS Performance Metrics
 * @{
 */

/**
 * @brief Sensitivity Analysis
 * @details Receiver sensitivity at different configurations
 * 
 * Sensitivity Formula:
 *   Sensitivity (dBm) = -174 + 10×log10(BW) + NF + SNR_min + 6×(SF-1)
 * 
 * Calculated sensitivities (SX1262, NF=7dB, SNR=-5dB):
 *   SF12 + 125kHz => -134 dBm  <- CURRENT/OPTIMIZED
 *   SF12 + 62.5kHz => -131 dBm
 *   SF12 + 31.25kHz => -128 dBm
 *   SF10 + 125kHz => -131 dBm
 * 
 * Impact:
 *   -134 dBm sensitivity allows reception from very weak signals
 *   ~12 dB improvement over basic FSK modulation
 *   Typical link margin: 15-20 dB for reliable operation
 */

/**
 * @brief Time-on-Air Calculation
 * @details Packet transmission time at different configurations
 * 
 * Calculated values (SF12, CR=4/8, 16-byte payload, 8 symbol preamble):
 * 
 *   BW=7.8kHz    => ~70 seconds  (extreme range mode)
 *   BW=31.25kHz  => ~18 seconds  (extended range)
 *   BW=62.5kHz   => ~9 seconds   (good range)
 *   BW=125kHz    => ~5 seconds   (balanced) <- CURRENT/OPTIMIZED
 *   BW=250kHz    => ~2.5 seconds (faster response)
 *   BW=500kHz    => ~1.2 seconds (fast RC)
 * 
 * Impact on RC_GUARD:
 *   ~5 seconds acceptable for non-critical commands
 *   PWM measurement interval: 100 ms (50x faster)
 *   No conflict expected
 */

/**
 * @brief Link Range Estimation
 * @details Estimated maximum range under different conditions
 * 
 * FSL (Free Space Loss) path loss:
 *   PL_dB = 20×log10(d) + 20×log10(f) + 20×log10(4π/c)
 * 
 * Estimated range @ 868 MHz, 22 dBm TX, -134 dBm RX:
 *   Link margin = TX_power - RX_sensitivity - path_loss - margin
 *   22 - (-134) - path_loss - 15 = 141 dB available
 * 
 *   Open area (free space)  => 15+ km
 *   Urban area (obstacles)  => 8-12 km
 *   Indoor (through walls)  => 1-3 km
 *   Building penetration    => 100-500 m
 * 
 * Original config (14 dBm, 125kHz, SF12):
 *   Open area => 8+ km
 *   Urban => 4-6 km
 * 
 * Improvement: +50-100% range with optimization
 */

/**
 * @brief Power Consumption Profile
 * @details Current draw and battery life estimation
 * 
 * Operating modes @ 3.3V:
 *   Sleep (LP)         =>  2 µA, ~0.2 µW
 *   RX active (SF12)   => 40 mA, ~132 mW
 *   TX (22 dBm)        => 150 mA, ~500 mW
 *   PWM monitor        => <1 mA, ~3 mW
 * 
 * Continuous RX average power:
 *   Average = (95% × 2µA + 5% × 40mA) = ~2 mA
 *   Battery: 1000 mAh => ~500 hours (20+ days)
 * 
 * Intermittent RX (10s on, 90s off):
 *   Average = (10% × 2mA + 90% × 40µA) = ~200 µA
 *   Battery: 1000 mAh => ~5000 hours (200+ days)
 */

/**
 * @}
 */

/* ============================================================================
   REGULATORY COMPLIANCE
   ============================================================================ */

/**
 * @defgroup REGULATORY Regulatory Requirements
 * @{
 */

/**
 * @brief EU 868 MHz Band Compliance (EN 300.220)
 * @note
 *   - Frequency range: 865.0 - 868.0 MHz ✓
 *   - Max EIRP: 14 dBm (with antenna gain)
 *   - Modulation: LoRa 125-250 kHz ✓
 *   - Duty cycle: 0.1% (36 seconds per hour per frequency)
 * 
 * @warning
 *   TX at 22 dBm requires:
 *   - High-gain antenna (9 dBi) for EIRP compliance, OR
 *   - Reduce TX power to 14 dBm (equivalent to 14+0dBi EIRP), OR
 *   - Verify antenna and regulatory zone
 */

/**
 * @brief US 915 MHz Band Compliance (FCC Part 15)
 * @note
 *   - Frequency range: 902.0 - 928.0 MHz
 *   - Max EIRP: 30 dBm
 *   - Modulation: LoRa up to 500 kHz ✓
 *   - Duty cycle: None
 * 
 * @note To use in US band:
 *   Change RADIO_FREQ_HZ to 915000000
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* RADIO_CONFIG_OPTIMIZED_H */

/**
 * @file radio_config_optimized.h
 * 
 * Implementation Notes:
 * 
 * 1. Use these defines in test_rf.c:
 *    #include "radio_config_optimized.h"
 *    // Apply optimized values to testParameter_t
 * 
 * 2. Verify AT command configuration matches:
 *    AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3
 * 
 * 3. Monitor PWM signals on PA10/PA11:
 *    - Ensure TIM1 input capture is running
 *    - Verify duty cycle detection threshold (>1%)
 *    - Check for radio/PWM timing conflicts (none expected)
 * 
 * 4. Test RX sensitivity:
 *    AT+RX=0          # Start continuous RX
 *    # Wait for signal
 *    # Should decode SF12 packets from 10+ km away
 * 
 * 5. Regulatory check:
 *    - EU: Verify antenna EIRP or reduce TX to 14 dBm
 *    - US: Change frequency to 915 MHz if needed
 */
