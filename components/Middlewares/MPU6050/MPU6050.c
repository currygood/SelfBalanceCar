#include "MPU6050.h"
#include "I2C_Bus.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief MPU6050寄存器写入
 * @param RegAddress 寄存器地址
 * @param Data 要写入的数据
*/
static void MPU6050_Write_Reg(uint8_t RegAddress, uint8_t Data) {
    uint8_t buf[2] = {RegAddress, Data};
    I2C_Bus_TransmitBlocking(MPU6050_ADDRESS<<1, buf, 2, 5);
}

/**
 * @note 对外API接口
*/

/**
 * @brief MPU6050初始化
*/
void MPU6050_Init()
{
    /*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_Write_Reg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_Write_Reg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_Write_Reg(MPU6050_SMPLRT_DIV, 0x07);		//采样率分频寄存器，配置采样率
	MPU6050_Write_Reg(MPU6050_CONFIG, 0x00);			//配置寄存器，配置DLPF
	MPU6050_Write_Reg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_Write_Reg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
    MPU6050_Write_Reg(MPU6050_CONFIG, 0x02);

}

/**
  * @brief 发起 MPU6050 数据采集请求（异步）
  * @note 调用后立即返回，不阻塞 CPU
*/
void MPU6050_Request_Data(void) {
     // 向 0x3B 地址发起 14 字节的 DMA 读取请求
    I2C_Bus_ReceiveAsync(MPU6050_ADDRESS<<1, MPU6050_ACCEL_XOUT_H, 14);
}

/**
  * @brief 解析读取到的原始数据
  * @param Data 指向数据结构体的指针
  * @return bool 解析是否成功
*/
bool MPU6050_Parse_Data(MPU6050_Data_Struct *Data)
{
    uint8_t *buffer = I2C_Bus_GetReceivedData();
    if(buffer)
    {
        Data->AccX  = (buffer[0] << 8) | buffer[1];
        Data->AccY  = (buffer[2] << 8) | buffer[3];
        Data->AccZ  = (buffer[4] << 8) | buffer[5];
        Data->GyroX = (buffer[8] << 8) | buffer[9];
        Data->GyroY = (buffer[10] << 8) | buffer[11];
        Data->GyroZ = (buffer[12] << 8) | buffer[13];

        return true;
    }
    return false;
}