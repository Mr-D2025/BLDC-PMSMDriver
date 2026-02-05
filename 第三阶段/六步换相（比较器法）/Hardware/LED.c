#include "led.h" 
#include "BLDC.h"
#include "Drv_TIM.h"
 
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

// LED控制函数，在TIM3中断中调用
void LED_Control(void)
{
    // 使用TIM3_counter作为时间基准
    uint32_t current_counter = TIM3_counter;
    
    switch(motor.run_flag)
    {
        case MOTOR_STOPPED:         // 已停止 - 常灭
            LED1 = 1;  // 灭
            break;
            
        case MOTOR_STARTING:        // 预定位并启动 - 快闪（10ms亮，10ms灭）
            // 每200个100us（20ms）为一个周期，前100个（10ms）亮
            if((current_counter % 2000) < 1000)
            {
                LED1 = 0;  // 亮
            }
            else
            {
                LED1 = 1;  // 灭
            }
            break;
            
        case MOTOR_RUNNING_OPENED:  // 开环运行 - 慢闪（500ms亮，500ms灭）
            // 每10000个100us（1秒）为一个周期，前5000个（500ms）亮
            if((current_counter % 10000) < 5000)
            {
                LED1 = 0;  // 亮
            }
            else
            {
                LED1 = 1;  // 灭
            }
            break;
            
        case MOTOR_RUNNING_CLOSED:  // 切入闭环运行 - 常亮
            LED1 = 0;  // 常亮
            break;
            
        case MOTOR_STOP_STALL:      // 堵转 - 三闪（200ms亮，100ms灭，重复3次后停顿）
            {
                // 12秒一个完整循环
                uint32_t cycle = current_counter % 120000;
                
                if(cycle < 90000)  // 前9秒：三闪
                {
                    // 每个闪烁周期300ms
                    uint32_t flash_cycle = cycle % 30;
                    if(flash_cycle < 2000)  // 前200ms亮
                    {
                        LED1 = 0;  // 亮
                    }
                    else  // 后100ms灭
                    {
                        LED1 = 1;  // 灭
                    }
                }
                else  // 后3秒：停顿灭
                {
                    LED1 = 1;  // 灭
                }
            }
            break;
            
        case MOTOR_RESTART:         // 重启 - 双闪（100ms亮，100ms灭，闪2次后停顿）
            {
                // 8秒一个完整循环
                uint32_t cycle = current_counter % 80000;
                
                if(cycle < 400)  // 前4秒：双闪
                {
                    // 每个闪烁周期200ms
                    uint32_t flash_cycle = cycle % 2000;
                    if(flash_cycle < 1000)  // 前100ms亮
                    {
                        LED1 = 0;  // 亮
                    }
                    else  // 后100ms灭
                    {
                        LED1 = 1;  // 灭
                    }
                }
                else  // 后4秒：停顿灭
                {
                    LED1 = 1;  // 灭
                }
            }
            break;
            
        case BYTE_LOW_VOLTAGE:      // 低压 - 1长2短（长300ms，短100ms）
            {
                // 10秒一个完整循环
                uint32_t cycle = current_counter % 100000;
                
                if(cycle < 30000)  // 前300ms：长亮
                {
                    LED1 = 0;  // 亮
                }
                else if(cycle < 40000)  // 第400ms：间隔灭
                {
                    LED1 = 1;  // 灭
                }
                else if(cycle < 50000)  // 第500ms：第1个短亮
                {
                    LED1 = 0;  // 亮
                }
                else if(cycle < 60000)  // 第600ms：间隔灭
                {
                    LED1 = 1;  // 灭
                }
                else if(cycle < 70000)  // 第700ms：第2个短亮
                {
                    LED1 = 0;  // 亮
                }
                else  // 第800ms-1000ms：停顿灭
                {
                    LED1 = 1;  // 灭
                }
            }
            break;
            
        default:                    // 默认情况
            LED1 = 1;  // 灭
            break;
    }
}
