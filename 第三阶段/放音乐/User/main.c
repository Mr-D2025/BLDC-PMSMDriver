#include "sys.h"
#include "delay.h"
#include "Drv_TIM.h"
#include "Drv_BLDC.h"
#include "BLDC.h"
#include "LED.h"

// 定义PWM参数
#define PWM_DUTY 70      // 70%占空比（直接使用百分比）

// 全局变量用于存储当前占空比设置
volatile uint16_t pwm_duty_percent = 70;  // 默认70%
const uint16_t musical_notes[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5};

int main(void)
{
	uint8_t step_counter = 0;  // 换相步骤计数器
    uint16_t pwm_duty_value;   // 实际PWM占空比值
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	delay_init(168);		  
	LED_Init();
	TIM3_Int_Init(5000,8400);	//定时器时钟84M，分频系数8400，所以84M/8400=10Khz的计数频率，计数5000次为500ms	
	
	IO_Init();
	motor_init();
	//PWM_Centre_Init();

	// 初始状态：停止所有输出
    SixStep_Stop();
    
    // 等待一段时间，让系统稳定
    delay_ms(100);
	
	while(1)
	{
//		motor_musical_beep(BLDC_Beep_Volume,musical_notes,6,BLDC_Beep_duration);
//        SixStep_Commutation(0, PWM_DUTY); 
//        // 更新步骤计数器
//        step_counter++;
//        if(step_counter >= 6) {
//            step_counter = 0;
//            
//        }
//        
//        // 延时控制换相频率
//        delay_ms(500);
//		TIM1_TB_Init(100, 42);
//		SixStep_Commutation(0, PWM_DUTY); 
//        delay_ms(500);
    }
}
	

