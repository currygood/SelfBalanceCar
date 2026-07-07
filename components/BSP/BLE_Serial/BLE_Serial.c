#include "BLE_Serial.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#include "usart.h"
#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Shared.h"


#define RX_FIXED_LEN  8      // 固定接收字节数
uint8_t rx_dma_buffer[RX_FIXED_LEN];
volatile bool rx_complete_flag = false;
volatile bool isCanTx=true;

EventGroupHandle_t BLESerial_System_EventGroup=NULL;

/**
 * @brief 蓝牙串口初始化
 * @param eventgroup 事件组句柄，用于通信同步
*/
void BLE_Serial_Init(EventGroupHandle_t eventgroup)
{
    // 启动 DMA 接收（固定长度，非循环模式）
    HAL_UART_Receive_DMA(&huart1, rx_dma_buffer, RX_FIXED_LEN);
    BLESerial_System_EventGroup = eventgroup;
}

bool BLE_Serial_SendBytes(uint8_t *data,uint16_t size,uint16_t timeout)
{
    uint16_t timeoutInit=HAL_GetTick();
    uint16_t timeoutCount;
    while(!isCanTx)        //要等上次的传输完成
    {
        timeoutCount=HAL_GetTick();
        if(timeoutCount-timeoutInit>timeout)
            return false;
    }
    isCanTx=false;
    HAL_UART_Transmit_DMA(&huart1,data,size);
    return true;
}

void BLE_Serial_ReadByte(uint8_t *buffer)
{
    memcpy(buffer,rx_dma_buffer,sizeof rx_dma_buffer);
    BL_Serial_ClearFlag();
    HAL_UART_Receive_DMA(&huart1, rx_dma_buffer, RX_FIXED_LEN);
}

bool BLE_Serial_isCanRead()
{
    return rx_complete_flag;
}

/**
 * @brief 清除蓝牙串口接收标志
*/
void BL_Serial_ClearFlag()
{
    rx_complete_flag=false;
    memset(rx_dma_buffer,0,sizeof rx_dma_buffer);
}

/**
 * @note 内部函数
*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance==USART1)
    {
        isCanTx=true;      //目前测试用的
        if(BLESerial_System_EventGroup)        //后面任务函数里面轮询获取任务通知就好
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xEventGroupSetBitsFromISR(BLESerial_System_EventGroup, EVENT_UART_TX_FINISH, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        rx_complete_flag = true;    //目前测试用的
        if(BLESerial_System_EventGroup)        //后面任务函数里面轮询获取任务通知就好
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xEventGroupSetBitsFromISR(BLESerial_System_EventGroup, EVENT_UART_RX_FINISH, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}