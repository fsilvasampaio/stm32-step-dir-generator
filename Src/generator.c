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

#define TEST_1_ENABLED 1

// array uses by axis DMA channels
static uint8_t DMA_array[GEN_AXIS_CNT][GEN_DMA_ARRAY_SIZE] = {{0}};

// links to the timers and DMA channels init structures
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern DMA_HandleTypeDef hdma_tim1_ch1;
extern DMA_HandleTypeDef hdma_tim2_ch1;
extern DMA_HandleTypeDef hdma_tim3_ch1_trig;
extern DMA_HandleTypeDef hdma_tim4_ch1;

// axis data array
static struct AXIS_t axes[GEN_AXIS_CNT] =
{
  {&htim1,  &hdma_tim1_ch1,      72000000,0,0,0,0},
  {&htim2,  &hdma_tim2_ch1,      72000000,0,0,0,0},
  {&htim3,  &hdma_tim3_ch1_trig, 72000000,0,0,0,0},
  {&htim4,  &hdma_tim4_ch1,      72000000,0,0,0,0}
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
 * low level steps generation function
 *
 * uses to generate a limit number of steps at the constant frequency
 */
void GEN_steps_output(uint8_t axis, uint16_t steps, uint32_t freq)
{
  // save last generation steps value
  axes[axis].steps = steps;

  // change prescaler/period only when new frequency is different
  if ( freq != axes[axis].freq )
  {
    // save last generation frequency value
    axes[axis].freq = freq;

    // calculate the period and prescaler
    axes[axis].presc = axes[axis].tim_freq / freq / 65536;
    axes[axis].period = axes[axis].tim_freq / freq / (axes[axis].presc + 1);

    // set timer's data
    __HAL_TIM_SET_AUTORELOAD(axes[axis].htim, axes[axis].period - 1);
    __HAL_TIM_SET_COMPARE(axes[axis].htim, TIM_CHANNEL_1, axes[axis].period/2 - 1);
    __HAL_TIM_SET_PRESCALER(axes[axis].htim, axes[axis].presc);
    // generate the Update event to apply the new prescaler
    axes[axis].htim->Instance->EGR |= (TIM_EGR_UG);
  }

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
 * systick update event handler
 *
 * uses in the SysTick_Handler()
 */
void GEN_SYSTICK_IRQHandler(void)
{
#if TEST_1_ENABLED
#define CNT 8
  static uint8_t axis = 0;
  static uint32_t freq[CNT] = {10,100,1000,10000,100000,200000,200000,500000};
  static int8_t freq_n = 0;

  // do it every second
  if ( freq_n < CNT && !(HAL_GetTick() % GEN_SYSTICK_IRQ_FREQ) )
  {
    // do it for all axes
    for ( axis = GEN_AXIS_CNT; axis--; )
    {
      GEN_steps_output(axis, 10, freq[freq_n]);
    }

    ++freq_n;
  }
#undef CNT
#endif
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
  DMA_array[axis][axes[axis].steps - 1] |= (TIM_CR1_CEN);
}
