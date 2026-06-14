#include "MPU6050.h"
#include "I2C_Bus.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void MPU6050_Write_Reg(uint8_t RegAddress, uint8_t Data) {
    uint8_t buf[2] = {RegAddress, Data};
    I2C_Bus_TransmitBlocking(MPU6050_ADDRESS, buf, 2, 5);
}

void MPU6050_Init()
{
    /*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_Write_Reg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_Write_Reg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_Write_Reg(MPU6050_SMPLRT_DIV, 0x09);		//采样率分频寄存器，配置采样率
	MPU6050_Write_Reg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_Write_Reg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_Write_Reg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}


/**
  * 函    数：发起 MPU6050 数据采集请求
  * 说    明：调用后立即返回，不阻塞 CPU
  */
/*笔记：
以下从三个层面解释它为什么能“自动”实现：
这不是 MPU6050 的规定，而是 I2C 协议的标准
你写的“先写地址、再重复起始、再读数据”的时序，在 I2C 官方标准里被称为 “Random Read（随机读取）” 或 “Memory Read（内存读取）”。
几乎所有的 I2C 传感器（EEPROM、陀螺仪、温湿度计）读取寄存器时都用这一套标准时序。
芯片设计公司（如 ST 意法半导体）深知这一点，所以在设计硬件 I2C 模块时，直接在 硬件电路（逻辑门状态机） 里写死了这套逻辑。
*/
void MPU6050_Request_Data(void) {
     // 向 0x3B 地址发起 14 字节的 DMA 读取请求
    I2C_Bus_ReceiveAsync(MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, 14);
}

/**
  * 函    数：解析读取到的原始数据
  * 参    数：Data 指向你的数据结构体
  */
bool MPU6050_Parse_Data(MPU6050_Data_Struct *Data)
{
    uint8_t *buffer = I2C_Bus_GetReceivedData();
    if(buffer)
    {
        Data->Accx  = (buffer[0] << 8) | buffer[1];
        Data->AccY  = (buffer[2] << 8) | buffer[3];
        Data->AccZ  = (buffer[4] << 8) | buffer[5];
        Data->GyroX = (buffer[8] << 8) | buffer[9];
        Data->GyroY = (buffer[10] << 8) | buffer[11];
        Data->GyroZ = (buffer[12] << 8) | buffer[13];

        return true;
    }
    return false;
}