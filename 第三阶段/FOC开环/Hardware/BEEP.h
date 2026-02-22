#ifndef __BEEP_H
#define __BEEP_H
#include "sys.h"
#include "Drv_Motor.h"

#define NOTE_C4  262    // Do-
#define NOTE_D4  294    // Re-
#define NOTE_E4  330    // Mi-
#define NOTE_F4  349    // Fa-
#define NOTE_G4  392    // Sol-
#define NOTE_A4  440    // La-
#define NOTE_B4  494    // Si-
#define NOTE_C5  523    // Do
#define NOTE_D5  587    // Re
#define NOTE_E5  659    // Mi
#define NOTE_F5  698    // Fa
#define NOTE_G5  783    // Sol
#define NOTE_A5  880    // La
#define NOTE_B5  987    // Si
#define NOTE_C6  1046   // Do+

#define NOTE_REST 0     // 休止符

// 蜂鸣器提示
#define BLDC_Beep_Volume 				(1)         // 蜂鸣器音量等级（1-5）
#define BLDC_Beep_duration 				(300)       // 蜂鸣持续时间（ms）

void BEEP_IO_Init(void);
void BEEP_OC_Init(void);
void SixStep_Commutation(SixStep_Phase step, uint16_t pwm_duty);
void SixStep_Stop(void);
void motor_musical_beep(uint16_t volume, const uint16_t *notes, uint8_t num_notes, uint16_t note_duration);
void motor_power_on_beep(uint16_t volume);


#endif
