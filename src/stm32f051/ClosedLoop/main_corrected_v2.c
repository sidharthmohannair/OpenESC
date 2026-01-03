/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file : main.c
  * @brief : BLDC Closed-Loop Control with Zero-Crossing Detection
  *          Architecture: Following AM32 reference implementation
  *          
  *  Timer Architecture:
  *  - TIM1:  PWM generation (20kHz)
  *  - TIM2:  Zero-crossing interval measurement (free-running, resets on ZC)
  *  - TIM3:  RC input capture
  *  - TIM14: Commutation scheduling (30° advance after ZC)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

#define SYSCLK_HZ 48000000U
#define PWM_FREQ_HZ 20000U
#define TIM_PSC_VALUE 0U
#define TIM_ARR_VALUE (SYSCLK_HZ / PWM_FREQ_HZ - 1U)
#define INITIAL_DUTY (TIM_ARR_VALUE/4U)
#define SOFT_DT_CYCLES 14U

/* RC Input defines */
#define RC_MIN_PULSE 1000U
#define RC_MAX_PULSE 2000U
#define RC_STARTUP_THRESHOLD 1100U
#define RC_TIMEOUT_MS 100U

#define MIN_DUTY_CYCLE 0U
#define MAX_DUTY_CYCLE TIM_ARR_VALUE
#define MIN_MOTOR_DUTY (TIM_ARR_VALUE/10U)

#define COMMUTATION_DELAY_MIN 2U
#define COMMUTATION_DELAY_MAX 10U

/* Zero-crossing detection parameters */
#define ZC_FILTER_LEVEL 3           // Number of samples for BEMF stability check
#define MIN_ZC_TIME_DIVISOR 2       // Minimum time = interval/2 before accepting ZC
#define ADVANCE_DIVISOR 6           // 30° advance = interval/6

/* Debug LED */
#define LED_PORT GPIOC
#define LED_PIN  LL_GPIO_PIN_13

/* ==================== GLOBAL VARIABLES ==================== */

/* RC Input */
volatile uint32_t rc_pulse_width = RC_MIN_PULSE;
volatile uint8_t rc_signal_valid = 0;
volatile uint32_t rc_capture_start = 0;
volatile uint32_t rc_last_update = 0;
volatile uint32_t system_tick = 0;

/* Motor control */
uint32_t current_duty = INITIAL_DUTY;
uint32_t commutation_delay = 10;
uint8_t motor_running = 0;

/* Closed-loop control */
volatile uint8_t closed_loop_enabled = 0;
volatile uint8_t current_step = 0;
volatile uint32_t commutation_interval = 10000;  // In microseconds
volatile uint32_t thiszctime = 0;                // Current ZC timestamp
volatile uint32_t last_comm_time = 0;
volatile uint32_t advance_time = 0;
volatile uint8_t bemf_rising_edge = 1;
volatile uint8_t filter_level = ZC_FILTER_LEVEL;

/* Debug counters - MONITOR THESE IN DEBUGGER */
volatile uint32_t startup_step_count = 0;
volatile uint32_t zc_detected_count = 0;        // Total ZC interrupts
volatile uint32_t valid_zc_count = 0;           // ZC that passed filter
volatile uint32_t zc_filtered_early_count = 0;  // Rejected: too early
volatile uint32_t zc_filtered_unstable_count = 0; // Rejected: BEMF unstable
volatile uint32_t tim14_fire_count = 0;         // Actual commutations in closed-loop
volatile uint32_t zc_timeout_count = 0;         // Timeout events
volatile uint32_t comp_toggle_count = 0;        // Comparator state changes (for testing)

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);   // Zero-crossing interval timer
static void MX_TIM3_Init(void);   // RC input capture
static void MX_TIM14_Init(void);  // Commutation timer
static void MX_COMP1_Init(void);

void Process_RC_Input(void);
uint32_t Map_RC_To_Duty(uint32_t pulse_width);
uint32_t Map_RC_To_CommDelay(uint32_t pulse_width);
static void Update_Comparator_Edge(uint8_t step);
static uint8_t Check_BEMF_Stable(void);

