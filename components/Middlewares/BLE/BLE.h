/*
BLE蓝牙获得命令->通知模块
*/
/*
    命令格式如根目录下面
*/

#include "main.h"
#include "BLE_Serial.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

typedef enum
{
    Fault=0,             // 无效命令/校验对应不上
    Move_BCommand,       // 动力+方向控制
    Scram_BCommand,      // 急停命令（忽略动力/方向字段）
    Calibration_BCommand,// 校准/复位命令
    PIDSet_BCommand,     // 调节pid参数命令
    PIDGet_BCommand,     // 获取pid参数命令
    PID_KP_ADD,
    PID_KP_Sub,
    PID_KI_ADD,
    PID_KI_Sub,
    PID_KD_ADD,
    PID_KD_Sub,
    PID_ON,
    PID_OFF,
    MECHANICAL_MEDIAN_ADD,
    MECHANICAL_MEDIAN_Sub,
}BCommand_Type;

typedef struct BLE_Command_Str
{
    BCommand_Type type;
    // 数值：
    int8_t power;
    int8_t direction;
    float kp;
    float ki;
    float kd;
}BLE_Command;

void BLE_Init(EventGroupHandle_t eventgrouphandler);

BLE_Command BLE_AnalyseCommand(uint8_t *Com);