#include "MTask.h"
#include "BLE_Serial.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "event_groups.h"
#include "Moitor.h"
#include "MPU6050.h"
#include "OLED.h"
#include "BLE.h"
#include "Shared.h"
#include "PID.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define MECHANICAL_MEDIAN               0.0f       //机械中值
#define ENCODER_GET_PERIOD              40          //编码器获取周期，单位ms
#define MOTOR_DEAD_ZONE_COMPENSATION    0           //电机死区补偿
#define CAR_FALL_ANGLE                  45.0f       //小车摔倒的边界角度

static float mechinalMedian = MECHANICAL_MEDIAN; // 机械中值，初始值为 0.06，可以通过蓝牙命令调节


// 目前调试: 速度环只留下kp,然后推小车,看多久能速度降为0,如果快,那kp大,如果慢,那kp小,然后再调整再加ki

static MPU6050_Data_Struct mpu6050Data;
static uint8_t PulseStr[100];
static float angleAcc = 0;
static float angleGyro = 0;
static float angleSmooth = 0;
static bool isPidOn=true;
static int16_t AvePWM = 0;
static int16_t DifPWM = 0;
static int16_t LeftPWM = 0;
static int16_t RightPWM = 0;
static float LeftSpeed = 0;
static float RightSpeed = 0;
static float AveSpeed = 0;
static float DifSpeed = 0;
static bool AvoidNeed = false; // 是否需要避障

static float RawAveSpeed = 0; // 没滤波的原始平均速度（新增）

// 直立环
PID_Str UprightPid = {
    .Kp = -70.0,
    .Ki = -0, 
    .Kd = -5.8,
    .Target = MECHANICAL_MEDIAN,
    .OutMax = 1000,   // 给电机留一点余量
    .OutMin = -1000,
};

// 速度环
PID_Str SpeedPid = {
    .Kp = 0.2,
    .Ki = 0.001,
    .Kd = 0,
    .Target = 0,
    .OutMax = 20,     
    .OutMin = -20,
};

PID_Str TurnPID = {
    .Kp = -2.0,
    .Ki = -0.05,
    .Kd = 0,
    .Target = 0,
    .OutMax = 150,
    .OutMin = -150,
};


