#include "I2C_Bus.h"
#include "i2c.h"
#include "Shared.h"
#include "event_groups.h"
#include <string.h>

/*
    内部使用的宏定义
*/
#define TX_BUF_SIZE  132
#define RX_BUF_SIZE  14

/*
    内部使用的全局变量
*/
static uint8_t tx_buf[TX_BUF_SIZE];
static uint8_t rx_buf[RX_BUF_SIZE];

// 状态标志（全部 volatile）
static volatile bool tx_busy = false;     // 发送 DMA 进行中
static volatile bool rx_busy = false;     // 接收 DMA 进行中
static volatile bool rx_complete = false; // 接收完成，数据有效


static EventGroupHandle_t I2C_EventGroupHandler;

/*
    内部调用的函数
*/

/**
 * @note 内部函数
*/

/**
 * @brief I2C_Bus内部调用的I2C总线是否空闲的函数
 * @return true:空闲 false:忙
*/
static bool I2C_IsHardwareReady(void) {
    return (HAL_I2C_GetState(&hi2c1) == HAL_I2C_STATE_READY);
}

/**
 * @brief I2C_Bus内部调用的发起 DMA 发送
 * @param devAddr：从设备地址
 * @param data 要传输的数据 
 * @param len  多少字节
 * @return I2C_Status   but只使用了下面的
 *          I2C_OK = 0, Success
 *          I2C_BUSY = 1, Busy
*/
static I2C_Status I2C_StartTransmit_DMA(uint8_t devAddr, uint8_t *data, uint16_t len) {
    if (tx_busy || rx_busy) return I2C_BUSY;
    if (!I2C_IsHardwareReady()) return I2C_BUSY;
    
    tx_busy = true;
    if (HAL_I2C_Master_Transmit_DMA(&hi2c1, devAddr, data, len) != HAL_OK) {
        tx_busy = false;
        return I2C_BUSY;
    }
    return I2C_OK;
}

/**
 * @brief I2C_Bus内部调用的发起 DMA 接收（内存读取模式）
 * @param devAddr：从设备地址
 * @param regAddr 要获取的地址
 * @param size  多少字节
 * @return I2C_Status   but只使用了下面的
 *          I2C_OK = 0, Success
 *          I2C_BUSY = 1, Busy
*/
static I2C_Status I2C_StartReceive_DMA(uint8_t devAddr, uint8_t regAddr, uint16_t size) {
    if (tx_busy || rx_busy) return I2C_BUSY;
    if (!I2C_IsHardwareReady()) return I2C_BUSY;
    if (size > RX_BUF_SIZE) size = RX_BUF_SIZE;
    
    rx_busy = true;
    rx_complete = false;
    tx_busy=true;
    if (HAL_I2C_Mem_Read_DMA(&hi2c1, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT, rx_buf, size) != HAL_OK) {
        rx_busy = false;
        return I2C_BUSY;
    }
    return I2C_OK;
}

/**
 * @note 对外API接口
*/

/**
 * @brief I2C_Bus初始化
*/
void I2C_Bus_Init(EventGroupHandle_t eventGroupHandler) {
    tx_busy = false;
    rx_busy = false;
    rx_complete = false;
    I2C_EventGroupHandler = eventGroupHandler;
}

I2C_Status I2C_Bus_TransmitAsync(uint8_t devAddr, uint8_t *data, uint16_t len) {
    if (len > TX_BUF_SIZE) len = TX_BUF_SIZE;
    memcpy(tx_buf, data, len);
    return I2C_StartTransmit_DMA(devAddr, tx_buf, len);
}

bool I2C_Bus_IsTxReady(void) {
    return (!tx_busy && !rx_busy && I2C_IsHardwareReady());
}

I2C_Status I2C_Bus_ReceiveAsync(uint8_t devAddr, uint8_t regAddr, uint16_t size) {
    return I2C_StartReceive_DMA(devAddr, regAddr, size);
}

uint8_t* I2C_Bus_GetReceivedData(void) {
    if (rx_complete) {
        rx_complete = false;  // 自动清除标志，数据只能被读取一次
        return rx_buf;
    }
    return NULL;
}

void I2C_Bus_ClearRxFlag(void) {
    rx_complete = false;
}

/* 阻塞发送（简单轮询等待） */
I2C_Status I2C_Bus_TransmitBlocking(uint8_t devAddr, uint8_t *data, uint16_t len, uint32_t timeoutMs) {
    uint32_t start = HAL_GetTick();
    while (!I2C_Bus_IsTxReady()) {
        if (HAL_GetTick() - start > timeoutMs) return I2C_TIMEOUT;
    }
    I2C_Status ret = I2C_Bus_TransmitAsync(devAddr, data, len);
    if (ret != I2C_OK) return ret;
    // 等待发送完成
    while (!I2C_Bus_IsTxReady()) {
        if (HAL_GetTick() - start > timeoutMs) return I2C_TIMEOUT;
    }
    return I2C_OK;
}

I2C_Status I2C_Bus_ReceiveBlocking(uint8_t devAddr, uint8_t regAddr, uint8_t *outBuf, uint16_t size, uint32_t timeoutMs) {
    uint32_t start = HAL_GetTick();
    while (!I2C_Bus_IsTxReady()) {   // 接收也需要总线空闲
        if (HAL_GetTick() - start > timeoutMs) return I2C_TIMEOUT;
    }
    I2C_Status ret = I2C_Bus_ReceiveAsync(devAddr, regAddr, size);
    if (ret != I2C_OK) return ret;
    while (1) {
        uint8_t *p = I2C_Bus_GetReceivedData();
        if (p != NULL) {
            memcpy(outBuf, p, size);
            return I2C_OK;
        }
        if (HAL_GetTick() - start > timeoutMs) return I2C_TIMEOUT;
    }
}

/**
 * @note 内部函数
*/

/* HAL 回调函数 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        tx_busy = false;
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        rx_busy = false;
        rx_complete = true;
        tx_busy=false;
        if(I2C_EventGroupHandler)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xEventGroupSetBitsFromISR(I2C_EventGroupHandler, EVENT_I2C_RX_FINISH, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

// 错误回调可选（简化，若出错则复位标志）
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C1) {
        tx_busy = false;
        rx_busy = false;
        rx_complete = false;
    }
}