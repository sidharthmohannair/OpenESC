README.txt
 Created on: Dec 20, 2025
     Author:  Aiswarya Babu, Muhammed Afsal


Project Title:
--------------
STM32F051 Open-Loop BLDC ESC with Potentiometer Control


Description:
------------
This project implements an open-loop 6-step commutation based BLDC motor
controller using the STM32F051 microcontroller. Motor speed is controlled
using a potentiometer connected to the ADC. PWM is generated using TIM1
at 20 kHz, and manual low-side control with software dead-time is used
to prevent shoot-through.

This firmware is intended for testing and development of BLDC ESC
hardware and follows the same testing procedure as a standard open-loop
STM32F051 ESC.


Microcontroller:
----------------
- STM32F051 (Cortex-M0)
- Clock: 48 MHz (HSI + PLL)


Key Features:
-------------
- Open-loop 6-step commutation
- Edge-aligned PWM at 20 kHz
- TIM1 advanced timer usage
- Software dead-time insertion
- ADC-based potentiometer speed control
- Low-pass filtered ADC input
- LL (Low Layer) driver based implementation


PWM Configuration:
------------------
- Timer: TIM1
- PWM Frequency: 20 kHz
- Prescaler: 0
- Auto Reload Value (ARR): 2399
- Channels:
  - CH1: Phase A high-side
  - CH2: Phase B high-side
  - CH3: Phase C high-side


GPIO Pin Mapping:
-----------------
High-Side PWM Outputs (TIM1):
- PA8  -> Phase A High
- PA9  -> Phase B High
- PA10 -> Phase C High

Low-Side GPIO Outputs:
- PA7  -> Phase A Low
- PB0  -> Phase B Low
- PB1  -> Phase C Low

ADC Input:
- PA0 -> Potentiometer (ADC_IN0)


ADC Configuration:
------------------
- ADC: ADC1
- Resolution: 12-bit
- Clock Source: HSI14 (STM32F0 specific)
- Channel: ADC Channel 0 (PA0)



Duty Cycle Control:
-------------------
- Minimum Duty: DUTY_MIN (safe startup)
- Maximum Duty: DUTY_MAX
- Potentiometer value is mapped to PWM duty cycle
- Same duty applied to all three phases


Commutation Sequence:
--------------------
6-step trapezoidal commutation sequence:

1. Phase A PWM, Phase B LOW, Phase C OFF
2. Phase C PWM, Phase B LOW, Phase A OFF
3. Phase C PWM, Phase A LOW, Phase B OFF
4. Phase B PWM, Phase A LOW, Phase C OFF
5. Phase B PWM, Phase C LOW, Phase A OFF
6. Phase A PWM, Phase C LOW, Phase B OFF

Each step has a fixed delay for open-loop operation.


Software Dead-Time:
-------------------
- Implemented using NOP delay cycles
- Prevents high-side and low-side shoot-through
- Adjustable using SOFT_DT_CYCLES macro


Testing Procedure:
------------------
The testing procedure is the SAME as standard open-loop STM32F051 ESC testing.

1. Verify power supply voltage and polarity
2. Ensure MOSFET gate driver connections are correct
3. Power the STM32 board without motor connected
4. Rotate potentiometer and verify PWM output using oscilloscope
5. Connect BLDC motor (no load)
6. Slowly increase potentiometer
7. Observe smooth commutation and motor rotation


NOTE:
-----
- This is an OPEN-LOOP controller
- No position or back-EMF feedback is used
- Intended only for testing, learning, and hardware validation


Development Environment:
------------------------
- STM32CubeIDE
- LL Drivers
-



     