void Control_Task(void *params)
{
    EventGroupHandle_t MTask_SystemEventGroup = Shared_Get_EventGroupHandler();
    EventBits_t bits=0;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_TASK_DELAY);
    static uint8_t encoderGetCount=0;


    MPU6050_Request_Data();
    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, xFrequency); // 5ms 周期

        if(AvoidNeed)
        {
            // 避障逻辑
            // 这里可以添加避障的控制逻辑，比如调整转向环的目标值等
            // 目前直接停下
            UprightPid.Target = MECHANICAL_MEDIAN; // 直立环目标值保持不变
            SpeedPid.Target = 0; // 速度环目标值保持不变
            TurnPID.Target = 0; // 转向环目标值保持不变
            // 能立住就行
        }

        bits = xEventGroupWaitBits(MTask_SystemEventGroup, EVENT_I2C_RX_FINISH, pdTRUE, pdFALSE, 0);
        if(bits & EVENT_I2C_RX_FINISH)
        {
            /* --- A. 直立环数据处理 --- */
            MPU6050_Parse_Data(&mpu6050Data);
            mpu6050Data.GyroX -= -21; // 零漂补偿
            mpu6050Data.GyroZ -= 21; // 零漂补偿

            angleAcc = COMPUTE_ANGLE_ACC((float)mpu6050Data.AccY, (float)mpu6050Data.AccZ);
            angleGyro = COMPUTE_ANGLE_GYRO(-(float)mpu6050Data.GyroX, (float)angleSmooth);         
            angleSmooth = COMPLEMENTARY_FILTER(angleAcc, angleGyro);
            
            float current_gyro_rate = (float)mpu6050Data.GyroX / SENSITIVITY; 

            // 摔倒保护
            if(angleSmooth > CAR_FALL_ANGLE || angleSmooth < -CAR_FALL_ANGLE) isPidOn = false;

            /* --- B. 速度环计算+直立环 (每 40ms 执行一次) --- */
            encoderGetCount++;
            if(encoderGetCount >= (ENCODER_GET_PERIOD / CONTROL_TASK_DELAY))
            {
                encoderGetCount = 0;
                MoitorSpeed_Str filteredSpeedStruct;
                MoitorSpeed_Str rawSpeedStruct;

                MoitorSpeed_Str moitroSpeed;
                Moitor_Update_Speed();
                Moitor_GetSpeed(&moitroSpeed); // 获取编码器速度 (RPM)
                
                LeftSpeed = moitroSpeed.speed_rpm[Moitor_Left];
                RightSpeed = moitroSpeed.speed_rpm[Moitor_Right];
                AveSpeed = (LeftSpeed + RightSpeed) / 2.0f;
                DifSpeed = LeftSpeed - RightSpeed;

                Moitor_GetSpeed(&filteredSpeedStruct);    // 获取滤波后的
                Moitor_GetRawSpeed(&rawSpeedStruct);      // 获取原始的
                // 计算滤波后的平均速度
                AveSpeed = (filteredSpeedStruct.speed_rpm[Moitor_Left] + filteredSpeedStruct.speed_rpm[Moitor_Right]) / 2.0f;
                
                // 计算原始的平均速度
                RawAveSpeed = (rawSpeedStruct.speed_rpm[Moitor_Left] + rawSpeedStruct.speed_rpm[Moitor_Right]) / 2.0f;

                if(isPidOn)
                {
                    // 速度环计算
                    SpeedPid.Actual = AveSpeed;
                    if(SpeedPid.ErrorInt > 500) SpeedPid.ErrorInt = 500; 
                    if(SpeedPid.ErrorInt < -500) SpeedPid.ErrorInt = -500;
                    // 注意：这里的 PID_Update 第二个参数传 0，因为速度环不需要陀螺仪微分
                    PID_Update(&SpeedPid,0); 
                    
                    /* 速度环输出作为直立环的目标值叠加 */
                    // SpeedPid.Out 决定了小车为了维持静止需要倾斜的角度
                    UprightPid.Target = MECHANICAL_MEDIAN + SpeedPid.Out;

                    // 注意：GyroZ 也要减去零漂补偿，类似 GyroX
                    float current_yaw_rate = (float)mpu6050Data.GyroZ / SENSITIVITY; 

                    // 2. 将转向环的输入改为角速度
                    TurnPID.Actual = current_yaw_rate; 

                    PID_Update(&TurnPID, 0);
                    DifPWM = TurnPID.Out;
                }
                else 
                {
                    SpeedPid.ErrorInt = 0; // 关闭时清空积分
                    UprightPid.Target = MECHANICAL_MEDIAN;
                    DifPWM = 0;
                }
            }

            /* --- C. 直立环执行 (5ms) --- */
            if(isPidOn)
            {
                UprightPid.Actual = angleSmooth;
                PID_Update(&UprightPid, current_gyro_rate);
                
                AvePWM = UprightPid.Out;
                // 将速度环的输出叠加到直立环的输出上
                LeftPWM = AvePWM + DifPWM; 
                RightPWM = AvePWM - DifPWM;

                // 电机死区补偿和限幅
                if(LeftPWM > 0)  LeftPWM += MOTOR_DEAD_ZONE_COMPENSATION;
                else if(LeftPWM < 0) LeftPWM -= MOTOR_DEAD_ZONE_COMPENSATION;
                
                if(RightPWM > 0)  RightPWM += MOTOR_DEAD_ZONE_COMPENSATION;
                else if(RightPWM < 0) RightPWM -= MOTOR_DEAD_ZONE_COMPENSATION;

                // 最终输出到电机
                Moitor_Set_PWMDirect(Moitor_Left, LeftPWM);
                Moitor_Set_PWMDirect(Moitor_Right, RightPWM);
            }
            else 
            {
                Moitor_Set_PWMDirect(Moitor_Left, 0);
                Moitor_Set_PWMDirect(Moitor_Right, 0);
            }

        }
        MPU6050_Request_Data();
    }
}

