#ifndef __DRV_TIM_H
#define __DRV_TIM_H
#include "sys.h"

#define control_period 0.0001f

extern volatile uint32_t TIM3_counter;

void TIM2_Init(void);
uint32_t Record_Commutation_Interval(void);
void TIM3_Int_Init(uint16_t arr,uint16_t psc);

#endif
