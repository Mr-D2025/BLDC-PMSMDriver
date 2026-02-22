#ifndef __PMSM_H
#define __PMSM_H
#include "sys.h"
#include "Drv_Motor.h"
#include "Drv_TIM.h"
#include "arm_math.h"

//电机状态枚举
typedef enum {
    MOTOR_STATE_DISABLED = 0,         // 电机关闭状态
    MOTOR_STATE_ENABLED,              // 电机使能状态
    MOTOR_STATE_CALIBRATING,          // 电机校准中
    MOTOR_STATE_FAULT                 // 电机故障状态
} Motor_State_t;

//电压相关结构体
typedef struct {
    float Ud;                         // d轴电压 (V)
    float Uq;                         // q轴电压 (V)
    float Ualpha;                     // alpha轴电压 (V)
    float Ubeta;                      // beta轴电压 (V)
    float Ua;                         // A相电压 (V)
    float Ub;                         // B相电压 (V)
    float Uc;                         // C相电压 (V)
} Voltage_TypeDef;

// PWM占空比相关结构体
typedef struct {
    uint16_t a;                        // A相占空比比较值
    uint16_t b;                        // B相占空比比较值
    uint16_t c;                        // C相占空比比较值
    float duty_a;                      // A相占空比 (0-1)
    float duty_b;                      // B相占空比 (0-1)
    float duty_c;                      // C相占空比 (0-1)
} PWM_TypeDef;

//电机控制结构体
typedef struct {
    //角度相关
    float shaft_angle;                  // 机械角度 (rad)
    float electrical_angle;             // 电角度 (rad)
    float zero_electric_angle;          // 零位电角度偏移 (rad)
    float delta_angle;                  // 角度增量 (rad)
    
    //电压相关
    Voltage_TypeDef volt;               // 电压结构体
    
    //PWM占空比相关
    PWM_TypeDef pwm;                    // PWM结构体
    
    //控制相关
    float target_velocity;              // 目标速度 (rad/s)
    float current_velocity;             // 当前速度 (rad/s)
  
    //状态标志
    volatile Motor_State_t state;       // 电机运行状态
    
} motor_struct_t;

//全局结构体对象声明
extern Voltage_TypeDef PMSM_VOLT;
extern PWM_TypeDef PMSM_PWM;
extern motor_struct_t PMSM;

//电机参数宏定义
#define VOLTAGE_POWER_SUPPLY    12.0f    // 电源电压 (V)
#define MOTOR_POLE_PAIRS        7        // 电机极对数
#define TARGET_VELOCITY         40.0f     // 默认目标速度 (rad/s)

//约束宏定义
#define Limit(amt, low, high) (((amt) < (low)) ? (low) : (((amt) > (high)) ? (high) : (amt)))


void Motor_Struct_Init(void);
void PWM_Struct_Init(void);
void VOLT_Struct_Init(void);
void motor_Init(void);

void Motor_Enable(void);
void Motor_Disable(void);
float normalizeAngle(float angle);
float electricalAngle(float shaft_angle);
void setPwm(void);
void setPhaseVoltage(float32_t Uq, float32_t Ud, float32_t angle_el);
void velocityOpenloop(void);
    
#endif
