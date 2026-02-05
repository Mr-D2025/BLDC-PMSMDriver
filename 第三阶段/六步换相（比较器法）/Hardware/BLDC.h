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

// 蜂鸣器提示
#define BLDC_Beep_Volume 				(1)         // 蜂鸣器音量等级（1-5）
#define BLDC_Beep_duration 				(300)       // 蜂鸣持续时间（ms）

// PWM相关宏定义
#define BLDC_MAX_DUTY                   (90)        // PWM的最大占空比
#define BLDC_MIN_DUTY                   (5)         // PWM的最小占空比
#define BLDC_PWM_DEADTIME               (5)         // PWM死区时间对应的占空比

// 启动参数
#define BLDC_START_DUTY                 (10)         // 启动初始占空比
#define BLDC_START_COMMUTATION_TIME     (6000)      // 启动初始占空比
#define BLDC_START_DELAY                (2000)      // 重启延时（单位：100us）

// 开环运行参数
#define BLDC_OPEN_LOOP_WAIT             (50)        // 开环运行等待的换相次数
#define BLDC_OPENED_ANGLE				(30)		// 开环进角，无中断换相时，延时多少度进行换相(0-30)
#define BLDC_OPEN_LOOP_TIMEOUT          (8000)      // 开环超时时间（us）

// 闭环运行参数
#define BLDC_CLOSE_LOOP_WAIT            (50)        // 闭环加速等待的换相次数
#define BLDC_CLOSED_ANGLE				(30)		// 闭环进角，进入比较器中断后，延时多少度进行换相(0-30)

// 速度控制参数
#define BLDC_SPEED_INCREMENTAL          (10)        // 速度调整间隔（调用次数）

// 堵转检测参数
#define BLDC_STALL_TIMEOUT              (2000)      // 堵转超时计数（单位：函数调用次数）
#define BLDC_COMMUTATION_FAILED_MAX     (200)     	// 换相错误最大次数 大于这个次数后认为电机堵转

//// 安全保护参数
//#define BLDC_OVERCURRENT_THRESHOLD    (10000)     // 过流保护阈值（mA）
//#define BLDC_OVERVOLTAGE_THRESHOLD    (16800)     // 过压保护阈值（mV）
//#define BLDC_UNDERVOLTAGE_THRESHOLD   (10800)     // 欠压保护阈值（mV）

// 电机状态枚举
typedef enum {
    MOTOR_STOPPED = 0,         // 已停止
    MOTOR_STARTING,            // 预定位并启动
	MOTOR_RUNNING_OPENED,      // 开环运行
    MOTOR_RUNNING_CLOSED,      // 切入闭环运行
	MOTOR_STOP_STALL,		   // 堵转
    MOTOR_RESTART,			   // 重启
	
	BYTE_LOW_VOLTAGE		   // 低压
} Motor_State_t;

// 电机控制结构体
typedef struct
{
    uint8_t   step;                   // 电机运行的步骤 (0-5)
    uint16_t  duty;                   // PWM占空比       用户改变电机速度时修改此变量 (0-100)
    uint16_t  duty_register;          // PWM占空比寄存器 用户不可操作 (实际寄存器值)
    volatile  uint8_t run_flag;       // 电机运行标志位 0:已停止 1:预定位并启动 2:开环运行 3:切入闭环运行
    float     motor_start_delay;      // 开环启动的换相的延时时间(ms)
    uint16_t  motor_start_wait;       // 开环启动时，统计换相的次数
    uint16_t  restart_delay;          // 电机延时重启计数
    uint16_t  commutation_failed_num; // 换相错误次数
    uint16_t  commutation_time[6];    // 最近6次换相时间(us)
    uint32_t  commutation_time_sum;   // 最近6次换相时间总和
    uint32_t  commutation_num;        // 统计换相次数
    uint32_t  filter_commutation_time_sum; // 滤波后的换相时间总和
//    uint32_t  speed_rpm;              // 电机转速(RPM)
    uint32_t  last_commutation_tick;  // 上次换相的时间戳
} motor_struct_t;

extern motor_struct_t motor;
extern uint16_t temp_commutation_time;
extern uint8_t xc_flag;

void TIM1_SetBeepFrequency(uint32_t frequency);
void motor_musical_beep(uint16_t volume, const uint16_t *notes, uint8_t num_notes, uint16_t note_duration);
void motor_Init(void);
void motor_control(void);
void motor_start(void);
void motor_stop(void);


#endif
