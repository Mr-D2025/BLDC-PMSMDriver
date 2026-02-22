#ifndef __DRV_BLDC_H
#define __DRV_BLDC_H
#include "sys.h"

typedef enum {	  // 六步换相状态定义（120°导通方式）
    STEP_AB = 0,  // A相上桥臂 + B相下桥臂
    STEP_AC,      // A相上桥臂 + C相下桥臂  
    STEP_BC,      // B相上桥臂 + C相下桥臂
    STEP_BA,      // B相上桥臂 + A相下桥臂
    STEP_CA,      // C相上桥臂 + A相下桥臂
    STEP_CB       // C相上桥臂 + B相下桥臂
} SixStep_Phase;

void TIM1_IO_Init(void);
void TIM1_TB_Init(uint32_t arr, uint32_t psc);
void TIM1_OC_Init(void);
void TIM1_BDT_Init(void);
void TIM1_IT_Init(void);

void PWM_Centre_Init(void);


#endif
