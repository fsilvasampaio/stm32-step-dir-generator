/**
  ******************************************************************************
  * File Name          : generator.c
  * Description        : steps generation functionality
  ******************************************************************************
  *
  * "AS IS"
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "stm32f1xx_hal.h"
#include "generator.h"




/* Global vars ---------------------------------------------------------------*/

// array uses by axis DMA channels
static uint8_t DMA_array[GEN_AXIS_CNT][GEN_DMA_ARRAY_SIZE] = {{0}};

static volatile uint8_t output_type[GEN_AXIS_CNT] = {0}; // output enabled flag
static volatile uint16_t steps_last[GEN_AXIS_CNT] = {0}; // current output steps count
static volatile uint32_t HCLK_freq = 0; // system core frequency

// links to the timers and DMA channels init structures
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_tim1_ch1;
extern DMA_HandleTypeDef hdma_tim2_ch1;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;
extern DMA_HandleTypeDef hdma_tim4_ch1;

// axis data structure
struct AXIS_t
{
  TIM_HandleTypeDef*  htim;       // link to timer's init structure
  DMA_HandleTypeDef*  hdma;       // link to timer's dma channel init structure

  int32_t             pos;        // actual position, steps
  uint32_t            freq;       // actual frequency, Hz
  int32_t             accel;      // actual acceleration, Hz/s
  uint32_t            min_freq;   // min frequency, Hz
  uint32_t            min_accel;  // min acceleration, Hz/s
  uint32_t            max_freq;   // max frequency, Hz
  uint32_t            max_accel;  // max acceleration, Hz/s
};
// axis data array
static struct AXIS_t axes[] = {
  {&htim1,  &hdma_tim1_ch1,      0, 0, 0, 10, 10, 1000000, 1000000},
#if GEN_AXIS_CNT >= 2
  {&htim2,  &hdma_tim2_ch1,      0, 0, 0, 10, 10, 1000000, 1000000},
#if GEN_AXIS_CNT >= 3
  {&htim3,  &hdma_tim3_ch1_trig, 0, 0, 0, 10, 10, 1000000, 1000000},
#if GEN_AXIS_CNT >= 4
  {&htim4,  &hdma_tim4_ch1,      0, 0, 0, 10, 10, 1000000, 1000000}
#endif
#endif
#endif
};




/* functions ------------------------------------------------------------------*/

/*
 * generation system core init
 *
 * uses in the main() after SystemClock_Config()
 */
void GEN_system_init(void)
{
  // set systick update frequency
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/GEN_SYSTICK_IRQ_FREQ);
}

/*
 * generation init
 *
 * uses in the main() before infinite loop start
 */
void GEN_init(void)
{
  // get and save real core frequency
  HCLK_freq = HAL_RCC_GetHCLKFreq();

  for ( uint8_t axis = GEN_AXIS_CNT; axis--; )
  {
    // fill the array with timer's CR1 values with timer enable bit
    // this array uses to stop timer immidiately after DMA transfer complete
    for (
      uint32_t  i = GEN_DMA_ARRAY_SIZE,
                v = axes[axis].htim->Instance->CR1 | (TIM_CR1_CEN);
      i--;
    ) {
      DMA_array[axis][i] = v;
    }

    /* reset the Preload enable bit for OC channel */
    axes[axis].htim->Instance->CCMR1 &= ~(TIM_CCMR1_OC1PE);
  }
}

/*
 * low level generation function
 *
 * uses to generate a limit number of steps at the constant frequency
 */
void GEN_steps_output_const(uint8_t axis, uint16_t steps, uint32_t freq)
{
  static uint32_t prescaler = 0;
  static uint32_t period = 0;

  // do nothing if output is already ON
  if ( output_type[axis] ) return;

  // set output params
  output_type[axis] = GEN_STEPS_OUTPUT_CONST;
  steps_last[axis] = steps;

  // calculate the period and prescaler
  prescaler = HCLK_freq / freq / 65536;
  period = HCLK_freq / freq / (prescaler + 1);

  // set timer's data
  __HAL_TIM_SET_AUTORELOAD(axes[axis].htim, period - 1);
  __HAL_TIM_SET_COMPARE(axes[axis].htim, TIM_CHANNEL_1, period/2 - 1);
  __HAL_TIM_SET_PRESCALER(axes[axis].htim, prescaler);
  // generate the Update event to apply the new prescaler
  axes[axis].htim->Instance->EGR |= (TIM_EGR_UG);

  // reset the CR1 timer enable bit in the DMA array cell
  // this uses to stop timer immidiately after DMA transfer complete
  DMA_array[axis][steps - 1] &= ~(TIM_CR1_CEN);

  /* Disable the peripheral */
  __HAL_DMA_DISABLE(axes[axis].hdma);
  /* Configure DMA Channel data length */
  axes[axis].hdma->Instance->CNDTR = steps;
  /* Configure DMA Channel destination address */
  axes[axis].hdma->Instance->CPAR = (uint32_t)&(axes[axis].htim->Instance->CR1);
  /* Configure DMA Channel source address */
  axes[axis].hdma->Instance->CMAR = (uint32_t)&DMA_array[axis][0];
  /* Enable the Peripheral */
  __HAL_DMA_ENABLE(axes[axis].hdma);
  /* Enable the transfer complete interrupt */
  __HAL_DMA_ENABLE_IT(axes[axis].hdma, DMA_IT_TC);
  /* Enable the TIM Capture/Compare 1 DMA request */
  __HAL_TIM_ENABLE_DMA(axes[axis].htim, TIM_DMA_CC1);
  /* Enable the Capture compare channel */
  TIM_CCxChannelCmd(axes[axis].htim->Instance, TIM_CHANNEL_1, TIM_CCx_ENABLE);
  /* Enable the main output */
  __HAL_TIM_MOE_ENABLE(axes[axis].htim);
  /* Enable the Peripheral */
  __HAL_TIM_ENABLE(axes[axis].htim);
}




/* Handlers ------------------------------------------------------------------*/

/*
 * systick update handler
 *
 * uses in the SysTick_Handler()
 */
void GEN_SYSTICK_IRQHandler(void)
{
  // TODO - frequency recalculation for the accelerated output
}

/*
 * DMA transfer complete handler
 *
 * uses in the DMA channel IRQ handlers
 */
void GEN_DMA_transfer_complete(uint8_t axis)
{
  /* Disable the TIM Capture/Compare 1 DMA request */
  __HAL_TIM_DISABLE_DMA(axes[axis].htim, TIM_DMA_CC1);
  /* Disable the Capture compare channel */
  TIM_CCxChannelCmd(axes[axis].htim->Instance, TIM_CHANNEL_1, TIM_CCx_DISABLE);
  /* Disable the Main Output */
  __HAL_TIM_MOE_DISABLE(axes[axis].htim);

  // set the CR1 timer enable bit in the DMA array cell
  DMA_array[axis][steps_last[axis] - 1] |= (TIM_CR1_CEN);

  // reset output params
  output_type[axis] = 0;
}
