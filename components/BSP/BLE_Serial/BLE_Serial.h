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


void BLE_Serial_Init(EventGroupHandle_t eventgroup);

bool BLE_Serial_SendBytes(uint8_t *data,uint16_t size,uint16_t timeout);

void BLE_Serial_ReadByte(uint8_t *buffer);

bool BLE_Serial_isCanRead();


#endif
