#include "sys.h"
#include "BLDC.h"
#include "delay.h"
#include "Drv_TIM.h"
#include "BEMF.h"

// 全局电机控制变量
motor_struct_t motor;
uint16_t temp_commutation_time = 0;
uint8_t xc_flag = 0;	
// 0-换相
// 1-消磁
// 2-消磁结束

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))


void motor_musical_beep(uint16_t volume, const uint16_t *notes, uint8_t num_notes, uint16_t note_duration)
{
    //  volume          音量大小（0-100）
    //  notes           音符频率数组
    //  num_notes       音符数量
    //  note_duration   每个音符持续时间(ms)
    uint16_t beep_duty;
    uint8_t phase_step = 0;
    
    // 计算鸣叫占空比
    beep_duty = ((volume > 100) ? 100 : volume);
    
    for(uint8_t i = 0; i < num_notes; i++)
    {
        uint16_t current_note = notes[i];
        
        if(current_note == 0)
        {
            // 休止符 - 停止所有输出
            SixStep_Stop();
        }
        else
        {
			uint16_t psc=84000000/100/current_note;
            TIM1_TB_Init(100,psc);
			
            // 使用六步换相函数来控制相序
            // 每次使用不同的相序组合产生声音
            switch(phase_step)
            {
                case 0:	// STEP_AB: A相上桥PWM，B相下桥导通
                    SixStep_Commutation(STEP_AB, beep_duty);
                    break;
                    
                case 1:	// STEP_AC: A相上桥PWM，C相下桥导通
					SixStep_Commutation(STEP_AC, beep_duty);
                    break;
                    
                case 2:	// STEP_BC: B相上桥PWM，C相下桥导通 
                    SixStep_Commutation(STEP_BC, beep_duty);
                    break;
                    
                case 3:	// STEP_BA: B相上桥PWM，A相下桥导通
                    SixStep_Commutation(STEP_BA, beep_duty);
                    break;
                    
                case 4:	// STEP_CA: C相上桥PWM，A相下桥导通
                    SixStep_Commutation(STEP_CA, beep_duty);
                    break;
                    
                case 5:	// STEP_CB: C相上桥PWM，B相下桥导通    
                    SixStep_Commutation(STEP_CB, beep_duty);
                    break;
            }
            
				phase_step=(phase_step+1)%6;  // 6种换相组合	
        }
        delay_ms(note_duration);		// 保持音符时长
    }
    SixStep_Stop();		// 播放结束，停止所有输出
}



//  电机上电鸣叫（三个音调对应三相）
void motor_power_on_beep(uint16_t volume)
{
    if(volume > 100) volume = 100;
   
    // 音调数组
	static const uint16_t musical_notes[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5};

	
    // 使用motor_musical_beep函数播放三个音符
    motor_musical_beep(volume, musical_notes, ARRAY_LEN(musical_notes), BLDC_Beep_duration);
    
    
    
    // 短暂静音
    delay_ms(50);
    
}


//  电机初始化
void motor_Init(void)
{
       // 变量清零
    motor.step = 0;
    motor.duty = 0;
    motor.duty_register = 0;
    motor.run_flag = MOTOR_STOPPED;
    motor.motor_start_delay = 10.0f;     // 初始换相延时10ms
    motor.motor_start_wait = 0;
    motor.restart_delay = 0;
    motor.commutation_failed_num = 0;
    motor.commutation_num = 0;
    motor.commutation_time_sum = 0;
    motor.filter_commutation_time_sum = 0;
//    motor.speed_rpm = 0;
//    motor.last_commutation_tick = 0;
	
	TIM1_TB_Init(100, 40);
	TIM1_OC_Init();
	TIM_CtrlPWMOutputs(TIM1, ENABLE);	//使能主输出
	TIM_Cmd(TIM1, ENABLE);				//使能定时器

    motor_power_on_beep(BLDC_Beep_Volume);
}

