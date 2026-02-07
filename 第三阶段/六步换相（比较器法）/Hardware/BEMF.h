#ifndef __BEMF_H
#define __BEMF_H
#include "sys.h"
#include "Drv_BLDC.h"

// 根据换相状态确定需要检测的边沿
typedef struct {
    uint8_t phase;      // 需要检测的相位 (0x01:A, 0x02:B, 0x04:C)
    uint8_t edge;       // 检测边沿 (0:下降沿, 1:上升沿, 2:双边沿)
    uint8_t step;  		// 当前换相状态
} BEMF_Detect_Config;

extern const BEMF_Detect_Config BEMF_Config[];

//反电动势过零检测中断初始化
void BEMF_ZeroCross_Init(void);
void BEMF_IRQ_Disable(void);
void BEMF_IRQ_Config(uint8_t step);


#endif
