# To do List

## Current Status:  Open-Loop Working

You have successfully implemented:
- 6-step commutation sequence (lines 187-200)
- PWM speed control via RC input (TIM3 input capture)
- Proper phase control (PWM high-side, GPIO low-side)
- Software dead-time protection (14 cycles)
- Variable speed from RC throttle

**Your motor is running in open-loop with fixed timing delays.**

---

## Next Step: Add Zero-Crossing Detection Hardware

### What We'll Add:

1. **Hardware comparator** to detect BEMF on floating phase
2. **Timer for precise commutation timing** (instead of `LL_mDelay`)
3. **Interrupt-driven commutation** (not polling in main loop)
4. **Hybrid mode** - test ZC while still using open-loop

---

## Step-by-Step Implementation Plan

### **Step 1: Add Comparator Hardware Configuration**

Add this after `MX_TIM3_Init()`:

```c
static void MX_COMP1_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_COMP_InitTypeDef COMP_InitStruct = {0};
  
  /* Enable GPIOA clock (already enabled, but ensure it) */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  
  /* Configure PA1 as analog input (COMP1 positive input) */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* Configure PA5 as analog input (COMP1 negative reference) */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  /* Enable SYSCFG clock for COMP */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  
  /* Configure Comparator */
  COMP_InitStruct.PowerMode = LL_COMP_POWERMODE_HIGHSPEED;
  COMP_InitStruct.InputPlus = LL_COMP_INPUT_PLUS_IO1;     // PA1
  COMP_InitStruct.InputMinus = LL_COMP_INPUT_MINUS_IO1;   // PA5 as reference
  COMP_InitStruct.InputHysteresis = LL_COMP_HYSTERESIS_MEDIUM;
  COMP_InitStruct.OutputPolarity = LL_COMP_OUTPUTPOL_NONINVERTED;
  LL_COMP_Init(COMP1, &COMP_InitStruct);
  
  /* Configure EXTI for comparator output */
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_21);        // COMP1 -> EXTI21
  LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);  // Start with rising
  
  /* Enable interrupt */
  NVIC_SetPriority(ADC1_COMP_IRQn, 1);
  NVIC_EnableIRQ(ADC1_COMP_IRQn);
  
  /* Enable comparator */
  LL_COMP_Enable(COMP1);
  
  /* Wait for comparator stabilization */
  for(volatile uint32_t i = 0; i < 10000; i++);
}
```

### **Step 2: Add Commutation Timer (TIM14)**

Replace fixed delays with hardware timer:

```c
static void MX_TIM14_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);
  
  /* Configure TIM14 for commutation timing */
  TIM_InitStruct.Prescaler = 47;  // 48MHz / 48 = 1MHz (1us tick)
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 10000;  // Initial 10ms
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM14, &TIM_InitStruct);
  
  /* Enable update interrupt */
  LL_TIM_EnableIT_UPDATE(TIM14);
  
  /* Enable TIM14 interrupt */
  NVIC_SetPriority(TIM14_IRQn, 0);  // High priority
  NVIC_EnableIRQ(TIM14_IRQn);
  
  /* Enable ARR preload */
  LL_TIM_EnableARRPreload(TIM14);
}
```

### **Step 3: Add Global Variables**

Add these after your existing global variables (around line 40):

```c
/* Closed-loop control variables */
volatile uint8_t closed_loop_enabled = 0;
volatile uint8_t current_step = 0;
volatile uint32_t zc_detected_count = 0;
volatile uint32_t zc_timestamp = 0;
volatile uint32_t last_comm_time = 0;
volatile uint32_t commutation_interval = 10000;  // in microseconds
volatile uint8_t bemf_rising_edge = 1;

/* Debug/monitoring */
volatile uint32_t zc_timeout_count = 0;
volatile uint32_t valid_zc_count = 0;
```

### **Step 4: Implement Comparator Interrupt (Zero-Crossing Detection)**

Add this interrupt handler:

```c
void ADC1_COMP_IRQHandler(void)
{
  if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_21))
  {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
    
    /* Record zero-crossing timestamp */
    zc_timestamp = TIM3->CNT;  // Use TIM3 as timebase
    zc_detected_count++;
    
    if(closed_loop_enabled)
    {
      /* Calculate time since last commutation */
      uint32_t time_since_comm = zc_timestamp - last_comm_time;
      
      /* Basic filtering - ignore if too early */
      if(time_since_comm > (commutation_interval / 4))
      {
        valid_zc_count++;
        
        /* Calculate 30-degree advance time */
        uint32_t advance_time = commutation_interval / 6;
        
        /* Schedule next commutation */
        LL_TIM_SetAutoReload(TIM14, advance_time);
        LL_TIM_SetCounter(TIM14, 0);
        LL_TIM_EnableCounter(TIM14);
      }
    }
  }
}
```

### **Step 5: Implement Commutation Timer Interrupt**

Replace your polling commutation with interrupt-driven:

```c
void TIM14_IRQHandler(void)
{
  if(LL_TIM_IsActiveFlag_UPDATE(TIM14))
  {
    LL_TIM_ClearFlag_UPDATE(TIM14);
    
    /* Stop timer until next ZC */
    LL_TIM_DisableCounter(TIM14);
    
    /* Perform commutation */
    Commutation_Step(current_step);
    last_comm_time = TIM3->CNT;
    
    /* Move to next step */
    current_step++;
    if(current_step >= 6) current_step = 0;
    
    /* Update comparator edge detection based on new step */
    Update_Comparator_Edge(current_step);
  }
}
```

### **Step 6: Add Comparator Edge Switching Function**

