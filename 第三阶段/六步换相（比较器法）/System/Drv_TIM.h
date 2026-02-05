#ifndef __DRV_TIM_H
#define __DRV_TIM_H
#include "sys.h"

extern uint32_t TIM3_counter;

void TIM2_Init(void);
void Record_Commutation_Interval(void);
void TIM3_Int_Init(void);
void TIM4_Int_Init(void);
void TIM4_Enable(uint16_t count_value);
void TIM4_Disable(void);


#endif
