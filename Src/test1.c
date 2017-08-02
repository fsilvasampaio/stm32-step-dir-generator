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
const uint32_t TEST1_out_freq[TEST1_PRESET_COUNT] = { // Hz
  20,50,100,500,1000,5000,10000,50000,1000000,1500000,2000000,3200000
};
const uint32_t TEST1_out_steps[TEST1_PRESET_COUNT] = { // pulses/steps
  10,20, 30, 40,  50,  60,   70,   80,     90,    100,    247,    333
};

volatile uint32_t TEST1_preset[AXIS_CNT] = {0};




/* Handlers ------------------------------------------------------------------*/
void TEST1_SYSTICK_IRQHandler(void)
{
  static uint8_t axis = 0;

  if ( !(HAL_GetTick() % SYSTICK_IRQ_FREQ) ) // every second
  {
    for ( axis = AXIS_CNT; axis--; )
    {
      // start output
      GEN_steps_output_const(
        axis,
        TEST1_out_steps[TEST1_preset[axis]],
        TEST1_out_freq[TEST1_preset[axis]]
      );

      // goto next preset
      if ( (++TEST1_preset[axis]) >= TEST1_PRESET_COUNT )
      {
        TEST1_preset[axis] = 0;
      }
    }
  }
}
