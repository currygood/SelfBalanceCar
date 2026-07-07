/*
    Moitor电机控制模块
*/

#include "main.h"
#include <stdbool.h>

/*
    可供外部使用的宏定义
*/
#define CONTROL_TASK_DELAY  5
#define MOITOR_NUM          2
#define PWM_MAX             1000

/*
    可供外部使用的结构体等抽象数据类型
*/
typedef enum
{
    Moitor_Left=0,
    Moitor_Right,
}Moitor_SerialNum;

typedef struct MoitorSpeed_str
{
    float speed_rpm[MOITOR_NUM];
}MoitorSpeed_Str;

/*
    可供外部使用的extern变量
*/

/*
    对外暴露的API接口
*/
// 电机驱动初始化
void Moitor_Init();

// 获取原始（未滤波）速度
void Moitor_GetRawSpeed(MoitorSpeed_Str *MSpeed);
// 速度更新函数，在固定周期的任务函数中调用
void Moitor_Update_Speed(void);

// 直接设置PWM值控制电机
bool Moitor_Set_PWMDirect(Moitor_SerialNum moitorNum,int16_t pwm);

// 设置左电机速度（rpm）
bool Moitor_Set_Left_Speed(float speed);

// 设置右电机速度（rpm）
bool Moitor_Set_Right_Speed(float speed);

// 获取滤波后的速度
void Moitor_GetSpeed(MoitorSpeed_Str *MSpeed);