void System_Task(void *params)
{
    EventGroupHandle_t MTask_SystemEventGroup = Shared_Get_EventGroupHandler();
    static uint8_t bytes[64];
    static BLE_Command command;
    static EventBits_t bits=0;
    // static uint8_t count=0;
    // static uint8_t countsout = 0;
    // static uint8_t plotCount = 0; // 用于控制波形打印频率
    // static int32_t xSum = 0;
    // static uint16_t counta = 0;

    while(1)
    {
        // memset(PulseStr,0,sizeof PulseStr);
        // // snprintf(PulseStr, sizeof PulseStr, "[plot,%f,%f]",angleAcc,angleGyro);   //这里输出的是[plot,,]   没有任何数值   因为stm32采用的c库是轻量级的，printf这里处理浮点数直接跳过
        
        // 打印波形
        // 处理 angleAcc
        // int intPart1 = (int)angleAcc;                                    // 整数部分
        // int fracPart1 = (int)(fabs(angleAcc - intPart1) * 1000);        // 小数部分*1000

        // // 处理 angleGyro
        // int intPart2 = (int)angleGyro;
        // int fracPart2 = (int)(fabs(angleGyro - intPart2) * 1000);

        // //用 %d 和 %03d 拼起来（%03d 保证小数位不足补0，如 1.2 -> 1.200）
        // // snprintf(PulseStr, sizeof(PulseStr), "[plot,%d.%03d,%d.%03d]", 
        // //         intPart1, fracPart1, intPart2, fracPart2);

        // // 处理 angleSmooth  平滑滤波后得到的
        // int intPart3 = (int)angleSmooth;
        // int fracPart3 = (int)(fabs(angleSmooth - intPart3) * 1000);

        // // 用 %d 和 %03d 拼起来（%03d 保证小数位不足补0，如 1.2 -> 1.200）
        // snprintf(PulseStr, sizeof(PulseStr), "[plot,%d.%03d,%d.%03d,%d.%03d]", 
        //         intPart1, fracPart1, intPart2, fracPart2,intPart3,fracPart3);           //红，绿，蓝
        // BLE_Serial_SendBytes(PulseStr,strlen(PulseStr),0);

        // snprintf(PulseStr, sizeof(PulseStr), "[plot,%d.%03d]", 
        //         intPart3,fracPart3);

        // 显示速度环的波形
        // 处理SpeedPid.Target
        // int intPart1 = (int)SpeedPid.Target;                                    // 整数部分
        // int fracPart1 = (int)(fabs(SpeedPid.Target - intPart1) * 1000);        // 小数部分*1000

        // // 处理 AveSpeed
        // int intPart2 = (int)AveSpeed;
        // int fracPart2 = (int)(fabs(AveSpeed - intPart2) * 1000);

        // snprintf(PulseStr, sizeof(PulseStr), "[plot,%d.%03d,%d.%03d]", 
        //         intPart1, fracPart1, intPart2, fracPart2);           //红，绿
        // BLE_Serial_SendBytes(PulseStr,strlen(PulseStr),0);

        // 显示速度环Out
        // countsout++;
        // if(countsout>50)
        // {
        //     countsout=0;
        //     int intPart1 = (int)(MECHANICAL_MEDIAN + SpeedPid.Out);                                    // 整数部分
        //     int fracPart1 = (int)(fabs(SpeedPid.Out - intPart1) * 1000);        // 小数部分*1000
        //     snprintf(PulseStr, sizeof(PulseStr), "speedpid out: %d.%03d\r\n", 
        //             intPart1, fracPart1);           //红，绿
        //     BLE_Serial_SendBytes(PulseStr,strlen(PulseStr),0);
        // }

        // 打印速度滤波波形
//         plotCount++;
//         if(plotCount >= 50) // 每50个周期打印一次
//         {
//             /**
//                 如何根据波形判断 ALPHA？
// 架空小车（轮子悬空），用手转动轮子，观察波形图：
// 如果波形如下（ALPHA 太小，如 0.1）：
// 红线（原始）：毛刺非常多，跳动剧烈。
// 绿线（滤波）：几乎重合在红线上，毛刺依然很多。
// 结论：滤波太弱，噪声会传给 PID 导致电机震动。
// 如果波形如下（ALPHA 太大，如 0.95）：
// 红线（原始）：有毛刺。
// 绿线（滤波）：非常平滑，是一条漂亮的曲线。
// 但是延迟极大：当你用手突然刹停轮子时，红线瞬间掉到 0，而绿线像滑梯一样，过了半秒钟才慢慢降到 0。
// 结论：延迟太大，这会导致你的平衡车出现“荡秋千”现象（相位滞后导致的发散性震荡）。
// 理想状态（ALPHA 约 0.7~0.8）：
// 红线（原始）：有毛刺。
// 绿线（滤波）：大部分高频毛刺被削平了，看起来比红线顺滑很多。
// 延迟表现：当你快速拨动轮子时，绿线能紧紧跟上红线的趋势，延迟感很微弱。
// 你的目标：
// 找到一个 ALPHA 值，使得绿线足够平滑（不传递高频抖动），同时又反应够快（延迟不超过 100-200ms）。
//             */
//             // 打印对比波形：原始速度 vs 滤波速度
//             // 处理 RawAveSpeed (红色)
//             int rawInt = (int)RawAveSpeed;
//             int rawFrac = (int)(fabs(RawAveSpeed - rawInt) * 1000);

//             // 处理 AveSpeed (绿色)
//             int fltInt = (int)AveSpeed;
//             int fltFrac = (int)(fabs(AveSpeed - fltInt) * 1000);

//             // 发送波形数据
//             // [plot, 原始速度, 滤波后速度]
//             snprintf(PulseStr, sizeof(PulseStr), "[plot,%d.%03d,%d.%03d]\r\n", 
//                     rawInt, rawFrac, fltInt, fltFrac);
//             BLE_Serial_SendBytes(PulseStr, strlen(PulseStr), 0);
//         }

        // 打印GyroX
        // xSum+=mpu6050Data.GyroX;
        // counta++;
        // snprintf(PulseStr, sizeof(PulseStr), "%d\r\n", 
        //         xSum/counta);
        // BLE_Serial_SendBytes(PulseStr, strlen(PulseStr), 7);


        // 打印GyroZ
        // xSum+=mpu6050Data.GyroZ;
        // counta++;
        // snprintf(PulseStr, sizeof(PulseStr), "%d\r\n", 
        //         xSum/counta);
        // BLE_Serial_SendBytes(PulseStr, strlen(PulseStr), 7);


        // 获取前方障碍物距离
        Ult_TrigGetDistance(MTask_SystemEventGroup);


        bits = xEventGroupWaitBits(MTask_SystemEventGroup,EVENT_UART_RX_FINISH,pdTRUE,pdFALSE,pdMS_TO_TICKS(0));
        if(bits&EVENT_UART_RX_FINISH)
        {
            // 解析命令
            BLE_Serial_ReadByte(bytes);
            command = BLE_AnalyseCommand(bytes);

            switch(command.type)
            {
                case Move_BCommand:
                    SpeedPid.Target = command.power; // 速度环目标值为蓝牙命令的 power
                    TurnPID.Target = -command.direction; // 直立环目标值为蓝牙命令的 direction

                    break;
                case Scram_BCommand:
                    SpeedPid.Target = 0; // 急停时速度环目标值为 0
                    TurnPID.Target = 0; // 急停时转向环目标值为 0

                    break;
                case Calibration_BCommand:
                    isPidOn=true;
                    UprightPid.ErrorInt=0;      // 校准时把误差积分归0

                    break;
                case PIDSet_BCommand:
                    UprightPid.Kp=command.kp;
                    UprightPid.Ki=command.ki;
                    UprightPid.Kd=command.kd;
                    PID_Init(&UprightPid);   // 调节pid参数时把pid状态重置一下，避免之前的误差积分等状态影响
                    break;

                case PID_KP_ADD:
                    UprightPid.Kp+=1;
                    BLE_Serial_SendBytes("kp+\r\n", sizeof("kp+\r\n"),0);
                    break;
                case PID_KP_Sub:
                    UprightPid.Kp-=1;
                    BLE_Serial_SendBytes("kp-\r\n", sizeof("kp-\r\n"),0);
                    break;
                case PID_KI_ADD:
                    UprightPid.Ki+=0.1;
                    BLE_Serial_SendBytes("ki+\r\n", sizeof("ki+\r\n"),0);
                    break;
                case PID_KI_Sub:
                    UprightPid.Ki-=0.1;
                    BLE_Serial_SendBytes("ki-\r\n", sizeof("ki-\r\n"),0);
                    break;
                case PID_KD_ADD:
                    UprightPid.Kd+=1;
                    BLE_Serial_SendBytes("kd+\r\n", sizeof("kd+\r\n"),0);
                    break;
                case PID_KD_Sub:
                    UprightPid.Kd-=1;
                    BLE_Serial_SendBytes("kd-\r\n", sizeof("kd-\r\n"),0);
                    break;

                case PID_ON:
                    isPidOn=true;
                    PID_Init(&UprightPid);   // 开启pid时把pid状态重置一下，避免之前的误差积分等状态影响
                    PID_Init(&SpeedPid);
                    PID_Init(&TurnPID);
                    DifPWM=0;
                    BLE_Serial_SendBytes("pid on\r\n", sizeof("pid on\r\n"),0);

                    break;

                case PID_OFF:
                    isPidOn=false;
                    BLE_Serial_SendBytes("pid off\r\n", sizeof("pid off\r\n"),0);

                    break;

                case MECHANICAL_MEDIAN_ADD:
                    mechinalMedian+=0.1;
                    UprightPid.Target = mechinalMedian; // 更新直立环的目标值
                    BLE_Serial_SendBytes("mechanical median +\r\n", sizeof("mechanical median +\r\n"),0);
                    break;

                case MECHANICAL_MEDIAN_Sub:
                    mechinalMedian-=0.1;
                    UprightPid.Target = mechinalMedian; // 更新直立环的目标值
                    BLE_Serial_SendBytes("mechanical median -\r\n", sizeof("mechanical median -\r\n"),0);
                    break;


                default:

                    break;
            }
        }
        else if(bits&EVENT_GPIO_ECHO)
        {
            // 获取前方障碍物距离
            Ult_TrigGetDistance(MTask_SystemEventGroup);
            uint16_t distance_cm = Ult_GetDistance();
            AvoidNeed = isNeedAvoid(distance_cm);  // 给主控任务一个标志，告诉它是否需要避障
        }


        // memset(PulseStr,0,sizeof PulseStr);
        // snprintf(PulseStr,sizeof PulseStr,"gyroX:%d\r\n",mpu6050Data.GyroX);
        // BLE_Serial_SendBytes(PulseStr, strlen(PulseStr), 0);
        
        // count++;
        // if(count>105)
        // {
        //     count=0;

        //     // //OLED显示mpu6050
        //     // OLED_ShowSignedNum(0, 0, mpu6050Data.AccX, 5,OLED_8X16);
        //     // OLED_ShowSignedNum(0, 16, mpu6050Data.AccY, 5,OLED_8X16);
        //     // OLED_ShowSignedNum(0, 32, mpu6050Data.AccZ, 5,OLED_8X16);
        //     // OLED_ShowSignedNum(55, 0, mpu6050Data.GyroX, 5,OLED_8X16);
        //     // OLED_ShowSignedNum(55, 16,mpu6050Data.GyroY, 5,OLED_8X16);
        //     // OLED_ShowSignedNum(55, 32, mpu6050Data.GyroZ, 5,OLED_8X16);
        //     // OLED_Update();
            
        //     MoitorSpeed_Str moitroSpeed;
        //     Moitor_GetSpeed(&moitroSpeed);
        //     LeftSpeed = moitroSpeed.speed_rpm[Moitor_Left];
        //     RightSpeed = moitroSpeed.speed_rpm[Moitor_Right];
        //     memset(PulseStr,0,sizeof PulseStr);
        //     snprintf(PulseStr, sizeof(PulseStr), "UKp:%d.%03d Uki:%d.%03d Ukd:%d.%03d ls:%d.%03d rs:%d.%03d\r\nSKp:%d.%03d Ski:%d.%03d Skd:%d.%03d\r\n", 
        //             (int)UprightPid.Kp, (int)(fabs(UprightPid.Kp - (int)UprightPid.Kp) * 1000),
        //             (int)UprightPid.Ki, (int)(fabs(UprightPid.Ki - (int)UprightPid.Ki) * 1000),
        //             (int)UprightPid.Kd, (int)(fabs(UprightPid.Kd - (int)UprightPid.Kd) * 1000),
        //             (int)moitroSpeed.speed_rpm[Moitor_Left],(int)(fabs(moitroSpeed.speed_rpm[Moitor_Left] - (int)moitroSpeed.speed_rpm[Moitor_Left]) * 1000),
        //             (int)moitroSpeed.speed_rpm[Moitor_Right],(int)(fabs(moitroSpeed.speed_rpm[Moitor_Right] - (int)moitroSpeed.speed_rpm[Moitor_Right]) * 1000),
        //             (int)SpeedPid.Kp, (int)(fabs(SpeedPid.Kp - (int)SpeedPid.Kp) * 1000),
        //             (int)SpeedPid.Ki, (int)(fabs(SpeedPid.Ki - (int)SpeedPid.Ki) * 1000),
        //             (int)SpeedPid.Kd, (int)(fabs(SpeedPid.Kd - (int)SpeedPid.Kd) * 1000)
        //     );
        //     BLE_Serial_SendBytes(PulseStr,strlen(PulseStr),0);
        // }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}