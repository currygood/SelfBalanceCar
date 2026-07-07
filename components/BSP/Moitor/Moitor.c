#include "Moitor.h"
#include "stm32f103xb.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_tim.h"
#include "tim.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


/*

规定最大速度7.3

四倍频，编码器线数为13   13*4=52个脉冲一圈   一个脉冲，stm32编码器计数+1
则__HAL_TIM_GET_COUNTER获得104  电机转了两圈

1:48电机    电机一圈脉冲数：减速比*编码器线数*程序倍频数字   也就是13*48*4

空载转速350rpm        1:48减速比，所以实际是7.3rpm
*/
#define RPM_MAX 300.0f  // 考虑到电池电压 5V 达不到 6V 的 350rpm，设为 300 比较稳妥
// 速度计算参数：电机每转脉冲数 = 减速比48 * 编码器线数13 * 四倍频4 = 2496
#define PULSES_PER_REV     2496.0f
// 速度更新周期（秒），需与获取编码器周期一致（40ms）
#define UPDATE_PERIOD_SEC  0.040f
#define PWM_ABSOLUTE_MAX   1000
// 滤波系数：0.0 到 1.0 之间。数值越大越平滑，但延迟越高。
#define SPEED_FILTER_ALPHA 0.5f         //目前测试只用了原始速度，没有滤波，后续可以调节这个值来实现滤波



// 静态变量：上次编码器值、当前速度（rpm）
static int16_t last_encoder[MOITOR_NUM];
static float current_speed_rpm[MOITOR_NUM];  // 原始速度
static float filtered_speed_rpm[MOITOR_NUM]; // 滤波后的速度


/**
 * @note 内部函数
*/

static void Moitor_Set_Compare(Moitor_SerialNum moitorno,uint16_t compare)
{
    if(compare>PWM_MAX)
        compare=PWM_MAX;
    if(moitorno==Moitor_Left)
        __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_1,compare);
    else if(moitorno==Moitor_Right)
        __HAL_TIM_SET_COMPARE(&htim1,TIM_CHANNEL_4,compare);
}

static uint8_t Moitor_ComputeDuty(float speed)
{
    return (speed/RPM_MAX)*PWM_MAX;
}

// __HAL_TIM_GET_COUNTER获得的是uint16_t   转换成int16_t 补码运算，65535也就是-1  能直接计算速度方向了
static int16_t Encoder_Get(Moitor_SerialNum moitorno)
{
	int16_t temp;
    if(moitorno==Moitor_Left)
	    temp = __HAL_TIM_GET_COUNTER(&htim3);
	else if(moitorno==Moitor_Right)
        temp = __HAL_TIM_GET_COUNTER(&htim4);
    return temp;
}



/**
 * @brief 获取指定电机的当前转速（rpm）
 * @param moitorno 电机序号：Moitor_Left 或 Moitor_Right
 * @return 转速值，正为正向，负为反向
 */
static float Moitor_Get_Speed_In(Moitor_SerialNum moitorno)
{
    if (moitorno >= MOITOR_NUM)
        return 0.0f;
    return filtered_speed_rpm[moitorno];
}


/**
 * @note 对外api接口
*/


void Moitor_GetRawSpeed(MoitorSpeed_Str *MSpeed)
{
    for(uint8_t i=0; i<MOITOR_NUM; ++i)
    {
        // current_speed_rpm 是你在 Update_Speed 里计算的 raw_speed
        MSpeed->speed_rpm[i] = current_speed_rpm[i]; 
    }
}

/**
 * @brief 速度更新函数，在固定周期的任务函数中调用
 * @note  计算左右电机当前转速（rpm），正负表示方向
 */
