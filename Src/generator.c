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
  TIM_HandleTypeDef*  htim; // link to timer's init structure
  DMA_HandleTypeDef*  hdma; // link to timer's dma channel init structure
  uint32_t            tim_freq; // axis timer base frequency, Hz

  int32_t             pos; // actual position, steps
  uint32_t            freq; // actual frequency, Hz
  int32_t             accel; // actual acceleration, Hz/s
  uint32_t            min_freq; // min frequency, Hz
  uint32_t            min_accel; // min acceleration, Hz/s
  uint32_t            max_freq; // max frequency, Hz
  uint32_t            max_accel; // max acceleration, Hz/s

  uint8_t             output_type; // output type flag
  uint16_t            steps_last; // last output steps count
  int32_t             pos_last; // last position
};
// axis data array
static struct AXIS_t axes[] = {
  {&htim1,  &hdma_tim1_ch1,      72000000, 0, 0, 0, 1, 1, 1000000, 1000000, 0, 0, 0},
#if GEN_AXIS_CNT >= 2
  {&htim2,  &hdma_tim2_ch1,      72000000, 0, 0, 0, 1, 1, 1000000, 1000000, 0, 0, 0},
#if GEN_AXIS_CNT >= 3
  {&htim3,  &hdma_tim3_ch1_trig, 72000000, 0, 0, 0, 1, 1, 1000000, 1000000, 0, 0, 0},
#if GEN_AXIS_CNT >= 4
  {&htim4,  &hdma_tim4_ch1,      72000000, 0, 0, 0, 1, 1, 1000000, 1000000, 0, 0, 0}
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
void GEN_steps_output_const(uint8_t axis, uint16_t steps, uint32_t freq)
{
  static uint32_t prescaler = 0;
  static uint32_t period = 0;

  // do nothing if output is already ON
  if ( axes[axis].output_type ) return;

  // set the params
  axes[axis].output_type = GEN_STEPS_OUTPUT_CONST; // set output type
  axes[axis].steps_last = steps; // save last generation steps value
  axes[axis].pos_last = axes[axis].pos; // save axis position before generation
  axes[axis].freq = freq; // set axis output frequency

  // calculate the period and prescaler
  prescaler = axes[axis].tim_freq / freq / 65536;
  period = axes[axis].tim_freq / freq / (prescaler + 1);

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

/*
 * low level steps generation function
 *
 * uses to generate a limit number of steps with selected frequency & acceleration
 *
 * axis         selected axis
 * steps        number of output steps
 * freq         output frequency in Hz, see accel's description below
 * accel        acceleration in Hz/s, negative values means decceleration
 *                if accel > 0, the freq it's target frequency.
 *                if accel < 0, the freq it's start frequency.
 *                if accel = 0, the freq it's const frequency.
 * accel_type   acceleration type:
 *                GEN_ACCEL_LINEAR - frequency increases with const acceleration
 *                GEN_ACCEL_S - frequency increases with s-curve acceleration
 */
void GEN_steps_output
(
  uint8_t axis, uint16_t steps, uint32_t freq, int32_t accel, uint8_t accel_type
)
{
  static uint32_t prescaler = 0;
  static uint32_t period = 0;

  // do nothing if output is already ON
  if ( axes[axis].output_type ) return;

  // set the params
  axes[axis].steps_last = steps; // save last generation steps value
  axes[axis].pos_last = axes[axis].pos; // save axis position before generation

  // select generation profile
  if ( accel ) // acceleration or decceleration
  {
    // set output type
    axes[axis].output_type = (accel_type == GEN_ACCEL_LINEAR) ?
      GEN_STEPS_OUTPUT_ACCEL_LINEAR :
      GEN_STEPS_OUTPUT_ACCEL_S;

    // acceleration
    if ( accel > 0 )
    {
      // set axis output frequency
      axes[axis].freq += (accel/GEN_SYSTICK_IRQ_FREQ) * ((1000*2*1/steps)^2) / 1000^2;
    }
    // decceleration
    else
    {
      axes[axis].freq = freq; // set axis output frequency
    }
  }
  else // output with constant frequency
  {
    axes[axis].output_type = GEN_STEPS_OUTPUT_CONST; // set output type
    axes[axis].freq = freq; // set axis output frequency
  }


  // TODO - set frequency recalculation data for the SYSTICK handler

  // TODO - set timer's data


#if 0
  // calculate the period and prescaler
  prescaler = axes[axis].tim_freq / freq / 65536;
  period = axes[axis].tim_freq / freq / (prescaler + 1);

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
#endif
}




/* Handlers ------------------------------------------------------------------*/

/*
 * systick update event handler
 *
 * uses in the SysTick_Handler()
 */
void GEN_SYSTICK_IRQHandler(void)
{
  static uint8_t axis = 0;

  // do it for all axes
  for ( axis = GEN_AXIS_CNT; axis--; )
  {
    // update the position if needed
    if ( axes[axis].hdma->Instance->CNDTR )
    {
      // TODO - calculate axis pos using DIR state
      axes[axis].pos  = axes[axis].pos_last
                      + (axes[axis].steps_last - axes[axis].hdma->Instance->CNDTR);
    }

    // TODO - frequency recalculation for the accelerated output
  }
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
  DMA_array[axis][axes[axis].steps_last - 1] |= (TIM_CR1_CEN);

  // set the params
  axes[axis].output_type = 0; // set output type
  axes[axis].freq = 0; // set axis output frequency
  // TODO - calculate axis pos using DIR state
  axes[axis].pos = axes[axis].pos_last + axes[axis].steps_last; // set axis position
}