static inline void short_delay_cycles(uint32_t cycles);
static inline void phaseA_PWM(void);
static inline void phaseB_PWM(void);
static inline void phaseC_PWM(void);
static inline void phaseA_LOW(void);
static inline void phaseB_LOW(void);
static inline void phaseC_LOW(void);
static inline void phaseA_OFF(void);
static inline void phaseB_OFF(void);
static inline void phaseC_OFF(void);
static inline void All_Phases_OFF(void);
static inline void Commutation_Step(uint8_t step);

/* ==================== MAIN ==================== */

int main(void)
{
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriority(SysTick_IRQn, 3);

  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();   // Zero-crossing timer
  MX_TIM3_Init();   // RC input
  MX_TIM14_Init();  // Commutation timer
  MX_COMP1_Init();  // BEMF comparator

  /* Initialize TIM1 PWM */
  LL_TIM_EnableARRPreload(TIM1);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH3);

  LL_TIM_OC_SetCompareCH1(TIM1, INITIAL_DUTY);
  LL_TIM_OC_SetCompareCH2(TIM1, INITIAL_DUTY);
  LL_TIM_OC_SetCompareCH3(TIM1, INITIAL_DUTY);

  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);

  LL_TIM_EnableAllOutputs(TIM1);
  LL_TIM_EnableCounter(TIM1);
  LL_TIM_GenerateEvent_UPDATE(TIM1);

  rc_last_update = system_tick;

  /* Signal LED: System ready */
  LL_GPIO_ResetOutputPin(LED_PORT, LED_PIN);  // LED ON
  LL_mDelay(500);
  LL_GPIO_SetOutputPin(LED_PORT, LED_PIN);    // LED OFF

  /* ==================== MAIN LOOP ==================== */
  while (1)
  {
    Process_RC_Input();

    if (motor_running && rc_signal_valid)
    {
      /* ===== OPEN-LOOP STARTUP (First 60 commutations) ===== */
      if (!closed_loop_enabled && startup_step_count < 60)
      {
        Commutation_Step(current_step);
        Update_Comparator_Edge(current_step);

        current_step++;
        if (current_step >= 6) current_step = 0;

        startup_step_count++;
        LL_mDelay(commutation_delay);

        /* Transition to closed-loop after 60 steps */
        if (startup_step_count == 60)
        {
          closed_loop_enabled = 1;
          commutation_interval = commutation_delay * 1000;  // ms to µs
          
          /* Reset TIM2 for zero-crossing timing */
          LL_TIM_SetCounter(TIM2, 0);
          last_comm_time = 0;
          
          /* Flash LED to indicate closed-loop transition */
          for(int i = 0; i < 3; i++) {
            LL_GPIO_TogglePin(LED_PORT, LED_PIN);
            LL_mDelay(50);
          }
        }
      }
      /* ===== CLOSED-LOOP OPERATION ===== */
      else if (closed_loop_enabled)
      {
        /* Monitor mode - commutation handled by interrupts */
        LL_mDelay(10);

        /* Safety timeout - if no valid ZC for 50ms, restart */
        if ((TIM2->CNT - thiszctime) > 50000)
        {
          closed_loop_enabled = 0;
          startup_step_count = 0;
          zc_timeout_count++;
          
          /* Flash LED rapidly to indicate timeout */
          for(int i = 0; i < 5; i++) {
            LL_GPIO_TogglePin(LED_PORT, LED_PIN);
            LL_mDelay(30);
          }
        }
      }
    }
    else
    {
      /* Motor stopped */
      All_Phases_OFF();
      closed_loop_enabled = 0;
      startup_step_count = 0;
      current_step = 0;
      LL_mDelay(10);
    }
  }
}

/* ==================== RC INPUT PROCESSING ==================== */

void Process_RC_Input(void)
{
  uint32_t current_time = system_tick;

  if ((current_time - rc_last_update) > RC_TIMEOUT_MS)
  {
    rc_signal_valid = 0;
    motor_running = 0;
    All_Phases_OFF();
    return;
  }

  if (!rc_signal_valid) return;

  uint32_t pulse = rc_pulse_width;

  if (pulse < RC_STARTUP_THRESHOLD)
  {
    motor_running = 0;
    All_Phases_OFF();
    return;
  }

  motor_running = 1;
  current_duty = Map_RC_To_Duty(pulse);
  commutation_delay = Map_RC_To_CommDelay(pulse);

  LL_TIM_OC_SetCompareCH1(TIM1, current_duty);
  LL_TIM_OC_SetCompareCH2(TIM1, current_duty);
  LL_TIM_OC_SetCompareCH3(TIM1, current_duty);
}