The comparator needs to change detection edge based on expected BEMF direction:

```c
static void Update_Comparator_Edge(uint8_t step)
{
  /* Determine expected BEMF direction for floating phase */
  switch(step)
  {
    case 0: // Phase C floating, expect rising BEMF
    case 2: // Phase B floating, expect rising BEMF  
    case 4: // Phase A floating, expect rising BEMF
      LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_21);
      LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);
      bemf_rising_edge = 1;
      break;
      
    case 1: // Phase C floating, expect falling BEMF
    case 3: // Phase B floating, expect falling BEMF
    case 5: // Phase A floating, expect falling BEMF
      LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_21);
      LL_EXTI_EnableFallingTrig_0_31(LL_EXTI_LINE_21);
      bemf_rising_edge = 0;
      break;
  }
}
```

### **Step 7: Modify Main Loop for Hybrid Testing**

Replace your main loop (lines 95-115) with:

```c
  uint8_t startup_step_count = 0;
  
  while (1)
  {
    Process_RC_Input();

    if (motor_running && rc_signal_valid)
    {
      /* Open-loop startup for first 60 commutations */
      if(!closed_loop_enabled && startup_step_count < 60)
      {
        Commutation_Step(current_step);
        Update_Comparator_Edge(current_step);
        
        current_step++;
        if (current_step >= 6) current_step = 0;
        
        startup_step_count++;
        LL_mDelay(commutation_delay);
        
        /* Transition to closed-loop after startup */
        if(startup_step_count == 60)
        {
          closed_loop_enabled = 1;
          commutation_interval = commutation_delay * 1000;  // Convert ms to us
          last_comm_time = TIM3->CNT;
        }
      }
      else if(closed_loop_enabled)
      {
        /* Motor running in closed-loop, just monitor */
        LL_mDelay(10);
        
        /* Safety timeout - if no ZC for long time, restart */
        if((TIM3->CNT - zc_timestamp) > 50000)  // 50ms timeout
        {
          closed_loop_enabled = 0;
          startup_step_count = 0;
          zc_timeout_count++;
        }
      }
    }
    else
    {
      All_Phases_OFF();
      closed_loop_enabled = 0;
      startup_step_count = 0;
      current_step = 0;
      LL_mDelay(10);
    }
  }
```

### **Step 8: Update Initialization in main()**

Add these calls in `main()` after line 74:

```c
  MX_TIM14_Init();   // Commutation timer
  MX_COMP1_Init();   // Comparator for BEMF
```

---

## Hardware Connections for Comparator

**Critical:** Your comparator needs to monitor BEMF. Connect:

- **PA1 (COMP1+)**: Connect to motor phase through resistor divider
  - Motor phase → 10kΩ resistor → PA1 → 10kΩ resistor → GND
  - This creates virtual neutral point
  
- **PA5 (COMP1-)**: Reference voltage (Vbus/2)
  - Connect capacitor voltage divider: Vbus → 10kΩ → PA5 → 10kΩ → GND

**OR** simpler for initial testing:
- PA1: Connect to Phase A through 10kΩ resistor
- PA5: Connect to 1.65V (half of 3.3V logic) using resistor divider

---

## Testing Procedure

### **Phase 1: Verify Comparator Working**

Add in your main loop (testing only):

```c
// Before while(1) loop
uint8_t comp_state = 0;
uint32_t comp_toggle_count = 0;

// Inside while(1), before Process_RC_Input()
uint8_t new_comp_state = LL_COMP_ReadOutputLevel(COMP1);
if(new_comp_state != comp_state)
{
  comp_state = new_comp_state;
  comp_toggle_count++;
  // Toggle LED or set breakpoint here to verify
}
```

**Expected:** When motor spins, comparator should toggle millions of times

### **Phase 2: Monitor Zero-Crossing Count**

Watch `zc_detected_count` and `valid_zc_count` variables in debugger while motor runs.

**Expected:** Should increment rapidly when motor spins

### **Phase 3: Enable Closed-Loop**

Once you see ZC counts increasing, the code above will automatically transition after 60 open-loop steps.

---

## Key Differences from AM32

Your code vs AM32:
- **You**: Simple GPIO low-side switching (good for learning)
- **AM32**: Complementary PWM with hardware dead-time

Your comparator will work the same way, but your phase connections are different:
- **AM32**: PA7, PB0, PB1 are low-side FETs (hardware controlled)
- **You**: PA7, PB0, PB1 are low-side GPIO (software controlled)

This means you need to **manually disable the phase GPIO** when it's floating to allow BEMF detection.

---

## Critical Code Addition: Disable Floating Phase

Modify your phase control functions to ensure floating phase is truly OFF:

```c
static inline void phaseA_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1);  // Disable high-side PWM
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);       // Ensure low-side OFF
  
  // CRITICAL: Make pin high-Z for BEMF detection
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_INPUT);
}
```

Do this for all three `phaseX_OFF()` functions, and change them back to OUTPUT mode in `phaseX_PWM()` and `phaseX_LOW()`:

```c
static inline void phaseA_PWM(void)
{
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);  // Re-enable output
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
}
```

---

## Summary

**What you need to do:**
1. Add comparator initialization code
2. Add TIM14 for commutation timing
3. Implement two interrupt handlers (COMP and TIM14)
4. Modify main loop for hybrid startup
5. Make floating phases high-impedance (critical!)
6. Connect hardware voltage dividers to PA1/PA5

**Testing sequence:**
1. Test comparator output toggles when motor spins
2. Verify ZC interrupt fires
3. Run hybrid mode (open-loop startup → closed-loop)
4. Tune filter levels and advance timing

