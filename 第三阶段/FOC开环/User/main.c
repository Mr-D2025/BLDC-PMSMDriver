#include "sys.h"
#include "delay.h"
#include "Drv_TIM.h"
#include "Drv_Motor.h"
#include "LED.h"
#include "arm_math.h"
#include "math.h"
#include "OLED.h"
#include "BEEP.h"
#include "PMSM.h"


int main(void)
{
	
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    
    delay_init(168);		  
    LED_Init();
    
    TIM3_Int_Init(100,84);	
    motor_Init();
    delay_ms(1000);
    PWM_Centre_Init();
    PMSM.target_velocity = TARGET_VELOCITY;
    Motor_Enable();
    
    
    

    while(1)
    {

    
    
    }
}
	

