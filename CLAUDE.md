# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an STM32WL55xx-based SubGHz RF testing application controlled via AT commands over UART. The project includes custom PWM monitoring functionality for RC signal control (RC_GUARD application).

**Hardware:**
- MCU: STM32WL55xx (Cortex-M4, dual-core capable)
- Radio: Built-in SX1262 LoRa/FSK transceiver (868/915 MHz SubGHz)
- Board: NUCLEO-WL55JC or custom RC_GUARD hardware
- PWM Inputs: PA10 (TIM1_CH3), PA11 (TIM1_CH4) for RC signal monitoring
- UART: PA2/PA3 (USART2) for AT command interface at 9600 baud

## Build System

### Building the Project

Use the GNU ARM Embedded Toolchain with the provided Makefile:

```bash
# Build all targets (elf, hex, bin)
make

# Clean build artifacts
make clean

# Build with optimized timer (optional)
make USE_OPTIMIZED_TIMER=1
```

**Build Configuration:**
- Toolchain: `arm-none-eabi-gcc` (must be in PATH or set `GCC_PATH`)
- Target: `SubGHz_Phy_AT_Slave.elf/.hex/.bin`
- Build directory: `build/`
- Linker script: `STM32WL55XX_FLASH_CM4.ld`
- Optimization: `-Og` (debug), can be changed to `-O2` for production
- Debug info: Enabled with `-g -gdwarf-2` when `DEBUG=1`

**Toolchain Detection:**
- Windows: Uses toolchain from PATH (no default `GCC_PATH`)
- Linux: Defaults to `../gcc-arm-none-eabi-linux-x64/bin`

### STM32CubeIDE

This project can also be built using STM32CubeIDE. The `.ioc` file is not in the root directory; this is a standalone application extracted from the STM32Cube firmware package.

**Important:** When regenerating code with STM32CubeMX, uncheck "Use Default Firmware Location" to avoid path issues.

## Code Architecture

### Directory Structure

```
Core/
├── Inc/                    # HAL configuration, peripherals, system headers
│   ├── main.h             # GPIO pin definitions (PWM1/2, LEDs, buttons)
│   ├── sys_conf.h         # Debug flags, power mode, trace configuration
│   ├── pwm_monitor.h      # PWM input capture monitoring API
│   └── stm32wlxx_hal_conf.h  # HAL module enables
├── Src/                    # Main application and peripheral implementations
│   ├── main.c             # Entry point, system init
│   ├── pwm_monitor.c      # PWM signal measurement (TIM1 input capture)
│   ├── pwm_if.c           # PWM interface wrapper
│   ├── stm32wlxx_it.c     # Interrupt handlers
│   └── stm32wlxx_hal_msp.c # HAL MSP initialization

SubGHz_Phy/
├── App/                    # Radio application layer
│   ├── app_subghz_phy.c   # Main radio application
│   ├── subghz_phy_app.c   # Radio state machine
│   ├── subg_at.c          # AT command parser
│   ├── subg_command.c     # AT command handlers
│   └── test_rf.c          # RF test modes (TX continuous, RX, packet)
└── Target/                 # Radio hardware abstraction
    ├── radio_board_if.c   # BSP interface for radio hardware
    ├── radio_conf.h       # SMPS, XTAL, TCXO configuration
    └── mw_log_conf.h      # Middleware logging settings

Middlewares/Third_Party/SubGHz_Phy/stm32_radio_driver/
    ├── radio.c            # Radio driver API
    ├── radio_driver.c     # Low-level radio control
    └── lr_fhss_mac.c      # LoRa FHSS MAC layer

Utilities/
    ├── timer/             # RTC-based software timers
    ├── trace/             # Advanced trace/logging
    ├── sequencer/         # Task sequencer for async operations
    ├── lpm/               # Low power manager
    └── misc/              # Memory, time utilities
```

### Key Architecture Patterns

#### 1. AT Command System

The application implements a UART-controlled AT command interface for RF testing:

- **Parser**: `SubGHz_Phy/App/subg_at.c` - Tokenizes incoming UART commands
- **Handler**: `SubGHz_Phy/App/subg_command.c` - Executes AT commands and formats responses
- **Commands**: See `subg_at.h` for full list (AT+TCONF, AT+TX, AT+RX, AT+VER, etc.)

**Command flow:**
```
UART RX → AT Parser → Command Handler → Radio API → Response via UART TX
```

