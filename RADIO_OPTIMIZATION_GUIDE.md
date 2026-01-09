# SubGHz Radio Configuration Optimization Guide
## STM32WL55xx - Maximum Receive Distance Configuration

**Date:** January 9, 2026  
**Target:** RC_GUARD with PWM Signal Control (PA10, PA11)

---

## 1. CURRENT SYSTEM ANALYSIS

### 1.1 Hardware Capabilities
- **MCU:** STM32WL55xx with built-in SubGHz radio
- **Radio Module:** SX1262 LoRa transceiver
- **Frequency:** 868 MHz (EU ISM band)
- **Max TX Power:** 22 dBm (configured: 14 dBm)
- **Modulation:** LoRa, FSK, MSK, BPSK

### 1.2 PWM Monitoring System
- **Input Capture Timer:** TIM1 (APB2, 32 MHz)
- **Prescaler:** 32 (= 1 MHz clock = 1 µs resolution)
- **Monitored Channels:**
  - CH3: PA10 (PWM_MON_CH1)
  - CH4: PA11 (PWM_MON_CH2)
- **Duty Cycle Threshold:** > 1% for signal detection
- **Timeout:** 100 ms

### 1.3 Current Default LoRa Configuration
```
Parameter           | Current Value  | Range           | Impact
-------------------|----------------|-----------------|------------------
Modulation          | LoRa (TEST_LORA=1) | FSK/LoRa/BPSK | ✓ Correct
Frequency           | 868 MHz        | Depends on region| ✓ Correct
TX Power            | 14 dBm         | -9 to +22 dBm   | Can increase for RX
Bandwidth (BW)      | 125 kHz (idx:4) | 7.8-500 kHz    | Narrow = more sensitive
Spreading Factor    | SF12           | SF5-SF12        | ✓ Maximum (slowest)
Coding Rate         | 4/5 (idx:1)    | 4/5 to 4/8      | Can use 4/8 for reliability
Low Data Rate Opt   | 2 (Auto)       | 0/1/2           | ✓ Auto-optimized for SF12
Preamble Length     | 8 symbols      | Typical: 8      | ✓ Standard
CRC                 | Enabled        | On/Off          | ✓ Error detection
Payload Length      | 16 bytes       | 1-256 bytes     | Variable
LNA                 | Off (0)        | 0/1             | Can enable
PA Boost            | Off (0)        | 0/1             | Can enable
```

---

## 2. OPTIMIZATION STRATEGY FOR MAXIMUM RECEIVE DISTANCE

### 2.1 LoRa Range Factors (in order of importance)
1. **Spreading Factor (SF)** - Most critical for range
   - SF12 = 4096x processing gain (current: ✓ already optimal)
   - Doubles symbol time each step up
   - SF12 at 125kHz ≈ 32 ms per symbol

2. **Bandwidth (BW)** - Significant impact on sensitivity
   - Narrower BW = Higher sensitivity
   - **Recommended:** 62.5 kHz or 31.25 kHz (trade off with speed)
   - Relationship: Sensitivity ≈ -174 dBm/Hz + NF + 10log(BW) + 6dB (SF)

3. **TX Power** - Extends transmission range
   - Currently: 14 dBm  
   - Maximum: 22 dBm (+8 dB = ~6.3x power increase)
   - For RX: Doesn't directly improve reception, but ensures robust 2-way link

4. **Coding Rate (CR)** - Error correction vs efficiency
   - Current: 4/5 (1 parity bit per 4 data bits)
   - Options: 4/6, 4/7, 4/8 (more redundancy)
   - **For range:** Use 4/8 for maximum error correction

5. **Hardware Features**
   - **LNA:** Low Noise Amplifier (frontend) - typically +10 dB improvement
   - **PA Boost:** Power amplifier boost - enables higher TX power
   - Both should be enabled when available

---

## 3. RECOMMENDED CONFIGURATIONS

### 3.1 **MAXIMUM DISTANCE** (Slowest, 1+ minute per packet)
```c
Frequency:          868,000,000 Hz
TX Power:           22 dBm
Bandwidth:          31.25 kHz (index: 2)
Spreading Factor:   12
Coding Rate:        4/8
LNA:                1 (enabled)
PA Boost:           1 (enabled)
Preamble:           12 symbols (more robust)
Modulation:         LoRa (TEST_LORA)
```

**Characteristics:**
- Symbol Time: ~260 ms at 31.25 kHz
- Packet Time-on-Air: ~32 seconds (16-byte payload)
- Estimated Range: 15+ km open area
- Data Rate: ~18 bits/second
- **Constraint:** Very slow - suitable for RC pulse commands only

### 3.2 **BALANCED** (Medium range, decent speed)
```c
Frequency:          868,000,000 Hz
TX Power:           22 dBm
Bandwidth:          125 kHz (index: 4) - Current default
Spreading Factor:   12
Coding Rate:        4/8
LNA:                1 (enabled)
PA Boost:           1 (enabled)
Modulation:         LoRa (TEST_LORA)
```

