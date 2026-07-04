/*
    TIM编码器和PWM驱动底层和电机控制
*/

#include "main.h"
#include <stdbool.h>


#define CONTROL_TASK_DELAY  5
#define MOITOR_NUM          2
#define PWM_MAX             1000

typedef enum
{
    Moitor_Left=0,
    Moitor_Right,
}Moitor_SerialNum;

typedef struct MoitorSpeed_str
{
    float speed_rpm[MOITOR_NUM];
}MoitorSpeed_Str;

void Moitor_Init();

void Moitor_GetRawSpeed(MoitorSpeed_Str *MSpeed);
void Moitor_Update_Speed(void);

bool Moitor_Set_PWMDirect(Moitor_SerialNum moitorNum,int16_t pwm);

bool Moitor_Set_Left_Speed(float speed);

bool Moitor_Set_Right_Speed(float speed);

void Moitor_GetSpeed(MoitorSpeed_Str *MSpeed);