#### 2. PWM Monitoring System

Custom input capture implementation for RC signal detection:

**Location**: `Core/Src/pwm_monitor.c`, `Core/Inc/pwm_monitor.h`

**Hardware configuration:**
- Timer: TIM1 (32 MHz APB2 clock / 32 = 1 MHz = 1 µs resolution)
- Channel 3: PA10 (PWM_MON_CH1)
- Channel 4: PA11 (PWM_MON_CH2)
- Mode: Input capture on both edges (rising + falling)
- Threshold: Signal detected when duty cycle > 1%
- Timeout: 100 ms

**Key functions:**
- `PWM_MON_Init()` - Configures TIM1 input capture
- `PWM_MON_Process()` - Call periodically to check for signal changes
- `PWM_MON_GetDutyCycle()` - Returns duty cycle percentage
- `PWM_MON_SignalDetected()` - Returns true if signal present
- `PWM_MON_SendResponse()` - Sends unsolicited UART response when signal changes

**Integration point**: The PWM monitor can trigger radio operations based on duty cycle thresholds (see `RADIO_OPTIMIZATION_GUIDE.md` section 4.3).

#### 3. Radio State Machine

Located in `SubGHz_Phy/App/subghz_phy_app.c`:

**States:**
- `STATE_IDLE` - Radio off, waiting for command
- `STATE_TX` - Transmitting packet or continuous wave
- `STATE_RX` - Receiving (continuous or packet mode)
- `STATE_TX_WAIT` - TX delay/timeout handling
- `STATE_ERROR` - Error state

**Event-driven architecture:**
- Radio events (RxDone, TxDone, RxError, etc.) trigger state transitions
- Callbacks from radio driver: `OnTxDone()`, `OnRxDone()`, `OnTxTimeout()`, `OnRxTimeout()`, `OnRxError()`

#### 4. Low Power Mode Integration

The application uses the STM32WL's low power modes between radio operations:

- **LPM Configuration**: `Core/Src/stm32_lpm_if.c`
- **Control**: `LOW_POWER_DISABLE` flag in `sys_conf.h`
- **Modes**: Stop2 (default), Sleep (when `LOW_POWER_DISABLE=1`)
- **Note**: Disable low power mode (`LOW_POWER_DISABLE=1`) for debugging

## Configuration Files

### Radio Configuration

**File**: `SubGHz_Phy/Target/radio_conf.h`

Key settings:
- `XTAL_FREQ`: 32 MHz crystal frequency
- `XTAL_DEFAULT_CAP_VALUE`: 0x20 (crystal trim)
- `TCXO_CTRL_VOLTAGE`: 1.7V TCXO control voltage
- `SMPS_DRIVE_SETTING_DEFAULT`: SMPS drive current (normal mode)
- `SMPS_DRIVE_SETTING_MAX`: SMPS drive current (TX low power)

### System Configuration

**File**: `Core/Inc/sys_conf.h`

Debug and power settings:
- `DEBUGGER_ENABLED`: 1 = Enable SWD/JTAG (set to 0 for production)
- `LOW_POWER_DISABLE`: 0 = Stop2 mode, 1 = Sleep only
- `APP_LOG_ENABLED`: Enable trace logs via UART
- `VERBOSE_LEVEL`: `VLEVEL_M` (medium verbosity)

**For debugging:**
```c
#define DEBUGGER_ENABLED 1
#define LOW_POWER_DISABLE 1  // Prevents Stop2 mode for stable debugging
```

### Radio Test Parameters

**File**: `SubGHz_Phy/App/test_rf.h`

RF test configuration structure (`testParameter_t`):
- `modulation`: 0=FSK, 1=LoRa, 2=BPSK
- `freq`: Frequency in Hz (e.g., 868000000)
- `power`: TX power in dBm (-9 to +22)
- `bandwidth`: LoRa BW index (0-6) or FSK bandwidth in Hz
- `loraSf_datarate`: LoRa spreading factor (5-12) or FSK datarate
- `codingRate`: LoRa coding rate (1=4/5, 2=4/6, 3=4/7, 4=4/8)
- `lna`: LNA enable (0=off, 1=on)
- `paBoost`: PA boost (0=off, 1=on)
- `payloadLen`: Payload length 1-256 bytes
- `fskDev`: FSK deviation in Hz
- `lowDrOpt`: LoRa low datarate optimization (0=off, 1=on, 2=auto)
- `BTproduct`: FSK Gaussian filter BT product (0-4)

