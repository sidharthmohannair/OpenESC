# OpenESC - STM32F051 Implementation

This directory contains STM32F051-based ESC firmware variants.

## 🚧 Status: Experimental

These implementations are currently in development and testing phase. For prototype use, please use the Arduino Nano version in `src/open_loop_esc/`.

## Firmware Variants

### 1. openloop_basic
Basic open-loop ESC implementation for STM32F051C8.

**Features:**
- Open-loop commutation
- Fixed speed control
- Minimal hardware configuration

**Use case:** Testing, initial bring-up

### 2. openloop_potentiometer
Open-loop ESC with analog speed control via potentiometer.

**Features:**
- ADC-based speed input
- Variable speed control
- Real-time adjustment

**Use case:** Manual speed control, testing

### 3. openloop_transmitter
Open-loop ESC with RC receiver input (PWM/PPM).

**Features:**
- PWM input decoding
- RC transmitter compatibility
- Flight-ready control

**Use case:** Drone/UAV applications

## Hardware Requirements

- **MCU:** STM32F051C8T6 (48-pin LQFP)
- **Clock:** 48 MHz
- **Flash:** 64KB
- **RAM:** 8KB
- **Peripherals:** TIM1 (6-step PWM), ADC (speed control)

## Building & Flashing

### Prerequisites
- STM32CubeIDE or STM32CubeMX
- ST-Link V2 programmer

### Build Instructions

#### Using STM32CubeIDE

1. **Import Project:**
   ```
   File → Import → General → Existing Projects into Workspace
   Select: src/stm32f051/openloop_potentiometer/
   ```

2. **Build:**
   ```
   Project → Build Project
   ```

3. **Flash:**
   ```
   Run → Debug (F11)
   ```

#### Using Command Line (Make)

```bash
cd src/stm32f051/openloop_potentiometer/
make clean
make -j$(nproc)

# Flash using st-flash
st-flash write Debug/OPENLOOP_F051POTIOMETER.bin 0x8000000
```

### Using STM32CubeProgrammer

```bash
STM32_Programmer_CLI -c port=SWD -w Debug/*.elf -v -rst
```

## Pin Configuration

Each project includes a `.ioc` file that defines the complete pin configuration. Open in STM32CubeMX to view/modify.

**Common Pins:**
- **TIM1_CH1/CH1N:** Phase A (High/Low)
- **TIM1_CH2/CH2N:** Phase B (High/Low)
- **TIM1_CH3/CH3N:** Phase C (High/Low)
- **ADC_IN0:** Potentiometer input (if applicable)
- **PA9/PA10:** USART1 TX/RX (debug)

## Debugging

### Serial Monitor
- **Baud Rate:** 115200
- **Connection:** USART1 (PA9=TX, PA10=RX)
- **Use:** ST-Link UART or USB-Serial adapter

### SWD Debug
- **SWDIO:** PA13
- **SWCLK:** PA14
- **GND:** Any GND pin

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Build fails: `cannot find -lnosys` | Add `-lnosys` to linker flags or use `--specs=nosys.specs` |
| Flash fails: `target not found` | Check ST-Link connection, try `st-flash --reset` |
| Motor doesn't spin | Check phase wiring, verify TIM1 PWM output with scope |
| Erratic behavior | Add dead-time, check bootstrap capacitors |

## Porting to Other STM32s

To port to other STM32 variants:

1. **STM32F103:** Use TIM1 (same), adjust clock to 72MHz
2. **STM32F4:** Use TIM1 or TIM8, 168MHz clock
3. **STM32G0:** Similar to F0, check pin mappings

Key considerations:
- Timer must support complementary PWM (TIM1/TIM8)
- Need at least 48MHz for smooth commutation
- Adjust dead-time for different MOSFETs

## Known Issues

- [ ] No current sensing yet (v3.0 feature)
- [ ] No BEMF feedback (v3.0 feature)
- [ ] Fixed commutation timing (not adaptive)
- [ ] No overcurrent protection

## Contributing

See main [CONTRIBUTING.md](../../CONTRIBUTING.md) for guidelines.

**STM32-specific guidelines:**
- Keep HAL library usage minimal for performance
- Document all pin assignments in `.ioc` file
- Test on actual hardware before submitting PR
- Include oscilloscope captures for phase switching

## License

Same as main project: MIT License

---


