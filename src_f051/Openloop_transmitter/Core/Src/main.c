/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file : main.c
  * @brief : Edge-aligned PWM + 6-step commutation with RC receiver control
  *          Single direction ESC (1000-2000us throttle range)
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

/* RC Input defines - Single direction mode */
#define RC_MIN_PULSE 1000U        /* Minimum throttle (motor OFF) */
#define RC_MAX_PULSE 2000U        /* Maximum throttle (full speed) */
#define RC_STARTUP_THRESHOLD 1100U /* Motor starts above this value */
#define RC_DEADBAND 20U           /* Small deadband at minimum */
#define RC_TIMEOUT_MS 100U

#define MIN_DUTY_CYCLE 0U
#define MAX_DUTY_CYCLE TIM_ARR_VALUE
#define MIN_MOTOR_DUTY (TIM_ARR_VALUE/10U) /* 10% minimum duty when running */

#define COMMUTATION_DELAY_MIN 2U
#define COMMUTATION_DELAY_MAX 2U

volatile uint32_t rc_pulse_width = RC_MIN_PULSE;
volatile uint8_t rc_signal_valid = 0;
volatile uint32_t rc_capture_start = 0;
volatile uint32_t rc_last_update = 0;
volatile uint32_t system_tick = 0;

uint32_t current_duty = INITIAL_DUTY;
uint32_t commutation_delay = 10;
uint8_t motor_running = 0;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM3_Init(void);
void Process_RC_Input(void);
uint32_t Map_RC_To_Duty(uint32_t pulse_width);
uint32_t Map_RC_To_CommDelay(uint32_t pulse_width);

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

int main(void)
{
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriority(SysTick_IRQn, 3);

  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM3_Init();

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

  uint8_t current_step = 0;

  while (1)
  {
    Process_RC_Input();

    if (motor_running && rc_signal_valid)
    {
      /* Forward direction only - commutation sequence */
      Commutation_Step(current_step);
      current_step++;
      if (current_step >= 6) current_step = 0;

      LL_mDelay(commutation_delay);
    }
    else
    {
      All_Phases_OFF();
      LL_mDelay(10);
    }
  }
}

void Process_RC_Input(void)
{
  uint32_t current_time = system_tick;

  /* Check for RC signal timeout */
  if ((current_time - rc_last_update) > RC_TIMEOUT_MS)
  {
    rc_signal_valid = 0;
    motor_running = 0;
    All_Phases_OFF();
    return;
  }

  if (!rc_signal_valid) return;

  uint32_t pulse = rc_pulse_width;

  /* Check if throttle is below startup threshold (motor OFF zone) */
  if (pulse < (RC_MIN_PULSE + RC_STARTUP_THRESHOLD - RC_MIN_PULSE))
  {
    motor_running = 0;
    All_Phases_OFF();
    return;
  }

  /* Motor running - map throttle to speed */
  motor_running = 1;
  current_duty = Map_RC_To_Duty(pulse);
  commutation_delay = Map_RC_To_CommDelay(pulse);

  /* Update PWM duty cycles */
  LL_TIM_OC_SetCompareCH1(TIM1, current_duty);
  LL_TIM_OC_SetCompareCH2(TIM1, current_duty);
  LL_TIM_OC_SetCompareCH3(TIM1, current_duty);
}

uint32_t Map_RC_To_Duty(uint32_t pulse_width)
{
  /* Clamp pulse width to valid range */
  if (pulse_width < RC_STARTUP_THRESHOLD)
    return MIN_DUTY_CYCLE;
  if (pulse_width > RC_MAX_PULSE)
    pulse_width = RC_MAX_PULSE;

  /* Map from RC_STARTUP_THRESHOLD to RC_MAX_PULSE -> MIN_MOTOR_DUTY to MAX_DUTY_CYCLE */
  uint32_t range_in = RC_MAX_PULSE - RC_STARTUP_THRESHOLD;
  uint32_t range_out = MAX_DUTY_CYCLE - MIN_MOTOR_DUTY;
  uint32_t pulse_offset = pulse_width - RC_STARTUP_THRESHOLD;

  return MIN_MOTOR_DUTY + ((pulse_offset * range_out) / range_in);
}

uint32_t Map_RC_To_CommDelay(uint32_t pulse_width)
{
  /* Clamp pulse width to valid range */
  if (pulse_width < RC_STARTUP_THRESHOLD)
    return COMMUTATION_DELAY_MAX;
  if (pulse_width > RC_MAX_PULSE)
    pulse_width = RC_MAX_PULSE;

  /* Map from RC_STARTUP_THRESHOLD to RC_MAX_PULSE -> DELAY_MAX to DELAY_MIN */
  /* Higher throttle = shorter delay = faster rotation */
  uint32_t range_in = RC_MAX_PULSE - RC_STARTUP_THRESHOLD;
  uint32_t range_out = COMMUTATION_DELAY_MAX - COMMUTATION_DELAY_MIN;
  uint32_t pulse_offset = pulse_width - RC_STARTUP_THRESHOLD;

  return COMMUTATION_DELAY_MAX - ((pulse_offset * range_out) / range_in);
}

static inline void Commutation_Step(uint8_t step)
{
  /* Single direction commutation sequence */
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

static inline void short_delay_cycles(uint32_t cycles)
{
  while (cycles--) { __NOP(); }
}

static inline void phaseA_PWM(void)
{
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
}

static inline void phaseB_PWM(void)
{
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
}

static inline void phaseC_PWM(void)
{
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
}

static inline void phaseA_LOW(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7);
}

static inline void phaseB_LOW(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0);
}

static inline void phaseC_LOW(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3);
  short_delay_cycles(SOFT_DT_CYCLES);
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

static inline void phaseA_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
}

static inline void phaseB_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
}

static inline void phaseC_OFF(void)
{
  LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
}

static inline void All_Phases_OFF(void)
{
  phaseA_OFF();
  phaseB_OFF();
  phaseC_OFF();
}

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

static void MX_TIM3_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_IC_InitTypeDef TIM_IC_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
  NVIC_SetPriority(TIM3_IRQn, 0);
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

static void MX_GPIO_Init(void) { }

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

void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}
