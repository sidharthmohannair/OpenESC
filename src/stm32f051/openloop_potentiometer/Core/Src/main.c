#include "main.h"

#define SYSCLK_HZ       48000000UL
#define PWM_FREQ_HZ     20000UL
#define TIM_PSC_VALUE   0U
#define TIM_ARR_VALUE   ((SYSCLK_HZ / PWM_FREQ_HZ) - 1U)  /* 2399 */

#define SOFT_DT_CYCLES  14U
#define DUTY_MIN        70U
#define DUTY_MAX        (TIM_ARR_VALUE - 70U)
volatile uint32_t system_tick = 0;

/* Function prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static uint16_t Read_Potentiometer(void);

static inline void short_delay_cycles(uint32_t c) { while(c--) __NOP(); }

static inline void phaseA_PWM(void) { LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7); short_delay_cycles(SOFT_DT_CYCLES); LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1); }
static inline void phaseB_PWM(void) { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0); short_delay_cycles(SOFT_DT_CYCLES); LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2); }
static inline void phaseC_PWM(void) { LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1); short_delay_cycles(SOFT_DT_CYCLES); LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3); }

static inline void phaseA_LOW(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1); short_delay_cycles(SOFT_DT_CYCLES); LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_7); }
static inline void phaseB_LOW(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2); short_delay_cycles(SOFT_DT_CYCLES); LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_0); }
static inline void phaseC_LOW(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3); short_delay_cycles(SOFT_DT_CYCLES); LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_1); }

static inline void phaseA_OFF(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH1); LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7); }
static inline void phaseB_OFF(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2); LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0); }
static inline void phaseC_OFF(void) { LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3); LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1); }

int main(void)
{
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriority(SysTick_IRQn, 3);

  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();

  /* Start PWM */
  LL_TIM_EnableARRPreload(TIM1);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2 | LL_TIM_CHANNEL_CH3);
  LL_TIM_OC_SetCompareCH1(TIM1, 0);
  LL_TIM_OC_SetCompareCH2(TIM1, 0);
  LL_TIM_OC_SetCompareCH3(TIM1, 0);

  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2 | LL_TIM_CHANNEL_CH3);
  LL_TIM_EnableAllOutputs(TIM1);
  LL_TIM_EnableCounter(TIM1);

  LL_mDelay(100);

  uint16_t duty = 0;

  while (1)
  {
    duty = Read_Potentiometer();

    LL_TIM_OC_SetCompareCH1(TIM1, duty);
    LL_TIM_OC_SetCompareCH2(TIM1, duty);
    LL_TIM_OC_SetCompareCH3(TIM1, duty);

    /* 6-Step Sequence */
    phaseA_PWM(); phaseB_LOW(); phaseC_OFF(); LL_mDelay(2);
    phaseC_PWM(); phaseB_LOW(); phaseA_OFF(); LL_mDelay(2);
    phaseC_PWM(); phaseA_LOW(); phaseB_OFF(); LL_mDelay(2);
    phaseB_PWM(); phaseA_LOW(); phaseC_OFF(); LL_mDelay(2);
    phaseB_PWM(); phaseC_LOW(); phaseA_OFF(); LL_mDelay(2);
    phaseA_PWM(); phaseC_LOW(); phaseB_OFF(); LL_mDelay(2);
  }
}

/* ==================== ADC + Potentiometer (F051 Correct!) ==================== */
static void MX_ADC1_Init(void)
{
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_ADC1);

  /* PA0 = ADC_IN0 */
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Enable HSI14 - dedicated ADC clock on F0 */
  LL_RCC_HSI14_Enable();
  while (!LL_RCC_HSI14_IsReady()) {}

  /* Calibration */
  LL_ADC_StartCalibration(ADC1);
  while (LL_ADC_IsCalibrationOnGoing(ADC1)) {}

  /* ADC config */
  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  ADC_InitStruct.Clock = LL_ADC_CLOCK_ASYNC;
  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  LL_ADC_Init(ADC1, &ADC_InitStruct);

  LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_0);

  LL_ADC_Enable(ADC1);
  while (!LL_ADC_IsActiveFlag_ADRDY(ADC1)) {}
}

static uint16_t Read_Potentiometer(void)
{
  static uint32_t smooth = 0;
  uint16_t raw;

  LL_ADC_REG_StartConversion(ADC1);
  while (!LL_ADC_IsActiveFlag_EOC(ADC1)) {}
  raw = LL_ADC_REG_ReadConversionData12(ADC1);
  LL_ADC_ClearFlag_EOC(ADC1);           // Mandatory on F0!

  smooth = smooth - (smooth >> 4) + raw;
  raw = (uint16_t)(smooth >> 4);

  uint32_t duty = DUTY_MIN + ((uint32_t)raw * (DUTY_MAX - DUTY_MIN)) / 4095UL;
  return (uint16_t)duty;
}

/* ==================== System & Peripherals ==================== */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1);

  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1);

  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
  LL_RCC_PLL_Enable();
  while(LL_RCC_PLL_IsReady() != 1);

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

  LL_Init1msTick(SYSCLK_HZ);
  LL_SetSystemCoreClock(SYSCLK_HZ);
}

static void MX_TIM1_Init(void)
{
  LL_TIM_InitTypeDef TIM_InitStruct = {0};
  LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {0};
  LL_TIM_BDTR_InitTypeDef BDTR_InitStruct = {0};

  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);

  TIM_InitStruct.Prescaler = TIM_PSC_VALUE;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
  TIM_InitStruct.Autoreload = TIM_ARR_VALUE;
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  LL_TIM_Init(TIM1, &TIM_InitStruct);
  LL_TIM_EnableARRPreload(TIM1);

  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.CompareValue = 0;
  TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH2, &TIM_OC_InitStruct);
  LL_TIM_OC_Init(TIM1, LL_TIM_CHANNEL_CH3, &TIM_OC_InitStruct);

  BDTR_InitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_ENABLE;
  LL_TIM_BDTR_Init(TIM1, &BDTR_InitStruct);
}

static void MX_GPIO_Init(void)
{
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB);

  /* High-side PWM: PA8, PA9, PA10 */
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Low-side: PA7, PB0, PB1 */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0 | LL_GPIO_PIN_1);
}

void Error_Handler(void) { while(1); }