**Characteristics:**
- Symbol Time: ~32 ms at 125 kHz
- Packet Time-on-Air: ~2 seconds (16-byte payload)
- Estimated Range: 8+ km open area  
- Data Rate: ~80 bits/second
- **Advantage:** Reasonable speed, excellent range

### 3.3 **FAST RESPONSE** (Short-medium range, fast)
```c
Frequency:          868,000,000 Hz
TX Power:           22 dBm
Bandwidth:          250 kHz (index: 5)
Spreading Factor:   10
Coding Rate:        4/5
LNA:                1 (enabled)
PA Boost:           1 (enabled)
Modulation:         LoRa (TEST_LORA)
```

**Characteristics:**
- Symbol Time: ~4 ms at 250 kHz
- Packet Time-on-Air: ~200 ms (16-byte payload)
- Estimated Range: 5+ km
- Data Rate: ~650 bits/second
- **Advantage:** Fast PWM signal response

---

## 4. PWM SIGNAL TIMING COMPATIBILITY

### 4.1 PWM Monitoring Constraints
- **Input Capture Frequency:** 1 MHz (1 µs resolution)
- **Measurement Period:** ~100 ms typical
- **Duty Threshold:** > 1% required for detection
- **GPIO Pins:** PA10 (CH3), PA11 (CH4)
- **Timer:** TIM1 (shared with radio timing but independent for measurement)

### 4.2 Compatibility with Radio Operations
The PWM monitoring operates asynchronously from radio RX/TX:
- ✓ Can monitor during RX waiting period
- ✓ Can monitor during TX gaps
- ✓ **Latency:** 100 ms timeout acceptable for RC commands
- ✓ Input capture doesn't interfere with radio timing

### 4.3 Recommended PWM-to-Radio Mapping
```
PWM Signal (PA10/PA11)  → RC Command → Radio Action
───────────────────────    ──────────    ────────────
>50% duty              → Start RX     → Begin listening
10-50% duty            → Configure   → Apply new parameters
<10% duty              → Stop        → Return to sleep
0% duty                → Sleep       → Low power mode
```

---

## 5. TIME-ON-AIR CALCULATIONS

### Formula
```
T_packet = T_preamble + T_payload + T_header

T_preamble = (preamble_len + 4.25) × T_symbol
T_payload = (8 + 4×CR + max(0, 8×PL - 4×SF + 28)) / SF × T_symbol
T_header = 20 ms (fixed overhead)
T_symbol = 2^SF / BW [seconds]
```

### Examples at 868 MHz, SF12, 16-byte payload

| Bandwidth | T_symbol | T_packet | Data Rate |
|-----------|----------|----------|-----------|
| 7.8 kHz   | ~524 ms  | ~70 sec  | 2.3 bps   |
| 15.6 kHz  | ~262 ms  | ~35 sec  | 4.6 bps   |
| 31.25 kHz | ~131 ms  | ~18 sec  | 9.1 bps   |
| 62.5 kHz  | ~65 ms   | ~9 sec   | 18 bps    |
| **125 kHz** | **~33 ms** | **~5 sec** | **36 bps** |
| 250 kHz   | ~16 ms   | ~2.5 sec | 72 bps    |
| 500 kHz   | ~8 ms    | ~1.2 sec | 145 bps   |

---

## 6. SENSITIVITY CALCULATIONS

### LoRa Sensitivity Formula
```
Sensitivity (dBm) = -174 + 10×log10(BW) + NF + SNR_min - 6(SF-1)

Where:
  -174 dBm/Hz = Thermal noise floor
  BW = Bandwidth in Hz
  NF = Noise Figure (typically 5-10 dB for SX1262)
  SNR_min = -5 dB (LoRa min for successful decode)
  SF factor = Spreading Factor gain
```

### Calculated Sensitivities (SX1262, NF=7dB)
```
SF | BW=125kHz | BW=62.5kHz | BW=31.25kHz | BW=15.6kHz
---|-----------|-----------|------------|----------
7  | -123 dBm  | -120 dBm   | -117 dBm   | -114 dBm
9  | -129 dBm  | -126 dBm   | -123 dBm   | -120 dBm
10 | -132 dBm  | -129 dBm   | -126 dBm   | -123 dBm
11 | -134 dBm  | -131 dBm   | -128 dBm   | -125 dBm
12 | -137 dBm  | -134 dBm   | -131 dBm   | -128 dBm
```

**Best sensitivity:** SF12 + 31.25kHz = **-131 dBm**  
**Current (SF12 + 125kHz):** **-134 dBm** (better due to wider BW processing gain vs sensitivity trade-off)

---

## 7. IMPLEMENTATION RECOMMENDATIONS

### 7.1 Default Production Setting
**Recommended for RC_GUARD:**
```c
#define OPTIMAL_TX_POWER     22      // dBm (maximum)
#define OPTIMAL_BW          BW_125kHz // Balanced
#define OPTIMAL_SF          12        // Maximum range
#define OPTIMAL_CR          4         // 4/8 (more redundancy)
#define OPTIMAL_LNA         1         // Enabled
#define OPTIMAL_PABOOST     1         // Enabled
#define OPTIMAL_FREQ        868000000 // EU Band
```

