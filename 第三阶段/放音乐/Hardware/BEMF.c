#include "sys.h"
#include "BEMF.h"
#include "Drv_BLDC.h"

SixStep_Phase CurrentStep = STEP_AB;  // 初始状态
uint16_t PWM_Duty = 500;              // 默认PWM占空比

//反电动势过零检测中断初始化
void BEMF_ZeroCross_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能GPIOC时钟和SYSCFG时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    
    // 配置PC6, PC7, PC8为输入模式（反电动势过零比较器输出）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // 比较器输出已经有上拉/下拉
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 配置外部中断线6,7,8
    // PC6 -> EXTI_Line6
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource6);
    // PC7 -> EXTI_Line7
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource7);
    // PC8 -> EXTI_Line8
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOC, EXTI_PinSource8);
    
    // 配置EXTI线6,7,8
    EXTI_InitStructure.EXTI_Line = EXTI_Line6 | EXTI_Line7 | EXTI_Line8;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 上升沿和下降沿都触发
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置NVIC中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;  // EXTI9_5包含线6,7,8
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 最高优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

//反电动势过零中断处理函数
void EXTI9_5_IRQHandler(void)
{
    static uint8_t last_bemf_state = 0;
    uint8_t current_bemf_state = 0;
    
    // 检查是否有中断发生
    if(EXTI_GetITStatus(EXTI_Line6) != RESET || 
       EXTI_GetITStatus(EXTI_Line7) != RESET || 
       EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
        // 读取当前反电动势比较器状态
        current_bemf_state = 0;
        current_bemf_state |= GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_6) ? 0x01 : 0x00; // A相
        current_bemf_state |= GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7) ? 0x02 : 0x00; // B相
        current_bemf_state |= GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8) ? 0x04 : 0x00; // C相
        
        // 判断哪个通道发生了变化
        uint8_t changed = last_bemf_state ^ current_bemf_state;
        
        // 根据当前换相状态和变化的BEMF信号决定下一个换相状态
        switch(CurrentStep) {
            case STEP_AB:  // A+ B- 导通，等待C相过零
                if(changed & 0x04) { // C相变化
                    SixStep_Commutation(STEP_AC, PWM_Duty);
                    CurrentStep = STEP_AC;
                }
                break;
                
            case STEP_AC:  // A+ C- 导通，等待B相过零
                if(changed & 0x02) { // B相变化
                    SixStep_Commutation(STEP_BC, PWM_Duty);
                    CurrentStep = STEP_BC;
                }
                break;
                
            case STEP_BC:  // B+ C- 导通，等待A相过零
                if(changed & 0x01) { // A相变化
                    SixStep_Commutation(STEP_BA, PWM_Duty);
                    CurrentStep = STEP_BA;
                }
                break;
                
            case STEP_BA:  // B+ A- 导通，等待C相过零
                if(changed & 0x04) { // C相变化
                    SixStep_Commutation(STEP_CA, PWM_Duty);
                    CurrentStep = STEP_CA;
                }
                break;
                
            case STEP_CA:  // C+ A- 导通，等待B相过零
                if(changed & 0x02) { // B相变化
                    SixStep_Commutation(STEP_CB, PWM_Duty);
                    CurrentStep = STEP_CB;
                }
                break;
                
            case STEP_CB:  // C+ B- 导通，等待A相过零
                if(changed & 0x01) { // A相变化
                    SixStep_Commutation(STEP_AB, PWM_Duty);
                    CurrentStep = STEP_AB;
                }
                break;
        }
        
        // 保存当前BEMF状态
        last_bemf_state = current_bemf_state;
        
        // 清除中断标志位
        EXTI_ClearITPendingBit(EXTI_Line6 | EXTI_Line7 | EXTI_Line8);
    }
}
