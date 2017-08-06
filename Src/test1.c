/**
  ******************************************************************************
  * File Name          : test1.c
  * Description        : steps generation test 1
  ******************************************************************************
  *
  * "AS IS"
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/

#include "stm32f1xx_hal.h"
#include "generator.h"
#include "test1.h"




/* Global vars ---------------------------------------------------------------*/

// output frequencies list, Hz
static const uint32_t out_freq[TEST1_PRESET_COUNT] =
{
  20,50,100,500,1000,5000,10000,50000,1000000,1500000,2000000,3200000
};
// output steps count
static const uint32_t out_steps[TEST1_PRESET_COUNT] =
{
  10,20, 30, 40,  50,  60,   70,   80,     90,    100,    247,    333
};
// preset ID for every axis
static volatile uint32_t preset[GEN_AXIS_CNT] = {0};




/* Handlers ------------------------------------------------------------------*/

/*
 * systick IRQ handler
 *
 * uses in the SysTick_Handler() to start test generation every second
 */
void TEST1_SYSTICK_IRQHandler(void)
{
  static uint8_t axis = 0;

  // do it every second
  if ( !(HAL_GetTick() % GEN_SYSTICK_IRQ_FREQ) )
  {
    // do it for the all axes
    for ( axis = GEN_AXIS_CNT; axis--; )
    {
      // start output
      GEN_steps_output_const(axis, out_steps[preset[axis]], out_freq[preset[axis]]);

      // preset ID ++
      if ( (++preset[axis]) >= TEST1_PRESET_COUNT ) preset[axis] = 0;
    }
  }
}
