#include "sys.h"
#include "BEEP.h"
#include "delay.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

void BEEP_IO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOB, ENABLE);
	
	// 配置上桥臂GPIO (PA8, PA9, PA10 - TIM1_CH1, CH2, CH3)
    // 这些引脚用作PWM输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);   // PA8 -> TIM1_CH1 (A相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);   // PA9 -> TIM1_CH2 (B相)
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);  // PA10 -> TIM1_CH3 (C相)
    
    // 配置下桥臂GPIO (PB13, PB14, PB15)
    // 这些引脚用作普通GPIO输出控制下桥臂的导通/关断
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 初始状态：所有下桥臂关闭
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
}

void BEEP_OC_Init(void)
{
    
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    
    // 配置PWM输出通道（只配置上桥臂的3个通道）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;         // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputState_Disable; // 禁用互补输出
    TIM_OCInitStructure.TIM_Pulse = 0;                        // 初始占空比为0
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 高电平有效
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    
    // 初始化三个通道
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);  // CH1 - A相
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);  // CH2 - B相
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);  // CH3 - C相
    
    //使能预装载寄存器
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    
}

// 六步换相函数（120°导通方式）
void SixStep_Commutation(SixStep_Phase step, uint16_t pwm_duty)
{
    // 先关闭所有下桥臂
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
    
    // 关闭所有上桥臂PWM（占空比设为0）
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
    
    // 根据六步换相表设置导通状态
    switch(step) {
        case STEP_AB:  // A+ B-
            TIM_SetCompare1(TIM1, pwm_duty);      // A相上桥臂PWM
            PBout(14)=1;     					  // B相下桥臂导通 (PB14)
            break;
            
        case STEP_AC:  // A+ C-
            TIM_SetCompare1(TIM1, pwm_duty);      // A相上桥臂PWM
            PBout(15)=1;     					  // C相下桥臂导通 (PB15)
            break;
            
        case STEP_BC:  // B+ C-
            TIM_SetCompare2(TIM1, pwm_duty);      // B相上桥臂PWM
            PBout(15)=1;     					  // C相下桥臂导通 (PB15)
            break;
            
        case STEP_BA:  // B+ A-
            TIM_SetCompare2(TIM1, pwm_duty);      // B相上桥臂PWM
            PBout(13)=1;    					  // A相下桥臂导通 (PB13)
            break;
            
        case STEP_CA:  // C+ A-
            TIM_SetCompare3(TIM1, pwm_duty);      // C相上桥臂PWM
            PBout(13)=1;     					  // A相下桥臂导通 (PB13)
            break;
            
        case STEP_CB:  // C+ B-
            TIM_SetCompare3(TIM1, pwm_duty);      // C相上桥臂PWM
            PBout(14)=1;    					  // B相下桥臂导通 (PB14)
            break;
    }
}

// 停止所有输出
void SixStep_Stop(void)
{
    // 关闭所有上桥臂PWM
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
    
    // 关闭所有下桥臂
    GPIO_ResetBits(GPIOB, GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15);
}

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
    
    TIM_CtrlPWMOutputs(TIM1, DISABLE);	//失能主输出
	TIM_Cmd(TIM1, DISABLE);				//失能定时器

    
    // 短暂静音
    delay_ms(50);
    
}
