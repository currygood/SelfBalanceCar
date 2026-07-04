/*
MPU6050陀螺仪加速度模块
*/

#include "main.h"
#include <stdint.h>
#include <stdbool.h>


#define MPU6050_ADDRESS  0x68   // 7-bit address

#define MPU6050_ADDRESS_Read    MPU6050_ADDRESS | 0x01
#define MPU6050_ADDRESS_Write   MPU6050_ADDRESS | 0x00

#define	MPU6050_SMPLRT_DIV		0x19
#define	MPU6050_CONFIG			0x1A
#define	MPU6050_GYRO_CONFIG		0x1B
#define	MPU6050_ACCEL_CONFIG	0x1C

#define	MPU6050_ACCEL_XOUT_H	0x3B
#define	MPU6050_ACCEL_XOUT_L	0x3C
#define	MPU6050_ACCEL_YOUT_H	0x3D
#define	MPU6050_ACCEL_YOUT_L	0x3E
#define	MPU6050_ACCEL_ZOUT_H	0x3F
#define	MPU6050_ACCEL_ZOUT_L	0x40
#define	MPU6050_TEMP_OUT_H		0x41
#define	MPU6050_TEMP_OUT_L		0x42
#define	MPU6050_GYRO_XOUT_H		0x43
#define	MPU6050_GYRO_XOUT_L		0x44
#define	MPU6050_GYRO_YOUT_H		0x45
#define	MPU6050_GYRO_YOUT_L		0x46
#define	MPU6050_GYRO_ZOUT_H		0x47
#define	MPU6050_GYRO_ZOUT_L		0x48

#define	MPU6050_PWR_MGMT_1		0x6B
#define	MPU6050_PWR_MGMT_2		0x6C
#define	MPU6050_WHO_AM_I		0x75

typedef struct
{
    int16_t AccX,AccY,AccZ;         //三轴加速度
    int16_t GyroX,GyroY,GyroZ;      //三轴陀螺仪
}MPU6050_Data_Struct;

// 根据MPU6050设计来的  加速度和陀螺仪都是满量程
#define PI                  3.14159f  //1g
#define ANGLE_MAX           180.0f      //角度范围
#define GYRO_MAX_RANGE      2000.0f     //当前配置的陀螺仪满量程范围（单位：°/s）。
#define GYRO_MAX_PRIMITIVE  32768.0f //陀螺仪数据的最大值
#define SENSITIVITY         16.4f    //2000量程的灵敏度，GX * (2000 / 32768)  等价于 GX/16.4    32768/2000=16.384  有点误差，所以使用手册的16.4   GX*一个数等于除以他的倒数
#define SAMPING_PERIOD      0.005f   //5ms获取一次  用于计算陀螺仪角度的
#define ACCELERATION_SHARE      0.05f
#define GYROSCOPE_SHARE         (1-ACCELERATION_SHARE)

/**
 * @note 宏函数
*/
// 加速度计角度计算（通用轴）
// ACC_SENS: 敏感轴加速度值（角度变化时，数值显著变化的分量，即原公式中的 atan2 分子）
// ACC_REF:  参考轴加速度值（角度变化时，数值相对稳定的参考分量，即原公式中的 atan2 分母）
#define COMPUTE_ANGLE_ACC(ACC_SENS, ACC_REF)   (-atan2((ACC_SENS), (ACC_REF)) / PI * ANGLE_MAX)

// 陀螺仪角度积分递推（通用轴）
// GYRO_RATE: 当前待计算轴的角速度原始值（ADC值或带符号的角速度，单位取决于你的 SENSITIVITY）
// ANGLE_LAST: 上一时刻该轴的角度值
#define COMPUTE_ANGLE_GYRO(GYRO_RATE, ANGLE_LAST) ((ANGLE_LAST) + ((GYRO_RATE) / SENSITIVITY) * SAMPING_PERIOD)

/**
 * @brief 互补滤波
 * @param AngleAcc 加速度计计算得到的偏航角
 * @param AngleGyro 陀螺仪计算得到的偏航角
*/
#define COMPLEMENTARY_FILTER(AngleAcc,AngleGyro)    (((AngleAcc)*ACCELERATION_SHARE)+((AngleGyro)*GYROSCOPE_SHARE))

void MPU6050_Init();

void MPU6050_Request_Data(void);
bool MPU6050_Parse_Data(MPU6050_Data_Struct *Data);