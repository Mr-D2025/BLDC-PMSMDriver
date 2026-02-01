#include "sys.h"
#include "BLDC.h"
#include "delay.h"

// 全局电机控制变量
motor_struct_t motor;


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
//	static const uint16_t musical_notes[] = {NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5};
//	
//    static const uint16_t melody_twinkle_star[] = {
//    NOTE_C5, NOTE_C5, NOTE_G5, NOTE_G5, NOTE_A5, NOTE_A5, NOTE_G5, NOTE_REST,
//    NOTE_F5, NOTE_F5, NOTE_E5, NOTE_E5, NOTE_D5, NOTE_D5, NOTE_C5, NOTE_REST};
	
	static const uint16_t melody_happy_birthday[] = {
    NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_F5, NOTE_E5, NOTE_REST,
    NOTE_C5, NOTE_C5, NOTE_D5, NOTE_C5, NOTE_G5, NOTE_F5, NOTE_REST,
    NOTE_C5, NOTE_C5, NOTE_C6, NOTE_A5, NOTE_F5, NOTE_E5, NOTE_D5, NOTE_REST,
    NOTE_A5, NOTE_A5, NOTE_A5, NOTE_F5, NOTE_G5, NOTE_F5, NOTE_REST};
	
    // 使用motor_musical_beep函数播放三个音符
    motor_musical_beep(volume, melody_happy_birthday, 29, BLDC_Beep_duration);
    
    
    
    // 短暂静音
    delay_ms(50);
    
}

//-------------------------------------------------------------------------------------------------------------------
//  @brief      电机初始化
//  @param      void
//  @return     void
//-------------------------------------------------------------------------------------------------------------------
void motor_init(void)
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
    motor.speed_rpm = 0;
    motor.last_commutation_tick = 0;
	
	TIM1_TB_Init(100, 42);
	TIM1_OC_Init();
	TIM_CtrlPWMOutputs(TIM1, ENABLE);	//使能主输出
	TIM_Cmd(TIM1, ENABLE);				//使能定时器

    motor_power_on_beep(BLDC_Beep_Volume);
	
}


