/*
    这是任务，模块交流之间的实现模块
*/
#ifndef __SHARED_H
#define __SHARED_H

#include "main.h"
#include "FreeRTOS.h"
#include "Task.h"
#include "event_groups.h"

/*
    对外的宏定义
*/
// 事件标志组
#define EVENT_UART_RX_FINISH    (1 << 0)   // bit0  uart的rx完成标志
#define EVENT_UART_TX_FINISH    (1 << 1)   // bit1  uart的tx完成标志
#define EVENT_GPIO_ECHO         (1 << 2)   // bit1  超声波测距完成标志
#define EVENT_I2C_RX_FINISH     (1 << 3)   // bit2  I2C总线RX完成


void Shared_Init();

EventGroupHandle_t Shared_Get_EventGroupHandler();


/*之后写到任务就要初始化
SystemEventGroup = xEventGroupCreate();
configASSERT(SystemEventGroup != NULL);  // 检查是否创建成功
*/

#endif
