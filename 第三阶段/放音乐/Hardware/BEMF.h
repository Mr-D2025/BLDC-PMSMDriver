#ifndef __BEMF_H
#define __BEMF_H
#include "sys.h"
#include "Drv_BLDC.h"

extern SixStep_Phase CurrentStep;  // 初始状态

//反电动势过零检测中断初始化
void BEMF_ZeroCross_Init(void);



#endif