void Moitor_Update_Speed(void)
{
    for (int i = 0; i < MOITOR_NUM; i++)
    {
        Moitor_SerialNum motor = (Moitor_SerialNum)i;
        int16_t now = Encoder_Get(motor);
        int16_t delta = now - last_encoder[i];

        // --- 核心修改点：极性对齐 ---
        // 你测试发现左轮往前推是负数，所以这里给左轮 delta 取反
        // 这样往前推时，左右轮 delta 就都是正数了
        if(motor == Moitor_Left) {
            delta = -delta;
        }

        // 计算瞬时转速 (RPM)
        float raw_speed = (float)delta * 60.0f / (PULSES_PER_REV * UPDATE_PERIOD_SEC);
        
        // 低通滤波
        filtered_speed_rpm[i] = (filtered_speed_rpm[i] * SPEED_FILTER_ALPHA) + 
                                (raw_speed * (1.0f - SPEED_FILTER_ALPHA));

        last_encoder[i] = now;
        current_speed_rpm[i] = raw_speed; 
    }
}

/**
 * @brief 电机初始化
*/
void Moitor_Init()
{
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_4);
    HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);
    HAL_TIM_Encoder_Start(&htim4,TIM_CHANNEL_ALL);
    // 初始化上次编码器值和当前速度
    for (int i = 0; i < MOITOR_NUM; i++)
    {
        last_encoder[i] = Encoder_Get((Moitor_SerialNum)i);
        current_speed_rpm[i] = 0.0f;
    }
}

/**
 * @brief 直接设置PWM值控制电机
 * @param moitorNum 电机序号：Moitor_Left 或 Moitor_Right
 * @param pwm PWM值，范围：-1000~1000
 * @return bool 设置是否成功
*/
bool Moitor_Set_PWMDirect(Moitor_SerialNum moitorNum,int16_t pwm)
{
    if(pwm<-PWM_MAX || pwm>PWM_MAX)
        return false;
    if(moitorNum==Moitor_Left)
    {
        if(pwm>0)
        {
            HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin, GPIO_PIN_RESET);
        }
        else 
        {
            HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin, GPIO_PIN_SET);
        }
    }
    else if(moitorNum==Moitor_Right)
    {
        if(pwm>0)
        {
            HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin, GPIO_PIN_SET);
        }
        else 
        {
            HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin, GPIO_PIN_RESET);
        }
    }
    
    Moitor_Set_Compare(moitorNum,abs(pwm));
}

/**
 * @brief 设置左电机速度
 * @param speed 目标速度（rpm），正为正向，负为反向
 * @return bool 设置是否成功
*/
bool Moitor_Set_Left_Speed(float speed)
{
    if(speed<-RPM_MAX||speed>RPM_MAX)
        return false;
    uint8_t duty = Moitor_ComputeDuty(abs(speed));
    if(speed>0)
    {
        HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin, GPIO_PIN_RESET);
    }
    else 
    {
        HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin, GPIO_PIN_SET);
    }
    Moitor_Set_Compare(Moitor_Left,duty);
    return true;
}

/**
 * @brief 设置右电机速度
 * @param speed 目标速度（rpm），正为正向，负为反向
 * @return bool 设置是否成功
*/
bool Moitor_Set_Right_Speed(float speed)
{
    if(speed<-RPM_MAX||speed>RPM_MAX)
        return false;
    uint8_t duty = Moitor_ComputeDuty(abs(speed));
    if(speed>0)
    {
        HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin, GPIO_PIN_RESET);
    }
    else 
    {
        HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin, GPIO_PIN_SET);
    }
    Moitor_Set_Compare(Moitor_Right,duty);
    return true;
}

/**
 * @brief 获取滤波后的电机速度
 * @param MSpeed 速度结构体指针，用于存储滤波后的速度值
*/
void Moitor_GetSpeed(MoitorSpeed_Str *MSpeed)
{
    for(uint8_t i=0;i<MOITOR_NUM;++i)
    {
        MSpeed->speed_rpm[i] = Moitor_Get_Speed_In((Moitor_SerialNum)i);
    }
}