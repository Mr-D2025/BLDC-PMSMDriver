#include "sys.h"
#include "PMSM.h"
#include "BEEP.h"
#include "math.h"
#include "arm_math.h"

Voltage_TypeDef PMSM_VOLT;
PWM_TypeDef PMSM_PWM;
motor_struct_t PMSM;

//电机结构体初始化
void Motor_Struct_Init(void)
{
    //角度相关
    PMSM.shaft_angle = 0.0f;
    PMSM.electrical_angle = 0.0f;
    PMSM.zero_electric_angle = 0.0f;
    PMSM.delta_angle = 0.0f;
    
    //控制相关
    PMSM.target_velocity = 0.0f;
    PMSM.current_velocity = 0.0f;
    
    //状态标志
    PMSM.state = MOTOR_STATE_DISABLED;
}

//PWM结构体初始化
void PWM_Struct_Init(void)
{
    PMSM_PWM.a = 0;
    PMSM_PWM.b = 0;
    PMSM_PWM.c = 0;
    PMSM_PWM.duty_a = 0.0f;
    PMSM_PWM.duty_b = 0.0f;
    PMSM_PWM.duty_c = 0.0f;
}

//电压结构体初始化
void VOLT_Struct_Init(void)
{
    PMSM_VOLT.Ud = 0.0f;
    PMSM_VOLT.Uq = 0.0f;
    PMSM_VOLT.Ualpha = 0.0f;
    PMSM_VOLT.Ubeta = 0.0f;
    PMSM_VOLT.Ua = 0.0f;
    PMSM_VOLT.Ub = 0.0f;
    PMSM_VOLT.Uc = 0.0f;
}

//  电机初始化
void motor_Init(void)
{
    // 初始化三个结构体
    Motor_Struct_Init();
    PWM_Struct_Init();
    VOLT_Struct_Init();
    
    BEEP_IO_Init();
	TIM1_TB_Init(100, 40);
	BEEP_OC_Init();
	TIM_CtrlPWMOutputs(TIM1, ENABLE);	//使能主输出
	TIM_Cmd(TIM1, ENABLE);				//使能定时器

    motor_power_on_beep(BLDC_Beep_Volume);
}

//电机使能
void Motor_Enable(void)
{
    PMSM.state = MOTOR_STATE_ENABLED;
    
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
}

// 电机关闭
void Motor_Disable(void)
{
    PMSM.state = MOTOR_STATE_DISABLED;
    
    TIM_SetCompare1(TIM1, 0);
    TIM_SetCompare2(TIM1, 0);
    TIM_SetCompare3(TIM1, 0);
}

// 归一化角度到 [0,2PI]
float normalizeAngle(float angle)
{
  float a = fmod(angle, 2*PI);   //取余运算可以用于归一化，列出特殊值例子算便知
  return (a >= 0) ? a : (a + 2*PI);  
}

//电角度求解
float electricalAngle(float shaft_angle)
{
    return shaft_angle * MOTOR_POLE_PAIRS;
}

void setPwm(void)
{
    
    PMSM_PWM.duty_a = Limit(PMSM_VOLT.Ua / VOLTAGE_POWER_SUPPLY, 0.0f, 1.0f);
    PMSM_PWM.duty_b = Limit(PMSM_VOLT.Ub / VOLTAGE_POWER_SUPPLY, 0.0f, 1.0f);
    PMSM_PWM.duty_c = Limit(PMSM_VOLT.Uc / VOLTAGE_POWER_SUPPLY, 0.0f, 1.0f);
    
    
    PMSM_PWM.a = (uint16_t)(PMSM_PWM.duty_a * 999);  // arr=100, 所以最大值99
    PMSM_PWM.b = (uint16_t)(PMSM_PWM.duty_b * 999);
    PMSM_PWM.c = (uint16_t)(PMSM_PWM.duty_c * 999);
    
    //更新PWM占空比
    TIM_SetCompare1(TIM1, PMSM_PWM.a);
    TIM_SetCompare2(TIM1, PMSM_PWM.b);
    TIM_SetCompare3(TIM1, PMSM_PWM.c);
}

//使用DSP库
void setPhaseVoltage(float32_t Uq, float32_t Ud, float32_t angle_el)
{
    float32_t sinVal, cosVal;
    float32_t Ialpha, Ibeta;
    float32_t Ia, Ib;
    
    //保存输入电压值到电压结构体
    PMSM_VOLT.Uq = Uq;
    PMSM_VOLT.Ud = Ud;
    
    //归一化电角度
    angle_el = normalizeAngle(angle_el + PMSM.zero_electric_angle);
    PMSM.electrical_angle = angle_el;
    
    //使用DSP库计算sin和cos (角度转换为度)
    arm_sin_cos_f32(angle_el *180.0f / PI, &sinVal, &cosVal);
    
    //逆Park变换: 从旋转坐标系(d,q)到静止坐标系(alpha,beta)
    arm_inv_park_f32(Ud, Uq, &Ialpha, &Ibeta, sinVal, cosVal);
    
    //保存alpha-beta电压到电压结构体
    PMSM_VOLT.Ualpha = Ialpha;
    PMSM_VOLT.Ubeta = Ibeta;
    
    //标准逆克拉克变换: 从(alpha,beta)到(a,b)
    //对于平衡三相系统，c = -(a + b)
    arm_inv_clarke_f32(Ialpha, Ibeta, &Ia, &Ib);
    
    //计算C相电压
    float32_t Ic = -(Ia + Ib);
    
    //添加电压偏置，实现SVPWM
    PMSM_VOLT.Ua = Ia + VOLTAGE_POWER_SUPPLY / 2;
    PMSM_VOLT.Ub = Ib + VOLTAGE_POWER_SUPPLY / 2;
    PMSM_VOLT.Uc = Ic + VOLTAGE_POWER_SUPPLY / 2;
    
    //设置PWM输出
    setPwm();
}

//开环速度控制函数
void velocityOpenloop(void)
{

    // 计算机械角度增量
    PMSM.delta_angle = PMSM.target_velocity * control_period ;
    PMSM.shaft_angle = normalizeAngle(PMSM.shaft_angle + PMSM.delta_angle);
    
    //计算当前速度
    PMSM.current_velocity = PMSM.delta_angle / control_period ;
    
    //根据电机状态执行相应操作
    if (PMSM.state == MOTOR_STATE_ENABLED) {
        //Uq设置 - 电源电压的1/3，Ud设为0
        float32_t Uq = VOLTAGE_POWER_SUPPLY / 20;
        float32_t Ud = 0.0f;
        
        //执行FOC
        setPhaseVoltage(Uq, Ud, electricalAngle(PMSM.shaft_angle));
    }
    else if (PMSM.state == MOTOR_STATE_DISABLED) {
        //电机关闭时输出0电压
        PMSM_VOLT.Ua = 0;
        PMSM_VOLT.Ub = 0;
        PMSM_VOLT.Uc = 0;
        setPwm();
    }
}

