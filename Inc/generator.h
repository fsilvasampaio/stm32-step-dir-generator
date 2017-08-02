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
#define AXIS_CNT        4
#define DMA_ARRAY_SIZE  512

/* don't touch this ----------------------------------------------------------*/
#define STEPS_OUTPUT_CONST          1
#define STEPS_OUTPUT_ACCEL_LINEAR   2
#define STEPS_OUTPUT_ACCEL_CIRC     4
#define STEPS_OUTPUT_ACCEL_S        8
#define DIR_OUTPUT                  16
#define SYSTICK_IRQ_FREQ            10000 // Hz

/* handlers ------------------------------------------------------------------*/
void GEN_system_init(void);
void GEN_init(void);
void GEN_SYSTICK_IRQHandler(void);
void GEN_DMA_transfer_complete(uint8_t axis);

/* functions ------------------------------------------------------------------*/
void GEN_steps_output_const(uint8_t axis, uint16_t steps, uint32_t freq);

#endif /* __GENERATOR_H */
