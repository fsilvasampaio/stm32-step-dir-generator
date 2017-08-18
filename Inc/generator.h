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

#define GEN_STEPS_OUTPUT_CONST          1 // steps output with const frequency
#define GEN_STEPS_OUTPUT_ACCEL_LINEAR   2 // ... with linear accelaration
#define GEN_STEPS_OUTPUT_ACCEL_S        4 // ... with s-curve accelaration
#define GEN_STEPS_OUTPUT_DECCEL_LINEAR  8 // ... with linear decelaration
#define GEN_STEPS_OUTPUT_DECCEL_S       16 // ... with s-curve deccelaration
#define GEN_DIR_OUTPUT                  128 // direction output
#define GEN_SYSTICK_IRQ_FREQ            10000 // Hz, systick update event frequency
#define GEN_ACCEL_LINEAR                1 // linear acceleration type
#define GEN_ACCEL_S                     2 // s-curve acceleration type




/* handlers ------------------------------------------------------------------*/

void GEN_SYSTICK_IRQHandler(void);
void GEN_DMA_transfer_complete(uint8_t axis);




/* functions ------------------------------------------------------------------*/

void GEN_system_init(void);
void GEN_init(void);
void GEN_steps_output_const(uint8_t axis, uint16_t steps, uint32_t freq);
void GEN_steps_output(uint8_t axis, uint16_t steps, uint32_t freq,
                      int32_t accel, uint8_t accel_type);




#endif /* __GENERATOR_H */