uint32_t Map_RC_To_Duty(uint32_t pulse_width)
{
  if (pulse_width < RC_STARTUP_THRESHOLD)
    return MIN_DUTY_CYCLE;
  if (pulse_width > RC_MAX_PULSE)
    pulse_width = RC_MAX_PULSE;

  uint32_t range_in = RC_MAX_PULSE - RC_STARTUP_THRESHOLD;
  uint32_t range_out = MAX_DUTY_CYCLE - MIN_MOTOR_DUTY;
  uint32_t pulse_offset = pulse_width - RC_STARTUP_THRESHOLD;

  return MIN_MOTOR_DUTY + ((pulse_offset * range_out) / range_in);
}

uint32_t Map_RC_To_CommDelay(uint32_t pulse_width)
{
  if (pulse_width < RC_STARTUP_THRESHOLD)
    return COMMUTATION_DELAY_MAX;
  if (pulse_width > RC_MAX_PULSE)
    pulse_width = RC_MAX_PULSE;

  uint32_t range_in = RC_MAX_PULSE - RC_STARTUP_THRESHOLD;
  uint32_t range_out = COMMUTATION_DELAY_MAX - COMMUTATION_DELAY_MIN;
  uint32_t pulse_offset = pulse_width - RC_STARTUP_THRESHOLD;

  return COMMUTATION_DELAY_MAX - ((pulse_offset * range_out) / range_in);
}

/* ==================== COMMUTATION ==================== */

static inline void Commutation_Step(uint8_t step)
{
  switch(step)
  {
    case 0: phaseA_PWM(); phaseB_LOW(); phaseC_OFF(); break;
    case 1: phaseC_PWM(); phaseB_LOW(); phaseA_OFF(); break;
    case 2: phaseC_PWM(); phaseA_LOW(); phaseB_OFF(); break;
    case 3: phaseB_PWM(); phaseA_LOW(); phaseC_OFF(); break;
    case 4: phaseB_PWM(); phaseC_LOW(); phaseA_OFF(); break;
    case 5: phaseA_PWM(); phaseC_LOW(); phaseB_OFF(); break;
    default: All_Phases_OFF(); break;
  }
}

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

/* Check BEMF stability - multi-sample filter like AM32 */
static uint8_t Check_BEMF_Stable(void)
{
  uint8_t expected_level = bemf_rising_edge ? LL_COMP_OUTPUT_LEVEL_LOW : LL_COMP_OUTPUT_LEVEL_HIGH;
  
  for(uint8_t i = 0; i < filter_level; i++)
  {
    if(LL_COMP_ReadOutputLevel(COMP1) != expected_level)
    {
      return 0;  // Unstable
    }
    /* Small delay between samples */
    for(volatile uint8_t d = 0; d < 10; d++);
  }
  return 1;  // Stable
}

/* ==================== PHASE CONTROL ==================== */

static inline void short_delay_cycles(uint32_t cycles)
{
  while (cycles--) { __NOP(); }
}

static inline void phaseA_PWM(void)
{
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
}

static inline void phaseB_PWM(void)
{
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
}

static inline void phaseC_PWM(void)
{
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
}

static inline void phaseA_LOW(void)
{
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7);
}

static inline void phaseB_LOW(void)
{
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0);
}

static inline void phaseC_LOW(void)
{
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

static inline void phaseA_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  /* Make pin high-Z for BEMF detection */
  LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_7, LL_GPIO_MODE_INPUT);
}

static inline void phaseB_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
  /* Make pin high-Z for BEMF detection */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);
}

static inline void phaseC_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
  /* Make pin high-Z for BEMF detection */
  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_INPUT);
}

static inline void All_Phases_OFF(void)
{
  phaseA_OFF();
  phaseB_OFF();
  phaseC_OFF();
}

/* ==================== INTERRUPT HANDLERS ==================== */

