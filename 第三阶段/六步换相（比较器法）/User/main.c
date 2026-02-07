#include "sys.h"
#include "delay.h"
#include "Drv_TIM.h"
#include "Drv_BLDC.h"
#include "BLDC.h"
#include "BEMF.h"
#include "LED.h"
#include "OLED.h"
#include "DEMO.h"

extern uint32_t tt;

int main(void)
{
	uint8_t step_counter = 0;  // 换相步骤计数器
    uint16_t pwm_duty_value;   // 实际PWM占空比值
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	delay_init(168);		  
	LED_Init();
	OLED_Init();
	PWM_IO_Init();
	motor_Init(); //电机初始化
	
	//电机初始化之后才可以初始化调制波PWM，否则PWM频率会出问题。
	PWM_Centre_Init();//中央对齐PWM初始化
	
	// 停止所有输出
    SixStep_Stop();
	BEMF_ZeroCross_Init();
	TIM2_Init();
	TIM3_Int_Init();	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms	
	TIM4_Int_Init();
	
	
    // 等待一段时间，让系统稳定
    delay_ms(100);
	
	delay_ms(1000);
	
	OLED_Clear();
	motor_start();
	TIM5_Init();
	OLED_ShowString(0,48,"duty:",16,1);
	while(1)
	{
		
		for(int i=0;i<6;i++)
		{
			if(i==1) OLED_ShowNum(40,48,motor.duty,3,16,1);
			OLED_Refresh();
			if((i>=0)&&(i<=2))
			{
				OLED_ShowNum(0,16*i,motor.commutation_time[i],5,16,1);
				OLED_Refresh();
				delay_ms(20);
			}
			else
			{
				OLED_ShowNum(5*8+8,16*i-3*16,motor.commutation_time[i],5,16,1);
				OLED_Refresh();
				delay_ms(20);
			}
		}
    }
}
	

