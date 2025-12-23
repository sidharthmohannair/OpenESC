Created on: Dec 20, 2025
     Author: AISWARYA BABU,MUHAMMED AFSAL
     # STM32F051 BLDC Motor Controller with 6-Step Commutation

## Project Overview

This project implements a **brushless DC (BLDC) motor controller** using the STM32F051C8T6 microcontroller with edge-aligned PWM and 6-step commutation. The firmware drives a three-phase motor using an IR2110 half-bridge driver IC to control high-side and low-side MOSFETs with proper dead-time management.

**Key Features:**
- 6-step commutation pattern for smooth motor control
- Software-based dead-time insertion (~291 ns @ 48 MHz)
- 20 kHz PWM frequency
- Half-bridge configuration with IR2110 gate driver
- GPIO-controlled low-side switches
- Optimized for low-cost motor control applications

---

## Hardware Requirements

### Microcontroller
- **STM32F051C8T6** (ARM Cortex-M0, 48 MHz)
- System clock: 48 MHz (HSI/2 * PLL12)

### Gate Driver IC
- **IR2110** Half-Bridge Driver
  - Bootstrap capacitor: 
  - Decoupling capacitors: 
  - Ceramic gate resistors:
  - Bootstrap diode: 

- **IRF3205**
  - Quantity: 6 (3 high-side + 3 low-side)
  
  

### Power Supply
-Input voltage: 12V-1A DC (typical)
-
### Motor
- **Three-phase brushless DC motor** (BLDC)
- Voltage rating: 12- DC 
### Wiring & Pin Configuration

**STM32F051 Pin Assignments:**

| Pin | Function | Description |
|-----|----------|-------------|
| PA8 | TIM1_CH1 | Phase A High-Side PWM |
| PA9 | TIM1_CH2 | Phase B High-Side PWM |
| PA10 | TIM1_CH3 | Phase C High-Side PWM |
| PA7 | GPIO_OUT | Phase A Low-Side (AL) |
| PB0 | GPIO_OUT | Phase B Low-Side (BL) |
| PB1 | GPIO_OUT | Phase C Low-Side (CL) |

**IR2110 Control Lines:**

| IR2110 Pin | STM32 Pin | Purpose |
|-----------|-----------|---------|
| (Phase A High) | PA8 | AH gate drive |
| (Phase A Low) | PA7 | AL gate drive |
| (Phase B High) | PA9 | BH gate drive |
|  (Phase B Low) | PB0 | BL gate drive |
|  (Phase C High) | PA10 | CH gate drive |
| (Phase C Low) | PB1 | CL gate drive |

---

## Software Requirements

- **STM32CubeIDE** v1.10.0 or later
- **STM32F0 LL drivers** (included with STM32CubeMX)
- **Compiler:** ARM GCC (bundled with STM32CubeIDE)
- **C Standard:** C99 or later

### Key Configuration Parameters

```c
#define SYSCLK_HZ 48000000U        /* System clock: 48 MHz */
#define PWM_FREQ_HZ 20000U         /* PWM frequency: 20 kHz */
#define TIM_ARR_VALUE 2399         /* Auto-reload (48MHz / 20kHz - 1) */
#define INITIAL_DUTY 1199          /* 50% initial duty cycle */
#define SOFT_DT_CYCLES 14U         /* ~291 ns dead-time @ 48 MHz */
```

---

## Building the Project

1. **Open STM32CubeIDE**
2. **Import Project:** File → Open Projects from File System → Select project folder
3. **Clean Build:**
   ```
   Project → Clean
   Project → Build All
   ```
4. **Check Console:** Verify no compilation errors
5. **Resolve Warnings:** Fix any missing includes or undefined references

---

## Flashing the Firmware

### Using STM32CubeIDE Built-in Debugger

1. Connect **ST-Link v2** debugger to the JTAG/SWD pins:
   - SWDIO → PA13
   - SWCLK → PA14
   - GND → GND
   - 3.3V → 3.3V

. In STM32CubeIDE:
   ```
   Run → Debug 
   ```

. The firmware will be flashed and execution pauses at main()


---

## Hardware Testing Procedure


. **Run firmware and observe oscilloscope:**
   - Phase A (PA8): PWM signal at 20 kHz, 50% duty
   - Phase B (PA9): Phase A signal
   - Phase C (PA10): Phase A signal
. **Verify commutation sequence:**
   - Capture 6 commutation steps (each ~1 ms)
   - Confirm proper phase transitions:
     - Step 1: AH PWM, BL low
     - Step 2: CH PWM, BL low
     - Step 3: CH PWM, AL low
     - Step 4: BH PWM, AL low
     - Step 5: BH PWM, CL low
     - Step 6: AH PWM, CL low



---


---

## Performance Specifications

| Parameter | Value |
|-----------|-------|
| PWM Frequency | 20 kHz |
| System Clock | 48 MHz |
| Software Dead-time | ~291 ns |
| Commutation Steps | 6 (120° phase shift) |
| Step Duration | 1 ms (100 RPM base speed) |
| Supported Voltage | 12V DC |
| 

---

## References

- [STM32F051 Datasheet](https://www.st.com/resource/en/datasheet/stm32f051c8.pdf)
- [IR2110 Gate Driver Datasheet](https://www.infineon.com/en/products/semiconductors/gate-drivers/ir2110)
- [IRF3205 MOSFET Datasheet](https://www.infineontech.com/dgdl/irf3205.pdf)
- STM32CubeIDE Documentation: Help → STM32CubeIDE Help

---



## License

This firmware is provided as-is for educational and commercial use. No warranty is provided.
