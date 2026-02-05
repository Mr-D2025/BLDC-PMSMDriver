#include "sys.h"
#include "Drv_TIM.h"
#include "BLDC.h"
#include "BEMF.h"
#include "led.h"
#include "OLED.h"

volatile uint32_t CommutationInterval = 0;  // 换相间隔（微秒）
volatile float MotorRPM = 0.0f;             // 电机转速（RPM）
uint32_t TIM3_counter = 0;

// 定时器2初始化用于记录换相时间
void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_InternalClockConfig(TIM2);
    
    TIM_TimeBaseInitStruct.TIM_Prescaler = 84 - 1;	   // 84MHz/84 = 1MHz
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStruct.TIM_Period = 0xFFFFFFFF;    // 32位最大值
    TIM_TimeBaseInitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;    // 重复计数器
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);
    
    TIM_Cmd(TIM2, ENABLE);
}

// 换相时记录间隔
void Record_Commutation_Interval(void)
{
    uint32_t current_count;
    TIM_Cmd(TIM2, DISABLE);// 停止定时器
    current_count = TIM_GetCounter(TIM2);// 读取当前计数值
    CommutationInterval = current_count;// 保存为换相间隔
    TIM_SetCounter(TIM2, 0);// 清零计数器
    TIM_Cmd(TIM2, ENABLE);// 重新启动定时器
}

//通用定时器3中断初始化,用于总时间控制
void TIM3_Int_Init(void)
{
	//arr：自动重装值。
	//psc：时钟预分频数
	
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	TIM_InternalClockConfig(TIM3);
	
	TIM_TimeBaseInitStruct.TIM_Period = 100-1; 	//自动重装载值
	TIM_TimeBaseInitStruct.TIM_Prescaler=84-1;  //定时器分频
	TIM_TimeBaseInitStruct.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStruct.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInitStruct.TIM_RepetitionCounter = 0;    // 重复计数器
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStruct);//初始化TIM3
	
	// 清除中断标志位
    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2; //抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=2; //子优先级2
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

//通用定时器4中断初始化，用于闭环换相控制
void TIM4_Int_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能TIM4时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    
    // 配置定时器4
    TIM_TimeBaseInitStructure.TIM_Period = 0xFFFF;          // 初始重装载值
    TIM_TimeBaseInitStructure.TIM_Prescaler = 84 - 1;       // 预分频系数，84MHz/84 = 1MHz = 1us
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Down; // 向下计数模式
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;    // 重复计数器
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
    
    // 配置为单脉冲模式（不自动重装）
    TIM_SelectOnePulseMode(TIM4, TIM_OPMode_Single);
    
    // 清除中断标志位
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    
    // 配置更新中断，但先不使能
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
	
	// 禁用自动重装载预装载
    TIM_ARRPreloadConfig(TIM4, DISABLE);
    
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;         // TIM4中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;      // 子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         // 使能IRQ通道
    NVIC_Init(&NVIC_InitStructure);
}

void TIM4_Enable(uint16_t count_value)
{
    // 确保定时器已停止
    TIM_Cmd(TIM4, DISABLE);
    
    // 清除中断标志位
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    
    // 设置自动重装载值（中断触发点）
    TIM_SetAutoreload(TIM4, count_value);  // 计数到count_value时触发中断
    
    // 重置计数器从0开始计数
    TIM_SetCounter(TIM4, 0);               // 从0开始向上计数
    
    // 使能更新中断
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
    
    // 使能定时器（单次模式）
    TIM_Cmd(TIM4, ENABLE);
}

//  禁用TIM4定时器和中断
void TIM4_Disable(void)
{
    // 禁用定时器
    TIM_Cmd(TIM4, DISABLE);
    
    // 禁用更新中断
    TIM_ITConfig(TIM4, TIM_IT_Update, DISABLE);
    
    // 清除中断标志位
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}


//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
		TIM3_counter++;
		if(0)
		{
			motor.run_flag = BYTE_LOW_VOLTAGE;
		}
		// 电池电压检测为最高优先级，当电压过低，就不转
		if(motor.run_flag != BYTE_LOW_VOLTAGE)
		{
//			// 输入的占空比为0，则为空闲状态
//			if(motor.duty == 0)
//			{
//				motor.run_flag = MOTOR_STOPPED;
//			}
//			else
//			{
//				// 输入的占空比不为0，则从空闲状态转为电机开始
//				if(motor.run_flag == MOTOR_STOPPED)
//				{
//					motor.run_flag = MOTOR_STARTING;
//				}
//			}
		}
		
		LED_Control();
		motor_control();
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}

uint32_t tt=0;
//  TIM4换相中断
void TIM4_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
    {
		
        uint16_t tim_com; 
        
        if(xc_flag == 0)
        {
            if(MOTOR_RUNNING_CLOSED == motor.run_flag)
            {
				
                // 换相
				motor.step = (motor.step+1)%6;
                SixStep_Commutation(motor.step,motor.duty_register);
                
                xc_flag = 1;
                
                // 计算消磁时间（10度）
                tim_com = motor.filter_commutation_time_sum / 36;
                
                // 停止定时器4
                TIM4_Disable();
                
                // 重新配置TIM4进行10度消磁延时
                TIM4_Enable(tim_com);
                
                // 数据处理：更新换相时间记录
                motor.commutation_time_sum -= motor.commutation_time[motor.step];
                motor.commutation_time[motor.step] = temp_commutation_time;
                motor.commutation_time_sum += temp_commutation_time;
                motor.commutation_num++;
                // 一阶低通滤波
                motor.filter_commutation_time_sum = (motor.filter_commutation_time_sum * 7 + motor.commutation_time_sum * 1) >> 3;
                
                // 等待稳定，再开始换相错误判断
                if((BLDC_CLOSE_LOOP_WAIT) < motor.commutation_num)
                {
                    // 本次换向60度的时间，在上一次换向一圈时间的45度到75度，否则认为换向错误
                    if((temp_commutation_time > (motor.filter_commutation_time_sum * 40 / 360)) && 
                       (temp_commutation_time < (motor.filter_commutation_time_sum * 80 / 360)))
                    {
                        // 延时减去换向失败计数器
                        if((motor.commutation_failed_num))
                        {
                            motor.commutation_failed_num--;
                        }
                    }
                    else
                    {
                        // 只有在占空比大于10%的时候，才进行换相错误判断。
//        				if(motor.duty_register >= 15)
                        {
                            motor.commutation_failed_num ++;
                            if(BLDC_COMMUTATION_FAILED_MAX < motor.commutation_failed_num)
                            {
                                motor_stop();
                                motor.run_flag = MOTOR_STOP_STALL;
                            }
                        }
                    }
                }
            }
        }
        else if(xc_flag == 1)
        {
            xc_flag = 2;
            
            // 消磁时间到，重新使能比较器
            // 等待消磁, 10度（硬件自动等待）
            // while(((TH0 << 8) | TL0) < (motor.filter_commutation_time_sum / 36));
            
            // 重新使能比较器中断（开始检测下一个过零点）
            BEMF_IRQ_Config(motor.step);
            
            // 关闭TIM4定时器
            TIM4_Disable();
        }
         // 清除中断标志位
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }
}