/* Zero-Crossing Detection - Following AM32 interruptRoutine() */
void ADC1_COMP_IRQHandler(void)
{
  if(LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_21))
  {
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_21);
    
    zc_detected_count++;  // Total ZC interrupts (for debugging)

    if(!closed_loop_enabled) return;  // Ignore during open-loop
    
    /* Record zero-crossing time */
    thiszctime = TIM2->CNT;
    
    /* FILTER 1: Minimum time check - ignore if too early */
    if(thiszctime < (commutation_interval / MIN_ZC_TIME_DIVISOR))
    {
      zc_filtered_early_count++;
      return;
    }
    
    /* FILTER 2: BEMF stability check - multi-sample like AM32 */
    if(!Check_BEMF_Stable())
    {
      zc_filtered_unstable_count++;
      return;
    }
    
    /* Valid zero-crossing detected! */
    valid_zc_count++;
    
    /* Update commutation interval with moving average (like AM32) */
    commutation_interval = (thiszctime + (3 * commutation_interval)) >> 2;
    
    /* Reset interval timer for next cycle */
    LL_TIM_SetCounter(TIM2, 0);
    
    /* Calculate 30-degree advance time */
    advance_time = commutation_interval / ADVANCE_DIVISOR;
    
    /* Schedule commutation via TIM14 */
    LL_TIM_SetAutoReload(TIM14, advance_time);
    LL_TIM_SetCounter(TIM14, 0);
    LL_TIM_EnableCounter(TIM14);
  }
}

/* Commutation Timer - Following AM32 PeriodElapsedCallback() */
void TIM14_IRQHandler(void)
{
  if(LL_TIM_IsActiveFlag_UPDATE(TIM14))
  {
    LL_TIM_ClearFlag_UPDATE(TIM14);
    LL_TIM_DisableCounter(TIM14);  // One-shot mode
    
    tim14_fire_count++;  // Count actual commutations
    
    /* Perform commutation */
    Commutation_Step(current_step);
    
    /* Move to next step */
    current_step++;
    if(current_step >= 6) current_step = 0;
    
    /* Update comparator edge for new floating phase */
    Update_Comparator_Edge(current_step);
  }
}

/* RC Input Capture */
void TIM3_IRQHandler(void)
{
  if (LL_TIM_IsActiveFlag_CC1(TIM3))
  {
    LL_TIM_ClearFlag_CC1(TIM3);
    uint32_t capture_value = LL_TIM_IC_GetCaptureCH1(TIM3);

    if (LL_TIM_IC_GetPolarity(TIM3, LL_TIM_CHANNEL_CH1) == LL_TIM_IC_POLARITY_RISING)
    {
      rc_capture_start = capture_value;
      LL_TIM_IC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_FALLING);
    }
    else
    {
      uint32_t pulse_width;
      if (capture_value >= rc_capture_start)
        pulse_width = capture_value - rc_capture_start;
      else
        pulse_width = (0xFFFF - rc_capture_start) + capture_value + 1;

      if (pulse_width >= 800 && pulse_width <= 2200)
      {
        rc_pulse_width = pulse_width;
        rc_signal_valid = 1;
        rc_last_update = system_tick;
      }
      LL_TIM_IC_SetPolarity(TIM3, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
    }
  }
}

/* ==================== PERIPHERAL INITIALIZATION ==================== */

void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {}

  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1) {}
  LL_RCC_HSI_SetCalibTrimming(16);

  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1) {}

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL) {}

  LL_Init1msTick(SYSCLK_HZ);
  LL_SetSystemCoreClock(SYSCLK_HZ);
}

static void MX_GPIO_Init(void)
{
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LED_PIN;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

  /* LED OFF initially */
  LL_GPIO_SetOutputPin(LED_PORT, LED_PIN);
}