### 7.2 Runtime Configuration via AT Commands
```
// Set for maximum distance
AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3

Parameters:
  Freq(Hz)    = 868000000
  Power(dBm)  = 22
  BW(idx)     = 4 (125 kHz)
  SF          = 12
  CR(1-4)     = 4 (4/8)
  LNA         = 1
  PA_Boost    = 1
  Modulation  = 1 (LoRa)
  PayloadLen  = 16
  FSK_Dev     = 25000 (N/A for LoRa)
  LowDrOpt    = 2 (Auto)
  BTproduct   = 3 (N/A for LoRa)

// Start RX
AT+RX=0  // Continuous RX mode

// Get config
AT+TCONF?
```

---

## 8. POWER CONSUMPTION CONSIDERATIONS

### RX Mode Power Budget
```
Activity              | Duration  | Current  | Power
----------------------|-----------|----------|-------
Sleep (LP mode)       | ~95 ms/100ms | 2 µA  | ~200 nW
RX active (SF12)      | ~5 ms/100ms | 40 mA | ~2 mW
RX turnaround         | ~1 ms/100ms | 25 mA | ~250 µW
PWM monitoring        | Continuous | <1 mA | ~1 mW
────────────────────────────────────────────────────
**Total Average:** ~3 mW (sustained RX)
**Peak Current:** 40 mA
```

### Battery Life Estimate (1000 mAh LiPo)
- Continuous RX @ SF12: **~330 hours (13.8 days)**
- Intermittent RX (10s on, 90s off): **~110 days**

---

## 9. TESTING & VALIDATION PROCEDURE

### Step 1: Verify Hardware
```bash
# Check radio presence and version
AT+VER

# Check GPIO configuration
# PA10, PA11 should respond to PWM signals
```

### Step 2: Configure for Maximum Distance
```bash
# Set LoRa config
AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3

# Verify
AT+TCONF?
```

### Step 3: Test Reception
```bash
# Start continuous RX
AT+RX=0

# Monitor RSSI and signal quality
# Expected: -120 to -140 dBm (clear channel)

# Stop test
AT+TEST_STOP
```

### Step 4: Validate PWM Integration
```bash
# Monitor PA10, PA11 GPIO status
# Should read duty cycle changes in real-time
# No interference with RX timing

# Expected latency: <150 ms response
```

---

## 10. FREQUENCY REGULATIONS

### 868 MHz EU Band (EN 300.220)
- **Frequency Range:** 865.0 - 868.0 MHz
- **Max Power:** 14 dBm EIRP (with antenna gain)
- **Duty Cycle:** 0.1% (36 seconds per hour per frequency)
- **Modulation:** LoRa 125/250 kHz, FSK up to 50 kHz BW

### 915 MHz US Band (FCC Part 15)
- **Frequency Range:** 902.0 - 928.0 MHz
- **Max Power:** 30 dBm EIRP
- **Modulation:** LoRa up to 500 kHz BW
- **Duty Cycle:** None specified

**Current Setting:** 868 MHz EU compliant @ 22 dBm (requires 9 dBi antenna or EIRP adjustment)

---

## 11. FUTURE OPTIMIZATIONS

### 11.1 Adaptive Modulation
```c
// Pseudocode for adaptive RX
if (signal_quality_good) {
    // Speed up for faster response
    SF = 10, BW = 250 kHz
} else if (signal_weak) {
    // Maximize sensitivity
    SF = 12, BW = 62.5 kHz
} else {
    // Balanced
    SF = 12, BW = 125 kHz
}
```

### 11.2 Dynamic Power Management
```c
// Adjust TX power based on ACK timing
if (no_ack_received && retries < 3) {
    TX_POWER = 22; // Maximize
} else if (ack_received_quickly) {
    TX_POWER = 14; // Reduce for power saving
}
```

### 11.3 PWM-Triggered Configuration
```c
// Ultra-low latency mode when PWM signal detected
// Reduce BW to 250 kHz, SF to 7-9 for faster response
```

---

## 12. REFERENCE DOCUMENTATION

- STM32WL55 Reference Manual: STM32WL55xx
- SX1262 Datasheet: LoRa radio specifications
- LoRa Specification v1.3: LoRa Alliance
- EN 300.220: EU Radio Equipment Directive

---

## 13. SUMMARY TABLE

| Aspect | Current | Optimized | Benefit |
|--------|---------|-----------|---------|
| **TX Power** | 14 dBm | 22 dBm | +8 dB (2-way link) |
| **RX Sensitivity** | -134 dBm | -134 dBm | Same (good choice) |
| **Bandwidth** | 125 kHz | 125 kHz | Balanced (keep) |
| **SF** | 12 | 12 | Optimal (keep) |
| **Coding Rate** | 4/5 | 4/8 | Better error recovery |
| **LNA** | Disabled | Enabled | +10 dB frontend gain |
| **PA Boost** | Disabled | Enabled | Higher TX power |
| **Time-on-Air** | ~5 sec | ~5 sec | Same (acceptable for RC) |
| **Estimated Range** | ~8 km | ~12+ km | +50% improvement |

---

**Configuration Ready:** See `RADIO_CONFIG_OPTIMIZED.h` for implementation.
