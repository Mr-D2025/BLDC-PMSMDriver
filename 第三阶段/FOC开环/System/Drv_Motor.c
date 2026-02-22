#include "sys.h"
#include "Drv_Motor.h"
#include "delay.h"

void TIM1_IO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOA和GPIOB时钟（同时使能TIM1时钟，通常在TIM初始化函数中使能，这里只做注释）
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            // 复用功能模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      // 高速
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;            // 上拉，防止引脚悬空，可根据实际电路调整
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 连接PAx到TIM1的AF 
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);   // PA8 -> TIM1_CH1 (A相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);   // PA9 -> TIM1_CH2 (B相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);  // PA10 -> TIM1_CH3 (C相)

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;            // 复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;      // 高速
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;          // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // 互补输出通常由定时器控制，可不带上拉，根据实际电路选择
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 连接PBx到TIM1的互补通道AF
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_TIM1);  // PB13 -> TIM1_CH1N (A相下桥)
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM1);  // PB14 -> TIM1_CH2N (B相下桥)
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM1);  // PB15 -> TIM1_CH3N (C相下桥)
    
}

void TIM1_TB_Init(uint32_t arr, uint32_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	
	// 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 配置TIM1时基单元
    TIM_TimeBaseStructure.TIM_Period = arr - 1;               // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc - 1;            // 预分频器
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1; // 向上向下计数模式
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
}

void TIM1_OC_Init(void)
{
    
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    
    // 配置PWM输出通道（只配置上桥臂的3个通道）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;         // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable; // 使能互补输出
    TIM_OCInitStructure.TIM_Pulse = 0;                        // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 高电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    
    // 初始化三个通道
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);  // CH1 - A相
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);  // CH2 - B相
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);  // CH3 - C相
    
    //使能预装载寄存器
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    
}

void TIM1_BDT_Init(void)
{
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
    
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;       // 运行模式下关闭状态使能
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;       // 空闲模式下关闭状态使能
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;            // 锁定级别1
    TIM_BDTRInitStructure.TIM_DeadTime = 0x20;                    // 死区时间值
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable;           // 使能刹车输入
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low; // 刹车低电平有效
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Disable; // 自动输出使能
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
}

void TIM1_IT_Init(void)
{
    // 配置TIM1更新中断的NVIC（优先级低于EXTI）
    NVIC_InitTypeDef NVIC_InitStructure;
	
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // 低于EXTI的0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
     // 清除更新标志
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);

	// 使能TIM1更新中断（用于PWM周期控制）
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
}

void PWM_Centre_Init(void)
{
    TIM1_IO_Init();
	TIM1_TB_Init(1000, 4);
	TIM1_OC_Init();
    TIM1_BDT_Init();
	//TIM1_IT_Init();
    TIM_CtrlPWMOutputs(TIM1, ENABLE);	//使能主输出
    TIM_Cmd(TIM1, ENABLE);				//使能定时器
}

// TIM1更新中断服务函数
void TIM1_UP_TIM10_IRQHandler(void)
{

    
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)	// 检查是否是TIM1更新中断
    {

        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);		// 清除TIM1更新中断标志
    }
}
