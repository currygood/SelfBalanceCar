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

#define MECHANICAL_MEDIAN               -0.1f       //机械中值
#define ENCODER_GET_PERIOD              40          //编码器获取周期，单位ms
#define MOTOR_DEAD_ZONE_COMPENSATION    0           //电机死区补偿
#define CAR_FALL_ANGLE                  45.0f       //小车摔倒的边界角度
#define UPRIGHT_PID_OUTMAX              PWM_MAX        //直立环的输出最大值
#define SPEED_PID_OUTMAX_FIX             20        //速度环的输出修正的角度最大值为20
#define TURN_PID_OUTMAX                  150        //角度环的输出DifPWM最大值为150
#define OLEDSHOW_FRAME_RATE              50        //OLED显示帧率，单位 50ms * 50 = 2500ms 一次显示刷新


static MPU6050_Data_Struct mpu6050Data;
static uint8_t OLED_Str[128];
static float angleAcc = 0;
static float angleGyro = 0;
static float angleSmooth = 0;
static bool isPidOn=false;
static int16_t AvePWM = 0;
static int16_t DifPWM = 0;
static int16_t LeftPWM = 0;
static int16_t RightPWM = 0;
static float LeftSpeed = 0;
static float RightSpeed = 0;
static float AveSpeed = 0;
static float DifSpeed = 0;
static bool AvoidNeed = false; // 是否需要避障

// 直立环
PID_Str UprightPid = {
    .Kp = -70.0,
    .Ki = -0, 
    .Kd = -5.8,
    .Target = MECHANICAL_MEDIAN,
    .OutMax = UPRIGHT_PID_OUTMAX,   // 给电机留一点余量
    .OutMin = -UPRIGHT_PID_OUTMAX,
};

// 速度环
PID_Str SpeedPid = {
    .Kp = 0.2,
    .Ki = 0.001,
    .Kd = 0,
    .Target = 0,
    .OutMax = SPEED_PID_OUTMAX_FIX,     
    .OutMin = -SPEED_PID_OUTMAX_FIX,
};

PID_Str TurnPID = {
    .Kp = -2.0,
    .Ki = -0.05,
    .Kd = 0,
    .Target = 0,
    .OutMax = TURN_PID_OUTMAX,
    .OutMin = -TURN_PID_OUTMAX,
};

/**
 * @brief 控制任务（平衡车主控）
 * @param params 任务参数
*/
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

/**
 * @brief 系统任务（命令处理、显示等）
 * @param params 任务参数
*/
void System_Task(void *params)
{
    EventGroupHandle_t MTask_SystemEventGroup = Shared_Get_EventGroupHandler();
    static uint8_t bytes[64];
    static BLE_Command command;
    static EventBits_t bits=0;
    static uint8_t OLED_Show_FrameRateCount = 0;

    while(1)
    {
        OLED_Show_FrameRateCount++;
        
        if(OLED_Show_FrameRateCount>=OLEDSHOW_FRAME_RATE)
        {
            OLED_Show_FrameRateCount = 0;
            memset(OLED_Str,0,sizeof(OLED_Str));

            int AveSpeedInt = (int)AveSpeed;
            int AveSpeedFloat = (int)(AveSpeed*100) - AveSpeedInt*100;
            snprintf(OLED_Str,sizeof(OLED_Str),"Speed: %2d.%2d",AveSpeedInt,AveSpeedFloat);
            OLED_ShowString(0,0,OLED_Str,OLED_8X16);

            OLED_Update();
        }

        // 获取前方障碍物距离
        Ult_TrigGetDistance(MTask_SystemEventGroup);
        bits = xEventGroupWaitBits(MTask_SystemEventGroup,EVENT_GPIO_ECHO,pdTRUE,pdFALSE,pdMS_TO_TICKS(0));
        if(bits&EVENT_GPIO_ECHO)
        {
            uint16_t distance_cm = Ult_GetDistance();
            uint16_t filtered_distance_cm = Ult_GetFilteredDistance(distance_cm);
            AvoidNeed = isNeedAvoid(filtered_distance_cm);  // 给主控任务一个标志，告诉它是否需要避障
        }
        else
        {
            AvoidNeed = false; // 没有收到回波信号，认为没有障碍物
        }

        bits = xEventGroupWaitBits(MTask_SystemEventGroup,EVENT_UART_RX_FINISH,pdTRUE,pdFALSE,pdMS_TO_TICKS(0));
        if(bits&EVENT_UART_RX_FINISH)
        {
            // 解析命令
            BLE_Serial_ReadByte(bytes);
            command = BLE_AnalyseCommand(bytes);

            switch(command.type)
            {
                case Move_BCommand:
                    if(AvoidNeed)   // 前方由障碍物，只能后退
                    {
                        if(command.power>0)
                        {
                            SpeedPid.Target = 0; // 避障时速度环目标值为 0
                        }
                        TurnPID.Target = 0; // 避障时转向环目标值为 0
                        break;
                    }
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

                default:

                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}