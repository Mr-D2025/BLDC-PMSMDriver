#ifndef __DEMO_H
#define __DEMO_H
#include "sys.h"
#include "BLDC.h"


void TIM5_Init(void);
void Motor_State_Control_Timer(Motor_State_t state);
void Update_Motor_Duty(void);


#endif
