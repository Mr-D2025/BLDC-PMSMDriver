#include "sys.h"
#include "BLDC.h"
#include "OLED.h"

// 全局变量声明

volatile uint8_t duty_direction = 0; // 0:递增，1:递减

/**
  * @brief  初始化定时器5用于2秒定时
  * @param  无
  * @retval 无
  */
void TIM5_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. 使能TIM5时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    
    // 2. 配置定时器基础参数
    // 假设系统时钟为168MHz，APB1预分频为4，所以TIM5时钟为84MHz
    // 定时器时钟 = 84MHz
    // 预分频 = 8400-1，得到10KHz的计数频率
    // 自动重装载值 = 20000-1，得到2秒的定时周期 (10000Hz / 20000 = 0.5Hz)
    TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 50000 - 1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);
    
	// 清除中断标志位
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    // 3. 使能TIM5更新中断
    TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);
    
    // 4. 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3; // 中等优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 5. 启动定时器5
    TIM_Cmd(TIM5, ENABLE);
}



/**
  * @brief  更新电机duty值，实现锯齿波效果
  * @param  无
  * @retval 无
  */
void Update_Motor_Duty(void)
{
    if (duty_direction == 0)
    {
        // 递增模式
        if (motor.duty < 80)
        {
            motor.duty += 5;
        }
        else 
        {
            // 达到上限，切换为递减模式
            duty_direction = 1;
            motor.duty -= 5;
            
        }
    }
    else
    {
        // 递减模式
        if (motor.duty > 10)
        {
            motor.duty -= 5;
        }
        else
        {
            // 达到下限，切换为递增模式
            duty_direction = 0;
            motor.run_flag = MOTOR_STOPPED;
        }
    }
    
    // 限制duty在0-100范围内
    if (motor.duty > 100)
    {
        motor.duty = 100;
    }
}

/**
  * @brief  TIM5全局中断服务函数
  * @param  无
  * @retval 无
  */
void TIM5_IRQHandler(void)
{
    // 检查更新中断标志位
    if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
    {
        
        
        // 只有在电机处于闭环运行时才更新duty
        if (motor.run_flag == MOTOR_RUNNING_CLOSED)
        {
            Update_Motor_Duty();
			
        }
    }
	// 清除中断标志位
    TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
}

