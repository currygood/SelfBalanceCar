/*
BLE蓝牙模块串口通信底层
*/

#ifndef __BLE_SERIAL_H
#define __BLE_SERIAL_H


#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

/*
    可供外部使用的宏定义
*/

/*
    可供外部使用的结构体等抽象数据类型
*/

/*
    可供外部使用的extern变量
*/

/*
    对外暴露的API接口
*/
// 蓝牙串口初始化
void BLE_Serial_Init(EventGroupHandle_t eventgroup);

// 蓝牙串口发送字节数据
bool BLE_Serial_SendBytes(uint8_t *data,uint16_t size,uint16_t timeout);

// 读取蓝牙串口接收到的字节数据
void BLE_Serial_ReadByte(uint8_t *buffer);

// 检查蓝牙串口是否有数据可读
bool BLE_Serial_isCanRead();


#endif