## AT Command Interface

### Connection

- **Baud rate**: 9600
- **Data bits**: 8
- **Stop bits**: 1
- **Parity**: None
- **Flow control**: None
- **Line ending**: CR+LF
- **Local echo**: Recommended to enable in terminal

### Common Commands

```
AT?                  - List all available commands
AT+VER               - Get firmware version
AT+TCONF?            - Get current radio configuration
AT+TCONF=<params>    - Set radio configuration
AT+TX=<mode>         - Start TX (0=packet, 1=continuous)
AT+RX=<mode>         - Start RX (0=continuous, 1=single)
AT+TEST_STOP         - Stop current test
```

### Radio Configuration Example

Set LoRa for maximum range (SF12, BW125, CR4/8, 22 dBm):

```
AT+TCONF=868000000:22:4:12:4:1:1:1:16:25000:2:3
```

Parameters (colon-separated):
1. Frequency (Hz): 868000000
2. Power (dBm): 22
3. Bandwidth index: 4 (125 kHz for LoRa)
4. SF/Datarate: 12 (SF12)
5. Coding rate: 4 (4/8)
6. LNA: 1 (enabled)
7. PA Boost: 1 (enabled)
8. Modulation: 1 (LoRa)
9. Payload length: 16
10. FSK deviation: 25000 (ignored for LoRa)
11. Low DR optimization: 2 (auto)
12. BT product: 3 (ignored for LoRa)

## Radio Range Optimization

See `RADIO_OPTIMIZATION_GUIDE.md` for comprehensive radio configuration guidance.

**Key takeaways for maximum range:**
- Use LoRa modulation (`TEST_LORA`)
- Spreading Factor: SF12 (maximum sensitivity)
- Bandwidth: 125 kHz (balanced) or 62.5 kHz (more sensitive, slower)
- Coding Rate: 4/8 (maximum error correction)
- TX Power: 22 dBm (maximum allowed)
- Enable LNA and PA Boost
- Expected range: 8-15 km line-of-sight depending on configuration

**Current settings vs optimal:**
- Current: SF12, BW125, CR4/5, 14 dBm, LNA off, PA boost off → ~8 km
- Optimal: SF12, BW125, CR4/8, 22 dBm, LNA on, PA boost on → ~12+ km

## PWM Integration with Radio

The PWM monitor can control radio operations based on RC signal input:

**Suggested mapping** (from `RADIO_OPTIMIZATION_GUIDE.md`):
- Duty > 50%: Start continuous RX mode
- Duty 10-50%: Configure radio parameters
- Duty < 10%: Stop radio
- Duty 0%: Enter low power mode

**Implementation location**: Modify `Core/Src/main.c` main loop or `PWM_MON_Process()` callback to add this logic.

## Debugging

### Enable Debug Mode

1. In `Core/Inc/sys_conf.h`:
   ```c
   #define DEBUGGER_ENABLED 1
   #define LOW_POWER_DISABLE 1
   ```

2. Build and flash the firmware

3. Attach debugger (SWD on PA13/PA14)

### Common Debug Points

**Breakpoint locations:**
- `main.c:main()` - Entry point
- `subg_command.c:CMD_*()` - AT command handlers
- `pwm_monitor.c:PWM_MON_InputCapture_Callback()` - PWM edge detection
- `subghz_phy_app.c:OnRxDone()` - Radio RX complete
- `radio.c:RadioSend()` - Radio TX initiation

**Watchpoints:**
- `pwm_mon_states[]` - PWM measurement data
- `testParameter` - Current radio configuration

### Serial Trace

Traces are output via UART at the configured baud rate when `APP_LOG_ENABLED=1`:
- Use a terminal (PuTTY, Tera Term, minicom) to view logs
- Set `VERBOSE_LEVEL` in `sys_conf.h` to adjust detail level

## Hardware Modifications

### GPIO Pin Definitions

Defined in `Core/Inc/main.h`:

**PWM Inputs:**
- `PWM1_Pin`: GPIO_PIN_8 (PA8) - Not used by PWM monitor
- `PWM2_Pin`: GPIO_PIN_9 (PA9) - Not used by PWM monitor
- PWM monitor actually uses PA10 and PA11 (TIM1_CH3/CH4)

