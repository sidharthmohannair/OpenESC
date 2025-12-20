/* USER CODE BEGIN Header */
/****Authors:** Aiswarya Babu, Muhammed Afsal
  ******************************************************************************
  * @file : main.c
  * @brief : Edge-aligned PWM + 6-step commutation (STM32F051, LL)
  *
  * System clock: 48 MHz (HSI/PLL)
  * PWM frequency: 20 kHz
  * TIM1 uses CH1/CH2/CH3 on PA8/PA9/PA10 as high-side PWM
  * Low-side MOSFETs driven by GPIO: PA7 (AL), PB0 (BL), PB1 (CL)
  * Software dead-time used (approx ~300 ns at 48 MHz)
  ******************************************************************************
  */
/* USER CODE END Header */
 
/* Includes ------------------------------------------------------------------*/
#include "main.h"
 
/* Private define ------------------------------------------------------------*/
#define SYSCLK_HZ 48000000U /* System clock now 48 MHz */
#define PWM_FREQ_HZ 20000U /* desired PWM frequency */
#define TIM_PSC_VALUE 0U
#define TIM_ARR_VALUE (SYSCLK_HZ / PWM_FREQ_HZ - 1U) /* 2399 for 48MHz/20kHz */
 
#define INITIAL_DUTY (TIM_ARR_VALUE/2U) /* 50% initial */
 
/* soft dead-time (cycles) — tune to get ~100-300ns for your MCU/clock */
#define SOFT_DT_CYCLES 14U /* ~291 ns @ 48 MHz */
 
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
 
/* helper commutation helpers */
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
 
int main(void)
{
  /* Enable necessary clocks for system config */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
 
  NVIC_SetPriority(SysTick_IRQn, 3);
 
  /* Configure system clock to 48 MHz */
  SystemClock_Config();
 
  /* Initialize peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
 
  /* === Start PWM outputs: TIM1 configured in MX_TIM1_Init() === */
  LL_TIM_EnableARRPreload(TIM1);
 
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_EnablePreload(TIM1, LL_TIM_CHANNEL_CH3);
 
  /* Set initial duty (50%) */
  LL_TIM_OC_SetCompareCH1(TIM1, INITIAL_DUTY);
  LL_TIM_OC_SetCompareCH2(TIM1, INITIAL_DUTY);
  LL_TIM_OC_SetCompareCH3(TIM1, INITIAL_DUTY);
 
  /* Enable capture/compare outputs for channels (these drive PA8/9/10) */
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH1);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
  LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
 
  /* Enable Main Output and Counter */
  LL_TIM_EnableAllOutputs(TIM1);
  LL_TIM_EnableCounter(TIM1);
  LL_TIM_GenerateEvent_UPDATE(TIM1);
 
  /* Simple 6-step commutation loop (for demonstration) */
  while (1)
  {
    /* STEP 1: AH (PWM) + BL (LOW) */
    phaseA_PWM(); phaseB_LOW(); phaseC_OFF();
    LL_mDelay(1);
 
    /* STEP 2: CH (PWM) + BL (LOW) */
    phaseC_PWM(); phaseB_LOW(); phaseA_OFF();
    LL_mDelay(1);
 
    /* STEP 3: CH (PWM) + AL (LOW) */
    phaseC_PWM(); phaseA_LOW(); phaseB_OFF();
    LL_mDelay(1);
 
    /* STEP 4: BH (PWM) + AL (LOW) */
    phaseB_PWM(); phaseA_LOW(); phaseC_OFF();
    LL_mDelay(1);
 
    /* STEP 5: BH (PWM) + CL (LOW) */
    phaseB_PWM(); phaseC_LOW(); phaseA_OFF();
    LL_mDelay(1);
 
    /* STEP 6: AH (PWM) + CL (LOW) */
    phaseA_PWM(); phaseC_LOW(); phaseB_OFF();
    LL_mDelay(1);
  }
}
 
/* ----------------------- helper functions ---------------------------------*/
 
/* short busy-wait loop used for tiny software dead-time.
   For 48 MHz: 1 cycle = 20.833 ns -> SOFT_DT_CYCLES ~14 => ~291 ns
*/
static inline void short_delay_cycles(uint32_t cycles)
{
  while (cycles--) {
    __NOP();
  }
}
 
/* Phase helper implementations
   Policy:
   - When enabling high-side PWM: ensure corresponding low-side GPIO is OFF (reset) first.
   - When enabling low-side: disable the PWM channel first, then set GPIO.
   - Insert tiny software dead-time (short_delay_cycles) between switching actions.
*/
 
static inline void phaseA_PWM(void)
{
  /* Ensure AL is OFF */
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  short_delay_cycles(SOFT_DT_CYCLES);
 
  /* Enable CH1 PWM (PA8) */
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
  /* Turn off CH1 PWM then enable AL */
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
 
/* ----------------------- peripheral init ---------------------------------*/
 
void SystemClock_Config(void)
{
  /* Configure system clock to 48 MHz: HSI/2 * PLL MUL12 = 48 MHz */
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {}
 
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1) {}
 
  LL_RCC_HSI_SetCalibTrimming(16);
 
  /* PLL = HSI/2 * 12 -> 48 MHz */
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
 
  /* Enable TIM1 clock */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);
 
  /* Basic timer configuration */
  TIM_InitStruct.Prescaler = TIM_PSC_VALUE;
  TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP; /* edge-aligned */
  TIM_InitStruct.Autoreload = TIM_ARR_VALUE; /* 2399 */
  TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
  TIM_InitStruct.RepetitionCounter = 0;
  LL_TIM_Init(TIM1, &TIM_InitStruct);
 
  /* Enable ARR preload so updates take effect at update event */
  LL_TIM_EnableARRPreload(TIM1);
 
  /* OC configuration for 3 channels */
  TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
  TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
  TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE; /* F0: complementary not used */
  TIM_OC_InitStruct.CompareValue = INITIAL_DUTY; /* 50% */
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
 
  /* BDTR: NOTE - hardware dead-time requires complementary outputs.
     On F051 we do NOT use CHxN, so BDTR dead-time is not functional for CHxN.
     Still set AutomaticOutput = ENABLE so MOE works.
  */
  TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
  TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
  TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
  TIM_BDTRInitStruct.DeadTime = 0; /* not used with no CHxN on F051 */
  TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
  TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
  TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_ENABLE;
  LL_TIM_BDTR_Init(TIM1, &TIM_BDTRInitStruct);
 
  /* GPIO: TIM1 CH1/CH2/CH3 -> PA8, PA9, PA10 (Alternate Function) */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
 
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_2; /* TIM1 AF on F0 */
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
 
  /* Low-side MOSFET pins -> use as plain GPIO outputs (PA7, PB0, PB1) */
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
 
  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
 
  GPIO_InitStruct.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
 
  /* Reset low-side outputs to OFF (low) */
  LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_7);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_0);
  LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_1);
}
 
static void MX_GPIO_Init(void)
{
  /* If CubeMX uses this function place the minimal required code here.
     We already initialized GPIOs in MX_TIM1_Init for simplicity. */
}
 
/* Error handler (optional) */
void Error_Handler(void)
{
  __disable_irq();
  while (1) { }
}