// 电机控制函数
void motor_control(void)
{
    static uint16_t stall_time_out_check = 0;
    static uint8_t bemf_filter_state = 0;
    static uint8_t last_bemf_state = 0;
    uint8_t current_bemf_state = 0;
    
    switch(motor.run_flag) {
        case MOTOR_STARTING: {           // 预定位并启动
            // 关闭BEMF中断
            BEMF_IRQ_Disable();
            motor.duty = BLDC_START_DUTY;
            // 初始化换相时间数组
            for(int i = 0; i < 6; i++) {
                motor.commutation_time[i] = BLDC_START_COMMUTATION_TIME;  // 6ms初始换相时间
            }
            motor.commutation_time_sum = 6 * BLDC_START_COMMUTATION_TIME;
            motor.filter_commutation_time_sum = motor.commutation_time_sum;
            
			/*
			预留的预定位代码，将返回从那一相开始。
			
			
			
			*/

            // 先不管预定位,直接强拉，默认从AB开始。
            motor.step = 0;
            
			motor.duty_register = motor.duty;
			SixStep_Commutation(STEP_AB,motor.duty_register); //强制定位
            
            motor.commutation_num = 0;
            stall_time_out_check = 0;
            motor.motor_start_wait = 0;
			
			//开始换相
            motor.step = (motor.step + 1) % 6;  
			SixStep_Commutation(motor.step,motor.duty_register);
			//计时开始
			TIM_Cmd(TIM2, DISABLE);
            TIM_SetCounter(TIM2, 0);
            TIM_Cmd(TIM2, ENABLE);

            // 切换到开环运行
            motor.run_flag = MOTOR_RUNNING_OPENED;
			
        }
        break;

        case MOTOR_RUNNING_OPENED: {     // 开环运行
            uint8_t motor_next_step_flag = 0;
            uint32_t current_time = TIM_GetCounter(TIM2);  // 当前计时值（微秒）
            uint8_t bemf_phase_to_check = BEMF_Config[motor.step].phase;  // 获取需要检测的相位
            
			switch (bemf_phase_to_check) 
			{
				case 0x01:  // A相 (PC6)
					current_bemf_state = (PCin(6));  // 读取PC6电平
					break;
				case 0x02:  // B相 (PC7)
					current_bemf_state = (PCin(7));  // 读取PC7电平
					break;
				case 0x04:  // C相 (PC8)
					current_bemf_state = (PCin(8));  // 读取PC8电平
					break;
			}
            
            // 软件滤波（8位移位滤波）
            bemf_filter_state = ((bemf_filter_state << 1) | (current_bemf_state)); 
            
            // 根据当前导通状态检测对应的过零沿
			if (motor.step < 6) {
				uint8_t target_edge = BEMF_Config[motor.step].edge;  // 0:下降沿, 1:上升沿
				
				if (target_edge == 0) 
				{  // 下降沿检测
					if (bemf_filter_state == 0x00) 
					{  // 检测到下降沿
						motor_next_step_flag = 1;
					}
				} 
				else if (target_edge == 1) 
				{  // 上升沿检测
					if (bemf_filter_state == 0xFF) 
					{  // 检测到上升沿
						motor_next_step_flag = 1;
					}
				}
			}
            
            if(motor_next_step_flag) 
			{
                // 获取换相间隔时间
                uint32_t interval = TIM_GetCounter(TIM2);
                
                // 记录换相间隔并重置TIM2
                Record_Commutation_Interval();  // 停止、读取、清零TIM2
                
                // 堵转计数器清零
                stall_time_out_check = 0;
                
                // 更新换相时间数组
                motor.commutation_time_sum -= motor.commutation_time[motor.step];
                motor.commutation_time[motor.step] = interval;
                motor.commutation_time_sum += motor.commutation_time[motor.step];
                
                // 换相计数增加
                motor.commutation_num++;

                // 一阶低通滤波
                motor.filter_commutation_time_sum = (motor.filter_commutation_time_sum * 3 + motor.commutation_time_sum * 1) >> 2;
                
                // 计算电角度延迟
                uint32_t delay_deg = motor.filter_commutation_time_sum *BLDC_OPENED_ANGLE/ 360;  
                
                // 等待延迟
                uint32_t start_wait = TIM_GetCounter(TIM2);
                while((TIM_GetCounter(TIM2) - start_wait) < delay_deg) 
				{
                    // 等待延迟完成
                }
                
                // 执行换相
				motor.step = (motor.step + 1) % 6;
                SixStep_Commutation(motor.step,motor.duty_register);

            }
			else 
			{
                // 堵转检测：超时没有检测到过零
                if(stall_time_out_check++ > BLDC_STALL_TIMEOUT) 
				{  // 约200ms超时（假设100us调用一次）
                    stall_time_out_check = 0;
                    motor.commutation_failed_num++;
                    if(motor.commutation_failed_num > 5) 
					{
                        motor.run_flag = MOTOR_STOP_STALL;
                    }
                }
            }
            
            // 判断是否切换到闭环
            if(motor.commutation_num > BLDC_OPEN_LOOP_WAIT) 
			{
				
                // 清零计数器
                motor.commutation_failed_num = 0;
                motor.commutation_num = 0;
                stall_time_out_check = 0;
                
                // 开启BEMF中断，切换到闭环运行
                BEMF_IRQ_Config(motor.step);
                motor.run_flag = MOTOR_RUNNING_CLOSED;
            }
            
            // 开环超时检测（设定时间没有换相）
            if(current_time > BLDC_OPEN_LOOP_TIMEOUT)	// 超时 
			{  
                // 重启计时器
                TIM_Cmd(TIM2, DISABLE);
                TIM_SetCounter(TIM2, 0);
                TIM_Cmd(TIM2, ENABLE);
               
                // 强制换相
                motor.step = (motor.step + 1) % 6;
                SixStep_Commutation(motor.step,motor.duty_register);
                
            }
        }
        break;
        
        case MOTOR_RUNNING_CLOSED: {     // 切入闭环运行
            // 使用中断自动换相，这里主要做状态监控和速度控制
            
            // 读取当前比较器状态用于堵转检测
            // 获取当前步需要检测的相位
			uint8_t target_phase = BEMF_Config[motor.step].phase;

			// 读取当前步对应的相比较器状态
			current_bemf_state = 0;
			if (target_phase == 0x01) 
			{  // A相
				current_bemf_state = PCin(6);
			} 
			else if (target_phase == 0x02) 
			{  // B相
				current_bemf_state = PCin(7);
			} 
			else if (target_phase == 0x04) 
			{  // C相
				current_bemf_state = PCin(8);
			}
		
            // 堵转检测：比较器状态长时间不变
            if(current_bemf_state != last_bemf_state) 
			{
                stall_time_out_check = 0;
                last_bemf_state = current_bemf_state;
            } 
			else 
			{
                if(stall_time_out_check++ > 2000) 
				{  // 约200ms超时
                    stall_time_out_check = 0;
                    motor.commutation_failed_num++;
                    if(motor.commutation_failed_num > 3) 
					{
                        motor.run_flag = MOTOR_STOP_STALL;
                    }
                }
            }
            
            // 闭环速度限幅
            if(motor.commutation_num < BLDC_CLOSE_LOOP_WAIT) 
			{
                if(motor.duty_register < BLDC_PWM_DEADTIME) 
				{
                    motor.duty_register = BLDC_PWM_DEADTIME;
                }
                if(motor.duty_register > BLDC_MAX_DUTY) 
				{
                    motor.duty_register = BLDC_MAX_DUTY;
                }
            }
            
            // 缓慢加减速控制
            if((motor.duty_register != motor.duty)&&(TIM3_counter % BLDC_SPEED_INCREMENTAL == 0))
			{
                if(motor.duty > motor.duty_register) 
				{
                    motor.duty_register++;
                    if(motor.duty_register > BLDC_MAX_DUTY) 
					{
                        motor.duty_register = BLDC_MAX_DUTY;
                    }
                } 
				else if(motor.duty < motor.duty_register) 
				{
                    motor.duty_register--;
                    if(motor.duty_register < BLDC_PWM_DEADTIME) 
					{
                        motor.duty_register = BLDC_PWM_DEADTIME;
                    }
                }
				
            }
        }
        break;
        
        case MOTOR_STOP_STALL: {         // 堵转
            // 停止电机
            SixStep_Stop();
            
            // 关闭BEMF中断
            EXTI_InitTypeDef EXTI_InitStruct;
            EXTI_InitStruct.EXTI_Line = EXTI_Line6 | EXTI_Line7 | EXTI_Line8;
            EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
            EXTI_InitStruct.EXTI_LineCmd = DISABLE;
            EXTI_Init(&EXTI_InitStruct);
            
            motor.run_flag = MOTOR_RESTART;
            motor.restart_delay = BLDC_START_DELAY;  // 重启延时
        }
        break;
        
        case MOTOR_RESTART: {            // 重启
            if(motor.restart_delay > 0) 
			{
                motor.restart_delay--;
            } 
			else 
			{
                motor.run_flag = MOTOR_STOPPED;
            }
        }
        break;
        
        case MOTOR_STOPPED: {
            // 停止状态，确保所有输出关闭
            SixStep_Stop();
            
            // 关闭BEMF中断
            EXTI_InitTypeDef EXTI_InitStruct;
            EXTI_InitStruct.EXTI_Line = EXTI_Line6 | EXTI_Line7 | EXTI_Line8;
            EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
            EXTI_InitStruct.EXTI_LineCmd = DISABLE;
            EXTI_Init(&EXTI_InitStruct);
            
            // 重置参数
            motor.duty_register = 0;
            motor.commutation_failed_num = 0;
            motor.commutation_num = 0;
        }
        break;
    }
}

// 电机启动函数
void motor_start(void)
{
    if(motor.run_flag == MOTOR_STOPPED) 
	{
        motor.duty_register = motor.duty;
        motor.run_flag = MOTOR_STARTING;
    }
}

// 电机停止函数
void motor_stop(void)
{
    motor.run_flag = MOTOR_STOPPED;
    motor.duty_register = 0;
    SixStep_Stop();
}