static void MX_TIM1_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
  LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {0};

  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

  TIM_InitStruct.Prescaler = TIM_PSC_VALUE;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = TIM_ARR_VALUE;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;
  LL_TIM_Init(TIM1, &TIM_InitStruct);
  LL_TIM_EnableARRPreload(TIM1);

  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
  TIM_OC_InitStruct.CompareValue = INITIAL_DUTY;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
  TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
  TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;

  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);

  LL_TIM_OC_DisableFast(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_DisableFast(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_DisableFast(TIM1, LL_TIM_CHANNEL_CH3);

  TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
  TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
  TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
  TIM_BDTRInitStruct.DeadTime = 0;
  TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
  TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
  TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_ENABLE;
  LL_TIM_BDTR_Init(TIM1, &TIM_BDTRInitStruct);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

/* TIM2: Zero-crossing interval timer (like AM32 INTERVAL_TIMER) */
static void MX_TIM2_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
  
  /* Configure as free-running microsecond counter */
  TIM_InitStruct.Prescaler = 47;  // 48MHz / 48 = 1MHz (1µs tick)
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 0xFFFFFFFF;  // 32-bit, won't overflow for hours
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM2, &TIM_InitStruct);
  
  LL_TIM_DisableARRPreload(TIM2);
  LL_TIM_SetClockSource(TIM2, LL_TIM_CLOCKSOURCE_INTERNAL);
  
  /* Start timer */
  LL_TIM_EnableCounter(TIM2);
}

/* TIM3: RC input capture */
static void MX_TIM3_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_IC_InitTypeDef TIM_IC_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
  NVIC_SetPriority(TIM3_IRQn, 2);  // Lower priority than ZC
  NVIC_EnableIRQ(TIM3_IRQn);

  TIM_InitStruct.Prescaler = 47;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 0xFFFF;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM3, &TIM_InitStruct);
  LL_TIM_DisableARRPreload(TIM3);
  LL_TIM_SetClockSource(TIM3, LL_TIM_CLOCKSOURCE_INTERNAL);

  TIM_IC_InitStruct.ICPolarity = LL_TIM_IC_POLARITY_RISING;
  TIM_IC_InitStruct.ICActiveInput = LL_TIM_ACTIVEINPUT_DIRECTTI;
  TIM_IC_InitStruct.ICPrescaler = LL_TIM_ICPSC_DIV1;
  TIM_IC_InitStruct.ICFilter = LL_TIM_IC_FILTER_FDIV1_N2;
  LL_TIM_IC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_IC_InitStruct);

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_1;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  LL_TIM_EnableIT_CC1(TIM3);
  LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);
  LL_TIM_EnableCounter(TIM3);
}

/* TIM14: Commutation scheduling timer (like AM32 COM_TIMER) */
static void MX_TIM14_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM14);

  TIM_InitStruct.Prescaler = 47;  // 48MHz / 48 = 1MHz (1µs tick)
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = 10000;  // Will be set dynamically
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM14, &TIM_InitStruct);

  LL_TIM_EnableIT_UPDATE(TIM14);
  NVIC_SetPriority(TIM14_IRQn, 0);  // Highest priority
  NVIC_EnableIRQ(TIM14_IRQn);

  LL_TIM_EnableARRPreload(TIM14);
  /* Don't start timer - will be started by ZC interrupt */
}

/* Comparator: BEMF detection */
static void MX_COMP1_Init(void)
{
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_COMP_InitTypeDef COMP_InitStruct = {0};

  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

  /* PA1: COMP1 positive input */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* PA5: COMP1 negative reference */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);

  COMP_InitStruct.PowerMode = LL_COMP_POWERMODE_HIGHSPEED;
  COMP_InitStruct.InputPlus = LL_COMP_INPUT_PLUS_IO1;
  COMP_InitStruct.InputMinus = LL_COMP_INPUT_MINUS_IO1;
  COMP_InitStruct.InputHysteresis = LL_COMP_HYSTERESIS_MEDIUM;
  COMP_InitStruct.OutputPolarity = LL_COMP_OUTPUTPOL_NONINVERTED;
  LL_COMP_Init(COMP1, &COMP_InitStruct);

  /* EXTI configuration */
  LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_21);
  LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_21);

  NVIC_SetPriority(ADC1_COMP_IRQn, 1);
  NVIC_EnableIRQ(ADC1_COMP_IRQn);

  LL_COMP_Enable(COMP1);

  /* Stabilization delay */
  for(volatile uint32_t i = 0; i < 10000; i++);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) 
  {
    /* Flash LED rapidly on error */
    LL_GPIO_TogglePin(LED_PORT, LED_PIN);
    for(volatile uint32_t i = 0; i < 100000; i++);
  }
}
