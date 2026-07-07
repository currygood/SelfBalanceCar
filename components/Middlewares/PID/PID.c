#include "PID.h"

#define KI_MAX      2000

/**
 * @note 对外API接口
*/

/**
 * @brief PID结构体初始化
 * @param pid PID结构体指针
*/
void PID_Init(PID_Str *pid)
{
    pid->Actual=0;
    pid->Error0=0;
    pid->Error1=0;
    pid->ErrorInt=0;
    pid->Out = 0;
}

/**
 * @note 对外API接口
*/

/**
 * @brief PID更新函数
 * @param pid PID结构体指针
 * @param gyro_rate 传入陀螺仪处理后的角速度（度/秒）
*/
void PID_Update(PID_Str *pid, float gyro_rate)
{
    /* 1. 获取本次误差 */
    pid->Error1 = pid->Error0;                  // 记录上次误差
    pid->Error0 = pid->Target - pid->Actual;    // 本次角度误差

    /* 2. 积分项处理 */
    if (pid->Ki != 0)
    {
        pid->ErrorInt += pid->Error0;
        // 增加积分限幅（抗饱和），防止车倒下时积分无限累加
        if (pid->ErrorInt > KI_MAX) pid->ErrorInt = KI_MAX;
        if (pid->ErrorInt < -KI_MAX) pid->ErrorInt = -KI_MAX;
    }
    else
    {
        pid->ErrorInt = 0;
    }

    /* 3. PID计算 */
    /* 比例项：Kp * 角度误差 */
    /* 积分项：Ki * 误差累加 */
    /* 微分项：直接使用陀螺仪角速度！不再使用 (E0-E1) */
    /* 这里的 gyro_rate 是度/秒，它能极大地压制高频震荡 */
    
    pid->Out = (pid->Kp * pid->Error0) + (pid->Ki * pid->ErrorInt) + (pid->Kd * gyro_rate);

    /* 4. 输出限幅 */
    if (pid->Out > pid->OutMax) pid->Out = pid->OutMax;
    if (pid->Out < pid->OutMin) pid->Out = pid->OutMin;
}