/*
PID算法实现
*/

#include "main.h"



/**
 * @note 宏定义
*/


/**
 * @note 结构体
*/
typedef struct PID_Struct
{
    float Target;		//目标值，由用户设定
	float Actual;		//实际值，从传感器读取
	float Out;			//输出值，作用于执行器
	
	float Kp;			//比例项权重
	float Ki;			//积分项权重
	float Kd;			//微分项权重
	
	float Error0;		//本次误差
	float Error1;		//上次误差
	float ErrorInt;		//误差积分
	
	float OutMax;		//输出限幅的最大值
	float OutMin;		//输出限幅的最小值
}PID_Str;



void PID_Init(PID_Str *pid);

void PID_Update(PID_Str *pid, float gyro_rate);