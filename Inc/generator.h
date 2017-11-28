/**
  ******************************************************************************
  * File Name          : generator.h
  * Description        : steps generation settings
  ******************************************************************************
  *
  * "AS IS"
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GENERATOR_H
#define __GENERATOR_H




/* settings ------------------------------------------------------------------*/

#define GEN_AXIS_CNT            4 // 1..4, max axis count
#define GEN_DMA_ARRAY_SIZE      512 // 1..1000, DMA transfer array size
#define GEN_SYSTICK_IRQ_FREQ    1000 // Hz, systick update event frequency




/* var types -----------------------------------------------------------------*/

// axis data structure
struct AXIS_t
{
  TIM_HandleTypeDef*  htim; // link to timer's init structure
  DMA_HandleTypeDef*  hdma; // link to timer's dma channel init structure
  uint32_t            tim_freq; // axis timer base frequency, Hz
  uint32_t            presc;
  uint32_t            period;
  uint16_t            steps;
  uint16_t            freq;
};




/* handlers ------------------------------------------------------------------*/

void GEN_SYSTICK_IRQHandler(void);
void GEN_DMA_transfer_complete(uint8_t axis);




/* functions -----------------------------------------------------------------*/

void GEN_system_init(void);
void GEN_init(void);
void GEN_steps_output(uint8_t axis, uint16_t steps, uint32_t freq);




#endif /* __GENERATOR_H */
