#include "led.h" 
#include "Drv_TIM.h"
#include "PMSM.h"
 
//初始化PF9和PF10为输出口.并使能这两个口的时钟		    
//LED IO初始化
void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);//使能GPIOF时钟

  //GPIOF9,F10初始化设置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  GPIO_Init(GPIOF, &GPIO_InitStructure);//初始化
	
  GPIO_SetBits(GPIOF,GPIO_Pin_9 | GPIO_Pin_10);//GPIOF9,F10设置高，灯灭
}

void LED_Control(void)
{
    static uint8_t idx = 0;
    
    switch (PMSM.state) {
        case MOTOR_STATE_DISABLED: 
            LED0 = 0; 
            break;
        
        case MOTOR_STATE_ENABLED:  // .-. R
            if (TIM3_counter >= 2000) {
                TIM3_counter = 0;
                idx = (idx + 1) % 8;
                LED0 = (idx == 0 || idx == 2 || idx == 3 || idx == 4) ? 0 : 1;
            }
            break;
            
        case MOTOR_STATE_CALIBRATING:  // ... S
            if (TIM3_counter >= 2000) {
                TIM3_counter = 0;
                idx = (idx + 1) % 9;
                LED0 = (idx == 0 || idx == 2 || idx == 4) ? 0 : 1;
            }
            break;
            
        case MOTOR_STATE_FAULT:  // --- O
            if (TIM3_counter >= 2000) {
                TIM3_counter = 0;
                idx = (idx + 1) % 18;
                LED0 = (idx < 3 || (idx >= 6 && idx < 9) || (idx >= 12 && idx < 15)) ? 0 : 1;
            }
            break;
            
        default: 
            LED0 = 1; 
            break;
    }
}




