#include "BLE.h"
#include "BLE_Serial.h"
#include <stdint.h>

/**
 * @note 对外API接口
*/

/**
 * @brief BLE模块初始化
 * @param eventgrouphandler 事件组句柄
*/
void BLE_Init(EventGroupHandle_t eventgrouphandler)
{
    BLE_Serial_Init( eventgrouphandler);
}

/**
 * @brief 解析蓝牙接收到的命令数据
 * @param Com 原始命令数据缓冲区
 * @return BLE_Command 解析后的命令结构体
*/
BLE_Command BLE_AnalyseCommand(uint8_t *Com)
{
    BLE_Command ReCommand;
    if(Com==NULL)
    {
        ReCommand;
        ReCommand.type=Fault;
        return ReCommand;
    }
    uint8_t checknum = (Com[0]+Com[1]+Com[2]+Com[3]+Com[4])&0xFF;            //先不处理这个
    if(Com[0]!=0xAA)
    {
        ReCommand.type=Fault;
        return ReCommand;
    }
    //一共有6个字节，但是第一个字节是帧头不处理，最后一个字节校验
    switch (Com[1]) {
        case 0x01:
            ReCommand.type=Move_BCommand;
            ReCommand.power=(int8_t)Com[2];
            ReCommand.direction=(int8_t)Com[3];
            break;
        case 0x02:      
            ReCommand.type=Scram_BCommand;      //急停都为0
            ReCommand.power=0;
            ReCommand.direction=0;
            break;
        case 0x03:
            ReCommand.type=Calibration_BCommand;
        
            break;
        case 0x04:
            ReCommand.type=PIDSet_BCommand;
            ReCommand.kp=(float)(Com[2]*1.0/10.0);   //步长0.1
            ReCommand.ki=(float)(Com[3]*1.0/100.0);  //步长0.01
            ReCommand.kd=(float)(Com[4]*1.0/10.0);   //步长0.1
            break;
        case 0x05:
            ReCommand.type=PIDGet_BCommand;
            break;
        case 0x06:
            if(Com[2]==0x00)
            {
                if(Com[3]==0x01)
                    ReCommand.type=PID_KP_ADD;
                else
                    ReCommand.type=PID_KP_Sub;
            }
            else if(Com[2]==0x01)
            {
                if(Com[3]==0x01)
                    ReCommand.type=PID_KI_ADD;
                else
                    ReCommand.type=PID_KI_Sub;
            }
            else if(Com[2]==0x02)
            {
                if(Com[3]==0x01)
                    ReCommand.type=PID_KD_ADD;
                else
                    ReCommand.type=PID_KD_Sub;
            }
            break;

        case 0x07:
            if(Com[2]==0x01)
                ReCommand.type=PID_ON;
            else
                ReCommand.type=PID_OFF;
            break;
    
        default:
            ReCommand.type=Fault;
            break;
    }
    return ReCommand;
}