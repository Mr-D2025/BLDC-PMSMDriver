#include "Drv_TIM.h"
#include "led.h"
#include "PMSM.h"

volatile uint32_t TIM3_counter=0;

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
uint32_t Record_Commutation_Interval(void)
{
    uint32_t current_count;
    TIM_Cmd(TIM2, DISABLE);// 停止定时器
    current_count = TIM_GetCounter(TIM2);// 读取当前计数值
    TIM_SetCounter(TIM2, 0);// 清零计数器
    TIM_Cmd(TIM2, ENABLE);// 重新启动定时器
	
	return current_count;
}

//通用定时器3中断初始化
void TIM3_Int_Init(uint16_t arr,uint16_t psc)
{
	//arr：自动重装值。
	//psc：时钟预分频数
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟
	
	TIM_TimeBaseInitStructure.TIM_Period = arr-1; 	//自动重装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc-1;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化TIM3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器3
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //定时器3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=2; //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

//定时器3中断服务函数
void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{
        TIM3_counter++;
        LED_Control();
		velocityOpenloop();
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位
}
