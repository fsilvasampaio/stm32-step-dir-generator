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

#define GEN_AXIS_CNT        4 // 1..4, max axis count
#define GEN_DMA_ARRAY_SIZE  512 // 1..1000, DMA transfer array size




/* don't touch this ----------------------------------------------------------*/

#define GEN_STEPS_OUTPUT_CONST          1
#define GEN_STEPS_OUTPUT_ACCEL_LINEAR   2
#define GEN_STEPS_OUTPUT_ACCEL_S        4
#define GEN_DIR_OUTPUT                  128
#define GEN_SYSTICK_IRQ_FREQ            1000 // Hz




/* handlers ------------------------------------------------------------------*/

void GEN_system_init(void);
void GEN_init(void);
void GEN_SYSTICK_IRQHandler(void);
void GEN_DMA_transfer_complete(uint8_t axis);




/* functions ------------------------------------------------------------------*/

void GEN_steps_output_const(uint8_t axis, uint16_t steps, uint32_t freq);




#endif /* __GENERATOR_H */
