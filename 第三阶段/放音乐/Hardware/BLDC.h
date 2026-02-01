#ifndef __BLDC_H
#define __BLDC_H
#include "sys.h"
#include "Drv_BLDC.h"


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

#define BLDC_Beep_Volume 1
#define BLDC_Beep_duration 300

// 电机状态枚举
typedef enum {
    MOTOR_STOPPED = 0,         // 已停止
    MOTOR_STARTING,            // 正在启动
    MOTOR_RUNNING_CLOSED,      // 切入闭环运行
    MOTOR_RUNNING              // 启动完成正常运行
} Motor_State_t;

// 电机控制结构体
typedef struct
{
    uint8_t   step;                   // 电机运行的步骤 (0-5)
    uint16_t  duty;                   // PWM占空比       用户改变电机速度时修改此变量 (0-100)
    uint16_t  duty_register;          // PWM占空比寄存器 用户不可操作 (实际寄存器值)
    volatile  uint8_t run_flag;       // 电机运行标志位 0:已停止 1：正在启动 2：切入闭环 3：启动完成正常运行
    float     motor_start_delay;      // 开环启动的换相的延时时间(ms)
    uint16_t  motor_start_wait;       // 开环启动时，统计换相的次数
    uint16_t  restart_delay;          // 电机延时重启计数
    uint16_t  commutation_failed_num; // 换相错误次数
    uint16_t  commutation_time[6];    // 最近6次换相时间(us)
    uint32_t  commutation_time_sum;   // 最近6次换相时间总和
    uint32_t  commutation_num;        // 统计换相次数
    uint32_t  filter_commutation_time_sum; // 滤波后的换相时间总和
    uint32_t  speed_rpm;              // 电机转速(RPM)
    uint32_t  last_commutation_tick;  // 上次换相的时间戳
} motor_struct_t;

void TIM1_SetBeepFrequency(uint32_t frequency);
void motor_musical_beep(uint16_t volume, const uint16_t *notes, uint8_t num_notes, uint16_t note_duration);
void motor_init(void);


extern motor_struct_t motor;

#endif
