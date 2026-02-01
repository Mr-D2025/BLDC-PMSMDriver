#include "sys.h"
#include "Drv_BLDC.h"
#include "BEMF.h"
#include "delay.h"

void IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
	
	// 配置上桥臂GPIO (PA8, PA9, PA10 - TIM1_CH1, CH2, CH3)
    // 这些引脚用作PWM输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);   // PA8 -> TIM1_CH1 (A相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);   // PA9 -> TIM1_CH2 (B相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);  // PA10 -> TIM1_CH3 (C相)
    
    // 配置下桥臂GPIO (PB13, PB14, PB15)
    // 这些引脚用作普通GPIO输出控制下桥臂的导通/关断
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始状态：所有下桥臂关闭
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
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
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;         // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable; // 禁用互补输出
    TIM_OCInitStructure.TIM_Pulse = 0;                        // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 高电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
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

void TIM1_IT_Init(void)
{
    // 配置TIM1更新中断的NVIC（优先级低于EXTI）
    NVIC_InitTypeDef NVIC_InitStructure;
	
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  // 低于EXTI的0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
	
	// 使能TIM1更新中断（用于PWM周期控制）
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
}

void PWM_Centre_Init(void)
{
	TIM1_TB_Init(100, 42);
	TIM1_OC_Init();
	TIM1_IT_Init();
    TIM_CtrlPWMOutputs(TIM1, ENABLE);	//使能主输出
    TIM_Cmd(TIM1, ENABLE);				//使能定时器
}

// 设置PWM占空比（0到arr-1）
void TIM1_SetPWM_Duty(uint16_t duty_a, uint16_t duty_b, uint16_t duty_c, uint16_t max_duty)
{
    // 限制占空比范围
    if(duty_a > max_duty) duty_a = max_duty;
    if(duty_b > max_duty) duty_b = max_duty;
    if(duty_c > max_duty) duty_c = max_duty;
    
    // 设置三个相的PWM占空比
    TIM_SetCompare1(TIM1, duty_a);  // A相 (PA8)
    TIM_SetCompare2(TIM1, duty_b);  // B相 (PA9)
    TIM_SetCompare3(TIM1, duty_c);  // C相 (PA10)
}

// 六步换相函数（120°导通方式）
void SixStep_Commutation(SixStep_Phase step, uint16_t pwm_duty)
{
    // 先关闭所有下桥臂
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    
    // 关闭所有上桥臂PWM（占空比设为0）
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
    
    // 根据六步换相表设置导通状态
    switch(step) {
        case STEP_AB:  // A+ B-
            TIM_SetCompare1(TIM1, pwm_duty);      // A相上桥臂PWM
            PBout(14)=1;     					  // B相下桥臂导通 (PB14)
            break;
            
        case STEP_AC:  // A+ C-
            TIM_SetCompare1(TIM1, pwm_duty);      // A相上桥臂PWM
            PBout(15)=1;     					  // C相下桥臂导通 (PB15)
            break;
            
        case STEP_BC:  // B+ C-
            TIM_SetCompare2(TIM1, pwm_duty);      // B相上桥臂PWM
            PBout(15)=1;     					  // C相下桥臂导通 (PB15)
            break;
            
        case STEP_BA:  // B+ A-
            TIM_SetCompare2(TIM1, pwm_duty);      // B相上桥臂PWM
            PBout(13)=1;    					  // A相下桥臂导通 (PB13)
            break;
            
        case STEP_CA:  // C+ A-
            TIM_SetCompare3(TIM1, pwm_duty);      // C相上桥臂PWM
            PBout(13)=1;     					  // A相下桥臂导通 (PB13)
            break;
            
        case STEP_CB:  // C+ B-
            TIM_SetCompare3(TIM1, pwm_duty);      // C相上桥臂PWM
            PBout(14)=1;    					  // B相下桥臂导通 (PB14)
            break;
    }
}

// 简单的六步换相顺序
void SixStep_Sequence_Run(uint16_t speed)
{
    static uint8_t step = 0;
    
    switch(step) {
        case 0: SixStep_Commutation(STEP_AB, speed); break;
        case 1: SixStep_Commutation(STEP_AC, speed); break;
        case 2: SixStep_Commutation(STEP_BC, speed); break;
        case 3: SixStep_Commutation(STEP_BA, speed); break;
        case 4: SixStep_Commutation(STEP_CA, speed); break;
        case 5: SixStep_Commutation(STEP_CB, speed); break;
    }
    
    step = (step + 1) % 6;
}

// 停止所有输出
void SixStep_Stop(void)
{
    // 关闭所有上桥臂PWM
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
    
    // 关闭所有下桥臂
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
}


// TIM1更新中断服务函数
void TIM1_UP_TIM10_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)	// 检查是否是TIM1更新中断
    {
		
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);		// 清除TIM1更新中断标志
    }
}
