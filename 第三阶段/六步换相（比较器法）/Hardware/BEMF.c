#include "sys.h"
#include "BEMF.h"
#include "Drv_TIM.h"
#include "BLDC.h"
#include "delay.h"

extern uint32_t tt;

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
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; // 比较器输出已经有上拉/下拉
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
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;	// 初始不触发
    EXTI_Init(&EXTI_InitStructure);
    
    // 配置NVIC中断优先级
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;  // EXTI9_5包含线6,7,8
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // 最高优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

// 换相顺序表（60°换相）
// 状态顺序：AB -> AC -> BC -> BA -> CA -> CB -> AB
const BEMF_Detect_Config BEMF_Config[] = {
    {0x04, 0, STEP_AB},  // STEP_AB: A+ B- 导通，监测C相下降沿 -> 换相到STEP_AC
    {0x02, 1, STEP_AC},  // STEP_AC: A+ C- 导通，监测B相上升沿 -> 换相到STEP_BC
    {0x01, 0, STEP_BC},  // STEP_BC: B+ C- 导通，监测A相下降沿 -> 换相到STEP_BA
    {0x04, 1, STEP_BA},  // STEP_BA: B+ A- 导通，监测C相上升沿 -> 换相到STEP_CA
    {0x02, 0, STEP_CA},  // STEP_CA: C+ A- 导通，监测B相下降沿 -> 换相到STEP_CB
    {0x01, 1, STEP_CB},  // STEP_CB: C+ B- 导通，监测A相上升沿 -> 换相到STEP_AB
};

void BEMF_IRQ_Disable(void)
{
	EXTI_InitTypeDef EXTI_InitStruct;
	
	EXTI_InitStruct.EXTI_Line = EXTI_Line6 | EXTI_Line7 | EXTI_Line8;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_LineCmd = DISABLE;
	EXTI_Init(&EXTI_InitStruct);
	
}

// 反电动势比较器中断使能配置函数
void BEMF_IRQ_Config(uint8_t step)
{
    if(step >= 6) return; // 安全检查
    
    BEMF_Detect_Config config = BEMF_Config[step];
    
    // 清除所有中断线
    EXTI_ClearITPendingBit(EXTI_Line6 | EXTI_Line7 | EXTI_Line8);
    
    // 禁用所有中断线
    EXTI_InitTypeDef EXTI_InitStruct;
    EXTI_InitStruct.EXTI_Line = EXTI_Line6 | EXTI_Line7 | EXTI_Line8;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStruct);
    
    // 设置中断触发边沿
    EXTITrigger_TypeDef trigger;
    switch(config.edge) {
        case 0:  // 下降沿
            trigger = EXTI_Trigger_Falling;
            break;
        case 1:  // 上升沿
            trigger = EXTI_Trigger_Rising;
            break;
        case 2:  // 双边沿
            trigger = EXTI_Trigger_Rising_Falling;
            break;
        default:
            break;
    }
    
    // 根据配置使能指定的中断线
    switch(config.phase) {
        case 0x01:  // A相
            EXTI_InitStruct.EXTI_Line = EXTI_Line6;
            EXTI_InitStruct.EXTI_Trigger = trigger;
            EXTI_InitStruct.EXTI_LineCmd = ENABLE;
            EXTI_Init(&EXTI_InitStruct);
            break;

        case 0x02:  // B相
            EXTI_InitStruct.EXTI_Line = EXTI_Line7;
            EXTI_InitStruct.EXTI_Trigger = trigger;
            EXTI_InitStruct.EXTI_LineCmd = ENABLE;
            EXTI_Init(&EXTI_InitStruct);
            break;
            
        case 0x04:  // C相
            EXTI_InitStruct.EXTI_Line = EXTI_Line8;
            EXTI_InitStruct.EXTI_Trigger = trigger;
            EXTI_InitStruct.EXTI_LineCmd = ENABLE;
            EXTI_Init(&EXTI_InitStruct);
            break;
    }
}

// 反电动势过零中断处理函数
void EXTI9_5_IRQHandler(void)
{
    // 检查是否有中断发生
    if(EXTI_GetITStatus(EXTI_Line6) != RESET || 
       EXTI_GetITStatus(EXTI_Line7) != RESET || 
       EXTI_GetITStatus(EXTI_Line8) != RESET)
    {
		// 获取当前换相状态的检测配置
        BEMF_Detect_Config config = BEMF_Config[motor.step];
        uint8_t measured_pin = 18;
        uint8_t expected_level = config.edge;
		uint8_t isin=0;
        
        // 根据相位配置测量对应引脚的电平
        if(config.phase == 0x01) 
        {
            measured_pin = 6;
        }
        else if(config.phase == 0x02) 
        {
            measured_pin = 7;
        }
        else if(config.phase == 0x04) 
        {
            measured_pin = 8;
        }
		
		if(motor.duty_register<35) delay_us(motor.filter_commutation_time_sum/600);
		else if(motor.duty_register<55) delay_us(motor.filter_commutation_time_sum/300);
		else if(motor.duty_register<75) delay_us(motor.filter_commutation_time_sum/60);
		else delay_us(motor.filter_commutation_time_sum/50);
		
		if(PCin(measured_pin)==expected_level) 
		{
			for(int i=0;i<10;i++)
			{
				__NOP;
			}
			if(PCin(measured_pin)==expected_level) 
			{
				isin=1;
			}
		}
		
		
		if(isin)
		{
			// BLDC_MOTOR_ANGLE度换相
			uint16_t delay_counts = motor.filter_commutation_time_sum * BLDC_CLOSED_ANGLE / 360;
			
			// 获取换相时间（从上次过零点到现在的时间）
			// 假设使用TIM2测量时间间隔
			temp_commutation_time = TIM_GetCounter(TIM2);
			
			// 重置状态标志
			xc_flag = 0;
			
			// 去除反电动势毛刺
//			if(temp_commutation_time < (motor.commutation_time_sum / 36))
//			{
//				tt++;
//				EXTI_ClearITPendingBit(EXTI_Line6 | EXTI_Line7 | EXTI_Line8);
//				return;
//			}
			
			// 停止并重置TIM2
			TIM_Cmd(TIM2, DISABLE);
			TIM_SetCounter(TIM2, 0);
			TIM_Cmd(TIM2, ENABLE);
			
			// 使用TIM4进行延时换相
			TIM4_Enable(delay_counts);
			
			// 失能比较器中断（直到下一次需要检测时再使能）
			BEMF_IRQ_Disable();
		}
        // 清除中断标志位
        EXTI_ClearITPendingBit(EXTI_Line6 | EXTI_Line7 | EXTI_Line8);
    }
}