**LEDs:**
- `LED1_Pin`: GPIO_PIN_15 (PB15)
- `LED2_Pin`: GPIO_PIN_9 (PB9)
- `LED3_Pin`: GPIO_PIN_11 (PB11)

**Buttons:**
- `BUT1_Pin`: GPIO_PIN_0 (PA0)
- `BUT2_Pin`: GPIO_PIN_1 (PA1)
- `BUT3_Pin`: GPIO_PIN_6 (PC6)

**UART:**
- `USARTx_TX_Pin`: GPIO_PIN_2 (PA2)
- `USARTx_RX_Pin`: GPIO_PIN_3 (PA3)

### Adding Custom Peripherals

When adding new peripherals:
1. Use STM32CubeMX to regenerate HAL initialization (optional)
2. Keep user code within `/* USER CODE BEGIN */` ... `/* USER CODE END */` sections
3. Update `Makefile` C_SOURCES if adding new `.c` files
4. Ensure GPIO clocks are enabled in `Core/Src/gpio.c`

## Testing

### Basic Functionality Test

1. Connect UART terminal (9600 8N1, CR+LF)
2. Reset board
3. Send `AT?` - Should respond with command list
4. Send `AT+VER` - Should respond with firmware version
5. Send `AT+TCONF?` - Should show current radio config

### Radio TX Test

```
AT+TCONF=868000000:14:4:12:1:0:0:1:16:25000:2:3
AT+TX=0
```
Transmits a single LoRa packet. Check with spectrum analyzer or second receiver.

### Radio RX Test

```
AT+TCONF=868000000:14:4:12:1:0:0:1:16:25000:2:3
AT+RX=0
```
Starts continuous RX mode. Should show RSSI/SNR when packets received.

### PWM Monitor Test

1. Connect PWM signal to PA10 (CH1) or PA11 (CH2)
2. Generate PWM signal (e.g., 1 kHz, 50% duty)
3. PWM monitor should send unsolicited response via UART when duty > 1%
4. Format: `+PWM_CH1: <duty_cycle>% <period>us`

## Common Issues

### Build fails with "arm-none-eabi-gcc: command not found"

- Ensure ARM GCC toolchain is installed and in PATH
- On Linux, set `GCC_PATH` in Makefile: `make GCC_PATH=/path/to/gcc-arm-none-eabi/bin`
- On Windows, add toolchain bin directory to system PATH

### UART not responding to AT commands

- Check baud rate: Must be 9600
- Check line endings: Use CR+LF
- Verify UART connection: PA2 (TX), PA3 (RX), GND
- Check `DEBUGGER_ENABLED` and `LOW_POWER_DISABLE` in `sys_conf.h`

### PWM monitor not detecting signal

- Verify signal is connected to PA10 (TIM1_CH3) or PA11 (TIM1_CH4), NOT PA8/PA9
- Check signal frequency (100 Hz - 10 kHz tested)
- Ensure duty cycle > 1%
- Verify TIM1 is properly initialized in `pwm_if.c`
- Check that `PWM_MON_Init()` is called from `main()`

### Radio not transmitting/receiving

- Verify antenna is connected (critical!)
- Check frequency is legal for region (868 MHz EU, 915 MHz US)
- Ensure radio configuration is valid (use `AT+TCONF?`)
- Check power supply can provide sufficient current (up to 40 mA TX)

### Debug connection lost after flashing

- Low power modes can disable SWD
- Set `LOW_POWER_DISABLE=1` in `sys_conf.h` before flashing
- Connect under reset if necessary
- Use STM32CubeProgrammer to re-enable debug via option bytes if locked out

## Important Notes

- **This is a test application**, not production firmware. It lacks proper error handling in many areas.
- **AT commands are not robust**: Malformed commands may cause undefined behavior.
- **No bootloader**: Updates require reflashing via SWD/JTAG.
- **Regulatory compliance**: Ensure transmission power and duty cycle comply with local regulations (see `RADIO_OPTIMIZATION_GUIDE.md` section 10).
- **PWM monitoring is custom code**: Not part of the original STM32Cube AT Slave example.

## Reference Documentation

- **STM32WL55 Reference Manual**: RM0453
- **STM32WL HAL Driver Documentation**: Included in STM32CubeWL package
- **SX1262 Datasheet**: LoRa radio specifications
- **LoRa Alliance**: LoRa modulation and MAC specifications
- **Radio Optimization Guide**: `RADIO_OPTIMIZATION_GUIDE.md` in this repository
