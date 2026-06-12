#ifndef __I2C_BUS_H
#define __I2C_BUS_H

#include "main.h"
#include <stdbool.h>

/* 状态枚举 */
typedef enum {
    I2C_OK = 0,
    I2C_BUSY = 1,
    I2C_TIMEOUT = 2,
} I2C_Status;

/* 初始化 */
void I2C_Bus_Init(void);

// 发送数据（异步），完成后通过回调或轮询检查状态
I2C_Status I2C_Bus_TransmitAsync(uint8_t devAddr, uint8_t *data, uint16_t len);
// 检查上次发送是否完成（true=空闲可发送，false=忙碌）
bool I2C_Bus_IsTxReady(void);

// 读取寄存器（异步），启动 DMA 接收
I2C_Status I2C_Bus_ReceiveAsync(uint8_t devAddr, uint8_t regAddr, uint16_t size);
// 检查上次接收是否完成并获取数据（若完成则返回数据指针，否则返回 NULL）
uint8_t* I2C_Bus_GetReceivedData(void);
// 手动清除接收完成标志（若不想用 GetReceivedData 的自动清除）
void I2C_Bus_ClearRxFlag(void);

/* ----- 阻塞辅助函数（用于初始化等低频操作）----- */
// 阻塞发送（带超时，ms），内部调用异步 + 等待
I2C_Status I2C_Bus_TransmitBlocking(uint8_t devAddr, uint8_t *data, uint16_t len, uint32_t timeoutMs);
// 阻塞读取
I2C_Status I2C_Bus_ReceiveBlocking(uint8_t devAddr, uint8_t regAddr, uint8_t *outBuf, uint16_t size, uint32_t timeoutMs);

#endif